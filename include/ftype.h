#ifndef _FTYPE_H_
#define _FTYPE_H_

enum file_type {
	file_type_csv,
	file_type_json,
	file_type_plain,
	file_type_xml,

	/* XXX This is a place holder */
	file_type_dummy,
};

enum file_type
detect_file(const char *file);

#endif /* _FTYPE_H_ */

