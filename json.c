#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>

#include "json.h"

#define FATAL(msg, ...) \
	fprintf(stderr, msg, ##__VA_ARGS__); \
	exit(1);

struct JSONContentIterator {
	const char *content;
	size_t len;
	size_t count;
};

static inline struct JSONContentIterator
init__JSONContentIterator(const char *content, size_t len);

static char
next_character__JSONContentIterator(struct JSONContentIterator *self, uint8_t byte_count);

static char
current_character__JSONContentIterator(struct JSONContentIterator *self, uint8_t byte_count);

static uint32_t
read_bytes__JSONContentIterator(struct JSONContentIterator *self, char (*get_character)(struct JSONContentIterator *self, uint8_t byte_count));

static inline uint32_t
next__JSONContentIterator(struct JSONContentIterator *self);

static inline uint32_t 
current__JSONContentIterator(struct JSONContentIterator *self);

static bool
expect__JSONContentIterator(struct JSONContentIterator *self, char expected);

static inline JSONValueString
init__JSONValueString(void);

static bool
push__JSONValueString(JSONValueString *self, uint32_t c);

static inline bool
is_empty__JSONValueString(JSONValueString *self);

static inline void
deinit__JSONValueString(const JSONValueString *self);

static inline JSONValueArray
init__JSONValueArray(struct JSONValue *buffer, size_t len);

static void
deinit__JSONValueArray(const JSONValueArray *self);

static inline JSONValueObjectKeyValue
init__JSONValueObjectKeyValue(JSONValueString key, struct JSONValue *value);

static bool
push__JSONValueObjectKeyValue(JSONValueObjectKeyValue **self);

static void
deinit__JSONValueObjectKeyValue(const JSONValueObjectKeyValue *self);

static inline JSONValueObject
init__JSONValueObject(JSONValueObjectKeyValue *buffer, size_t len);

static void
deinit__JSONValueObject(const JSONValueObject *self);

static inline JSONValue
init_number__JSONValue(double number);

static inline JSONValue
init_string__JSONValue(JSONValueString string);

static inline JSONValue
init_boolean__JSONValue(bool boolean);

static inline JSONValue
init_array__JSONValue(JSONValueArray array);

static inline JSONValue
init_object__JSONValue(JSONValueObject object);

static inline JSONValue
init_null__JSONValue();

static void
deinit__JSONValue(const JSONValue *self);

static inline JSONValueResult
init_ok__JSONValueResult(JSONValue value);

static inline JSONValueResult
init_err__JSONValueResult(enum JSONValueResultError kind, const char *msg);

static inline bool
is_err__JSONValueResult(const JSONValueResult *self);

static inline const JSONValue *
unwrap__JSONValueResult(const JSONValueResult *self);

static void
deinit__JSONValueResult(const JSONValueResult *self);

static JSONValueResult
parse_name__JSON(struct JSONContentIterator *iter);

static JSONValueResult
parse_key__JSON(struct JSONContentIterator *iter);

static JSONValueResult
parse_value__JSON(struct JSONContentIterator *iter);

static JSONValueResult
parse_content__JSON(const char *content, size_t content_len);

struct JSONContentIterator
init__JSONContentIterator(const char *content, size_t len)
{
	return (struct JSONContentIterator){
		.content = content,
		.len = len,
		.count = 0
	};
}

char
next_character__JSONContentIterator(struct JSONContentIterator *self, uint8_t byte_count)
{
	if (self->count >= self->len) {
		if (byte_count == 0) {
			return '\0';
		} else {
			FATAL("Malformed character");
		}
	}

	return self->content[++self->count];
}

char
current_character__JSONContentIterator(struct JSONContentIterator *self, uint8_t byte_count)
{
	if (self->count + byte_count >= self->len) {
		if (byte_count == 0) {
			return '\0';
		} else {
			FATAL("Malformed character");
		}
	}

	return self->content[self->count + byte_count];
}

uint32_t
read_bytes__JSONContentIterator(struct JSONContentIterator *self, char (*get_character)(struct JSONContentIterator *self, uint8_t byte_count))
{
	char c1 = get_character(self, 1);
	uint32_t res;

	// See RFC 3629:
	//
	// 3.  UTF-8 definition
	//
	// [...]
    // 
    // Char. number range  |        UTF-8 octet sequence
    //    (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
	if ((c1 & 0x80) == 0) {
		res = c1;
	} else if ((c1 & 0xE0) == 0xC0) {
		char c2 = get_character(self, 2);

		res = (c1 & 0x1F) << 6 | c2 & 0x3F;
	} else if ((c1 & 0xF0) == 0xE0) {
		char c2 = get_character(self, 2);
		char c3 = get_character(self, 3);

		res = (c1 & 0xF) << 12 | (c2 & 0x3F) << 6 | c3 & 0x3F;
	} else if ((c1 & 0xF8) == 0xF0) {
		char c2 = get_character(self, 2);
		char c3 = get_character(self, 3);
		char c4 = get_character(self, 4);

		res = (c1 & 0x7) << 18 | (c2 & 0x3F) << 12 | (c3 & 0x3F) << 6 | c4 & 0x3F;
	} else {
		FATAL("Invalid character: %c", c1);
	}

	if (res > 0x10FFFF) {
		FATAL("Invalid character (codepoint): %d", res);
	}

	return res;
}

uint32_t
next__JSONContentIterator(struct JSONContentIterator *self)
{
	return read_bytes__JSONContentIterator(self, &next_character__JSONContentIterator);
}

uint32_t
current__JSONContentIterator(struct JSONContentIterator *self)
{
	return read_bytes__JSONContentIterator(self, &current_character__JSONContentIterator);
}

bool
expect__JSONContentIterator(struct JSONContentIterator *self, char expected)
{
	if (current__JSONContentIterator(self) == expected) {
		next__JSONContentIterator(self);

		return true;
	}

	return false;
}

JSONValueString
init__JSONValueString(void)
{
	return (JSONValueString){
		.buffer = NULL,
		.len = 0,
		.capacity = 8,
	};
}

bool
push__JSONValueString(JSONValueString *self, uint32_t c)
{
	uint8_t byte_count;

	if (c <= 0x7F) {
		byte_count = 1;
	} else if (c <= 0x7FF) {
		byte_count = 2;
	} else if (c <= 0xFFFF) {
		byte_count = 3;
	} else if (c <= 0x10FFFF) {
		byte_count = 4;
	} else {
		assert(0 && "unreachable: invalid codepoint");
	}

	if (!self->buffer) {
		self->buffer = malloc(self->capacity);	
	} else if (self->len + byte_count + 1 == self->capacity) {
		self->capacity *= 2;
		self->buffer = realloc(self->buffer, self->capacity);
	}

	if (!self->buffer) {
		return false;
	}

	switch (byte_count) {
		case 1:
			self->buffer[self->len++] = c;

			break;
		case 2:
			self->buffer[self->len++] = ((c >> 6 ) & 0x1F) | 0xC0;
			self->buffer[self->len++] = (c & 0x3F) | 0x80;

			break;
		case 3:
			self->buffer[self->len++] = ((c >> 12) & 0xF) | 0xE0;
			self->buffer[self->len++] = ((c >> 6) & 0x3F) | 0x80;
			self->buffer[self->len++] = (c & 0x3F) | 0x80;

			break;
		case 4:
			self->buffer[self->len++] = ((c >> 18) & 0x7) | 0xF0;
			self->buffer[self->len++] = ((c >> 12) & 0x3F) | 0x80;
			self->buffer[self->len++] = ((c >> 6) & 0x3F) | 0x80;
			self->buffer[self->len++] = (c & 0x3F) | 0x80;

			break;
		default:
			assert(0 && "unreachable");
	}

	self->buffer[self->len] = 0;

	return true;
}

bool
is_empty__JSONValueString(JSONValueString *self)
{
	return self->len == 0;
}

void
deinit__JSONValueString(const JSONValueString *self)
{
	free(self->buffer);
}

JSONValueArray
init__JSONValueArray(struct JSONValue *buffer, size_t len)
{
	return (JSONValueArray){
		.buffer = buffer,
		.len = len
	};
}

void
deinit__JSONValueArray(const JSONValueArray *self)
{
	for (size_t i = 0; i < self->len; ++i) {
		deinit__JSONValue(&self->buffer[i]);
	}

	free(self->buffer);
}

JSONValueObjectKeyValue
init__JSONValueObjectKeyValue(JSONValueString key, struct JSONValue *value)
{
	return (JSONValueObjectKeyValue){
		.key = key,
		.value = value
	};
}

void
deinit__JSONValueObjectKeyValue(const JSONValueObjectKeyValue *self)
{
	deinit__JSONValueString(&self->key);
	deinit__JSONValue(self->value);
	free(self->value);
}

JSONValueObject
init__JSONValueObject(JSONValueObjectKeyValue *buffer, size_t len)
{
	return (JSONValueObject){
		.buffer = buffer,
		.len = len
	};
}

void
deinit__JSONValueObject(const JSONValueObject *self)
{
	for (size_t i = 0; i < self->len; ++i) {
		deinit__JSONValueObjectKeyValue(&self->buffer[i]);
	}

	free(self->buffer);
}

JSONValue
init_number__JSONValue(double number)
{
	return (JSONValue){
		.kind = JSON_VALUE_KIND_NUMBER,
		.number = number
	};
}

JSONValue
init_string__JSONValue(JSONValueString string)
{
	return (JSONValue){
		.kind = JSON_VALUE_KIND_STRING,
		.string = string
	};
}

JSONValue
init_boolean__JSONValue(bool boolean)
{
	return (JSONValue){
		.kind = JSON_VALUE_KIND_BOOLEAN,
		.boolean = boolean
	};
}

JSONValue
init_array__JSONValue(JSONValueArray array)
{
	return (JSONValue){
		.kind = JSON_VALUE_KIND_ARRAY,
		.array = array
	};
}

JSONValue
init_object__JSONValue(JSONValueObject object)
{
	return (JSONValue){
		.kind = JSON_VALUE_KIND_OBJECT,
		.object = object
	};
}

JSONValue
init_null__JSONValue()
{
	return (JSONValue){
		.kind = JSON_VALUE_KIND_NULL
	};
}

void
deinit__JSONValue(const JSONValue *self)
{
	switch (self->kind) {
		case JSON_VALUE_KIND_NUMBER:
		case JSON_VALUE_KIND_BOOLEAN:
		case JSON_VALUE_KIND_NULL:
			break;
		case JSON_VALUE_KIND_STRING:
			deinit__JSONValueString(&self->string);

			break;
		case JSON_VALUE_KIND_ARRAY:
			deinit__JSONValueArray(&self->array);

			break;
		case JSON_VALUE_KIND_OBJECT:
			deinit__JSONValueObject(&self->object);

			break;
		default:
			// UNREACHABLE:
			break;
	}
}

JSONValueResult
init_ok__JSONValueResult(JSONValue value)
{
	return (JSONValueResult){
		.kind = JSON_VALUE_RESULT_KIND_OK,
		.ok = value,
	};
}

JSONValueResult
init_err__JSONValueResult(enum JSONValueResultError kind, const char *msg)
{
	return (JSONValueResult){
		.kind = JSON_VALUE_RESULT_KIND_ERR,
		.err = {
			.kind = kind,
			.msg = msg
		},
	};
}

bool
is_err__JSONValueResult(const JSONValueResult *self)
{
	return self->kind == JSON_VALUE_RESULT_KIND_ERR;
}

const JSONValue *
unwrap__JSONValueResult(const JSONValueResult *self)
{
	assert(self->kind == JSON_VALUE_RESULT_KIND_ERR && "expected ok");

	return &self->ok;
}

void
deinit__JSONValueResult(const JSONValueResult *self)
{
	switch (self->kind) {
		case JSON_VALUE_RESULT_KIND_OK:
			deinit__JSONValue(&self->ok);

			break;
		case JSON_VALUE_RESULT_KIND_ERR:
			break;
		default:
			// UNREACHABLE:
			break;
	}
}

JSONValueResult
parse_name__JSON(struct JSONContentIterator *iter)
{
	JSONValueString name = init__JSONValueString();
	uint32_t current = current__JSONContentIterator(iter);

	while (current && current != '"') {
		if (!push__JSONValueString(&name, current)) {
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_OUT_OF_MEMORY, "Out of memory");
		}

		current = next__JSONContentIterator(iter);
	}

	if (!expect__JSONContentIterator(iter, '"')) {
		return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected `\"`");
	}

	return init_ok__JSONValueResult(init_string__JSONValue(name));
}

