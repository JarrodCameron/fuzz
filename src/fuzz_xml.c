#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <libxml/tree.h>
#include <libxml/xmlsave.h>
#include <libxml/parser.h>

#include "fuzz_xml.h"
#include "utils.h"
#include "safe.h"

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

/* Helper functions */
static void fuzz(struct state *s);
static void fuzz_buffer_overflow(struct state *s);
static xmlNodePtr sXmlDocGetRootElement(xmlDocPtr doc);
static const char *dbg_xmlElementType(xmlElementType type);
static int save_doc(struct state *s);

static struct {
	xmlDocPtr doc;
	xmlSaveCtxtPtr ctx;
} xml = {0};

/* Here we add functions used for fuzzing.
 * Each function tests for something different. */
static void (*fuzz_payloads[])(struct state *) = {
	fuzz_buffer_overflow,
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

	xml.ctx = xmlSaveToFd(state->payload_fd, NULL, 0);
	if (!xml.ctx)
		panic("Could not init context for xml doc\n");

	fuzz(state);
}

void
free_handle_xml(UNUSED struct state *state)
{
	/* TODO Check these return values */
	xmlFreeDoc(xml.doc);
	xmlCleanupParser();
	xmlSaveClose(xml.ctx);
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
void
do_buffer_overflow(struct state *s, xmlNode *ptr)
{
	for (xmlNode *curr = ptr; curr; curr = curr->next) {
		if (curr->type == XML_ELEMENT_NODE) {

			const xmlChar *old = curr->name;
			xmlNodeSetName(curr, TOXMLCHAR(BIG));
			save_doc(s);
			deploy();
			xmlNodeSetName(curr, old);

		}
		do_buffer_overflow(s, curr->children);
	}
}

static
void
fuzz_buffer_overflow(struct state *s)
{
	xmlNodePtr ptr = sXmlDocGetRootElement(xml.doc);
	do_buffer_overflow(s, ptr);
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

//	sftruncate(s->payload_fd, (off_t) bytes);

	return bytes;
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
