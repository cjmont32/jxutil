/*
 * Copyright (c) 2018, Cory Montgomery
 */

#include <jx_util.h>
#include <stdio.h>
#include <string.h>

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
		return false;
	}

	if (jx_parse(cntx, json, strlen(json)) == -1 || (value = jx_get_result(cntx)) == NULL) {
		jx_free(cntx);
		return false;
	}

	jx_free(cntx);

	return value;
}

bool execute_simple_tests()
{
	int i;
	int n_tests;
	bool tests_passed;

	jx_value * value;

	n_tests = sizeof(simple_tests) / sizeof(struct json_test);	
	tests_passed = true;

	printf("----------------------------------------------------------\n");

	for (i = 0; i < n_tests; i++) {
		bool passed = false;

		if ((value = parse_json_string(simple_tests[i].json)) != NULL) {
			passed = true;
			jxv_free(value);
		}

		printf("Json: %s\nResult: %s\nExpected Result: %s\nMessage: %s\n",
			simple_tests[i].json,
			passed ? "Passed" : "Failed",
			simple_tests[i].should_pass ? "Pass" : "Fail",
			!passed ? jx_get_error_message() : "OK");

		if (passed != simple_tests[i].should_pass) {
			tests_passed = false;
			break;
		}

		if (i < n_tests - 1) {
			printf("----------------------------------------------------------\n");
		}
	}

	printf("----------------------------------------------------------\n");

	return tests_passed;
}

int main()
{
	bool tests_passed;

	tests_passed = execute_simple_tests();

	if (tests_passed) {
		printf("All tests passed!\n");
	}

	return 0;
}