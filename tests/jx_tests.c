/*
 * Copyright (c) 2018, Cory Montgomery
 */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <jx_util.h>

enum
{
	DEFAULT,
	RUN_TESTS,
	CHECK_STRING,
	SHOW_USAGE
} cmd_action;

struct
{
	unsigned char halt     : 1;
	unsigned char verbose  : 1;
} cmd_opts;

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
	{ true, "[ [ 9, 3, 2], [ 1.5, 99.9999, 0.9999 ], [ -40 ], -99.5e-4 ]" },

	{ false, "" },
	{ false, "99" },
	{ false, "[45,]" },
	{ false, "[ 32$ ]" },
	{ false, "[,1]" },
	{ false, "[5, 2]]" },
	{ false, "[ 99, 3, $, 45 ]" },
	{ false, "[ 33, 44.#2, 70 ]" },
	{ false, "[ [ 9, 3, 2], [ 1.5, 99.9999, 0.9999 ], [ -40 ], -99.5e-4, [[[[[[ 25, 35, 99, foo, 76 ]]]]]] ]" },

	{ true, "[ \"This is a test string.\", \"π = 3.15159...\", \"\\\\\", \"\\\"\", \"This is a string\\nwith multiple\\nlines.\" ]" },
	{ true, "[ \"]\", \"Another string.\", 0 ] "},
	{ true, "[ \"]]]][[[,,\\\\,,\\\"\", \"[1, 2, 3, 4, 5, 6, 7]\", \"[\", \"[1,2,3,\" ]" },

	{ true, "[ \"\\uD801\\uDC37\\u03c0\\ud801\\udc37\" ] " },

	{ false, "[ \"\\uDC37\\uD801\" ] " },
	{ false, "[ \"\\uDC37\" ] " },
	{ false, "[ \"\\uD801\" ] " },
	{ false, "[ \"\\u0000\" ] " },

	{ false, "[ \"\n\" ] " },
	{ false, "[\"]" },
	{ false, "[\"]\"" }
};

void sum_func(jx_value * number, void * ptr)
{
	double * sum;

	if (number == NULL || jxv_get_type(number) != JX_TYPE_NUMBER) {
		return;
	}

	if (ptr == NULL) {
		return;
	}

	sum = (double *)ptr;

	*sum += number->v.vf;
}

bool iterate_array(jx_value * array, void * ptr, void (*f)(jx_value * value, void * ptr))
{
	int i;
	jx_value * value;

	if (array == NULL || jxv_get_type(array) != JX_TYPE_ARRAY) {
		return false;
	}

	if (f == NULL) {
		return false;
	}

	for (i = 0; i < jxa_get_length(array); i++) {
		value = jxa_get(array, i);

		f(value, ptr);
	}

	return true;
}

bool execute_muli_part_parse_test()
{
	int i;

	jx_cntx * cntx;
	jx_value * array;

	double sum = 0.0;
	double expected_sum = 2050.0;

	const char * json[] = {
		"[ 1024, 99, 24, ",
		"-35, -788.0, 2048, -3",
		"22 ]"
	};

	printf("Testing parsing from multiple strings:\n");

	if ((cntx = jx_new()) == NULL) {
		fprintf(stderr, "Error allocating context: %s\n", strerror(errno));
		return false;
	}

	for (i = 0; i < 3; i++) {
		if (jx_parse_json(cntx, json[i], strlen(json[i])) == -1) {
			jx_free(cntx);
			fprintf(stderr, "%s\n", jx_get_error_message(cntx));
			return false;
		}
	}

	if ((array = jx_get_result(cntx)) == NULL) {
		jx_free(cntx);
		fprintf(stderr, "%s\n", jx_get_error_message(cntx));
		return false;
	}

	jx_free(cntx);

	printf("Successfully loaded array of numbers\n");

	if (!iterate_array(array, &sum, sum_func)) {
		jxv_free(array);
		fprintf(stderr, "%s\n", jx_get_error_message(cntx));
		return false;
	}

	jxv_free(array);

	if (sum != expected_sum) {
		fprintf(stderr, "Incorrect sum %.0f computed from array, expected %.0f.\n", sum, expected_sum);
		return false;
	}
	else {
		printf("Computed correct sum\n");
	}

	return true;
}

