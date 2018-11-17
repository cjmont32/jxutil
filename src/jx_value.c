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

int jx_trie_index_for_byte(unsigned char byte)
{
	/* allowed control characters */
	switch (byte) {
		case '\b':
			return 0;
		case '\f':
			return 1;
		case '\n':
			return 2;
		case '\r':
			return 3;
		case '\t':
			return 4;
		default:
			break;
	};

	/* control character */
	if (byte < 0x20 || byte == 0x7f) {
		return -1;
	}

	byte -= 27;

	if (byte >= JX_NODE_CNT) {
		return -1;
	}

	return byte;
}

unsigned char jx_trie_byte_for_index(int index)
{
	char cntl_bytes[] = { '\b', '\f', '\n', '\r', '\t' };

	if (index < 5) {
		return cntl_bytes[index];
	}

	return index + 27;
}

jx_trie_node * jx_trie_add_key(jx_trie_node * node, unsigned char * key, int key_i)
{
	if (node == NULL || key == NULL) {
		return NULL;
	}

	if (key[key_i] == '\0') {
		return node;
	}
	else {
		int i = jx_trie_index_for_byte(key[key_i]);

		if (i == -1) {
			return NULL;
		}

		if (node->child_nodes[i] == NULL) {
			jx_trie_node * new_node = calloc(1, sizeof(jx_trie_node));

			if (new_node == NULL) {
				return NULL;
			}

			new_node->byte = key[key_i];

			node->child_nodes[i] = new_node;
		}

		return jx_trie_add_key(node->child_nodes[i], key, ++key_i);
	}
}

jx_trie_node * jx_trie_get_key(jx_trie_node * node, unsigned char * key, int key_i)
{
	if (node == NULL || key == NULL) {
		return NULL;
	}

	if (key[key_i] == '\0') {
		return node;
	}
	else {
		int i = jx_trie_index_for_byte(key[key_i]);

		if (i == -1 || node->child_nodes[i] == NULL) {
			return NULL;
		}

		return jx_trie_get_key(node->child_nodes[i], key, ++key_i);
	}
}

jx_value * jx_trie_del_key(jx_trie_node * node, unsigned char * key, int key_i)
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
		int i = jx_trie_index_for_byte(key[key_i]);

		if (i == -1 || node->child_nodes[i] == NULL) {
			return NULL;
		}
		else {
			jx_value * value = jx_trie_del_key(node->child_nodes[i], key, ++key_i);

			/* prune branch if it's empty */
			if (value != NULL) {
				bool match = false;

				if (node->child_nodes[i]->value != NULL) {
					match = true;
				}
				else {
					int j;

					for (j = 0; j < JX_NODE_CNT; j++) {
						if (node->child_nodes[i]->child_nodes[j] != NULL) {
							match = true;
							break;
						}
					}
				}

				if (!match) {
					free(node->child_nodes[i]);
					node->child_nodes[i] = NULL;
				}
			}

			return value;
		}
	}
}

bool jx_trie_iterate_keys(jx_trie_node * node, jx_value * str, jxd_iter_cb iter, void * ptr)
{
	int i;

	if (node == NULL || str == NULL || iter == NULL) {
		return false;
	}

	if (!jxs_push(str, node->byte)) {
		return false;
	}

	if (node->value != NULL) {
		char * key = jxs_get_str(str);

		iter(key, node->value, ptr);
	}

	for (i = 0; i < JX_NODE_CNT; i++) {
		if (node->child_nodes[i] != NULL) {
			jx_trie_iterate_keys(node->child_nodes[i], str, iter, ptr);
		}
	}

	jxs_pop(str);

	return true;
}

void jx_trie_free_branch(jx_trie_node * node)
{
	int i;

	if (node == NULL) {
		return;
	}

	for (i = 0; i < JX_NODE_CNT; i++) {
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

bool jxd_set(jx_value * dict, char * key, jx_value * value)
{
	jx_trie_node * node;

	if (dict == NULL || dict->type != JX_TYPE_OBJECT || key == NULL || value == NULL) {
		return false;
	}

	node = jx_trie_add_key(dict->v.vp, (unsigned char *)key, 0);

	if (node == NULL) {
		return false;
	}

	if (node->value != NULL) {
		jxv_free(node->value);
	}

	node->value = value;

	return true;
}

jx_value * jxd_del(jx_value * dict, char * key)
{
	if (dict == NULL || dict->type != JX_TYPE_OBJECT || key == NULL) {
		return NULL;
	}

	return jx_trie_del_key(dict->v.vp, (unsigned char * )key, 0);
}

jx_value * jxd_get(jx_value * dict, char * key)
{
	jx_trie_node * node;

	if (dict == NULL || dict->type != JX_TYPE_OBJECT || key == NULL) {
		return NULL;
	}

	node = jx_trie_get_key(dict->v.vp, (unsigned char * )key, 0);

	if (node == NULL) {
		return NULL;
	}

	return node->value;
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

bool jxd_set_number(jx_value * dict, char * key, double num)
{
	jx_value * value = jxv_number_new(num);

	if (value == NULL) {
		return false;
	}

	if (!jxd_set(dict, key, value)) {
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

bool jxd_set_bool(jx_value * dict, char * key, bool value)
{
	return jxd_set(dict, key, jxv_bool_new(value));
}

bool jxd_get_bool(jx_value * dict, char * key, bool * found)
{
	jx_value * value = jxd_get(dict, key);

	if (found) {
		*found = jxv_get_type(value) == JX_TYPE_BOOL;
	}

	return jxv_get_bool(value);
}

bool jxd_set_string(jx_value * dict, char * key, char * value)
{
	jx_value * str = jxs_new(value);

	if (!str) {
		return false;
	}

	if (!jxd_set(dict, key, str)) {
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

bool jxd_iterate(jx_value * dict, jxd_iter_cb iter, void * ptr)
{
	bool success;

	jx_value * str;

	if (dict == NULL || dict->type != JX_TYPE_OBJECT || iter == NULL) {
		return false;
	}

	str = jxs_new(NULL);

	if (str == NULL) {
		return false;
	}

	success = jx_trie_iterate_keys(dict->v.vp, str, iter, ptr);

	jxv_free(str);

	return success;
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