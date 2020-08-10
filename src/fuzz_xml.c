#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/parser.h>

#include "fs.h"
#include "fuzz_xml.h"
#include "safe.h"
#include "utils.h"

/* Cast string to a "const xmlChar *" */
#define TOXMLCHAR(S) ((const unsigned char *) S)

/* Examples can be found on:
 *     http://www.xmlsoft.org/examples/index.html
 *
 * The daddy of documentation:
 *     http://www.xmlsoft.org/html/index.html
 *
 * The xmlNodePtr can be found in:
 *     /usr/include/libxml2/libxml/tree.h
 */

/* Handle for the iterate_xmldoc function */
typedef void (*iter_handle)(void *, xmlNodePtr);

/* Helper functions */
static int save_doc(struct state *s);
static uint64_t get_num_nodes(xmlNode *root);
static void do_buffer_overflow(void *data, xmlNode *ptr);
static void do_fuzz_tag_attrs(void *data, xmlNodePtr ptr);
static void fuzz(struct state *s);
static void fuzz_buffer_overflow(struct state *s);
static void fuzz_populate(UNUSED struct state *s);
static void fuzz_props_fmt_str(struct state *s);
static void fuzz_refresh(struct state *s);
static void fuzz_tag_attrs(struct state *s);
static void fuzz_tag_fmt_str(struct state *s);
static void get_node(void *data, xmlNode *curr);
static void handle_props_fmt_str(void *data, xmlNode *curr);
static void handle_tag_fmt_str(void *data, xmlNode *curr);
static void insert_node(void *data, xmlNode *curr);
static void iterate_xmldoc(xmlNodePtr root, void *data, iter_handle ih);
static xmlChar *sXmlStrdup(const xmlChar *cur);
static xmlNode *sXmlCopyNode(xmlNode *node, int extended);
static xmlNodePtr sXmlAddChild(xmlNodePtr cur, xmlNodePtr elem);
static xmlNodePtr sXmlAddChildList(xmlNodePtr cur, xmlNodePtr elem);
static xmlNodePtr sXmlAddNextSibling(xmlNodePtr cur, xmlNodePtr elem);
static xmlNodePtr sXmlAddPrevSibling(xmlNodePtr cur, xmlNodePtr elem);
static xmlNodePtr sXmlAddSibling(xmlNodePtr cur, xmlNodePtr elem);
static xmlNodePtr sXmlDocGetRootElement(xmlDocPtr doc);
static void fuzz_big_print(struct state *s);
static void fuzz_clone_root(struct state *s);
static void xml_deploy(struct state *s);

static struct {

	/* This are variables that will be free in the case of a memory leak */
	xmlChar *dummy_char; /* for xmlFree() */

	xmlDocPtr doc;
	xmlSaveCtxtPtr ctx;

	uint64_t num_nodes; /* Total number of nodes in our xml.doc */
} xml = {0};

/* Here we add functions used for fuzzing.
 * Each function tests for something different. */
static void (*fuzz_payloads[])(struct state *) = {
	fuzz_buffer_overflow, /* Buffer overflow tag names */
	fuzz_tag_attrs,       /* Buffer overflow tag properties */
	fuzz_tag_fmt_str,     /* Change tag names to format strings */
	fuzz_props_fmt_str,   /* Change tag properties to format strings */
	fuzz_big_print,       /* Print the xml.doc a few times */
	fuzz_clone_root,      /* Insert the root node inside of the tree */
	fuzz_populate,        /* Add random nodes to the xml.doc */
	fuzz_refresh,         /* Refresh nodes in the xml.doc */
};

void
fuzz_handle_xml(struct state *state)
{
	/*
	 * this initialize the library and check potential ABI mismatches
	 * between the version it was compiled for and the actual shared
	 * library used.
	 */
	LIBXML_TEST_VERSION

	xml.doc = xmlReadMemory(
		state->mem,
		state->stat.st_size,
		"/dev/null",
		NULL,
		0
	);
	if (!xml.doc)
		panic("Failed to init xml parser :(\n");

	/* We use XML_SAVE_NO_DECL so that we don't get the version number when
	 * dumping the xml file */
	xml.ctx = xmlSaveToFd(state->payload_fd, NULL, XML_SAVE_NO_DECL);
	if (!xml.ctx)
		panic("Could not init context for xml doc\n");

	xml.num_nodes = get_num_nodes(sXmlDocGetRootElement(xml.doc));

	fuzz(state);
}

void
free_handle_xml(UNUSED struct state *state)
{
	if (xml.dummy_char)
		xmlFree(xml.dummy_char);

	xmlFreeDoc(xml.doc);
	xmlCleanupParser();

	if (xmlSaveClose(xml.ctx) == -1)
		panic("Error: xmlSaveClose(%p)\n", xml.ctx);
}