bool execute_simple_tests()
{
	int i;
	int n_tests, n_passed;
	bool output = false;

	jx_cntx * cntx;
	jx_value * value;

	printf("Executing simple tests:\n");

	n_tests = sizeof(simple_tests) / sizeof(struct json_test);	
	n_passed = 0;

	for (i = 0; i < n_tests; i++) {
		bool passed = false;

		if ((cntx = jx_new()) == NULL) {
			fprintf(stderr, "Error allocating context: %s\n", strerror(errno));
			break;
		}

		jx_parse_json(cntx, simple_tests[i].json, strlen(simple_tests[i].json));

		if ((value = jx_get_result(cntx)) != NULL) {
			jxv_free(value);
			passed = true;
		}

		if (cmd_opts.verbose || passed != simple_tests[i].should_pass) {
			if (!output) {
				printf("----------------------------------------------------------\n");
				output = true;
			}

			printf("Json: %s\nResult: %s\nExpected Result: %s\nMessage: %s\n",
				simple_tests[i].json,
				passed ? "Passed" : "Failed",
				simple_tests[i].should_pass ? "Pass" : "Fail",
				!passed ? jx_get_error_message(cntx) : "OK");

			printf("----------------------------------------------------------\n");
		}

		if (passed == simple_tests[i].should_pass) {
			n_passed++;
		}

		jx_free(cntx);
	}

	printf("%d of %d Tests Passed (%.1f%%)\n", n_passed, n_tests, ((float)n_passed / (float)n_tests) * 100);

	return n_tests == n_passed;
}

bool run_tests()
{
	if (!execute_muli_part_parse_test()) {
		return false;
	}

	printf("\n");

	if (!execute_simple_tests()) {
		return false;
	}

	return true;
}

bool validate_json_string(const char * json)
{
	jx_cntx * cntx;
	jx_value * value;

	if (json == NULL) {
		return false;
	}

	if ((cntx = jx_new()) == NULL) {
		fprintf(stderr, "Error allocating context: %s\n", strerror(errno));
		return false;
	}

	jx_parse_json(cntx, json, strlen(json));

	if ((value = jx_get_result(cntx)) == NULL) {
		jx_free(cntx);
		fprintf(stderr, "%s\n", jx_get_error_message(cntx));
		return false;
	}

	printf("JSON OK\n");

	jxv_free(value);
	jx_free(cntx);

	return true;
}

void print_usage()
{
	printf(
		"Usage: jx_tests [-apv] [-c json_string]\n"
		"   -a              Run all tests (default).\n"
		"   -c json_string  Validate that json_string is syntatically correct.\n"
		"   -p              Halt execution before stopping.\n"
		"   -v              Enable verbose output.\n"
	);
}

int main(int argc, char ** argv)
{
	char * json;
	char opt;
	int i, j, t, len;

	json = NULL;
	i = 1;

	while (i < argc) {
		if (argv[i][0] == '-') {
			t = i;
			len = strlen(argv[i]);

			for (j = 1; j < len; j++) {
				opt = argv[t][j];

				if (opt == 'a') {
					if (cmd_action != DEFAULT) {
						cmd_action = SHOW_USAGE;
						i++;
						continue;
					}

					cmd_action = RUN_TESTS;
				}
				else if (opt == 'c') {
					if (i + 1 == argc) {
						cmd_action = SHOW_USAGE;
					}

					if (cmd_action != DEFAULT) {
						cmd_action = SHOW_USAGE;
						i += 2;
						continue;
					}

					cmd_action = CHECK_STRING;

					json = argv[++i];					
				}
				else if (opt == 'v') {
					cmd_opts.verbose = true;
				}
				else if (opt == 'p') {
					cmd_opts.halt = true;
				}
				else {
					cmd_action = SHOW_USAGE;
				}
			}
		}
		else {
			cmd_action = SHOW_USAGE;
		}

		i++;
	}

	switch (cmd_action) {
		case DEFAULT:
		case RUN_TESTS:
			if (!run_tests()) {
				return 1;
			}
			break;
		case CHECK_STRING:
			if (!validate_json_string(json)) {
				return 1;
			}
			break;
		case SHOW_USAGE:
			print_usage();
			return 1;
	}

	if (cmd_opts.halt) {
		printf("\nTest for leaks on PID [%d], then press enter to exit:", getpid());
		getchar();
	}

	return 0;
}