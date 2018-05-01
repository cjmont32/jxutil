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

#define JX_ARRAY_STATE_DEFAULT 0
#define JX_ARRAY_STATE_NEW_MEMBER 1
#define JX_ARRAY_STATE_SEPARATOR 2

#define JX_NUM_IS_VALID        (1 << 0)
#define JX_NUM_ACCEPT_SIGN     (1 << 1)
#define JX_NUM_ACCEPT_DIGITS   (1 << 2)
#define JX_NUM_ACCEPT_DEC_PT   (1 << 3)
#define JX_NUM_ACCEPT_EXP      (1 << 4)
#define JX_NUM_HAS_DIGITS      (1 << 5)
#define JX_NUM_HAS_DEC_PT      (1 << 6)
#define JX_NUM_HAS_EXP         (1 << 7)
#define JX_NUM_DEFAULT         JX_NUM_ACCEPT_SIGN | JX_NUM_ACCEPT_DIGITS

#define JX_DEFAULT_OBJECT_STACK_SIZE 8
#define JX_DEFAULT_ARRAY_SIZE 8
#define JX_ERROR_MESSAGE_MAX_SIZE 2048
#define JX_BUF_SIZE 26

static char jx_buf[JX_BUF_SIZE];
static int jx_buf_pos;

static jx_error jx_err;
static char jx_error_message[JX_ERROR_MESSAGE_MAX_SIZE];
static const char * const jx_error_messages[JX_ERROR_GUARD] =
{
	"OK",
	"Error: Invalid context used.",
	"LIBC Error: errno: (%d), message: (%s).",
	"Syntax Error [%lu:%lu]: Root value must be either an array or an object.",
	"Syntax Error [%lu:%lu]: Illegal characters outside of root object, starting with (%c).",
	"Syntax Error [%lu:%lu]: Missing token (%s).",
	"Syntax Error [%lu:%lu]: Unexpected token.",
	"Syntax Error [%lu:%lu]: Illegal token (%s).",
	"Syntax Error [%lu:%lu]: Incomplete JSON object."
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
	if (cntx == NULL) {
		return;
	}

	while (jx_get_mode(cntx) != JX_MODE_UNDEFINED) {
		jx_frame * frame;

		frame = jx_top(cntx);

		if (frame->value != NULL) {
			jxv_free(frame->value);
		}

		if (frame->return_value != NULL) {
			jxv_free(frame->return_value);
		}

		jx_pop_mode(cntx);
	}

	jxv_free(cntx->object_stack);

	free(cntx);
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

void jx_set_syntax_error(jx_cntx * cntx, jx_error error, ...)
{
	va_list ap;

	if (cntx == NULL) {
		return;
	}

	jx_err = error;

	va_start(ap, error);
	vsnprintf(jx_error_message, JX_ERROR_MESSAGE_MAX_SIZE, jx_error_messages[error], ap);
	va_end(ap);

	cntx->syntax_errors = true;
}

jx_error jx_get_error()
{
	return jx_err;
}

const char * const jx_get_error_message()
{
	return jx_error_message;	
}

jx_frame * jx_top(jx_cntx * cntx)
{
	jx_value * value;

	if (cntx == NULL) {
		return NULL;
	}

	value = jxa_top(cntx->object_stack);

	if (value == NULL || value->v.vp == NULL) {
		return NULL;
	}

	return value->v.vp;
}

bool jx_push_mode(jx_cntx * cntx, jx_mode mode)
{
	jx_frame * frame;

	if (cntx == NULL || cntx->object_stack == NULL) {
		return false;
	}

	if ((frame = calloc(1, sizeof(jx_frame))) == NULL) {
		jx_set_error(JX_ERROR_LIBC);
		return false;
	}

	frame->mode = mode;

	if (!jxa_push_ptr(cntx->object_stack, frame)) {
		free(frame);
		return false;
	}

	return true;
}

void jx_pop_mode(jx_cntx * cntx)
{
	jx_value * value;

	if (cntx == NULL || cntx->object_stack == NULL) {
		return;
	}

	value = jxa_pop(cntx->object_stack);

	jxv_free(value);
}

void jx_set_mode(jx_cntx * cntx, jx_mode mode)
{
	jx_frame * frame;

	if ((frame = jx_top(cntx)) == NULL) {
		return;
	}

	frame->mode = mode;
}

jx_mode jx_get_mode(jx_cntx * cntx)
{
	jx_frame * frame;

	if ((frame = jx_top(cntx)) == NULL) {
		return JX_MODE_UNDEFINED;
	}

	return frame->mode;
}

void jx_set_state(jx_cntx * cntx, jx_state state)
{
	jx_frame * frame;

	frame = jx_top(cntx);

	if (frame == NULL) {
		return;
	}

	frame->state = state;
}

jx_state jx_get_state(jx_cntx * cntx)
{
	jx_frame * frame;

	frame = jx_top(cntx);

	if (frame == NULL) {
		return -1;	
	}

	return frame->state;
}

void jx_set_value(jx_cntx * cntx, jx_value * value)
{
	jx_frame * frame;

	frame = jx_top(cntx);

	if (frame == NULL) {
		return;
	}

	frame->value = value;
}

jx_value * jx_get_value(jx_cntx * cntx)
{
	jx_frame * frame;

	frame = jx_top(cntx);

	if (frame == NULL) {
		return NULL;
	}

	return frame->value;
}

void jx_set_return(jx_cntx * cntx, jx_value * value)
{
	jx_frame * frame;

	frame = jx_top(cntx);

	if (frame == NULL) {
		return;
	}

	frame->return_value = value;
}

jx_value * jx_get_return(jx_cntx * cntx)
{
	jx_frame * frame;

	frame = jx_top(cntx);

	if (frame == NULL) {
		return NULL;
	}

	return frame->return_value;
}

long jx_find_token(jx_cntx * cntx, const char * src, long pos, long end_pos)
{
	if (cntx == NULL) {
		jx_set_error(JX_ERROR_INVALID_CONTEXT);
		return -1;
	}

	if (cntx->inside_token) {
		return pos;
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

	if (pos > end_pos) {
		return -1;
	}

	return pos;
}

int jx_parse_array(jx_cntx * cntx, const char * src, long pos, long end_pos)
{
	jx_value * ret;

	ret = jx_get_return(cntx);

	if (ret != NULL) {
		jx_value * array = jx_get_value(cntx);

		if (!jxa_push(array, ret)) {
			return -1;
		}

		jx_set_return(cntx, NULL);

		jx_set_state(cntx, JX_ARRAY_STATE_NEW_MEMBER);
	}

	/* ----------------------------------------------------------------------*
	 *                       Overview of Array States                        *
	 * ----------------------------------------------------------------------*
	 * [      # DEFAULT STATE     - accept: member, closing bracket          *
	 * [ m    # NEW MEMBER STATE  - accept: seperator token, closing bracket *
	 * [ m,   # SEPARATOR STATE   - accept: member                           *
	 * ]      # TERMINAL STATE    - return array to previous frame           *
	 * ----------------------------------------------------------------------*/
	if (src[pos] == ',') {
		if (jx_get_state(cntx) != JX_ARRAY_STATE_NEW_MEMBER) {
			jx_set_syntax_error(cntx, JX_ERROR_UNEXPECTED_TOKEN, cntx->line, cntx->col);
			return -1;
		}

		pos++;

		cntx->col++;

		jx_set_state(cntx, JX_ARRAY_STATE_SEPARATOR);
	}
	else if (src[pos] == ']') {
		jx_value * array;

		if (jx_get_state(cntx) == JX_ARRAY_STATE_SEPARATOR) {
			jx_set_syntax_error(cntx, JX_ERROR_UNEXPECTED_TOKEN, cntx->line, cntx->col);
			return -1;
		}

		array = jx_get_value(cntx);

		jx_pop_mode(cntx);
		jx_set_return(cntx, array);

		pos++;

		cntx->col++;
		cntx->depth--;

		if (jx_get_mode(cntx) == JX_MODE_START) {
			jx_set_mode(cntx, JX_MODE_DONE);
		}
	}
	else {
		if (jx_get_state(cntx) == JX_ARRAY_STATE_NEW_MEMBER) {
			jx_set_syntax_error(cntx, JX_ERROR_EXPECTED_TOKEN, cntx->line, cntx->col, ",");
			return -1;
		}
	}

	return pos;
}

int jx_parse_number(jx_cntx * cntx, const char * src, long pos, long end_pos)
{
	jx_frame * frame;
	jx_state state;

	char c;

	bool symbol_end = false;

	if ((frame = jx_top(cntx)) == NULL) {
		return -1;
	}

	state = frame->state;

	while (pos <= end_pos) {
		c = src[pos];

		if (c == '-' || c == '+') {
			if (!(state & JX_NUM_ACCEPT_SIGN)) {
				jx_set_syntax_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"illegal position for sign character in number");
				return -1;
			}

			state &= ~(JX_NUM_ACCEPT_SIGN | JX_NUM_IS_VALID);
		}
		else if (c >= '0' && c <= '9') {
			if (!(state & JX_NUM_ACCEPT_DIGITS)) {
				jx_set_syntax_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"invalid number");
				return -1;
			}

			if (c == '0' && !(state & JX_NUM_HAS_DIGITS)) {
				state &= ~JX_NUM_ACCEPT_DIGITS;
			}

			if (!(state & (JX_NUM_HAS_DEC_PT | JX_NUM_HAS_EXP))) {
				state |= JX_NUM_ACCEPT_DEC_PT;
			}

			if (!(state & JX_NUM_HAS_EXP)) {
				state |= JX_NUM_ACCEPT_EXP;
			}

			state &= ~JX_NUM_ACCEPT_SIGN;
			state |= (JX_NUM_HAS_DIGITS | JX_NUM_IS_VALID);
		}
		else if (c == '.') {
			if (!(state & JX_NUM_ACCEPT_DEC_PT)) {
				jx_set_syntax_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"illegal position for decimal point in number");
				return -1;
			}

			state |= (JX_NUM_HAS_DEC_PT | JX_NUM_ACCEPT_DIGITS);
			state &= ~(JX_NUM_ACCEPT_DEC_PT | JX_NUM_ACCEPT_EXP | JX_NUM_IS_VALID);
		}
		else if (c == 'e' || c == 'E') {
			if (!(state & JX_NUM_ACCEPT_EXP)) {
				jx_set_syntax_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"illegal position for exponent in number");
				return -1;
			}

			state |= (JX_NUM_HAS_EXP | JX_NUM_ACCEPT_SIGN | JX_NUM_ACCEPT_DIGITS);
			state &= ~(JX_NUM_IS_VALID | JX_NUM_ACCEPT_EXP | JX_NUM_ACCEPT_DEC_PT);
		}
		else {
			symbol_end = true;
			break;
		}

		if (jx_buf_pos == JX_BUF_SIZE - 1) {
			jx_set_syntax_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
				"number too large");
			return -1;
		}

		jx_buf[jx_buf_pos++] = src[pos++];

		cntx->col++;
	}

	if (symbol_end) {
		if (state & JX_NUM_IS_VALID) {
			jx_buf[jx_buf_pos] = '\0';

			jx_buf_pos = 0;

			frame->value = jxv_number_new(strtod(jx_buf, NULL));
		}
		else {
			jx_set_syntax_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
				"invalid number");
			return -1;
		}
	}

	frame->state = state;

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

	if (jx_get_mode(cntx) == JX_MODE_UNDEFINED) {
		jx_push_mode(cntx, JX_MODE_START);
	}

	pos = 0;
	end_pos = n_bytes - 1;

	while (pos <= end_pos) {
		pos = jx_find_token(cntx, src, pos, end_pos);

		if (pos == -1) {
			break;
		}

		if (jx_get_mode(cntx) == JX_MODE_PARSE_ARRAY) {
			long new_pos = jx_parse_array(cntx, src, pos, end_pos);

			if (new_pos == -1) {
				return -1;
			}

			if (pos != new_pos) {
				pos = new_pos;
				continue;
			}
		}
		else if (jx_get_mode(cntx) == JX_MODE_PARSE_NUMBER) {
			jx_value * number;

			pos = jx_parse_number(cntx, src, pos, end_pos);

			if (pos == -1) {
				return -1;
			}

			if ((number = jx_get_value(cntx)) != NULL) {
				jx_pop_mode(cntx);
				jx_set_return(cntx, number);

				cntx->inside_token = false;
			}

			continue;
		}

		if (jx_get_mode(cntx) == JX_MODE_DONE) {
			jx_set_syntax_error(cntx, JX_ERROR_TRAILING_CHARS, cntx->line, cntx->col, src[pos]);
			return -1;
		}

		if (cntx->depth == 0 && src[pos] != '[') {
			jx_set_syntax_error(cntx, JX_ERROR_INVALID_ROOT, cntx->line, cntx->col);
			return -1;
		}
		
		if (src[pos] == '[') {
			jx_value * array;

			if (!jx_push_mode(cntx, JX_MODE_PARSE_ARRAY)) {
				return -1;
			}

			array = jxa_new(JX_DEFAULT_ARRAY_SIZE);

			if (array == NULL) {
				return -1;
			}

			jx_set_value(cntx, array);

			pos++;

			cntx->col++;
			cntx->depth++;
		}
		else if (src[pos] == '-' || (src[pos] >= '0' && src[pos] <= '9')) {
			bool success = jx_push_mode(cntx, JX_MODE_PARSE_NUMBER);

			if (!success) {
				return -1;
			}

			cntx->inside_token = true;

			jx_set_state(cntx, JX_NUM_DEFAULT);
		}
		else {
			char token[2];

			token[0] = src[pos];
			token[1] = '\0';

			jx_set_syntax_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col, token);

			return -1;
		}
	}

	return jx_get_mode(cntx) == JX_MODE_DONE;
}

jx_value * jx_get_result(jx_cntx * cntx)
{
	jx_value * ret;

	if (cntx == NULL) {
		jx_set_error(JX_ERROR_INVALID_CONTEXT);
		return NULL;
	}

	if (cntx->syntax_errors) {
		return NULL;
	}

	if (jx_get_mode(cntx) != JX_MODE_DONE) {
		jx_set_syntax_error(cntx, JX_ERROR_INCOMPLETE_OBJECT, cntx->line, cntx->col);
		return NULL;
	}

	ret = jx_get_return(cntx);

	jx_set_return(cntx, NULL);

	return ret;
}