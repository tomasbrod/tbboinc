/* Serialization support
 * Tomas Veronika Brod
*/
#pragma once

enum SerializeType {
		SER_END=0,  // sentinel (needed?)
		SER_VOID, // ignored when parsing strictly
		SER_INT, SER_UNSIGNED, SER_BOOL, // default to 0
		SER_INT64,
		SER_DOUBLE, // default to nan?? or 0
		SER_DOUBLEFIN, // only finite allowed, else 0
		SER_CHARAR, // char[LEN]
		SER_STRING, // std::string
		SER_XML, // char[LEN] of the inner XML (outer xml if SER_CONT)
		SER_STRUCT, // sub-structure
		SER_CUSTOM, // sub-structure but different??
		SER_VECTOR, // vector of sub-structures
};

enum SerlializeFlag {
		SER_ATTR=1, // stored in elemtn attribute
		SER_OPT=2,  // optional field (TODO)
		SER_CONT=4, // contained within it's own element
};

struct SerializeFieldDesc {
	const char* name;
	enum SerializeType type;
	size_t size;
	size_t offset;
	short unsigned flags;
	int (*custom)(void* self, const SerializeFieldDesc& descr, XML_PARSER& xp, bool writing);
};