/* Does the $$$ */
static
void
fuzz(struct state *s)
{
	while (1) {
		uint32_t idx = roll_dice(0, ARRSIZE(fuzz_payloads)-1);
		fuzz_payloads[idx](s);
	}
}

static
uint64_t
get_num_nodes(xmlNode *root) /* TODO This function should use the iterate function */
{
	if (!root)
		return 0;

	uint64_t total = 0;

	for (xmlNode *curr = root; curr; curr = curr->next) {
		if (curr->type != XML_ELEMENT_NODE)
			continue;
		total += get_num_nodes(curr->children) + 1;
	}

	return total;
}

static
void
fuzz_clone_root(struct state *s)
{
	check();

	xmlNode *root = sXmlDocGetRootElement(xml.doc);

	xmlNode *copy = sXmlCopyNode(root, 1 /* recursive */);

	if (root->children) {
		check();
		sXmlAddNextSibling(root->children, copy);
	} else {
		check();
		sXmlAddChildList(root, copy);
	}

	save_doc(s);
	xml_deploy(s);
}

static
void
do_buffer_overflow(void *data, xmlNode *ptr)
{
	struct state *s = data;

	if (ptr->type != XML_ELEMENT_NODE)
		return;

	/* 10% chance of testing bof */
	if (coin_flip(10))
		return;

	xml.dummy_char = sXmlStrdup(ptr->name);
	xmlNodeSetName(ptr, TOXMLCHAR(BIG));
	save_doc(s);
	xml_deploy(s);
	xmlNodeSetName(ptr, xml.dummy_char);
	xmlFree(xml.dummy_char);
	xml.dummy_char = NULL;
}

static
void
fuzz_buffer_overflow(struct state *s)
{
	check();
	iterate_xmldoc (
		sXmlDocGetRootElement(xml.doc),
		s,
		do_buffer_overflow
	);
}

static
void
do_fuzz_tag_attrs(void *data, xmlNodePtr ptr)
{
	xmlAttr *attr;
	struct state *s = data;

	for (attr = ptr->properties; attr; attr = attr->next) {

		/* 10% chance of testing bof */
		if (coin_flip(10))
			return;

		xml.dummy_char = xmlGetProp(ptr, attr->name);
		if (!xml.dummy_char)
			continue;

		xmlSetProp(ptr, attr->name, TOXMLCHAR(BIG));
		save_doc(s);
		xml_deploy(s);

		xmlSetProp(ptr, attr->name, xml.dummy_char);

		xmlFree(xml.dummy_char);
		xml.dummy_char = NULL;
	}
}

static
void
fuzz_tag_attrs(struct state *s)
{
	check();
	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		s,
		do_fuzz_tag_attrs
	);
}

/* TODO We could segfault if we iterate too many time. To fix this we need a
 * stack to store all possible iterations */
static
void
iterate_xmldoc(xmlNodePtr root, void *data, iter_handle ih)
{
	/* I am a comp2521 god */
	for (xmlNodePtr curr = root; curr; curr = curr->next) {
		ih(data, curr);
		iterate_xmldoc(curr->children, data, ih);
	}
}

static
void
handle_tag_fmt_str(void *data, xmlNode *curr)
{
	struct state *s = data;

	if (curr->type != XML_ELEMENT_NODE)
		return;

	if (coin_flip(5))
		return;

	xml.dummy_char = sXmlStrdup(curr->name);

	for (uint32_t i = 0; i < ARRSIZE(fmt_strings); i++) {
		xmlNodeSetName(curr, TOXMLCHAR(fmt_strings[i]));
		save_doc(s);
		xml_deploy(s);
	}

	xmlNodeSetName(curr, xml.dummy_char);

	xmlFree(xml.dummy_char);
	xml.dummy_char = NULL;
}

static
void
handle_props_fmt_str(void *data, xmlNode *curr)
{
	struct state *s = data;

	if (coin_flip(5))
		return;

	for (xmlAttr *attr = curr->properties; attr; attr = attr->next) {

		xml.dummy_char = xmlGetProp(curr, attr->name);
		if (!xml.dummy_char)
			continue;

		for (uint32_t i = 0; i < ARRSIZE(fmt_strings); i++) {
			xmlSetProp(curr, attr->name, TOXMLCHAR(fmt_strings[i]));
			save_doc(s);
			xml_deploy(s);
		}

		xmlSetProp(curr, attr->name, xml.dummy_char);

		xmlFree(xml.dummy_char);
		xml.dummy_char = NULL;
	}
}

static
void
fuzz_tag_fmt_str(struct state *s)
{
	check();
	iterate_xmldoc(sXmlDocGetRootElement(xml.doc), s, handle_tag_fmt_str);
}

