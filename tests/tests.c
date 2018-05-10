/*
 * Copyright (c) 2018, Cory Montgomery
 */

#include <jx_util.h>
#include <stdio.h>
#include <string.h>

static bool verbose = false;

typedef enum
{
	RUN_TESTS,
	CHECK_STRING
} test_action;

struct json_test
{
	bool should_pass;	
	const char * const json;
} simple_tests[] = {
	{ true, "[]" },
	{ true, "[[]]" },
	{ true, "[ [], [], [[[]]] ]" },

	{ false, "" },
	{ false, "[" },
	{ false, "[[]" },
	{ false, "]" },
	{ false, "[]]" },
	{ false, "[,]" },
	{ false, "[ [], ] " },
	{ false, "[ [,] ] " },
	{ false, "[ [], [] [], [] ] " },

	{ true, " [ 5 ] " },
	{ true, " [ 1024 ] " },
	{ true, "[ -10E+6, -1.5e4, -1.5, -1, -1E-6, 0, 1.5, 2, 3.14, 1024, 10e+6 ]" },
	{ true, "[ -1.5e4, -1.5, -1, 0, 0.5, 2, 3.14, 1024 ]" },
	{ true, "[[[1024]]]" },
	{ true, "[[[5, 9 ]]]" },
	{ true, "[ [ 9, 3, 2], [ 1.5, 99.9999, 0.9999 ], [ -40 ], -99.5e-4 ]"},

	{ false, "" },
	{ false, "99" },
	{ false, "[45,]" },
	{ false, "[ 32$ ]" },
	{ false, "[,1]" },
	{ false, "[5, 2]]" },
	{ false, "[ 99, 3, $, 45 ]" },
	{ false, "[ 33, 44.#2, 70 ]" },
	{ false, "[ [ 9, 3, 2], [ 1.5, 99.9999, 0.9999 ], [ -40 ], -99.5e-4, [[[[[[ 25, 35, 99, foo, 76 ]]]]]] ]"},
};

jx_value * parse_json_string(const char * json)
{
	jx_cntx * cntx;
	jx_value * value;

	cntx = jx_new();

	if (cntx == NULL) {
		return NULL;
	}

	if (jx_parse(cntx, json, strlen(json)) == -1 || (value = jx_get_result(cntx)) == NULL) {
		jx_free(cntx);
		return NULL;
	}

	jx_free(cntx);

	return value;
}

void execute_simple_tests()
{
	int i;
	int n_tests, n_passed;
	bool output = false;

	jx_value * value;

	n_tests = sizeof(simple_tests) / sizeof(struct json_test);	
	n_passed = 0;

	for (i = 0; i < n_tests; i++) {
		bool passed = false;

		if ((value = parse_json_string(simple_tests[i].json)) != NULL) {
			jxv_free(value);
			passed = true;
		}

		if (verbose || passed != simple_tests[i].should_pass) {
			if (!output) {
				printf("----------------------------------------------------------\n");
				output = true;
			}

			printf("Json: %s\nResult: %s\nExpected Result: %s\nMessage: %s\n",
				simple_tests[i].json,
				passed ? "Passed" : "Failed",
				simple_tests[i].should_pass ? "Pass" : "Fail",
				!passed ? jx_get_error_message() : "OK");

			printf("----------------------------------------------------------\n");
		}

		if (passed == simple_tests[i].should_pass) {
			n_passed++;
		}
	}

	printf("%d of %d Tests Passed (%.1f%%)\n", n_passed, n_tests, ((float)n_passed / (float)n_tests) * 100);
}

bool validate_json_string(const char * json)
{
	jx_value * value;

	if (json == NULL) {
		return false;
	}

	if ((value = parse_json_string(json)) == NULL) {
		fprintf(stderr, "%s\n", jx_get_error_message());
		return false;
	}

	printf("JSON OK\n");

	jxv_free(value);

	return true;
}

void print_useage()
{
	printf("tests [-v] [-c json_string] --run_tests\n");
}

int main(int argc, char ** argv)
{
	test_action action;
	char * json;
	int i;

	action = RUN_TESTS;
	json = NULL;
	i = 1;

	while (i < argc) {
		if (strcmp(argv[i], "-v") == 0) {
			verbose = true;
		}
		else if (strcmp(argv[i], "--run_tests") == 0) {
			action = RUN_TESTS;
		}
		else {
			if (i + 1 == argc) {
				print_useage();
				return 1;
			}

			if (strcmp(argv[i], "-c") == 0) {
				action = CHECK_STRING;
				json = argv[++i];
			}
			else {
				print_useage();
				return 1;
			}
		}

		i++;
	}

	switch (action) {
		case RUN_TESTS:
			execute_simple_tests();
			break;
		case CHECK_STRING:
			validate_json_string(json);
			break;
	}

	return 0;
}