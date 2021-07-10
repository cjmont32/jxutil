/*
 * jx_value.c
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

#define JX_VALUE_INTERNAL

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

#include <jx_value.h>

jx_type jxv_get_type(jx_value * value)
{
    if (value == NULL) {
        return JX_TYPE_UNDEF;
    }

    return value->type;
}

jx_value * jxv_new(jx_type type)
{
    jx_value * value;

    if ((value = calloc(1, sizeof(jx_value))) == NULL) {
        return NULL;
    }

    value->type = type;

    return value;
}

jx_value * jxa_new(size_t capacity)
{
    jx_value * array;

    if ((array = jxv_new(JX_TYPE_ARRAY)) == NULL) {
        return NULL;
    }

    if ((array->v.vpp = malloc(sizeof(jx_value *) * capacity)) == NULL) {
        free(array);
        return NULL;
    }

    array->size = capacity;

    return array;
}

size_t jxa_get_length(jx_value * array)
{
    if (array == NULL || array->type != JX_TYPE_ARRAY) {
        return 0;
    }

    return array->length;
}

jx_type jxa_get_type(jx_value * array, size_t i)
{
    jx_value * value;

    if ((value = jxa_get(array, i)) == NULL) {
        return JX_TYPE_UNDEF;
    }

    return value->type;
}

jx_value * jxa_get(jx_value * array, size_t i)
{
    if (array == NULL || array->type != JX_TYPE_ARRAY || i >= array->length) {
        return NULL;
    }

    return array->v.vpp[i];
}

bool jxa_push(jx_value * array, jx_value * value)
{
    if (array == NULL || array->type != JX_TYPE_ARRAY) {
        return false;
    }

    if (array->length == array->size) {
        void ** newArray;
        size_t newSize;

        newSize = array->size * 2;
        newArray = realloc(array->v.vpp, sizeof(jx_value *) * newSize);

        if (newArray == NULL) {
            return false;
        }

        array->v.vpp = newArray;
        array->size = newSize;
    }

    array->v.vpp[array->length++] = value;

    return true;
}

jx_value * jxa_pop(jx_value * array)
{
    if (array == NULL || array->type != JX_TYPE_ARRAY || array->length == 0) {
        return NULL;
    }

    return array->v.vpp[--array->length];
}

jx_value * jxa_top(jx_value * array)
{
    if (array == NULL || array->type != JX_TYPE_ARRAY || array->length == 0) {
        return NULL;
    }

    return array->v.vpp[array->length - 1];
}

bool jxa_push_number(jx_value * array, double num)
{
    jx_value * value;

    if (array == NULL || array->type != JX_TYPE_ARRAY) {
        return false;
    }

    if ((value = jxv_number_new(num)) == NULL) {
        return false;
    }

    if (!jxa_push(array, value)) {
        jxv_free(value);
        return false;
    }

    return true;
}

double jxa_get_number(jx_value * array, size_t i)
{
    jx_value * value;

    value = jxa_get(array, i);

    if (value == NULL || value->type != JX_TYPE_NUMBER) {
        return 0.0;
    }

    return value->v.vf;
}

bool jxa_push_ptr(jx_value * array, void * ptr)
{
    jx_value * value;

    if (array == NULL || array->type != JX_TYPE_ARRAY) {
        return false;
    }

    if ((value = jxv_new(JX_TYPE_PTR)) == NULL) {
        return false;
    }

    value->v.vp = ptr;

    if (!jxa_push(array, value)) {
        jxv_free(value);
        return false;
    }

    return true;
}

void * jxa_get_ptr(jx_value * array, size_t i)
{
    if (array == NULL || array->type != JX_TYPE_ARRAY) {
        return NULL;
    }

    return jxv_get_ptr(jxa_get(array, i));
}

void * jxv_get_ptr(jx_value * value)
{
    if (value == NULL || value->type != JX_TYPE_PTR) {
        return NULL;
    }

    return value->v.vp;
}

void jx_trie_reduce_key_charset(char * dst_key, unsigned char * src_key, size_t dst_size)
{
    int src_i;
    int dst_i;

    if (dst_key == NULL || src_key == NULL) {
        return;
    }

    src_i = 0;

    for (dst_i = 0; dst_i < dst_size - 2; dst_i += 2) {
        if (!src_key[src_i]) {
            break;
        }

        dst_key[dst_i] = (src_key[src_i] >> 4) + 1;
        dst_key[dst_i + 1] = (src_key[src_i] & 0x0F) + 1;

        src_i++;
    }

    dst_key[dst_i] = '\0';
}

void jx_trie_expand_key_charset(unsigned char * dst_key, char * src_key, size_t dst_size)
{
    int src_i;
    int dst_i;

    if (dst_key == NULL || src_key == NULL) {
        return;
    }

    src_i = 0;

    for (dst_i = 0; dst_i < dst_size - 1; dst_i++) {
        if (!src_key[src_i] || !src_key[src_i + 1]) {
            break;
        }

        dst_key[dst_i] = ((src_key[src_i] - 1) << 4) | (src_key[src_i + 1] - 1);

        src_i += 2;
    }

    dst_key[dst_i] = '\0';
}

/* Traverse the tree in an order determined by the value of each character in
 * the key, allocating new nodes as needed until a null byte is encountered
 * (or a memory allocation error occurs).
 *
 * The node for the last character in the string is returned, so that the
 * caller can set the object on the it (and free the old one, if required). */
