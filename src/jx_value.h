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
	JX_TYPE_ARRAY,
	JX_TYPE_NUMBER,
	JX_TYPE_PTR
} jx_type;

typedef struct
{
	union {
		double vf;
		void * vp;
		void ** vpp;
	} v;

	jx_type type;

	size_t size;
	size_t length;
} jx_value;

jx_type jxv_get_type(jx_value * value);
jx_value * jxa_new(size_t capacity);
size_t jxa_get_length(jx_value * array);
jx_type jxa_get_type(jx_value * array, size_t i);
jx_value * jxa_get(jx_value * array, size_t i);
bool jxa_push(jx_value * array, jx_value * value);
jx_value * jxa_pop(jx_value * array);
jx_value * jxa_top(jx_value * array);
bool jxa_push_number(jx_value * array, double num);
double jxa_get_number(jx_value * arr, size_t i);
jx_value * jxv_number_new(double num);
bool jxa_push_ptr(jx_value * array, void * ptr);
void jxv_free(jx_value * value);