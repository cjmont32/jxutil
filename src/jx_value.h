/*
 * jx_value.h
 * Copyright (c) 2018, Cory Montgomery
 * All Rights Reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    1. Redistributions of source code must retain the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer.
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials
 *       provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <stdbool.h>

typedef enum
{
    JX_TYPE_UNDEF,
    JX_TYPE_NULL,
    JX_TYPE_ARRAY,
    JX_TYPE_OBJECT,
    JX_TYPE_NUMBER,
    JX_TYPE_BOOL,
    JX_TYPE_STRING,
    JX_TYPE_PTR
} jx_type;

struct jx_value_t;
struct jx_trie_node_t;

#ifdef JX_VALUE_INTERNAL

typedef struct jx_value_t
{
    union {
        bool vb;
        double vf;
        void *vp;
        void **vpp;
    } v;

    jx_type type;

    size_t size;
    size_t length;

    bool error;
} jx_value;

typedef struct jx_trie_node_t
{
    struct jx_trie_node_t *child_nodes[16];

    struct jx_value_t *value;

    char byte;
} jx_trie_node;

#else

typedef struct jx_value_t jx_value;
typedef struct jx_trie_node_t jx_trie_node;
typedef struct jx_iter_t jx_iter;

#endif

typedef void (*jxd_iter_cb)(const char *key, jx_value *value, void *ptr);

jx_type jxv_get_type(jx_value *value);

jx_value *jxa_new(size_t capacity);
size_t jxa_get_length(jx_value *array);
jx_type jxa_get_type(jx_value *array, size_t i);
jx_value *jxa_get(jx_value *array, size_t i);
bool jxa_push(jx_value *array, jx_value *value);
jx_value *jxa_pop(jx_value *array);
jx_value *jxa_top(jx_value *array);
bool jxa_push_number(jx_value *array, double num);
double jxa_get_number(jx_value *arr, size_t i);
bool jxa_push_ptr(jx_value *array, void *ptr);
void *jxa_get_ptr(jx_value *array, size_t i);
void *jxv_get_ptr(jx_value *value);

jx_value *jxd_new();
bool jxd_put(jx_value *dict, char *key, jx_value *value);
jx_value *jxd_get(jx_value *dict, char *key);
jx_value *jxd_del(jx_value *dict, char *key);
bool jxd_del_free(jx_value *dict, char *key);
bool jxd_has(jx_value *dict, char *key);
jx_type jxd_get_type(jx_value *dict, char *key, bool *found);
bool jxd_put_number(jx_value *dict, char *key, double num);
double jxd_get_number(jx_value *dict, char *key, bool *found);
bool jxd_put_bool(jx_value *dict, char *key, bool value);
bool jxd_get_bool(jx_value *dict, char *key, bool *found);
bool jxd_put_string(jx_value *dict, char *key, char *value);
char *jxd_get_string(jx_value *dict, char *key, bool *found);
bool jxd_iterate(jx_value *dict, jxd_iter_cb cb_func, void *ptr);

jx_value *jxv_number_new(double num);
double jxv_get_number(jx_value *value);

jx_value *jxs_new(const char *src);
bool jxs_append_jxs(jx_value *dst, jx_value *src);
bool jxs_append_str(jx_value *dst, char *src);
bool jxs_append_fmt(jx_value *dst, char *fmt, ...);
bool jxs_append_chr(jx_value *dst, char c);
bool jxs_push(jx_value *str, char c);
char jxs_top(jx_value *str);
char jxs_pop(jx_value *str);
char *jxs_get_str(jx_value *str);

jx_value *jxv_null();
bool jxv_is_null(jx_value *value);

jx_value *jxv_bool_new(bool value);
bool jxv_get_bool(jx_value *value);

bool jxv_is_valid(jx_value *value);

void jxv_free(jx_value *value);