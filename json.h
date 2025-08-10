#ifndef JSON_H
#define JSON_H

#include <stddef.h>
#include <stdbool.h>

enum JSONValueKind {
	JSON_VALUE_KIND_NUMBER,
	JSON_VALUE_KIND_STRING,
	JSON_VALUE_KIND_BOOLEAN,
	JSON_VALUE_KIND_ARRAY,
	JSON_VALUE_KIND_OBJECT,
	JSON_VALUE_KIND_NULL
};

typedef struct JSONValueString {
	char *buffer;
	size_t len;
	size_t capacity;
} JSONValueString;

typedef struct JSONValueArray {
	struct JSONValue *buffer;
	size_t len;
	size_t capacity;
} JSONValueArray;

typedef struct JSONValueObjectKeyValue {
	JSONValueString key;
	struct JSONValue *value;
} JSONValueObjectKeyValue;

typedef struct JSONValueObjectBucket {
	JSONValueObjectKeyValue pair;
	struct JSONValueObjectBucket *next;
} JSONValueObjectBucket;

typedef struct JSONValueObjectKeyValueMap {
	JSONValueObjectBucket **buckets;
	size_t len;
	size_t capacity;
} JSONValueObjectKeyValueMap;

typedef struct JSONValueObject {
	JSONValueObjectKeyValueMap map;
} JSONValueObject;

typedef struct JSONValue {
	enum JSONValueKind kind;
	union {
		JSONValueString number;
		JSONValueString string;
		bool boolean;
		JSONValueArray array;
		JSONValueObject object;
	};
} JSONValue;

char *
to_string__JSONValue(const JSONValue *self);

enum JSONValueResultKind {
	JSON_VALUE_RESULT_KIND_OK,
	JSON_VALUE_RESULT_KIND_ERR
};

enum JSONValueResultError {
	JSON_VALUE_RESULT_ERROR_PARSE_FAILED,
	JSON_VALUE_RESULT_ERROR_OUT_OF_MEMORY
};

typedef struct JSONValueResult {
	enum JSONValueResultKind kind;
	union {
		JSONValue ok;
		struct {
			enum JSONValueResultError kind;
			const char *msg;
		} err;
	};
} JSONValueResult;

bool
is_err__JSONValueResult(const JSONValueResult *self);

const JSONValue *
unwrap__JSONValueResult(const JSONValueResult *self);

void
deinit__JSONValueResult(const JSONValueResult *self);

JSONValueResult
parse__JSON(const char *content, size_t content_len);

#endif // JSON_H