JSONValueResult
parse_key__JSON(struct JSONContentIterator *iter)
{
	assert(expect__JSONContentIterator(iter, '"') && "unreachable");

	JSONValueResult name_result = parse_name__JSON(iter);

	if (is_err__JSONValueResult(&name_result)) {
		return name_result;
	}

	if (!expect__JSONContentIterator(iter, ':')) {
		return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected `:`");
	}

	return name_result;
}

JSONValueResult
parse_value__JSON(struct JSONContentIterator *iter)
{
}

JSONValueResult
parse_content__JSON(const char *content, size_t content_len)
{
	struct JSONContentIterator iter = init__JSONContentIterator(content, content_len);

	next__JSONContentIterator(&iter); // Skip `{`

	char current = '\0';
	JSONValueObjectKeyValue *object_key_value = NULL;
	
	while ((current = next__JSONContentIterator(&iter))) {
		switch (current) {
			case '\"': {
				JSONValueResult key_result = parse_key__JSON(&iter);

				if (is_err__JSONValueResult(&key_result)) {
					return key_result;
				}

				JSONValueResult value_result = parse_value__JSON(&iter);

				if (is_err__JSONValueResult(&value_result)) {
					return value_result;
				}

				break;
			}
			default:
				if (isspace(current)) {
					continue;
				}

				return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Invalid character");
		}
	}
}

JSONValueResult
parse__JSON(const char *content, size_t content_len)
{
	if (!content) {
		return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "No content");
	}

	char *begin = strchr(content, '{');

	if (!begin) {
		return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected `{`");
	}

	content_len -= begin - content;
	content = begin;

	return parse_content__JSON(content, content_len);
}
