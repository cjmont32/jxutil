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
	/* valid inputs */
	{ true, "[]" },
/*	{ true, "[ ]" },
	{ true, "[[[]]]" },

	{ true, " [ 5 ] " },
	{ true, " [ 1024 ] " },
	{ true, "[ -1.5e4, -1.5, -1, 0, 0.5, 2, 3.14, 1024 ]" },
	{ true, "[[[1024]]]" },
	{ true, "[[[5, 9 ]]]" },
	{ true, "[ [ 9, 3, 2], [ 1.5, 99.9999, 0.9999 ], [ -40 ] ]"},
*/

	/* invalid inputs */
/*
	{ false, "" },
	{ false, "99" },
	{ false, "[" },
	{ false, "]" },
	{ false, "[,]" },
	{ false, "[45,]" },
	{ false, "[,1]" },
	{ false, "[]]" },
	{ false, "[5, 2]]" },
	{ false, "[ 99, 3, $, 45 ]" },
	{ false, "[ 33, 44.#2, 70 ]" }
*/
};

bool execute_simple_tests()
{
	int i;
	int n_tests;

	bool tests_passed = true;

	n_tests = sizeof(simple_tests) / sizeof(struct json_test);	

	for (i = 0; i < n_tests; i++) {
		jx_cntx * cntx;
		bool passed;

		cntx = jx_new();

		if (cntx == NULL) {
			fprintf(stderr, "%s: %s\n", __FUNCTION__, jx_get_error_message());
			return false;
		}

		if (jx_parse(cntx, simple_tests[i].json, strlen(simple_tests[i].json)) == 1) {
			passed = true;
		}

		if (passed != simple_tests[i].should_pass) {
			tests_passed = false;
		}

		printf("----------------------------------------------------------\n");
		printf("Json: %s\nResult: %s\nExpected Result: %s\nStatus: %s\n",
			simple_tests[i].json,
			passed ? "Passed" : "Failed",
			simple_tests[i].should_pass ? "Pass" : "Fail",
			jx_get_error_message());
		printf("----------------------------------------------------------\n");

		jx_free(cntx);
	}

	return tests_passed;
}

jx_value * gen_fibonacci_array(unsigned int n_terms)
{
	jx_value * array;

	unsigned int i, n;
	unsigned int t1, t2, t3;

	bool success;

	if ((array = jxa_new(10)) == NULL) {
		fprintf(stderr, "%s: %s\n", __FUNCTION__, jx_get_error_message());
		return NULL;
	}

	n = 0;
	t1 = 1;
	t2 = 2;

	while (n++ < n_terms) {
		if (n <= 2) {
			if (!(success = jxa_push_number(array, n))) {
				break;
			}

			continue;
		}

		t3 = t2 + t1;
		t1 = t2;
		t2 = t3;

		if (!(success = jxa_push_number(array, t3))) {
			break;
		}
	}

	if (!success) {
		jxv_free(array);
		fprintf(stderr, "%s: %s\n", __FUNCTION__, jx_get_error_message());
		return NULL;
	}

	return array;
}

bool iterate_array(jx_value * array, void * ptr, void (*f)(void * ptr, jx_value * value))
{
	int i;

	if (array == NULL || jxv_get_type(array) != JX_TYPE_ARRAY) {
		return false;
	}

	if (f == NULL) {
		return false;
	}

	for (i = 0; i < jxa_get_length(array); i++) {
		jx_value * value;

		value = jxa_get(array, i);

		f(ptr, value);
	}

	return true;
}

void even_sum(void * ptr, jx_value * value)
{
	unsigned long * sum;
	unsigned long term;

	if (ptr == NULL || value == NULL || jxv_get_type(value) != JX_TYPE_NUMBER) {
		return;
	}

	term = (unsigned long)value->v.vf;

	if (term % 2 == 0) {
		sum = (unsigned long *)ptr;

		*sum += (unsigned long)value->v.vf;
	}
}

bool execute_dynamic_array_test()
{
	jx_value * array;

	unsigned long sum = 0;

	array = gen_fibonacci_array(30);

	if (array == NULL) {
		fprintf(stderr, "%s [%d]: %s\n", __FUNCTION__, __LINE__, jx_get_error_message());
		return false;
	}

	if (!iterate_array(array, &sum, even_sum)) {
		fprintf(stderr, "%s [%d]: %s\n", __FUNCTION__, __LINE__, jx_get_error_message());
		return false;
	}

	printf("%s: sum of even terms [%lu]\n", __FUNCTION__, sum);

	jxv_free(array);

	return true;
}

int main()
{
	bool tests_passed;

	execute_dynamic_array_test();

	tests_passed = execute_simple_tests();

	if (tests_passed) {
		printf("All tests passed!\n");
	}

	return 0;
}