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
#include "mutations.h"
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
static const char *dbg_xmlElementType(xmlElementType type);
static xmlNode * get_random_node_from_doc(void);
static int save_doc(struct state *s);
static uint64_t get_num_nodes(xmlNode *root);
static void add_node_random(xmlNode *curr, xmlNode *new);
static void do_buffer_overflow(void *data, xmlNode *ptr);
static void do_fuzz_tag_attrs(void *data, xmlNodePtr ptr);
static void fuzz(struct state *s);
static void fuzz_big_print(struct state *s);
static void fuzz_buffer_overflow(struct state *s);
static void fuzz_clone_root(struct state *s);
static void fuzz_cont_bad_ints(struct state *s);
static void fuzz_cont_bof(struct state *s);
static void fuzz_cont_fmt(struct state *s);
static void fuzz_populate(struct state *s);
static void fuzz_props_bad_ints(struct state *s);
static void fuzz_props_fmt_str(struct state *s);
static void fuzz_refresh(struct state *s);
static void fuzz_rnd_flip(struct state *s);
static void fuzz_rnd_shift(struct state *s);
static void fuzz_single_fmt_props(struct state *s);
static void fuzz_single_populate(struct state *s);
static void fuzz_single_shuffle(struct state *s);
static void fuzz_tag_attrs(struct state *s);
static void fuzz_tag_fmt_str(struct state *s);
static void get_node(void *data, xmlNode *curr);
static void handle_cont_bof(void *data, xmlNode *curr);
static void handle_cont_fmt(void *data, xmlNode *curr);
static void handle_props_bad_ints(void *data, xmlNode *curr);
static void handle_props_fmt_str(void *data, xmlNode *curr);
static void handle_single_fmt_props(void *data, xmlNode *curr);
static void handle_tag_fmt_str(void *data, xmlNode *curr);
static void hanlde_cont_bad_ints(void *data, xmlNode *curr);
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

static struct {

	/* This are variables that will be free in the case of a memory leak */
	xmlChar *dummy_char; /* for xmlFree() */
	xmlNode *dummy_nodes[10]; /* for xmlFreeNode() */

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
	fuzz_props_bad_ints,  /* Fuzz properties for bad ints */
	fuzz_cont_bad_ints,   /* Fuzz content for bad ints */
	fuzz_cont_bof,        /* Fuzz buffer overflow of content */
	fuzz_cont_fmt,        /* Fuzz format strings of content */
	fuzz_rnd_shift,       /* Fuzz random parts by bit shifting */
	fuzz_rnd_flip,        /* Fuzz random parts by bit flipping */
};

/* Fuzzing strats that are used once and once only */
static void (*fuzz_single_payloads[])(struct state *s) = {
	fuzz_single_fmt_props, /* Fuzz all properties for format strings */
	fuzz_refresh,          /* Refresh the state */
	fuzz_single_shuffle,   /* Fuzz by shuffling nodes around */
	fuzz_refresh,          /* Refresh the state */
	fuzz_single_populate,  /* Shuffle and duplicate nodes */
	fuzz_refresh,          /* Refresh the state */
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

	for (uint64_t i = 0; i < ARRSIZE(xml.dummy_nodes); i++) {
		if (xml.dummy_nodes[i])
			xmlFreeNode(xml.dummy_nodes[i]);
	}

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
	for (uint64_t i = 0; i < ARRSIZE(fuzz_single_payloads); i++)
		fuzz_single_payloads[i](s);

	while (1) {
		uint32_t idx = roll_dice(0, ARRSIZE(fuzz_payloads)-1);
		fuzz_payloads[idx](s);
	}
}

static
void
handle_single_fmt_props(void *data, xmlNode *curr)
{
	struct state *s = data;

	if (curr->type != XML_ELEMENT_NODE)
		return;

	for (xmlAttr *attr = curr->properties; attr; attr = attr->next) {

		xml.dummy_char = xmlGetProp(curr, attr->name);
		if (!xml.dummy_char)
			continue;

		for (uint64_t i = 0; i < ARRSIZE(fmt_strings); i++) {
			const char *fmt = fmt_strings[i];

			xmlSetProp(curr, attr->name, TOXMLCHAR(fmt));
			save_doc(s);
			deploy();
		}

		xmlSetProp(curr, attr->name, xml.dummy_char);

		xmlFree(xml.dummy_char);
		xml.dummy_char = NULL;
	}
}

