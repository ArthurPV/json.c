#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "json.h"

struct JSONContentIterator {
	const char *content;
	size_t len;
	size_t count;
};

static inline struct JSONContentIterator
init__JSONContentIterator(const char *content, size_t len);

char
next__JSONContentIterator(struct JSONContentIterator *self);

char
current__JSONContentIterator(struct JSONContentIterator *self);

static inline JSONValueString
init__JSONValueString(char *buffer, size_t len);

static inline void
deinit__JSONValueString(const JSONValueString *self);

static inline JSONValueArray
init__JSONValueArray(struct JSONValue *buffer, size_t len);

static void
deinit__JSONValueArray(const JSONValueArray *self);

static inline JSONValueObjectKeyValue
init__JSONValueObjectKeyValue(JSONValueString key, struct JSONValue *value);

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

static void
deinit__JSONValueResult(const JSONValueResult *self);

static JSONValueResult
parse_key__JSON(char **current);

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
next__JSONContentIterator(struct JSONContentIterator *self)
{
	if (self->count >= self->len) {
		return '\0';
	}

	return self->content[++self->count];
}

char
current__JSONContentIterator(struct JSONContentIterator *self)
{
	if (self->count >= self->len) {
		return '\0';
	}

	return self->content[self->count];
}

JSONValueString
init__JSONValueString(char *buffer, size_t len)
{
	return (JSONValueString){
		.buffer = buffer,
		.len = len
	};
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
parse_content__JSON(const char *content, size_t content_len)
{
	struct JSONContentIterator iter = init__JSONContentIterator(content, content_len);

	next__JSONContentIterator(&iter); // Skip `{`

	char current = '\0';
	
	while ((current = next__JSONContentIterator(&iter))) {
		switch (current) {
			case '\"':
				break;
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
