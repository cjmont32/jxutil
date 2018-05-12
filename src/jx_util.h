/* 
 * jx_util.h
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

#include <jx_value.h>
#include <stdbool.h>
#include <stdlib.h>

#define JX_ERROR_BUF_MAX_SIZE 2048

typedef int jx_state;

typedef enum
{
	JX_ERROR_NONE,
	JX_ERROR_INVALID_CONTEXT,
	JX_ERROR_LIBC,
	JX_ERROR_INVALID_ROOT,
	JX_ERROR_TRAILING_CHARS,
	JX_ERROR_EXPECTED_TOKEN,
	JX_ERROR_UNEXPECTED_TOKEN,
	JX_ERROR_ILLEGAL_TOKEN,
	JX_ERROR_INCOMPLETE_OBJECT,
	JX_ERROR_GUARD
} jx_error;

typedef enum
{
	JX_MODE_UNDEFINED,
	JX_MODE_START,
	JX_MODE_PARSE_ARRAY,
	JX_MODE_PARSE_NUMBER,
	JX_MODE_DONE
} jx_mode;

typedef enum
{
	JX_TOKEN_NONE,
	JX_TOKEN_ARRAY_BEGIN,
	JX_TOKEN_ARRAY_SEPARATOR,
	JX_TOKEN_ARRAY_END,
	JX_TOKEN_NUMBER
} jx_token_type;

typedef struct
{
	jx_value * value;
	jx_value * return_value;

	jx_state state;

	jx_mode mode;
} jx_frame;

typedef struct
{
	size_t line;	
	size_t col;
	size_t depth;

	jx_value * object_stack;

	bool inside_token;

	char error_msg[JX_ERROR_BUF_MAX_SIZE];
	jx_error error;
} jx_cntx;

jx_cntx * jx_new();
void jx_free(jx_cntx * cntx);

#ifdef JX_INTERNAL
void jx_set_error(jx_cntx * cntx, jx_error error, ...);

jx_frame * jx_top(jx_cntx * cntx);
bool jx_push_mode(jx_cntx * cntx, jx_mode mode);
void jx_pop_mode(jx_cntx * cntx);
void jx_set_mode(jx_cntx * cntx, jx_mode mode);
jx_mode jx_get_mode(jx_cntx * cntx);
void jx_set_state(jx_cntx * cntx, jx_state state);
jx_state jx_get_state(jx_cntx * cntx);
void jx_set_value(jx_cntx * cntx, jx_value * value);
jx_value * jx_get_value(jx_cntx * cntx);
void jx_set_return(jx_cntx * cntx, jx_value * value);
jx_value * jx_get_return(jx_cntx * cntx);
#endif

jx_error jx_get_error(jx_cntx * cntx);
const char * const jx_get_error_message(jx_cntx * cntx);

int jx_parse_json(jx_cntx * cntx, const char * src, long n_bytes);

jx_value * jx_get_result(jx_cntx * cntx);