jx_trie_node * jx_trie_add_key(jx_trie_node * node, char * key, int key_i)
{
    if (node == NULL || key == NULL) {
        return NULL;
    }

    if (key[key_i] == '\0') {
        return node;
    }
    else {
        int next_i = key[key_i] - 1;

        if (node->child_nodes[next_i] == NULL) {
            jx_trie_node * new_ch_node = calloc(1, sizeof(jx_trie_node));

            if (new_ch_node == NULL) {
                return NULL;
            }

            new_ch_node->byte = key[key_i];

            node->child_nodes[next_i] = new_ch_node;
        }

        return jx_trie_add_key(node->child_nodes[next_i], key, ++key_i);
    }
}

/* Traverse the tree in an order determined by the value of each character in
 * the key, but don't allocate new child nodes, return NULL if the key cannot
 * be found. */
jx_trie_node * jx_trie_get_key(jx_trie_node * node, char * key, int key_i)
{
    if (node == NULL || key == NULL) {
        return NULL;
    }

    if (key[key_i] == '\0') {
        return node;
    }
    else {
        int next_i = key[key_i] - 1;

        if (node->child_nodes[next_i] == NULL) {
            return NULL;
        }

        return jx_trie_get_key(node->child_nodes[next_i], key, ++key_i);
    }
}

/* Traverse the tree in character order of the key, if an object is found,
 * remove the object from the tree. The object itself is not freed but
 * returned to the caller to be used, or freed if not needed. */
jx_value * jx_trie_del_key(jx_trie_node * node, char * key, int key_i)
{
    if (node == NULL || key == NULL) {
        return NULL;
    }

    if (key[key_i] == '\0') {
        jx_value * value = node->value;

        node->value = NULL;

        return value;
    }
    else {
        int next_i = key[key_i] - 1;

        if (node->child_nodes[next_i] == NULL) {
            return NULL;
        }

        jx_value * value = jx_trie_del_key(node->child_nodes[next_i], key, ++key_i);

        /* We have successfully found a key in the trie, check to see if the child
         * branch at our current index is empty and needs to be pruned. */
        if (value != NULL) {
            bool match = false;

            if (node->child_nodes[next_i]->value != NULL) {
                match = true;
            }
            else {
                int j;

                for (j = 0; j < 16; j++) {
                    if (node->child_nodes[next_i]->child_nodes[j] != NULL) {
                        match = true;
                        break;
                    }
                }
            }

            if (!match) {
                free(node->child_nodes[next_i]);
                node->child_nodes[next_i] = NULL;
            }
        }

        return value;
    }
}

