#include "json.h"

#include <stdio.h>
#include <stdlib.h>

int main() {
	char content[] = {
		"   {"
		"  \"name\": \"🤣😁ąTheo\\n\\u0061\\n🥶\",\n"
		"  \"age\": 16,\n"
		"  \"prices\":       [1, 2, 3, 4, 5, 6],\n"
		"  \"x\": true,\n"
		"  \"y\": false,\n"
		"  \"z\": null\n,\n"
		"  \"myobject\"      : { \"a\": 1, \"b\": 2 },\n"
		"  \"myobject2\"      : { \"a\": 1, \"b\": 2 },\n"
		"  \"myfloat\": -3.14,\n"
		"  \"scientific_number\": 3.1000e+3,\n"
		"  \"little_number\": 0.005,\n"
		"  \"cities\": [\"Paris\", \"Madrid\"],\n"
		"  \"peoples\": [{\"name\": \"Amanda\", \"age\": 25}, {\"name\": \"Jessica\", \"age\": 28}]\n"
		"}     "
	};
	size_t content_len = sizeof(content) / sizeof(*content) - 1;

	JSONValueResult result = parse__JSON(content, content_len);

	if (result.kind == JSON_VALUE_RESULT_KIND_ERR) {
		printf("Error: %s\n", result.err.msg);
	} else {
		char *result_s = to_string__JSONValue(unwrap__JSONValueResult(&result));

		printf("%s\n", result_s);

		free(result_s);
	}

	deinit__JSONValueResult(&result);
}
