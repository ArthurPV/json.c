#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>

#include "json.h"

#define FATAL(msg, ...) \
	fprintf(stderr, "FATAL(%d): "msg, __LINE__, ##__VA_ARGS__); \
	exit(1);

#define UNREACHABLE(msg, ...) \
	fprintf(stderr, "UNREACHABLE(%d): "msg, __LINE__, ##__VA_ARGS__); \
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
expect_character__JSONContentIterator(struct JSONContentIterator *self, char expected);

static bool
expect_characters__JSONContentIterator(struct JSONContentIterator *self, char *expected, size_t expected_len);

static inline JSONValueString
init__JSONValueString(void);

static bool
eq__JSONValueString(const JSONValueString *self, const JSONValueString *other);

static bool
push__JSONValueString(JSONValueString *self, uint32_t c);

static inline bool
is_empty__JSONValueString(JSONValueString *self);

static inline void
deinit__JSONValueString(const JSONValueString *self);

static inline JSONValueArray
init__JSONValueArray(void);

static bool 
push__JSONValueArray(JSONValueArray *self, JSONValue value);

static void
deinit__JSONValueArray(const JSONValueArray *self);

static inline JSONValueObjectKeyValue
init__JSONValueObjectKeyValue(JSONValueString key, JSONValue value);

static void
deinit__JSONValueObjectKeyValue(const JSONValueObjectKeyValue *self);

static inline JSONValueObjectKeyValueMap
init__JSONValueObjectKeyValueMap(void);

static bool
push__JSONValueObjectKeyValueMap(JSONValueObjectKeyValueMap *self, JSONValueObjectKeyValue value);

static void
deinit__JSONValueObjectKeyValueMap(const JSONValueObjectKeyValueMap *self);

static inline JSONValueObject
init__JSONValueObject(void);

static inline bool
add_member__JSONValueObject(JSONValueObject *self, JSONValueString key, JSONValue value);

static inline void
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
parse_array_value__JSON(struct JSONContentIterator *iter);

#define PARSE_OBJECT_NO_ERROR 0
#define PARSE_OBJECT_EXPECTED_MEMBER 1
#define PARSE_OBJECT_EXPECTED_VALUE_SEPARATOR 2
#define PARSE_OBJECT_INVALID_MEMBER_NAME 3
#define PARSE_OBJECT_INVALID_MEMBER_VALUE 4
#define PARSE_OBJECT_OUT_OF_MEMORY 5

static uint32_t 
parse_object_member_value__JSON(struct JSONContentIterator *iter, JSONValueObject *object);

static JSONValueResult
parse_object_value__JSON(struct JSONContentIterator *iter);

#define PARSE_STRING_NO_ERROR 0
#define PARSE_STRING_UNKNOWN_ESCAPE 1
#define PARSE_STRING_OUT_OF_MEMORY 2
#define PARSE_STRING_INVALID_UNICODE_ESCAPE 3

static inline bool
is_hex_character__JSON(uint32_t c);

static uint32_t
parse_string_escape_value__JSON(struct JSONContentIterator *iter, JSONValueString *string);

static JSONValueResult
parse_string_value__JSON(struct JSONContentIterator *iter);

#define PARSE_NUMBER_NO_ERROR 0
#define PARSE_NUMBER_OUT_OF_MEMORY 1
#define PARSE_NUMBER_EXPECTED_TO_HAVE_DIGITS 2

static uint32_t
parse_number_minus_value__JSON(struct JSONContentIterator *iter, JSONValueString *number);

static uint32_t
parse_number_digits_value__JSON(struct JSONContentIterator *iter, JSONValueString *number);

static uint32_t
parse_number_integer_value__JSON(struct JSONContentIterator *iter, JSONValueString *number);

static uint32_t
parse_number_frac_value__JSON(struct JSONContentIterator *iter, JSONValueString *number);

static uint32_t
parse_number_exp_value__JSON(struct JSONContentIterator *iter, JSONValueString *number);

static JSONValueResult
parse_number_value__JSON(struct JSONContentIterator *iter);

static JSONValueResult
parse_true_value__JSON(struct JSONContentIterator *iter);

static JSONValueResult
parse_false_value__JSON(struct JSONContentIterator *iter);

static JSONValueResult
parse_null_value__JSON(struct JSONContentIterator *iter);

static JSONValueResult
parse_value__JSON(struct JSONContentIterator *iter);

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

	return self->content[self->count++];
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
	char c1 = get_character(self, 0);
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
	//
	// [...]
	if ((c1 & 0x80) == 0) {
		res = c1;
	} else if ((c1 & 0xE0) == 0xC0) {
		char c2 = get_character(self, 1);

		res = (c1 & 0x1F) << 6 | c2 & 0x3F;
	} else if ((c1 & 0xF0) == 0xE0) {
		char c2 = get_character(self, 1);
		char c3 = get_character(self, 2);

		res = (c1 & 0xF) << 12 | (c2 & 0x3F) << 6 | c3 & 0x3F;
	} else if ((c1 & 0xF8) == 0xF0) {
		char c2 = get_character(self, 1);
		char c3 = get_character(self, 2);
		char c4 = get_character(self, 3);

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
expect_character__JSONContentIterator(struct JSONContentIterator *self, char expected)
{
	if (current__JSONContentIterator(self) == expected) {
		next__JSONContentIterator(self);

		return true;
	}

	return false;
}

bool
expect_characters__JSONContentIterator(struct JSONContentIterator *self, char *expected, size_t expected_len)
{
	for (size_t i = 0; i < expected_len; ++i) {
		if (expected[i] != self->content[self->count++]) {
			return false;
		}
	}

	return true;
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
eq__JSONValueString(const JSONValueString *self, const JSONValueString *other)
{
	if (self->len == 0 && other->len == 0) {
		return true;
	} else if (self->len != other->len) {
		return false;
	}

	for (size_t i = 0; i < self->len; ++i) {
		if (self->buffer[i] != other->buffer[i]) {
			return false;
		}
	}

	return true;
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
		UNREACHABLE("invalid codepoint");
	}

	if (!self->buffer) {
		self->buffer = malloc(self->capacity);	
	} else if (self->len + byte_count + 1 >= self->capacity) {
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
			UNREACHABLE("Impossible byte count");
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
init__JSONValueArray(void)
{
	return (JSONValueArray){
		.buffer = NULL,
		.len = 0,
		.capacity = 8
	};
}

bool
push__JSONValueArray(JSONValueArray *self, JSONValue value)
{
	if (!self->buffer) {
		self->buffer = malloc(self->capacity);
	} else if (self->len + 1 >= self->capacity) {
		self->capacity *= 2;
		self->buffer = realloc(self->buffer, self->capacity);
	}

	if (!self->buffer) {
		return false;
	}

	self->buffer[self->len++] = value;

	return true;
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
init__JSONValueObjectKeyValue(JSONValueString key, JSONValue value)
{
	JSONValue *value_ptr = malloc(sizeof(JSONValue));

	if (!value_ptr) {
		FATAL("Out of memory");
	}

	*value_ptr = value;

	return (JSONValueObjectKeyValue){
		.key = key,
		.value = value_ptr
	};
}

void
deinit__JSONValueObjectKeyValue(const JSONValueObjectKeyValue *self)
{
	deinit__JSONValueString(&self->key);
	deinit__JSONValue(self->value);
	free(self->value);
}

JSONValueObjectKeyValueMap
init__JSONValueObjectKeyValueMap(void)
{
	return (JSONValueObjectKeyValueMap){
		.buffer = NULL,
		.len = 0,
		.capacity = 8
	};
}

bool
push__JSONValueObjectKeyValueMap(JSONValueObjectKeyValueMap *self, JSONValueObjectKeyValue value)
{
	if (!self->buffer) {
		self->buffer = malloc(sizeof(JSONValueObjectKeyValue) * self->capacity);
	} else if (self->len == self->capacity) {
		self->capacity *= 2;
		self->buffer = realloc(self->buffer, sizeof(JSONValueObjectKeyValue) * self->capacity);
	}

	if (!self->buffer) {
		return false;
	}

	self->buffer[self->len++] = value;

	return true;
}

void
deinit__JSONValueObjectKeyValueMap(const JSONValueObjectKeyValueMap *self)
{
	for (size_t i = 0; i < self->len; ++i) {
		deinit__JSONValueObjectKeyValue(&self->buffer[i]);
	}

	free(self->buffer);
}

JSONValueObject
init__JSONValueObject(void)
{
	return (JSONValueObject){
		.map = init__JSONValueObjectKeyValueMap(),
	};
}

bool
add_member__JSONValueObject(JSONValueObject *self, JSONValueString key, JSONValue value)
{
	return push__JSONValueObjectKeyValueMap(&self->map, init__JSONValueObjectKeyValue(key, value));
}

void
deinit__JSONValueObject(const JSONValueObject *self)
{
	deinit__JSONValueObjectKeyValueMap(&self->map);
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
			UNREACHABLE("unknown JSON value");
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
			UNREACHABLE("Unknown result");
	}
}

JSONValueResult
parse_array_value__JSON(struct JSONContentIterator *iter)
{
	// See RFC 8259:
	//
	// 5.  Arrays
	//
	// [...]
	//
	// array = begin-array [ value *( value-separator value ) ] end-array
	//
	// [...]
	uint32_t current = current__JSONContentIterator(iter);
	JSONValueArray array = init__JSONValueArray();

	while (current && current != ']') {
		JSONValueResult value_result = parse_value__JSON(iter);

		if (is_err__JSONValueResult(&value_result)) {
			return value_result;
		}

		const JSONValue *value = unwrap__JSONValueResult(&value_result);

		if (!push__JSONValueArray(&array, *value)) {
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_OUT_OF_MEMORY, "Out of memory");
		}

		if (!(current__JSONContentIterator(iter) == ']' || expect_character__JSONContentIterator(iter, ','))) {
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected `,`");
		}

		current = current__JSONContentIterator(iter);
	}

	return init_ok__JSONValueResult(init_array__JSONValue(array));
}

uint32_t 
parse_object_member_value__JSON(struct JSONContentIterator *iter, JSONValueObject *object)
{
	if (!expect_character__JSONContentIterator(iter, '"')) {
		return PARSE_OBJECT_EXPECTED_MEMBER;
	}

	JSONValueResult name_result = parse_string_value__JSON(iter);

	if (is_err__JSONValueResult(&name_result)) {
		return PARSE_OBJECT_INVALID_MEMBER_NAME;
	} else if (!expect_character__JSONContentIterator(iter, ':')) {
		return PARSE_OBJECT_EXPECTED_VALUE_SEPARATOR;
	}

	JSONValueResult value_result = parse_value__JSON(iter);

	if (is_err__JSONValueResult(&value_result)) {
		return PARSE_OBJECT_INVALID_MEMBER_VALUE;
	}
	
	const JSONValue *name = unwrap__JSONValueResult(&name_result);
	const JSONValue *value = unwrap__JSONValueResult(&value_result);

	if (!add_member__JSONValueObject(object, name->string, *value)) {
		return PARSE_OBJECT_OUT_OF_MEMORY;
	}

	return PARSE_OBJECT_NO_ERROR;
}

JSONValueResult
parse_object_value__JSON(struct JSONContentIterator *iter)
{
	// See RFC 8259:
	//
	// 4.  Objects
	//
	// [...]
	//
	// object = begin-object [ member *( value-separator member ) ]
    //          end-object
	// member = string name-separator value
	//
	// [...]
	if (!expect_character__JSONContentIterator(iter, '{')) {
		return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected to have `{`");
	}

	uint32_t current = current__JSONContentIterator(iter);
	JSONValueObject object = init__JSONValueObject();
	uint32_t res;

	while (current && current != '}') {
		if ((res = parse_object_member_value__JSON(iter, &object))) {
			goto handle_err;
		}

		current = current__JSONContentIterator(iter);
	}

	return init_ok__JSONValueResult(init_object__JSONValue(object));

handle_err:
	switch (res) {
		case PARSE_OBJECT_EXPECTED_MEMBER:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected member");
		case PARSE_OBJECT_EXPECTED_VALUE_SEPARATOR:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected value separator");
		case PARSE_OBJECT_INVALID_MEMBER_NAME:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Invalid member name");
		case PARSE_OBJECT_INVALID_MEMBER_VALUE:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Invalid member value");
		case PARSE_OBJECT_OUT_OF_MEMORY:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_OUT_OF_MEMORY, "Out of memory");
		default:
			UNREACHABLE("Unknown error");
	}
}

bool
is_hex_character__JSON(uint32_t c)
{
	return isdigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

uint32_t
parse_string_escape_value__JSON(struct JSONContentIterator *iter, JSONValueString *string)
{
	uint32_t current = next__JSONContentIterator(iter);
	uint32_t character_to_add;

	switch (current) {
		case '"':
			character_to_add = '\"';

			break;
		case '\\':
			character_to_add = '\\';

			break;
		case '/':
			character_to_add = '/';

			break;
		case 'b':
			character_to_add = '\b';

			break;
		case 'f':
			character_to_add = '\f';

			break;
		case 'n':
			character_to_add = '\n';

			break;
		case 'r':
			character_to_add = '\r';

			break;
		case 't':
			character_to_add = '\t';

			break;
		case 'u': {
			uint32_t u1 = next__JSONContentIterator(iter);
			uint32_t u2 = next__JSONContentIterator(iter);
			uint32_t u3 = next__JSONContentIterator(iter);
			uint32_t u4 = next__JSONContentIterator(iter);

			if (is_hex_character__JSON(u1) &&
				is_hex_character__JSON(u2) &&
				is_hex_character__JSON(u3) &&
				is_hex_character__JSON(u4)) {
				char unicode_escape_value_s[] = {
					'0', 'x', (char)u1, (char)u2, (char)u3, (char)u4, '\0'
				};

				character_to_add = strtol(&unicode_escape_value_s[0], NULL, 0);	
			} else {
				return PARSE_STRING_INVALID_UNICODE_ESCAPE;
			}

			break;
		}
		default:
			return PARSE_STRING_UNKNOWN_ESCAPE;
	}

	if (!push__JSONValueString(string, character_to_add)) {
		return PARSE_STRING_OUT_OF_MEMORY;
	}

	return PARSE_STRING_NO_ERROR;
}

JSONValueResult
parse_string_value__JSON(struct JSONContentIterator *iter)
{
	// See RFC 8259:
	//
	// 7.  Strings
	//
	// [...]
	//
	// string = quotation-mark *char quotation-mark
    // char = unescaped /
    //     escape (
    //         %x22 /          ; "    quotation mark  U+0022
    //         %x5C /          ; \    reverse solidus U+005C
    //         %x2F /          ; /    solidus         U+002F
    //         %x62 /          ; b    backspace       U+0008
    //         %x66 /          ; f    form feed       U+000C
    //         %x6E /          ; n    line feed       U+000A
    //         %x72 /          ; r    carriage return U+000D
    //         %x74 /          ; t    tab             U+0009
    //         %x75 4HEXDIG )  ; uXXXX                U+XXXX
    // escape = %x5C              ; \
    // quotation-mark = %x22      ; "
    // unescaped = %x20-21 / %x23-5B / %x5D-10FFFF
	//
	// [...]
	uint32_t res = PARSE_STRING_NO_ERROR;
	uint32_t current;
	JSONValueString string = init__JSONValueString();

	while ((current = next__JSONContentIterator(iter)) && current != '"') {
		switch (current) {
			case '\\':
				if (parse_string_escape_value__JSON(iter, &string)) {
					goto handle_err;
				}

				break;
			default:
				if (current < 0x20) {
					return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Characters greater than 0x0 and less than 0x20 are invalid");
				} else if (!push__JSONValueString(&string, current)) {
					return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_OUT_OF_MEMORY, "Out of memory");
				}
		}
	}

	return init_ok__JSONValueResult(init_string__JSONValue(string));

handle_err:
	switch (res) {
		case PARSE_STRING_UNKNOWN_ESCAPE:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Unknown escape");
		case PARSE_STRING_OUT_OF_MEMORY:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_OUT_OF_MEMORY, "Out of memory");
		case PARSE_STRING_INVALID_UNICODE_ESCAPE:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Invalid unicode escape");
		default:
			UNREACHABLE("Unknown error");
	}
}

uint32_t
parse_number_minus_value__JSON(struct JSONContentIterator *iter, JSONValueString *number)
{
	uint32_t current = current__JSONContentIterator(iter);

	if (current == '-') {
		next__JSONContentIterator(iter);

		if (!push__JSONValueString(number, current)) {
			return PARSE_NUMBER_OUT_OF_MEMORY;
		}
	}

	return PARSE_NUMBER_NO_ERROR;
}

uint32_t
parse_number_digits_value__JSON(struct JSONContentIterator *iter, JSONValueString *number)
{
	uint32_t current = current__JSONContentIterator(iter);

	while (isdigit(current)) {
		if (!push__JSONValueString(number, current)) {
			return PARSE_NUMBER_OUT_OF_MEMORY;
		}

		current = next__JSONContentIterator(iter);
	}

	return PARSE_NUMBER_NO_ERROR;
}

uint32_t
parse_number_integer_value__JSON(struct JSONContentIterator *iter, JSONValueString *number)
{
	uint32_t current = current__JSONContentIterator(iter);

	if (!isdigit(current)) {
		return PARSE_NUMBER_EXPECTED_TO_HAVE_DIGITS;
	} else if (current == '0') {
		if (!push__JSONValueString(number, current)) {
			return PARSE_NUMBER_OUT_OF_MEMORY;
		}

		next__JSONContentIterator(iter);

		return PARSE_NUMBER_NO_ERROR;
	}

	return parse_number_digits_value__JSON(iter, number);
}

uint32_t
parse_number_frac_value__JSON(struct JSONContentIterator *iter, JSONValueString *number)
{
	uint32_t current = current__JSONContentIterator(iter);

	if (current == '.') {
		if (!push__JSONValueString(number, current)) {
			return PARSE_NUMBER_OUT_OF_MEMORY;
		}

		current = next__JSONContentIterator(iter);

		if (!isdigit(current)) {
			return PARSE_NUMBER_EXPECTED_TO_HAVE_DIGITS;
		}

		return parse_number_digits_value__JSON(iter, number);
	}

	return PARSE_NUMBER_NO_ERROR;
}

uint32_t
parse_number_exp_value__JSON(struct JSONContentIterator *iter, JSONValueString *number)
{
	uint32_t current = current__JSONContentIterator(iter);

	if (current != 'e' && current != 'E') {
		return PARSE_NUMBER_NO_ERROR;
	}

	if (!push__JSONValueString(number, current)) {
		return PARSE_NUMBER_OUT_OF_MEMORY;
	}

	current = next__JSONContentIterator(iter);

	if (current == '+' || current == '-') {
		if (!push__JSONValueString(number, current)) {
			return PARSE_NUMBER_OUT_OF_MEMORY;
		}

		current = next__JSONContentIterator(iter);
	}

	if (!isdigit(current)) {
		return PARSE_NUMBER_EXPECTED_TO_HAVE_DIGITS;
	}

	return parse_number_digits_value__JSON(iter, number);
}

JSONValueResult
parse_number_value__JSON(struct JSONContentIterator *iter)
{
	// See RFC 8259:
	//
	// 6.  Numbers
	//
	// [...]
	//
	// number = [ minus ] int [ frac ] [ exp ]
    // decimal-point = %x2E       ; .
    // digit1-9 = %x31-39         ; 1-9
    // e = %x65 / %x45            ; e E
    // exp = e [ minus / plus ] 1*DIGIT
    // frac = decimal-point 1*DIGIT
	// int = zero / ( digit1-9 *DIGIT )
    // minus = %x2D               ; -
    // plus = %x2B                ; +
    // zero = %x30                ; 0
	//
	// [...]
	JSONValueString number = init__JSONValueString();
	uint32_t res = PARSE_NUMBER_NO_ERROR;

	if ((res = parse_number_minus_value__JSON(iter, &number))) {
		goto handle_err;
	} else if ((res = parse_number_integer_value__JSON(iter, &number))) {
		goto handle_err;
	} else if ((res = parse_number_frac_value__JSON(iter, &number))) {
		goto handle_err;
	} else if ((res = parse_number_exp_value__JSON(iter, &number))) {
		goto handle_err;
	}

	return init_ok__JSONValueResult(init_string__JSONValue(number));

handle_err:
	switch (res) {
		case PARSE_NUMBER_OUT_OF_MEMORY:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_OUT_OF_MEMORY, "Out of memory");
		case PARSE_NUMBER_EXPECTED_TO_HAVE_DIGITS:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected to have digits");
		default:
			UNREACHABLE("Unknown error");
	}
}

JSONValueResult
parse_true_value__JSON(struct JSONContentIterator *iter)
{
	char expected[] = "true";

	if (!expect_characters__JSONContentIterator(iter, expected, sizeof(expected) - 1)) {
		return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected `true`");
	}

	return init_ok__JSONValueResult(init_boolean__JSONValue(true));
}

JSONValueResult
parse_false_value__JSON(struct JSONContentIterator *iter)
{
	char expected[] = "false";

	if (!expect_characters__JSONContentIterator(iter, expected, sizeof(expected) - 1)) {
		return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected `false`");
	}

	return init_ok__JSONValueResult(init_boolean__JSONValue(false));
}

JSONValueResult
parse_null_value__JSON(struct JSONContentIterator *iter)
{
	char expected[] = "null";

	if (!expect_characters__JSONContentIterator(iter, expected, sizeof(expected) - 1)) {
		return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Expected `null`");
	}

	return init_ok__JSONValueResult(init_null__JSONValue());
}

JSONValueResult
parse_value__JSON(struct JSONContentIterator *iter)
{
	switch (current__JSONContentIterator(iter)) {
		case '[':
			return parse_array_value__JSON(iter);
		case '{':
			return parse_object_value__JSON(iter);
		case '"':
			return parse_string_value__JSON(iter);
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			return parse_number_value__JSON(iter);
		case 't':
			return parse_true_value__JSON(iter);
		case 'f':
			return parse_false_value__JSON(iter);
		case 'n':
			return parse_null_value__JSON(iter);
		default:
			return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "Unexpected character");
	}
}

JSONValueResult
parse__JSON(const char *content, size_t content_len)
{
	if (!content) {
		return init_err__JSONValueResult(JSON_VALUE_RESULT_ERROR_PARSE_FAILED, "No content");
	}

	struct JSONContentIterator iter = init__JSONContentIterator(content, content_len);

	return parse_object_value__JSON(&iter);
}