static
void
fuzz_props_fmt_str(struct state *s)
{
	check();
	iterate_xmldoc(sXmlDocGetRootElement(xml.doc), s, handle_props_fmt_str);
}

static
void
fuzz_big_print(struct state *s)
{
	check();
	off_t bytes = 0;
	uint64_t niters;
	int ret;

	/* This is an expensive operation so we don't want to do this everytime */
	if (roll_dice(1, 10000) == 10)
		return;

	check();

	slseek(s->payload_fd, 0, SEEK_SET);

	for (niters = roll_dice(100, 10000); niters; niters--) {

		ret = xmlSaveDoc(xml.ctx, xml.doc);
		if (ret == -1)
			panic("Failed to save xml doc in fuzz_big_print\n");

		ret = xmlSaveFlush(xml.ctx);
		if (ret == -1)
			panic("Failed to flush xml doc");

		/* comp6447 makes me scared of type casting ints */
		bytes += (off_t) ret;
	}

	sftruncate(s->payload_fd, (off_t) bytes);

	xml_deploy(s);
}

static
void
insert_node(void *data, xmlNode *curr)
{
	uint64_t *index = GETARG(uint64_t *, data, 0);
	xmlNode **new = GETARG(xmlNode **, data, 1);

	if (curr->type == XML_TEXT_NODE)
		return;

	if (*index == 0) {
		switch (roll_dice(1, 5)) {
		case 1:
			sXmlAddChild(curr, *new);
			break;

		case 2:
			sXmlAddChildList(curr, *new);
			break;

		case 3:
			sXmlAddNextSibling(curr, *new);
			break;

		case 4:
			sXmlAddPrevSibling(curr, *new);
			break;

		case 5:
			sXmlAddSibling(curr, *new);
			break;

		default:
			panic("wtf");
		}

		/* Clear *new to indicate the node has been inserted */
		*new = NULL;
	}

	*index = *index - 1;
}

static
void
get_node(void *data, xmlNode *curr)
{
	uint64_t *index = GETARG(uint64_t *, data, 0);
	xmlNode **ret = GETARG(xmlNode **, data, 1);

	if (curr->type == XML_TEXT_NODE)
		return;

	if (*index == 0)
		*ret = curr;

	*index = *index - 1;
}

static
void
fuzz_populate(UNUSED struct state *s)
{
	check();
	void *args[2];
	xmlNode *curr = NULL;
	uint64_t index;

	for (int i = 0; i < 20; i++) {

		index = roll_dice(0, xml.num_nodes-1);

		args[0] = &index; /* Argument: uint64_t* */
		args[1] = &curr;  /* Return value: xmlNode** */

		iterate_xmldoc(
			sXmlDocGetRootElement(xml.doc),
			&args,
			get_node
		);

		if (!curr) /* some error? */
			return;

		xmlNode *copy = sXmlCopyNode(curr, 1 /* recursive */);

		uint64_t new_num_nodes = get_num_nodes(copy);

		index = roll_dice(0, xml.num_nodes-1);

		args[0] = &index; /* Arugment: Which node to insert into tree */
		args[1] = &copy;  /* Arugment: The node that will be inserted */

		iterate_xmldoc(
			sXmlDocGetRootElement(xml.doc),
			&args,
			insert_node
		);

		if (copy) {
			/* The node was not inserted for some reason */
			xmlFree(copy);
			return;
		}

		xml.num_nodes += new_num_nodes;

		save_doc(s);
		xml_deploy(s);
	}
}

static
void
fuzz_refresh(struct state *s)
{
	check();

	xmlFreeDoc(xml.doc);

	xml.doc = xmlReadMemory(
		s->mem,
		s->stat.st_size,
		"/dev/null",
		NULL,
		0
	);
	if (!xml.doc)
		panic("Failed to init xml parser in fuzz_refresh\n");

	xml.num_nodes = get_num_nodes(sXmlDocGetRootElement(xml.doc));
}

static
xmlNodePtr
sXmlDocGetRootElement(xmlDocPtr doc)
{
	xmlNodePtr ret = xmlDocGetRootElement(doc);
	if (ret == NULL)
		panic("File to get root node with: xmlDocGetRootElement(%p)\n", doc);
	return ret;
}

static
int
save_doc(struct state *s)
{
	int bytes;

	slseek(s->payload_fd, 0, SEEK_SET);
	sftruncate(s->payload_fd, 0);

	/* Quote from libxml documentation:
	 *     todo: The function is not fully implemented yet as it does not
	 *           return the byte count but 0 instead
	 */
	bytes = xmlSaveDoc(xml.ctx, xml.doc);
	if (bytes == -1)
		panic("Failed to save xml doc\n");

	/* Why tf does this reutrn an int??? */
	bytes = xmlSaveFlush(xml.ctx);
	if (bytes == -1)
		panic("Failed to flush xml doc");

	return bytes;
}