/* Find all stored keys, invoking the callback passed in for each node that has an object.
 *
 * Details:
 *
 * Perform a depth first traversal of the trie (index tree), starting at the node passed in. This
 * search order discovers keys in lexicograpic order, in terms of byte value for each character.
 *
 * Note that to traverse the entire trie this function should be passed the root node of the trie,
 * and the prefix argument should be initialized (to a jx_value string) but be empty. The function
 * will recursively call itself to visit child nodes, and invoke the callback function for each
 * node that has a stored object, passing the callback the key, stored object, and the pointer
 * passed as the fourth arugment of this function. */
 bool jx_trie_iterate_keys(jx_trie_node * node, jx_value * prefix, jxd_iter_cb cb_func, void * ptr)
{
    int i;

    if (node == NULL || prefix == NULL || cb_func == NULL) {
        return false;
    }

    if (!jxs_push(prefix, node->byte)) {
        return false;
    }

    if (node->value != NULL) {
        int orig_key_size = (prefix->length / 2) + 1;

        char * orig_key = alloca(orig_key_size);
        char * prefix_key = jxs_get_str(prefix);

        jx_trie_expand_key_charset((unsigned char *)orig_key, prefix_key, orig_key_size);

        cb_func(orig_key, node->value, ptr);
    }

    for (i = 0; i < 16; i++) {
        if (node->child_nodes[i] != NULL) {
            jx_trie_iterate_keys(node->child_nodes[i], prefix, cb_func, ptr);
        }
    }

    jxs_pop(prefix);

    return true;
}

void jx_trie_free_branch(jx_trie_node * node)
{
    int i;

    if (node == NULL) {
        return;
    }

    for (i = 0; i < 16; i++) {
        if (node->child_nodes[i] != NULL) {
            jx_trie_free_branch(node->child_nodes[i]);
        }
    }

    if (node->value != NULL) {
        jxv_free(node->value);
    }

    free(node);
}

jx_value * jxd_new()
{
    jx_value * value;

    if ((value = jxv_new(JX_TYPE_OBJECT)) == NULL) {
        return NULL;
    }

    if ((value->v.vp = calloc(1, sizeof(jx_trie_node))) == NULL) {
        jxv_free(value);
        return NULL;
    }

    return value;
}

bool jxd_put(jx_value * dict, char * key, jx_value * value)
{
    char * lookup_key;
    int lookup_key_size;

    jx_trie_node * node;

    if (dict == NULL || dict->type != JX_TYPE_OBJECT || key == NULL || value == NULL) {
        return false;
    }

    lookup_key_size = (strlen(key) * 2) + 1;
    lookup_key = alloca(lookup_key_size);

    jx_trie_reduce_key_charset(lookup_key, (unsigned char *)key, lookup_key_size);

    node = jx_trie_add_key(dict->v.vp, lookup_key, 0);

    if (node == NULL) {
        return false;
    }

    if (node->value != NULL) {
        jxv_free(node->value);
    }

    node->value = value;

    return true;
}

jx_value * jxd_get(jx_value * dict, char * key)
{
    char * lookup_key;
    int lookup_key_size;

    jx_trie_node * node;

    if (dict == NULL || dict->type != JX_TYPE_OBJECT || key == NULL) {
        return NULL;
    }

    lookup_key_size = (strlen(key) * 2) + 1;
    lookup_key = alloca(lookup_key_size);

    jx_trie_reduce_key_charset(lookup_key, (unsigned char *)key, lookup_key_size);

    node = jx_trie_get_key(dict->v.vp, lookup_key, 0);

    if (node == NULL) {
        return NULL;
    }

    return node->value;
}

jx_value * jxd_del(jx_value * dict, char * key)
{
    char * lookup_key;
    int lookup_key_size;

    if (dict == NULL || dict->type != JX_TYPE_OBJECT || key == NULL) {
        return NULL;
    }

    lookup_key_size = (strlen(key) * 2) + 1;
    lookup_key = alloca(lookup_key_size);

    jx_trie_reduce_key_charset(lookup_key, (unsigned char *)key, lookup_key_size);

    return jx_trie_del_key(dict->v.vp, lookup_key, 0);
}

