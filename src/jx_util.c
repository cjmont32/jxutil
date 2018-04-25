/* 
 * jx_util.c
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
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>

#define JX_DEFAULT_OBJECT_STACK_SIZE 8
#define JX_DEFAULT_ARRAY_SIZE 8
#define JX_ERROR_MESSAGE_MAX_SIZE 2048

static jx_error jx_err;
static char jx_error_message[JX_ERROR_MESSAGE_MAX_SIZE];
static const char * const jx_error_messages[JX_ERROR_GUARD] =
{
	"OK",
	"Error: Invalid context used.",
	"LIBC Error: errno: (%d), message: (%s).",
	"Syntax Error [%lu:%lu]: Root value must be either an array or an object.",
	"Syntax Error [%lu:%lu]: Illegal trailing characters.",
	"Syntax Error [%lu:%lu]: Illegal token [%c]."
};

jx_cntx * jx_new()
{
	jx_cntx * cntx;

	if ((cntx = calloc(1, sizeof(jx_cntx))) == NULL) {
		jx_set_error(JX_ERROR_LIBC);
		return NULL;
	}

	if ((cntx->object_stack = jxa_new(JX_DEFAULT_OBJECT_STACK_SIZE)) == NULL) {
		jx_set_error(JX_ERROR_LIBC);
		free(cntx);
		return NULL;
	}

	cntx->line = 1;
	cntx->col = 1;

	return cntx;
}

void jx_free(jx_cntx * cntx)
{
}

void jx_set_error(jx_error error, ...)
{
	va_list ap;

	jx_err = error;

	if (error == JX_ERROR_LIBC) {
		snprintf(jx_error_message, JX_ERROR_MESSAGE_MAX_SIZE, jx_error_messages[error],
			errno, strerror(errno));
	}
	else {
		va_start(ap, error);
		vsnprintf(jx_error_message, JX_ERROR_MESSAGE_MAX_SIZE, jx_error_messages[error], ap);
		va_end(ap);
	}
}

jx_error jx_get_error()
{
	return jx_err;
}

const char * const jx_get_error_message()
{
	return jx_error_message;	
}

jx_state jx_mode(jx_cntx * cntx)
{
	return 0;
}

bool jx_push_mode(jx_cntx * cntx, jx_state mode)
{
	return false;
}

void jx_pop_mode(jx_cntx * cntx)
{
}

void jx_set_value(jx_cntx * cntx, jx_value * value)
{
}

void jx_set_return(jx_cntx * cntx, jx_value * value)
{
}

long jx_find_token(jx_cntx * cntx, const char * src, long pos, long end_pos)
{
	if (cntx == NULL) {
		jx_set_error(JX_ERROR_INVALID_CONTEXT);
		return -1;
	}

	if (pos > end_pos) {
		return -1;
	}

	while (pos <= end_pos) {
		if (src[pos] == ' ') {
			cntx->col++;
			pos++;
		}
		else if (src[pos] == '\t') {
			cntx->col += 3;
			pos++;
		}
		else if (src[pos] == '\n' || src[pos] == '\v') {
			cntx->col = 1;
			cntx->line++;
			pos++;
		}
		else {
			break;
		}
	}

	return pos;
}

int jx_parse(jx_cntx * cntx, const char * src, long n_bytes)
{
	long pos;
	long end_pos;

	if (cntx == NULL) {
		jx_set_error(JX_ERROR_INVALID_CONTEXT);
		return -1;
	}

	if (cntx->syntax_errors) {
		return -1;
	}

	pos = 0;
	end_pos = n_bytes - 1;

	while (pos <= end_pos) {
		if (jx_mode(cntx) == JX_STATE_FIND_TOKEN) {
			pos = jx_find_token(cntx, src, pos, end_pos);

			if (pos == -1) {
				break;
			}

			if (cntx->depth == 0 && src[pos] != '[') {
				cntx->syntax_errors = true;
				jx_set_error(JX_ERROR_INVALID_ROOT, cntx->line, cntx->col);
				return -1;
			}

			printf("found token: (%c)\n", src[pos]);

			if (src[pos] == '[') {
				jx_value * array;

				array = jxa_new(JX_DEFAULT_ARRAY_SIZE);

				if (array == NULL) {
					return -1;
				}

				if (!jx_push_mode(cntx, JX_STATE_PARSE_ARRAY)) {
					return -1;
				}

				jx_set_value(cntx, array);

				if (cntx->depth == 0) {
					cntx->root_value = array;
				}

				pos++;
				cntx->col++;
				cntx->depth++;
			}
			else if (src[pos] == '-' || (src[pos] >= '0' && src[pos] <= '9')) {
				if (!jx_push_mode(cntx, JX_STATE_PARSE_NUMBER)) {
					return -1;
				}
			}
			else {
				cntx->syntax_errors = true;
				jx_set_error(JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col, src[pos]);
				return -1;
			}
		}
		else if (jx_mode(cntx) == JX_STATE_PARSE_ARRAY) {
			pos = jx_find_token(cntx, src, pos, end_pos);

			if (pos == -1) {
				break;
			}

			if (src[pos] == ',') {

			}
			else if (src[pos] == ']') {

			}
			else {
				if (!jx_push_mode(cntx, JX_STATE_FIND_TOKEN)) {
					return -1;
				}
			}
		}
		else if (jx_mode(cntx) == JX_STATE_PARSE_NUMBER) {
		}
	}

	return 0;
}