static
void
fuzz_single_fmt_props(struct state *s)
{
	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		s,
		handle_single_fmt_props
	);
}

static
void
fuzz_single_populate(struct state *s)
{
	xmlNode *tmp, *root;

	for (uint64_t i = 0; i < ARRSIZE(xml.dummy_nodes); i++) {
		tmp = get_random_node_from_doc();
		if (!tmp)
			goto fuzz_single_populate_error;
		xml.dummy_nodes[i] = sXmlCopyNode(tmp, 1 /* recursive */);
	}

	root = sXmlDocGetRootElement(xml.doc);

	for (uint64_t i = 0; i < 50000; i++) {

		tmp = xml.dummy_nodes[roll_dice(0, ARRSIZE(xml.dummy_nodes)-1)];
		tmp = sXmlCopyNode(tmp, 1 /* recursive */);

		sXmlAddChildList(root, tmp);

		root = root->children;
		while (root && root->type != XML_ELEMENT_NODE)
			root = root->next;

		if (!root)
			break;

		if (i % 1000 == 0) {
			save_doc(s);
			deploy();
		}
	}

	save_doc(s);
	deploy();

fuzz_single_populate_error:
	for (uint64_t i = 0; i < ARRSIZE(xml.dummy_nodes); i++) {
		if (xml.dummy_nodes[i])
			xmlFreeNode(xml.dummy_nodes[i]);
		xml.dummy_nodes[i] = NULL;
	}
}

static
xmlNode *
get_random_node_from_doc(void)
{
	void *args[2];
	xmlNode *curr = NULL;
	uint64_t index;

	if (xml.num_nodes <= 2)
		return NULL; /* not enough nodes */

	index = roll_dice(1, xml.num_nodes-1);

	args[0] = &index; /* Argument: uint64_t* */
	args[1] = &curr;  /* Return value: xmlNode** */

	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		&args,
		get_node
	);

	return curr;
}

static
void
fuzz_single_shuffle(struct state *s)
{
	xmlNode *copy, *root;

	root = sXmlDocGetRootElement(xml.doc);

	for (uint64_t i = 0; i < 15; i++) {

		copy = sXmlCopyNode(root, 1 /* recursive */);
		if (coin_flip(50)) {
			sXmlAddChildList(root, copy);
		} else {
			sXmlAddChild(root, copy);
		}

		save_doc(s);
		deploy();
	}
}

static
uint64_t
get_num_nodes(xmlNode *root)
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
	xmlNode *root = sXmlDocGetRootElement(xml.doc);

	xmlNode *copy = xmlCopyNode(root, 1 /* recursive */);
	if (!copy)
		return;

	xml.num_nodes += get_num_nodes(copy);

	add_node_random(root, copy);

	save_doc(s);
	deploy();
}

static
void
add_node_random(xmlNode *curr, xmlNode *new)
{
	switch (roll_dice(1, 5)) {
	case 1:
		sXmlAddChild(curr, new);
		break;

	case 2:
		sXmlAddChildList(curr, new);
		break;

	case 3:
		sXmlAddNextSibling(curr, new);
		break;

	case 4:
		sXmlAddPrevSibling(curr, new);
		break;

	case 5:
		sXmlAddSibling(curr, new);
		break;

	default:
		panic("wtf");
	}
}

static
void
do_buffer_overflow(void *data, xmlNode *ptr)
{
	struct state *s = data;

	if (ptr->type != XML_ELEMENT_NODE)
		return;

	/* 10% chance of testing bof */
	if (coin_flip(90))
		return;

	xml.dummy_char = sXmlStrdup(ptr->name);
	xmlNodeSetName(ptr, TOXMLCHAR(BIG));
	save_doc(s);
	deploy();
	xmlNodeSetName(ptr, xml.dummy_char);
	xmlFree(xml.dummy_char);
	xml.dummy_char = NULL;
}