bool jxd_del_free(jx_value * dict, char * key)
{
    jx_value * v = jxd_del(dict, key);

    jxv_free(v);

    return v != NULL;
}

bool jxd_iterate(jx_value * dict, jxd_iter_cb cb_func, void * ptr)
{
    bool success;

    jx_value * prefix;

    if (dict == NULL || dict->type != JX_TYPE_OBJECT || cb_func == NULL) {
        return false;
    }

    prefix = jxs_new(NULL);

    if (prefix == NULL) {
        return false;
    }

    success = jx_trie_iterate_keys(dict->v.vp, prefix, cb_func, ptr);

    jxv_free(prefix);

    return success;
}

bool jxd_has_key(jx_value * dict, char * key)
{
    return jxd_get(dict, key) != NULL;
}

jx_type jxd_get_type(jx_value * dict, char * key, bool * found)
{
    jx_value * value = jxd_get(dict, key);

    if (found != NULL) {
        *found = value != NULL;
    }

    return jxv_get_type(value);
}

bool jxd_put_number(jx_value * dict, char * key, double num)
{
    jx_value * value = jxv_number_new(num);

    if (value == NULL) {
        return false;
    }

    if (!jxd_put(dict, key, value)) {
        jxv_free(value);
        return false;
    }

    return true;
}

double jxd_get_number(jx_value * dict, char * key, bool * found)
{
    jx_value * value = jxd_get(dict, key);

    if (found) {
        *found = jxv_get_type(value) == JX_TYPE_NUMBER;
    }

    return jxv_get_number(value);
}

bool jxd_put_bool(jx_value * dict, char * key, bool value)
{
    return jxd_put(dict, key, jxv_bool_new(value));
}

bool jxd_get_bool(jx_value * dict, char * key, bool * found)
{
    jx_value * value = jxd_get(dict, key);

    if (found) {
        *found = jxv_get_type(value) == JX_TYPE_BOOL;
    }

    return jxv_get_bool(value);
}

bool jxd_put_string(jx_value * dict, char * key, char * value)
{
    jx_value * str = jxs_new(value);

    if (!str) {
        return false;
    }

    if (!jxd_put(dict, key, str)) {
        jxv_free(str);
        return false;
    }

    return true;
}

char * jxd_get_string(jx_value * dict, char * key, bool * found)
{
    jx_value * str = jxd_get(dict, key);

    if (found) {
        *found = jxv_get_type(str) == JX_TYPE_STRING;
    }

    return jxs_get_str(str);
}

jx_value * jxv_number_new(double num)
{
    jx_value * value;

    if ((value = jxv_new(JX_TYPE_NUMBER)) == NULL) {
        return NULL;
    }

    value->v.vf = num;

    return value;
}

double jxv_get_number(jx_value * v)
{
    if (v == NULL || v->type != JX_TYPE_NUMBER) {
        return NAN;
    }

    return v->v.vf;
}

jx_value * jxs_new(const char * src)
{
    jx_value * str;

    if ((str = jxv_new(JX_TYPE_STRING)) == NULL) {
        return NULL;
    }

    if (src == NULL) {
        str->length = 0;
    }
    else {
        str->length = strlen(src);
    }

    str->size = 16;

    while (str->size < str->length + 1) {
        str->size *= 2;
    }

    if ((str->v.vp = malloc(sizeof(char) * str->size)) == NULL) {
        jxv_free(str);
        return NULL;
    }

    if (src == NULL) {
        ((char *)str->v.vp)[0] = '\0';
    }
    else {
        strcpy(str->v.vp, src);
    }

    return str;
}

char * jxs_get_str(jx_value * str)
{
    if (str == NULL || str->type != JX_TYPE_STRING) {
        return NULL;
    }

    return str->v.vp;
}