static
xmlNode *
sXmlCopyNode(xmlNode *node, int extended)
{
	xmlNode *ret = xmlCopyNode(node, extended);
	if (!ret)
		panic("Error: xmlCopyNode(%p, %d)\n", node, extended);
	return ret;
}

static
xmlChar *
sXmlStrdup(const xmlChar *cur)
{
	xmlChar *ret = xmlStrdup(cur);
	if (!ret)
		panic("Error: xmlStrdup(\"%s\")\n", cur);
	return ret;
}

static
xmlNodePtr
sXmlAddChild(xmlNodePtr cur, xmlNodePtr elem)
{
	xmlNodePtr ret = xmlAddChild(cur, elem);
	if (!ret)
		panic("Error: xmlAddChild(%p, %p)\n", cur, elem);
	return ret;
}

static
xmlNodePtr
sXmlAddChildList(xmlNodePtr cur, xmlNodePtr elem)
{
	xmlNodePtr ret = xmlAddChildList(cur, elem);
	if (!ret)
		panic("Error: xmlAddChildList(%p, %p)\n", cur, elem);
	return ret;
}

static
xmlNodePtr
sXmlAddNextSibling(xmlNodePtr cur, xmlNodePtr elem)
{
	xmlNodePtr ret = xmlAddNextSibling(cur, elem);
	if (!ret)
		panic("Error: xmlAddNextSibling(%p, %p)\n", cur, elem);
	return ret;
}

static
xmlNodePtr
sXmlAddPrevSibling(xmlNodePtr cur, xmlNodePtr elem)
{
	xmlNodePtr ret = xmlAddPrevSibling(cur, elem);
	if (!ret)
		panic("Error: xmlAddPrevSibling(%p, %p)\n", cur, elem);
	return ret;
}

static
xmlNodePtr
sXmlAddSibling(xmlNodePtr cur, xmlNodePtr elem)
{
	xmlNodePtr ret = xmlAddSibling(cur, elem);
	if (!ret)
		panic("Error: xmlAddSibling(%p, %p)\n", cur, elem);
	return ret;
}

/* The victim program usuall aborts() if there is can error. It's likely that
 * if we keep mutating input then it will keep aborting, therefore we start
 * fresh stop wasting time */
static
void
xml_deploy(struct state *s)
{
	int wstatus = deploy();
	if (WIFSIGNALED(wstatus) && WTERMSIG(wstatus) == SIGABRT) {

		/* 10% chance to refresh */
		if (coin_flip(10)) {
			fuzz_refresh(s);
		}
	}
}

UNUSED
static
const char *
dbg_xmlElementType(xmlElementType type)
{
	switch (type) {
		case XML_ELEMENT_NODE:       return "XML_ELEMENT_NODE";
		case XML_ATTRIBUTE_NODE:     return "XML_ATTRIBUTE_NODE";
		case XML_TEXT_NODE:          return "XML_TEXT_NODE";
		case XML_CDATA_SECTION_NODE: return "XML_CDATA_SECTION_NODE";
		case XML_ENTITY_REF_NODE:    return "XML_ENTITY_REF_NODE";
		case XML_ENTITY_NODE:        return "XML_ENTITY_NODE";
		case XML_PI_NODE:            return "XML_PI_NODE";
		case XML_COMMENT_NODE:       return "XML_COMMENT_NODE";
		case XML_DOCUMENT_NODE:      return "XML_DOCUMENT_NODE";
		case XML_DOCUMENT_TYPE_NODE: return "XML_DOCUMENT_TYPE_NODE";
		case XML_DOCUMENT_FRAG_NODE: return "XML_DOCUMENT_FRAG_NODE";
		case XML_NOTATION_NODE:      return "XML_NOTATION_NODE";
		case XML_HTML_DOCUMENT_NODE: return "XML_HTML_DOCUMENT_NODE";
		case XML_DTD_NODE:           return "XML_DTD_NODE";
		case XML_ELEMENT_DECL:       return "XML_ELEMENT_DECL";
		case XML_ATTRIBUTE_DECL:     return "XML_ATTRIBUTE_DECL";
		case XML_ENTITY_DECL:        return "XML_ENTITY_DECL";
		case XML_NAMESPACE_DECL:     return "XML_NAMESPACE_DECL";
		case XML_XINCLUDE_START:     return "XML_XINCLUDE_START";
		case XML_XINCLUDE_END:       return "XML_XINCLUDE_END";
		case XML_DOCB_DOCUMENT_NODE: return "XML_DOCB_DOCUMENT_NODE";
	}
	return "[UNKOWN]";
}
