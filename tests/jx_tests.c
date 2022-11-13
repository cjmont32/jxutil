/*
 * Copyright (c) 2018, Cory Montgomery
 */

#include <stdio.h>
#include <fcntl.h>
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
    { true, "[ true, false, null, null, false, true, [true,false,null,null,false], null ]" },

    { false, "" },
    { false, "99" },
    { false, "[45,]" },
    { false, "[ 32$ ]" },
    { false, "[,1]" },
    { false, "[5, 2]]" },
    { false, "[ 99, 3, $, 45 ]" },
    { false, "[ 33, 44.#2, 70 ]" },
    { false, "[ [ 9, 3, 2], [ 1.5, 99.9999, 0.9999 ], [ -40 ], -99.5e-4, [[[[[[ 25, 35, 99, foo, 76 ]]]]]] ]" },

    { true, "[ \"\", \"This is a test string.\", \"π = 3.15159...\", \"\\\\\", \"\\\"\", \"This is a string\\nwith multiple\\nlines.\" ]" },
    { true, "[ \"]\", \"Another string.\", 0 ] "},
    { true, "[ \"]]]][[[,,\\\\,,\\\"\", \"[1, 2, 3, 4, 5, 6, 7]\", \"[\", \"[1,2,3,\" ]" },

    { true, "[ \"\\uD801\\uDC37\\u03c0\\ud801\\udc37\" ] " },
    { false, "[ \"\\uD83D\\uDC7E = 👾\", 👾 ]" },

    { false, "[ \"\\uDC37\\uD801\" ] " },
    { false, "[ \"\\uDC37\" ] " },
    { false, "[ \"\\uD801\" ] " },
    { false, "[ \"\\u0000\" ] " },
    { false, "[1, 2, 3.14, 👾, 5]" },
    { false, "[ \"\x7f\" ]" },
    { false, "[ \"\\u007f\" ] " },
    { false, "[ \x06 ]" },
    { false, "[ π ]" }, /* Enable extension JX_EXT_UTF8_PI */
    { false, "[ \x80\xcf ] " },

    { true, "{}" },
    { true, "{ \"\" : \"\" }" },
    { true, "{ \"[}}{]][,[[[[[}}}\" : \",\\\"}[]][\" } " },
    { true, "{ \"π\" : 3.14159, \"boolean\": true, \"array\": [true, false, 0.1, \"\", {}], \"object\": {} }" },
    { true, "[ {}, { \"\" : \"\" }, { \"true\": true, \"false\": false, \"null\": null } ] " },

    { false, "{,}" },
    { false, "{:}" },
    { false, "{:,}" },
    { false, "{\":,5\":,}" },
    { false, "{\"\"::32}" },
    { false, "{ 34 : \"\" }" },
    { false, "{  : \"\" }" },
    { false, "{ \"\" : }" },
    { false, "{ \"\" : 34234, }" },
    { false, "{ \"\" \"\": \"\" }" },
    { false, "{ \"\" : \"\" \"\" }" },
    { false, "{ \"\" : \"\", \"\" }" },
    { false, "{ \"\" : \"\", [] }" },
    { false, "[1, 2, 3, } " },
    { false, "{ \"\": \"\" ] " },
    { false, "{" },
    { false, "{ \"\" " },
    { false, "{ \"\" : " }
};

void sum_func(jx_value *number, void *ptr)
{
    double *sum;

    if (number == NULL || jxv_get_type(number) != JX_TYPE_NUMBER) {
        return;
    }

    if (ptr == NULL) {
        return;
    }

    sum = (double *)ptr;

    *sum += jxv_get_number(number);
}