bool jxs_resize(jx_value * str, size_t size)
{
    size_t new_size;
    char * new_str;

    if (str->size >= size) {
        return false;
    }

    new_size = str->size;

    while (new_size < size) {
        new_size *= 2;
    }

    new_str = realloc(str->v.vp, new_size);

    if (new_str == NULL) {
        return false;
    }

    str->v.vp = new_str;
    str->size = new_size;

    return true;
}

bool jxs_append_str(jx_value * dst, char * src)
{
    int new_length;

    if (dst == NULL || dst->type != JX_TYPE_STRING) {
        return false;
    }

    new_length = dst->length + strlen(src);

    if (dst->size < new_length + 1) {
        if (!jxs_resize(dst, new_length + 1)) {
            return false;
        }
    }

    strcat(dst->v.vp, src);

    dst->length = new_length;

    return true;
}

bool jxs_append_fmt(jx_value * dst, char * fmt, ...)
{
    int new_length;
    va_list ap;

    if (dst == NULL || dst->type != JX_TYPE_STRING) {
        return false;
    }

    va_start(ap, fmt);
    new_length = dst->length + vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);

    if (dst->size < new_length + 1) {
        if (!jxs_resize(dst, new_length + 1)) {
            return false;
        }
    }

    va_start(ap, fmt);
    vsprintf(dst->v.vp + dst->length, fmt, ap);
    va_end(ap);

    dst->length = new_length;

    return true;
}

bool jxs_append_chr(jx_value * dst, char c)
{
    if (dst == NULL || dst->type != JX_TYPE_STRING) {
        return false;
    }

    if (c == '\0') {
        return true;
    }

    if (dst->size < dst->length + 2) {
        if (!jxs_resize(dst, dst->length + 2)) {
            return false;
        }
    }

    ((char*)dst->v.vp)[dst->length++] = c;
    ((char*)dst->v.vp)[dst->length] = '\0';

    return true;
}

bool jxs_push(jx_value * str, char c)
{
    return jxs_append_chr(str, c);
}

char jxs_pop(jx_value * str)
{
    char * ptr;
    char c;

    if (str == NULL || str->type != JX_TYPE_STRING || str->length == 0) {
        return '\0';
    }

    ptr = (char *)str->v.vp;

    c = ptr[--str->length];

    ptr[str->length] = '\0';

    return c;
}

jx_value * jxv_null()
{
    static bool init = false;
    static jx_value val_null;

    if (!init) {
        val_null.type = JX_TYPE_NULL;
        init = true;
    }

    return &val_null;
}

bool jxv_is_null(jx_value * value)
{
    if (value == NULL) {
        return false;
    }

    return value->type == JX_TYPE_NULL;
}

jx_value * jxv_bool_new(bool value)
{
    static bool init = false;

    static jx_value val_true;
    static jx_value val_false;

    if (!init) {
        val_true.type = JX_TYPE_BOOL;
        val_true.v.vb = true;

        val_false.type = JX_TYPE_BOOL;
        val_false.v.vb = false;

        init = true;
    }

    return (value) ? &val_true : &val_false;
}

bool jxv_get_bool(jx_value * value)
{
    if (value == NULL || value->type != JX_TYPE_BOOL) {
        return false;
    }

    return value->v.vb;
}

void jxv_free(jx_value * value)
{
    jx_type type;

    if (value == NULL) {
        return;
    }

    type = jxv_get_type(value);

    if (type == JX_TYPE_STRING || type == JX_TYPE_PTR) {
        if (value->v.vp != NULL) {
            free(value->v.vp);
        }
    }
    else if (type == JX_TYPE_ARRAY) {
        int i;

        for (i = 0; i < jxa_get_length(value); i++) {
            jxv_free(jxa_get(value, i));
        }

        free(value->v.vpp);
    }
    else if (type == JX_TYPE_OBJECT) {
        jx_trie_free_branch(value->v.vp);
    }
    else if (type == JX_TYPE_NULL || type == JX_TYPE_BOOL) {
        return;
    }

    free(value);
}