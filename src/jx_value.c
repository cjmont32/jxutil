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

#define JX_INTERNAL

#include <jx_util.h>

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
		jx_set_error(JX_ERROR_LIBC);
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
		jx_set_error(JX_ERROR_LIBC);
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
	jx_value * value;

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
			jx_set_error(JX_ERROR_LIBC);
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

jx_value * jxv_number_new(double num)
{
	jx_value * value;

	if ((value = jxv_new(JX_TYPE_NUMBER)) == NULL) {
		return NULL;
	}

	value->v.vf = num;

	return value;
}

void jxv_free(jx_value * value)
{
}