static
void
fuzz_buffer_overflow(struct state *s)
{
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
		if (coin_flip(90))
			continue;

		xml.dummy_char = xmlGetProp(ptr, attr->name);
		if (!xml.dummy_char)
			continue;

		xmlSetProp(ptr, attr->name, TOXMLCHAR(BIG));
		save_doc(s);
		deploy();

		xmlSetProp(ptr, attr->name, xml.dummy_char);

		xmlFree(xml.dummy_char);
		xml.dummy_char = NULL;
	}
}

static
void
fuzz_tag_attrs(struct state *s)
{
	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		s,
		do_fuzz_tag_attrs
	);
}

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

	if (coin_flip(90))
		return;

	xml.dummy_char = sXmlStrdup(curr->name);

	const char *fmt = fmt_strings[roll_dice(0, ARRSIZE(fmt_strings)-1)];

	xmlNodeSetName(curr, TOXMLCHAR(fmt));
	save_doc(s);
	deploy();

	xmlNodeSetName(curr, xml.dummy_char);

	xmlFree(xml.dummy_char);
	xml.dummy_char = NULL;
}

static
void
handle_props_fmt_str(void *data, xmlNode *curr)
{
	struct state *s = data;

	for (xmlAttr *attr = curr->properties; attr; attr = attr->next) {

		if (coin_flip(90))
			continue; /* yeah nah brah */

		xml.dummy_char = xmlGetProp(curr, attr->name);
		if (!xml.dummy_char)
			continue;

		const char *fmt = fmt_strings[roll_dice(0, ARRSIZE(fmt_strings)-1)];

		xmlSetProp(curr, attr->name, TOXMLCHAR(fmt));
		save_doc(s);
		deploy();

		xmlSetProp(curr, attr->name, xml.dummy_char);

		xmlFree(xml.dummy_char);
		xml.dummy_char = NULL;
	}
}

static
void
fuzz_tag_fmt_str(struct state *s)
{
	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		s,
		handle_tag_fmt_str
	);
}

static
void
fuzz_props_fmt_str(struct state *s)
{
	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		s,
		handle_props_fmt_str
	);
}

static
void
fuzz_big_print(struct state *s)
{
	off_t bytes = 0;
	uint64_t niters;
	int ret;

	/* This is an expensive operation so we don't want to do this everytime */
	if (roll_dice(1, 10000) == 10)
		return;

	slseek(s->payload_fd, 0, SEEK_SET);

	for (niters = roll_dice(100, 1000); niters; niters--) {

		ret = xmlSaveDoc(xml.ctx, xml.doc);
		if (ret == -1)
			panic("Failed to save xml doc in fuzz_big_print\n");

		ret = xmlSaveFlush(xml.ctx);
		if (ret == -1)
			break;

		/* comp6447 makes me scared of type casting ints */
		bytes += (off_t) ret;
	}

	sftruncate(s->payload_fd, (off_t) bytes);

	deploy();
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
		add_node_random(curr, *new);

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
fuzz_populate(struct state *s)
{
	void *args[2];
	xmlNode *curr = NULL;
	uint64_t index;

	if (xml.num_nodes <= 2)
		return; /* not enough nodes */

	for (uint64_t i = 0; i < 20; i++) {

		index = roll_dice(1, xml.num_nodes-1);

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
			xmlFreeNode(copy);
			return;
		}

		xml.num_nodes += new_num_nodes;

		save_doc(s);
		deploy();
	}
}