bool iterate_array(jx_value *array, void *ptr, void (*f)(jx_value *value, void *ptr))
{
    int i;
    jx_value *value;

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

    jx_cntx *cntx;
    jx_value *array;

    double sum = 0.0;
    double expected_sum = 2050.0;

    const char * json[] = {
        "[ 1024, 99, 24, ",
        "-35, -788.0, 2048, -3",
        "22 ]"
    };

    printf("Testing parsing from multiple buffers:\n");

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

jx_value *parse_json_from_file(const char *filename)
{
    jx_cntx *cntx;

    jx_value *obj;

    if ((cntx = jx_new()) == NULL) {
        fprintf(stderr, "Error allocating context: %s\n", strerror(errno));
        return NULL;
    }

    obj = jx_obj_from_file(cntx, filename);

    if (obj == NULL) {
        if (jx_get_error(cntx) == JX_ERROR_LIBC) {
            fprintf(stderr, "Error reading file [%s]\n", strerror(errno));

            if (errno == ENOENT) {
                printf("Hint: Try again from the root of the project tree.\n");
            }
        }
        else {
            fprintf(stderr, "%s\n", jx_get_error_message(cntx));
        }
    }

    jx_free(cntx);

    return obj;
}

jx_value *parse_json_from_file2(const char *filename)
{
    jx_cntx *cntx;
    jx_value *object;

    int fd, bytes_read;

    char buf[2048];

    if ((fd = open(filename, O_RDONLY)) == -1 && errno == ENOENT) {
        printf("Error: %s\n", strerror(errno));
        printf("Hint: Try again from the root of the project tree.\n");
        return NULL;
    }

    if (fd == -1) {
        printf("%s\n", strerror(errno));
        return NULL;
    }

    if ((cntx = jx_new()) == NULL) {
        fprintf(stderr, "Error allocating context: %s\n", strerror(errno));
        return NULL;
    }

    while ((bytes_read = read(fd, buf, 2048)) > 0) {
        char *ptr = buf;

        // Feed json to parser one-byte at a time as a stress test
        // BAD practice in general
        while (bytes_read--) {
            jx_parse_json(cntx, ptr++, 1);
        }
    }

    if (bytes_read == -1) {
        jx_free(cntx);
        close(fd);
        fprintf(stderr, "Error reading file: %s\n", strerror(errno));
        return NULL;
    }

    object = jx_get_result(cntx);

    if (object == NULL) {
        fprintf(stderr, "%s\n", jx_get_error_message(cntx));
    }

    jx_free(cntx);

    return object;
}

bool strings_test()
{
    jx_value *arr;

    int i, max;
    bool success;

    const char *string_tests[] = {
        "",
        ",{\"foo\":{}}],[[[\\,,,]",

        "Arrival,2016,\"Denis Villeneuve\"\n"
        "Inception,2010,\"Christopher Nolan\"\n"
        "Interstellar,2014,\"Christopher Nolan\"\n"
        "\"Rogue One\",2016,\"Gareth Edwards\"",

        "\xcf\x80 = 3.14159....",
        "\xe2\x82\xac\x31\x30\x30 is approximately $120",
        "Any unicode character should be fine, e.g. [\xf0\x90\x90\xb7].",
        "Would you prefer \xe2\x98\x95 or \xf0\x9f\x8d\xba or \xf0\x9f\x8d\xb7?"
    };

    printf("Testing parsing array of strings:\n");

    if ((arr = parse_json_from_file("tests/json/strings.json")) == NULL) {
        return false;
    }

    max = sizeof(string_tests) / sizeof(char *) - 1;
    success = true;

    for (i = 0; i < jxa_get_length(arr); i++) {
        char *str;

        str = jxs_get_str(jxa_get(arr, i));

        if (str == NULL) {
            fprintf(stderr, "Error: Non-string type found in array.\n");
            success = false;
            break;
        }

        if (i > max) {
            fprintf(stderr, "Errno: string at index %d not found in test strings [%s].\n", i, str);
            success = false;
            break;
        }

        if (strcmp(string_tests[i], str) != 0) {
            fprintf(stderr, "Error: Strings didn't match [%s:%s].\n", string_tests[i], str);
            success = false;
            break;
        }
    }

    if (success)
        printf("All strings passed\n");

    jxv_free(arr);

    return success;
}

bool execute_ext_utf8_pi_test()
{
    jx_cntx *cntx;

    printf("Testing extension [JX_EXT_UTF8_PI]\n");

    if ((cntx = jx_new()) == NULL) {
        fprintf(stderr, "Error allocating context: %s\n", strerror(errno));
        return false;
    }

    jx_set_extensions(cntx, JX_EXT_UTF8_PI);

    jx_parse_json(cntx, "[\xcf", 2);

    if (jx_parse_json(cntx, "\x80]", 2) != 1) {
        fprintf(stderr, "Error: %s\n", jx_get_error_message(cntx));
        jx_free(cntx);
        return false;
    }

    printf("Success\n");

    jx_free(cntx);

    return true;
}

bool execute_simple_tests()
{
    int i;
    int n_tests, n_passed;
    bool output = false;

    jx_cntx *cntx;
    jx_value *value;

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
    if (!execute_simple_tests()) {
        return false;
    }

    printf("\n");

    if (!strings_test()) {
        return false;
    }

    printf("\n");

    if (!execute_muli_part_parse_test()) {
        return false;
    }

    printf("\n");

    if (!execute_ext_utf8_pi_test()) {
        return false;
    }

    return true;
}

bool validate_json_string(const char * json)
{
    jx_cntx *cntx;
    jx_value *value;

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

int main(int argc, char **argv)
{
    char *json;
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