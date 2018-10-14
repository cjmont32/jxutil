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

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

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

jx_value * jxv_number_new(double num)
{
	jx_value * value;

	if ((value = jxv_new(JX_TYPE_NUMBER)) == NULL) {
		return NULL;
	}

	value->v.vf = num;

	return value;
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

	if (dst->size < dst->length + 2) {
		if (!jxs_resize(dst, dst->length + 2)) {
			return false;
		}
	}

	((char*)dst->v.vp)[dst->length++] = c;
	((char*)dst->v.vp)[dst->length] = '\0';

	return true;
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
	else if (type == JX_TYPE_NULL || type == JX_TYPE_BOOL) {
		return;
	}

	free(value);
}