static
void
fuzz_refresh(struct state *s)
{
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
		return 0; /* should throw an error but what ever */

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

static
void
handle_props_bad_ints(void *data, xmlNode *curr)
{
	struct state *s = data;

	for (xmlAttr *attr = curr->properties; attr; attr = attr->next) {

		if (coin_flip(90))
			continue; /* yeah nah brah */

		xml.dummy_char = xmlGetProp(curr, attr->name);
		if (!xml.dummy_char)
			continue;

		if (!isint0((const char *) xml.dummy_char)) {
			xmlFree(xml.dummy_char);
			xml.dummy_char = NULL;
			continue;
		}

		for (uint64_t i = 0; i < ARRSIZE(bad_nums); i++) {

			const char *str = bad_nums[i].s;

			xmlSetProp(curr, attr->name, TOXMLCHAR(str));
			save_doc(s);
			deploy();
		}

		xmlSetProp(curr, attr->name, xml.dummy_char);

		xmlFree(xml.dummy_char);
		xml.dummy_char = NULL;
	}
}

static
void
fuzz_props_bad_ints(struct state *s)
{
	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		s,
		handle_props_bad_ints
	);
}

static
void
handle_cont_bof(void *data, xmlNode *curr)
{
	struct state *s = data;

	if (curr->type != XML_TEXT_NODE)
		return;

	if (coin_flip(90))
		return;

	xml.dummy_char = xmlNodeGetContent(curr);

	xmlNodeSetContent(curr, TOXMLCHAR(BIG));
	save_doc(s);
	deploy();

	xmlNodeSetContent(curr, xml.dummy_char);
	xmlFree(xml.dummy_char);
	xml.dummy_char = NULL;
}

static
void
hanlde_cont_bad_ints(void *data, xmlNode *curr)
{
	struct state *s = data;

	if (curr->type != XML_TEXT_NODE)
		return;

	if (coin_flip(90))
		return;

	xml.dummy_char = xmlNodeGetContent(curr);

	if (!isint0((const char *) xml.dummy_char)) {
		xmlFree(xml.dummy_char);
		xml.dummy_char = NULL;
		return;
	}

	for (uint64_t i = 0; i < ARRSIZE(bad_nums); i++) {
		const char *num = bad_nums[i].s;

		xmlNodeSetContent(curr, TOXMLCHAR(num));
		save_doc(s);
		deploy();
	}

	xmlNodeSetContent(curr, xml.dummy_char);
	xmlFree(xml.dummy_char);
	xml.dummy_char = NULL;
}

static
void
handle_cont_fmt(void *data, xmlNode *curr)
{
	struct state *s = data;

	if (curr->type != XML_TEXT_NODE)
		return;

	if (coin_flip(90))
		return;

	xml.dummy_char = xmlNodeGetContent(curr);

	const char *fmt = fmt_strings[roll_dice(0, ARRSIZE(fmt_strings)-1)];

	xmlNodeSetContent(curr, TOXMLCHAR(fmt));
	save_doc(s);
	deploy();

	xmlNodeSetContent(curr, xml.dummy_char);
	xmlFree(xml.dummy_char);
	xml.dummy_char = NULL;
}

static
void
fuzz_cont_fmt(struct state *s)
{
	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		s,
		handle_cont_fmt
	);
}

static
void
fuzz_cont_bof(struct state *s)
{
	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		s,
		handle_cont_bof
	);
}

static
void
fuzz_cont_bad_ints(struct state *s)
{
	iterate_xmldoc(
		sXmlDocGetRootElement(xml.doc),
		s,
		hanlde_cont_bad_ints
	);
}

static
void
fuzz_rnd_shift(struct state *s)
{
	uint32_t bytes, start_range;

	bytes = save_doc(s);
	if (bytes <= 32) {
		bit_shift_in_range(s->payload_fd, 0, 32);
		return;
	}

	start_range = roll_dice(0, bytes-32);
	bit_shift_in_range(s->payload_fd, start_range, 32);
}

static
void
fuzz_rnd_flip(struct state *s)
{
	uint32_t bytes, start_range;
	const uint32_t maxlen = 8;

	bytes = save_doc(s);
	if (bytes <= maxlen) {
		bit_shift_in_range(s->payload_fd, 0, maxlen);
		return;
	}

	start_range = roll_dice(0, bytes-maxlen);
	bit_flip_in_range(s->payload_fd, start_range, maxlen);
}

/* Only used for debugging */
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
