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

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include <jx_util.h>

#define JX_ARRAY_STATE_DEFAULT       0
#define JX_ARRAY_STATE_NEW_MEMBER    1
#define JX_ARRAY_STATE_SEPARATOR     2

#define JX_NUM_IS_VALID              (1 << 0)
#define JX_NUM_ACCEPT_SIGN           (1 << 1)
#define JX_NUM_ACCEPT_DIGITS         (1 << 2)
#define JX_NUM_ACCEPT_DEC_PT         (1 << 3)
#define JX_NUM_ACCEPT_EXP            (1 << 4)
#define JX_NUM_HAS_DIGITS            (1 << 5)
#define JX_NUM_HAS_DEC_PT            (1 << 6)
#define JX_NUM_HAS_EXP               (1 << 7)
#define JX_NUM_DEFAULT               JX_NUM_ACCEPT_SIGN | JX_NUM_ACCEPT_DIGITS

#define JX_STRING_ESCAPE             (1 << 0)
#define JX_STRING_UTF8               (1 << 1)
#define JX_STRING_UNICODE            (1 << 2)
#define JX_STRING_SURROGATE          (1 << 3)
#define JX_STRING_END                (1 << 4)

#define JX_DEFAULT_OBJECT_STACK_SIZE 8
#define JX_DEFAULT_ARRAY_SIZE        8

static const char * const jx_error_messages[JX_ERROR_GUARD] =
{
	"OK",
	"Invalid Context",
	"LIBC Error: errno: (%d), message: (%s).",
	"Syntax Error [%lu:%lu]: Root value must be either an array or an object.",
	"Syntax Error [%lu:%lu]: Illegal characters outside of root object, starting with (%c).",
	"Syntax Error [%lu:%lu]: Missing token (%s).",
	"Syntax Error [%lu:%lu]: Unexpected token (%s).",
	"Syntax Error [%lu:%lu]: Illegal token (%s).",
	"Syntax Error [%lu:%lu]: Incomplete JSON object."
};

jx_cntx * jx_new()
{
	jx_cntx * cntx;

	if ((cntx = calloc(1, sizeof(jx_cntx))) == NULL) {
		return NULL;
	}

	if ((cntx->object_stack = jxa_new(JX_DEFAULT_OBJECT_STACK_SIZE)) == NULL) {
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

void jx_set_error(jx_cntx * cntx, jx_error error, ...)
{
	va_list ap;

	if (cntx == NULL) {
		return;
	}

	cntx->error = error;

	if (error == JX_ERROR_LIBC) {
		snprintf(cntx->error_msg, JX_ERROR_BUF_MAX_SIZE, jx_error_messages[error],
			errno, strerror(errno));
	}
	else {
		va_start(ap, error);
		vsnprintf(cntx->error_msg, JX_ERROR_BUF_MAX_SIZE, jx_error_messages[error], ap);
		va_end(ap);
	}
}

jx_error jx_get_error(jx_cntx * cntx)
{
	if (cntx == NULL) {
		return JX_ERROR_INVALID_CONTEXT;
	}

	return cntx->error;
}

const char * const jx_get_error_message(jx_cntx * cntx)
{
	if (cntx == NULL) {
		return jx_error_messages[JX_ERROR_INVALID_CONTEXT];
	}

	return cntx->error_msg;
}

void jx_set_extensions(jx_cntx * cntx, jx_ext_set ext)
{
	if (cntx == NULL) {
		return;
	}

	cntx->ext = ext;
}

jx_frame * jx_top(jx_cntx * cntx)
{
	if (cntx == NULL) {
		return NULL;
	}

	return jxv_get_ptr(jxa_top(cntx->object_stack));
}

bool jx_push_mode(jx_cntx * cntx, jx_mode mode)
{
	jx_frame * frame;

	if (cntx == NULL || cntx->object_stack == NULL) {
		return false;
	}

	if ((frame = calloc(1, sizeof(jx_frame))) == NULL) {
		jx_set_error(cntx, JX_ERROR_LIBC);
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

bool utf16_surrogate(uint16_t value)
{
	return value >= 0xD800 && value <= 0xDFFF;
}

bool utf16_high_surrogate(uint16_t value)
{
	return value >= 0xD800 && value <= 0xDBFF;
}

bool utf16_low_surrogate(uint16_t value)
{
	return value >= 0xDC00 && value <= 0xDFFF;
}

int utf16_decode(uint16_t pair[2])
{
	int code;

	if (!utf16_surrogate(pair[0])) {
		return pair[0];
	}

	if (!utf16_high_surrogate(pair[0])) {
		return -1;
	}

	if (!utf16_low_surrogate(pair[1])) {
		return -1;
	}

	code = (pair[0] - 0xD800) << 10;
	code |= pair[1] - 0xDC00;
	code += 0x10000;

	return code;
}

int utf8_length(unsigned char * src)
{
	if (src == NULL) {
		return -1;
	}

	if ((src[0] & 0x80) == 0) { /* 7-bit ASCII byte */
		return 1;
	}
	else if ((src[0] & 0xc0) == 0x80) { /* continuation byte */
		return -1;
	}
	else { /* multi-byte character */
		int i = 0;

		while (src[0] & (1 << (7 - i))) {
			i++;

			if (i > 4) { /* not a valid UTF-8 byte */
				return -1;
			}
		}

		return i;
	}
}

int utf8_length_for_value(int code_point)
{
	if (code_point < 0) {
		return -1;
	}

	if (code_point <= 0x7f) {
		return 1;
	}
	else if (code_point <= 0x7ff) {
		return 2;
	}
	else if (code_point <= 0xffff) {
		return 3;
	}
	else if (code_point <= 0x10ffff) {
		return 4;
	}
	else {
		return -1;
	}
}

bool unicode_to_utf8(char out[5], int code_point)
{
	int bytes, byte, bit_in, bit_out;

	if (out == NULL) {
		return false;
	}

	if ((bytes = utf8_length_for_value(code_point)) == -1) {
		return false;
	}

	byte = 0;

	if (bytes == 1) {
		out[0] = code_point;
		out[1] = '\0';
		return true;
	}
	else if (bytes == 2) {
		bit_in = 10;
		bit_out = 4;

		out[0] = 0xC0;
	}
	else if (bytes == 3) {
		bit_in = 15;
		bit_out = 3;

		out[0] = 0xE0;
	}
	else {
		bit_in = 20;
		bit_out = 2;

		out[0] = 0xF0;
	}

	while (bit_in >= 0) {
		if (code_point & (1 << bit_in)) {
			out[byte] |= (1 << bit_out);
		}

		bit_in--;
		bit_out--;

		if (bit_out < 0) {
			out[++byte] = 0x80;
			bit_out = 5;
		}
	}

	out[bytes] = '\0';

	return true;
}

long jx_find_token(jx_cntx * cntx, const char * src, long pos, long end_pos)
{
	if (cntx == NULL) {
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

jx_token jx_token_type(const char * src, long pos)
{
	if (src[pos] == '[') {
		return JX_TOKEN_ARRAY_BEGIN;
	}
	else if (src[pos] == ',') {
		return JX_TOKEN_ARRAY_SEPARATOR;
	}
	else if (src[pos] == ']') {
		return JX_TOKEN_ARRAY_END;
	}
	else if (src[pos] == '-' || (src[pos] >= '0' && src[pos] <= '9')) {
		return JX_TOKEN_NUMBER;
	}
	else if (src[pos] == '"') {
		return JX_TOKEN_STRING;
	}
	else if (src[pos] >= 'a' && src[pos] <= 'z') {
		return JX_TOKEN_KEYWORD;
	}
	else if (((unsigned char)src[pos] & 0xC0) == 0xC0) {
		return JX_TOKEN_UNICODE;
	}
	else {
		return JX_TOKEN_NONE;
	}
}

jx_utoken jx_unicode_token_type(jx_cntx * cntx)
{
	if (cntx->ext & JX_EXT_UTF8_PI) {
		/* U+03C0 (GREEK LOWERCASE PI) */
		if (strncmp(cntx->uni_tok, "\xCF\x80", cntx->uni_tok_len) == 0) {
			return JX_UNI_LOWER_PI;
		}
	}

	return JX_UNI_UNSUPPORTED;
}

jx_value * jx_unicode_token_object(jx_utoken type)
{
	if (type == JX_UNI_LOWER_PI) {
		return jxv_number_new(3.14159);
	}

	return NULL;
}

bool jx_start_token(jx_token token)
{
	switch (token) {
		case JX_TOKEN_ARRAY_BEGIN:
		case JX_TOKEN_NUMBER:
		case JX_TOKEN_STRING:
		case JX_TOKEN_KEYWORD:
		case JX_TOKEN_UNICODE:
			return true;
		default:
			return false;
	}
}

void jx_illegal_token(jx_cntx * cntx, const char * src, long pos, long end_pos)
{
	unsigned char * buf;
	char * token_name;
	char token[2];

	if (cntx == NULL || src == NULL) {
		return;
	}

	buf = (unsigned char *)src;

	if (buf[pos] < 0x20) {
		token_name = "control character";
	}
	else if (buf[pos] > 0x7f) {
		token_name = "illegal character";
	}
	else {
		token[0] = src[pos];
		token[1] = '\0';

		token_name = token;
	}

	jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col, token_name);
}

long jx_parse_array(jx_cntx * cntx, const char * src, long pos, long end_pos, bool * next_token)
{
	jx_value * ret;
	jx_token token;

	if (cntx == NULL || src == NULL || next_token == NULL) {
		return -1;
	}

	if (pos < 0) {
		return -1;
	}

	ret = jx_get_return(cntx);

	if (ret != NULL) {
		jx_value * array = jx_get_value(cntx);

		if (!jxa_push(array, ret)) {
			jx_set_error(cntx, JX_ERROR_LIBC);
			return -1;
		}

		jx_set_return(cntx, NULL);

		jx_set_state(cntx, JX_ARRAY_STATE_NEW_MEMBER);
	}

	token = jx_token_type(src, pos);

	/* ----------------------------------------------------------------------*
	 *                       Overview of Array States                        *
	 * ----------------------------------------------------------------------*
	 * [      # DEFAULT STATE     - accept: member, closing bracket          *
	 * [ m    # NEW MEMBER STATE  - accept: seperator token, closing bracket *
	 * [ m,   # SEPARATOR STATE   - accept: member                           *
	 * ]      # TERMINAL STATE    - return array to previous frame           *
	 * ----------------------------------------------------------------------*/
	if (token == JX_TOKEN_ARRAY_SEPARATOR) {
		if (jx_get_state(cntx) != JX_ARRAY_STATE_NEW_MEMBER) {
			jx_set_error(cntx, JX_ERROR_UNEXPECTED_TOKEN, cntx->line, cntx->col, ",");
			return -1;
		}

		pos++;

		cntx->col++;

		jx_set_state(cntx, JX_ARRAY_STATE_SEPARATOR);

		*next_token = true;
	}
	else if (token == JX_TOKEN_ARRAY_END) {
		jx_value * array;

		if (jx_get_state(cntx) == JX_ARRAY_STATE_SEPARATOR) {
			jx_set_error(cntx, JX_ERROR_UNEXPECTED_TOKEN, cntx->line, cntx->col, ",");
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

		*next_token = true;
	}
	else {
		if (token == JX_TOKEN_NONE) {
			jx_illegal_token(cntx, src, pos, end_pos);
			return -1;
		}

		if (jx_get_state(cntx) == JX_ARRAY_STATE_NEW_MEMBER) {
			jx_set_error(cntx, JX_ERROR_EXPECTED_TOKEN, cntx->line, cntx->col, ",");
			return -1;
		}

		*next_token = false;
	}

	return pos;
}

long jx_parse_number(jx_cntx * cntx, const char * src, long pos, long end_pos, bool * done)
{
	jx_frame * frame;
	jx_state state;

	char c;

	bool symbol_end = false;

	if (cntx == NULL || src == NULL || done == NULL) {
		return -1;
	}

	if (pos < 0) {
		return -1;
	}

	if ((frame = jx_top(cntx)) == NULL) {
		return -1;
	}

	state = frame->state;

	while (pos <= end_pos) {
		c = src[pos];

		if (c == '-' || c == '+') {
			if (!(state & JX_NUM_ACCEPT_SIGN)) {
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"illegal position for sign character in number");
				return -1;
			}

			state &= ~(JX_NUM_ACCEPT_SIGN | JX_NUM_IS_VALID);
		}
		else if (c >= '0' && c <= '9') {
			if (!(state & JX_NUM_ACCEPT_DIGITS)) {
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
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
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"illegal position for decimal point in number");
				return -1;
			}

			state |= (JX_NUM_HAS_DEC_PT | JX_NUM_ACCEPT_DIGITS);
			state &= ~(JX_NUM_ACCEPT_DEC_PT | JX_NUM_ACCEPT_EXP | JX_NUM_IS_VALID);
		}
		else if (c == 'e' || c == 'E') {
			if (!(state & JX_NUM_ACCEPT_EXP)) {
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
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

		if (cntx->tok_buf_pos == JX_TOKEN_BUF_SIZE - 1) {
			jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
				"number too large");
			return -1;
		}

		cntx->tok_buf[cntx->tok_buf_pos++] = src[pos++];

		cntx->col++;
	}

	if (symbol_end) {
		if (state & JX_NUM_IS_VALID) {
			jx_value * number;

			cntx->tok_buf[cntx->tok_buf_pos] = '\0';
			cntx->tok_buf_pos = 0;

			number = jxv_number_new(strtod(cntx->tok_buf, NULL));

			if (number == NULL) {
				jx_set_error(cntx, JX_ERROR_LIBC);
				return -1;
			}

			frame->value = number;

			*done = true;
		}
		else {
			jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
				"invalid number");
			return -1;
		}
	}

	frame->state = state;

	return pos;
}

long jx_parse_unicode_seq(jx_cntx * cntx, const char * src, long pos, long end_pos, bool * done)
{
	jx_state state;

	if (cntx == NULL || src == NULL || done == NULL) {
		return -1;
	}

	if (pos < 0) {
		return -1;
	}

	state = jx_get_state(cntx);

	while (pos <= end_pos) {
		int index = cntx->code_index;

		if (src[pos] >= '0' && src[pos] <= '9') {
			cntx->code[index] <<= 4;
			cntx->code[index] |= (src[pos++] - '0');
			cntx->shifts++;
			cntx->col++;
		}
		else if (src[pos] >= 'a' && src[pos] <= 'f') {
			cntx->code[index] <<= 4;
			cntx->code[index] |= (src[pos++] - 'a') + 0xa;
			cntx->shifts++;
			cntx->col++;
		}
		else if (src[pos] >= 'A' && src[pos] <= 'F') {
			cntx->code[index] <<= 4;
			cntx->code[index] |= (src[pos++] - 'A') + 0xa;
			cntx->shifts++;
			cntx->col++;
		}
		else {
			jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
				"illegal unicode escape sequence");

			return -1;
		}

		if (cntx->shifts == 4) {
			break;
		}
	}

	if (cntx->shifts == 4) {
		int code_point = -1;

		if (state & JX_STRING_SURROGATE) {
			/* Check to make sure that we have a low surrogate value,
			 * and decode, otherwise return error. */
			if (utf16_low_surrogate(cntx->code[1])) {
				code_point = utf16_decode(cntx->code);
				state &= ~JX_STRING_SURROGATE;
				cntx->code_index = 0;
			}
			else {
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"illegal surrogate pair in unicode escape sequence");

				return -1;
			}
		}
		else {
			/* Non-surrogates are single 16-bit numbers that
			 * correspond one-to-one with characters in the BMP
			 * (Basic Multilingual Plane) or (plane-0) */
			if (!utf16_surrogate(cntx->code[0])) {
				code_point = cntx->code[0];
			}
			else {
			 	/* The first surrogate should always be the high surrogate,
				 * and we must wait for the low surrogate before we can
				 * decode. */
				if (utf16_high_surrogate(cntx->code[0])) {
					state |= JX_STRING_SURROGATE;
					cntx->code_index = 1;
				}
				else {
					jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
						"illegal surrogate pair in unicode escape sequence");

					return -1;
				}
			}
		}

		if (code_point != -1) {
			if (code_point >= 0x20 && code_point != 0x7f) {
				char utf8_buf[5];

				jx_value * str = jx_get_value(cntx);

				if (unicode_to_utf8(utf8_buf, code_point)) {
					jxs_append_str(str, utf8_buf);
				}
				else {
					jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
						"illegal character in string");

					return -1;
				}
			}
			else {
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"control character in string");

				return -1;
			}
		}

		cntx->shifts = 0;

		*done = true;
	}

	jx_set_state(cntx, state);

	return pos;
}

long jx_parse_string(jx_cntx * cntx, const char * src, long pos, long end_pos, bool * done)
{
	jx_frame * frame;
	jx_value * str;

	unsigned char * buf;

	jx_state state;

	bool escape_done;

	if (cntx == NULL || src == NULL || done == NULL) {
		return -1;
	}

	if (pos < 0) {
		return -1;
	}

	if ((frame = jx_top(cntx)) == NULL) {
		return -1;
	}

	buf = (unsigned char *)src;
	state = frame->state;
	str = frame->value;

	while (pos <= end_pos) {
		if (state & JX_STRING_ESCAPE) {
			if (buf[pos] != 'u' && (state & JX_STRING_SURROGATE)) {
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"invalid unicode character in string");

				return -1;
			}

			switch (buf[pos]) {
				case '"':
				case '\\':
				case '/':
					jxs_append_chr(str, buf[pos]);
					break;
				case 'b':
					jxs_append_chr(str, '\b');
					break;
				case 'f':
					jxs_append_chr(str, '\f');
					break;
				case 'n':
					jxs_append_chr(str, '\n');
					break;
				case 'r':
					jxs_append_chr(str, '\r');
					break;
				case 't':
					jxs_append_chr(str, '\t');
					break;
				case 'u':
					state |= JX_STRING_UNICODE;
					break;
				default:
					jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
						"unrecognized escape sequence");

					return -1;
			}

			state &= ~JX_STRING_ESCAPE;
		}
		else if (state & JX_STRING_UTF8) {
			while (pos <= end_pos) {
				if ((buf[pos] & 0xC0) != 0x80) {
					jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
						"illegal character in string");
					return -1;
				}

				jxs_append_chr(str, buf[pos++]);

				if (--cntx->uni_tok_len == 0) {
					state &= ~JX_STRING_UTF8;
					cntx->col++;
					break;
				}
			}

			continue;
		}
		else if (state & JX_STRING_UNICODE) {
			escape_done = false;

			jx_set_state(cntx, state);

			pos = jx_parse_unicode_seq(cntx, src, pos, end_pos, &escape_done);

			if (pos == -1) {
				return -1;
			}

			state = jx_get_state(cntx);

			if (escape_done) {
				state &= ~JX_STRING_UNICODE;
			}

			continue;
		}
		else {
			if (buf[pos] != '\\' && (state & JX_STRING_SURROGATE)) {
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
					"invalid unicode character in string");

				return -1;
			}

			if (buf[pos] == '\\') {
				state |= JX_STRING_ESCAPE;
			}
			else if ((buf[pos] & 0xC0) == 0xC0) {
				int len = utf8_length(buf + pos);

				if (len == -1) {
					jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
						"illegal character in string");
					return -1;
				}

				jxs_append_chr(str, buf[pos++]);

				len--;

				state = JX_STRING_UTF8;

				cntx->uni_tok_len = len;

				continue;
			}
			else if (buf[pos] == '"') {
				state = JX_STRING_END;
			}
			else {
				if (buf[pos] >= 0x20 && buf[pos] <= 0x7e) {
					jxs_append_chr(str, buf[pos]);
				}
				else {
					jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col,
						"control character in string");
					return -1;
				}
			}
		}

		pos++;
		cntx->col++;

		if (state == JX_STRING_END) {
			*done = true;
			break;
		}
	}

	frame->state = state;

	return pos;
}

long jx_parse_keyword(jx_cntx * cntx, const char * src, long pos, long end_pos, bool * done)
{
	if (cntx == NULL || src == NULL || done == NULL) {
		return -1;
	}

	while (pos <= end_pos) {
		if (cntx->tok_buf_pos >= 5 || !(src[pos] >= 'a' && src[pos] <= 'z')) {
			jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col, cntx->tok_buf);
			return -1;
		}

		cntx->tok_buf[cntx->tok_buf_pos++] = src[pos++];
		cntx->tok_buf[cntx->tok_buf_pos] = '\0';

		if (cntx->tok_buf_pos < 4) {
			continue;
		}

		if (strcmp(cntx->tok_buf, "null") == 0) {
			jx_set_value(cntx, jxv_null());

			cntx->col += cntx->tok_buf_pos;

			*done = true;

			break;
		}
		else if (strcmp(cntx->tok_buf, "true") == 0) {
			jx_set_value(cntx, jxv_bool_new(true));

			cntx->col += cntx->tok_buf_pos;

			*done = true;

			break;
		}
		else if (strcmp(cntx->tok_buf, "false") == 0) {
			jx_set_value(cntx, jxv_bool_new(false));

			cntx->col += cntx->tok_buf_pos;

			*done = true;

			break;
		}
	}

	return pos;
}

long jx_parse_utf8(jx_cntx * cntx, const char * src, long pos, long end_pos, bool * done)
{
	if (cntx == NULL || src == NULL || done == NULL) {
		return -1;
	}

	while (pos <= end_pos) {
		/* If non-continuation byte found after starting byte in sequence. */
		if (cntx->uni_tok_i > 0 && ((unsigned char)src[pos] & 0xC0) != 0x80) {
			jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col, "illegal character");
			return -1;
		}

		cntx->uni_tok[cntx->uni_tok_i++] = src[pos++];

		/* Check if we have a complete sequence. */
		if (cntx->uni_tok_i == cntx->uni_tok_len) {
			jx_value * value;
			jx_utoken type;

			cntx->uni_tok[cntx->uni_tok_i] = '\0';

			if ((type = jx_unicode_token_type(cntx)) == JX_UNI_UNSUPPORTED) {
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col, cntx->uni_tok);
				return -1;
			}

			value = jx_unicode_token_object(type);

			if (value == NULL) {
				jx_set_error(cntx, JX_ERROR_LIBC);
				return -1;
			}

			jx_set_value(cntx, value);

			cntx->col++;

			*done = true;

			break;
		}
	}

	return pos;
}

int jx_parse_json(jx_cntx * cntx, const char * src, long n_bytes)
{
	long pos;
	long end_pos;

	jx_mode mode;
	jx_token token;

	if (cntx == NULL || src == NULL) {
		return -1;
	}

	if (cntx->error != JX_ERROR_NONE) {
		return -1;
	}

	if (jx_get_mode(cntx) == JX_MODE_UNDEFINED) {
		if (!jx_push_mode(cntx, JX_MODE_START)) {
			return -1;
		}
	}

	pos = 0;
	end_pos = n_bytes - 1;

	while (pos <= end_pos) {
		pos = jx_find_token(cntx, src, pos, end_pos);

		if (pos == -1) {
			break;
		}

		mode = jx_get_mode(cntx);

		if (mode == JX_MODE_PARSE_ARRAY) {
			bool next_token = false;

			pos = jx_parse_array(cntx, src, pos, end_pos, &next_token);

			if (pos == -1) {
				return -1;
			}

			if (next_token) {
				continue;
			}
		}
		else if (
			mode == JX_MODE_PARSE_NUMBER ||
			mode == JX_MODE_PARSE_STRING ||
			mode == JX_MODE_PARSE_KEYWORD ||
			mode == JX_MODE_PARSE_UTF8) {
			bool done = false;

			switch (mode) {
				case JX_MODE_PARSE_NUMBER:
					pos = jx_parse_number(cntx, src, pos, end_pos, &done);
					break;
				case JX_MODE_PARSE_STRING:
					pos = jx_parse_string(cntx, src, pos, end_pos, &done);
					break;
				case JX_MODE_PARSE_KEYWORD:
					pos = jx_parse_keyword(cntx, src, pos, end_pos, &done);
					break;
				case JX_MODE_PARSE_UTF8:
					pos = jx_parse_utf8(cntx, src, pos, end_pos, &done);
					break;
				default:
					break;
			}

			if (pos == -1) {
				return -1;
			}

			if (done) {
				jx_value * v = jx_get_value(cntx);

				jx_pop_mode(cntx);
				jx_set_return(cntx, v);

				cntx->inside_token = false;
			}

			continue;
		}
		else if (mode == JX_MODE_DONE) {
			jx_set_error(cntx, JX_ERROR_TRAILING_CHARS, cntx->line, cntx->col, src[pos]);
			return -1;
		}

		token = jx_token_type(src, pos);

		if (!jx_start_token(token)) {
			jx_illegal_token(cntx, src, pos, end_pos);
			return -1;
		}

		if (cntx->depth == 0 && token != JX_TOKEN_ARRAY_BEGIN) {
			jx_set_error(cntx, JX_ERROR_INVALID_ROOT, cntx->line, cntx->col);
			return -1;
		}

		if (token == JX_TOKEN_ARRAY_BEGIN) {
			jx_value * array;

			array = jxa_new(JX_DEFAULT_ARRAY_SIZE);

			if (array == NULL) {
				jx_set_error(cntx, JX_ERROR_LIBC);
				return -1;
			}

			if (!jx_push_mode(cntx, JX_MODE_PARSE_ARRAY)) {
				jxv_free(array);
				return -1;
			}

			jx_set_value(cntx, array);

			pos++;

			cntx->col++;
			cntx->depth++;
		}
		else if (token == JX_TOKEN_NUMBER) {
			if (!jx_push_mode(cntx, JX_MODE_PARSE_NUMBER)) {
				return -1;
			}

			cntx->inside_token = true;

			jx_set_state(cntx, JX_NUM_DEFAULT);
		}
		else if (token == JX_TOKEN_STRING) {
			jx_value * str;

			str = jxs_new(NULL);

			if (str == NULL) {
				jx_set_error(cntx, JX_ERROR_LIBC);
				return -1;
			}

			if (!jx_push_mode(cntx, JX_MODE_PARSE_STRING)) {
				jxv_free(str);
				return -1;
			}

			jx_set_value(cntx, str);

			pos++;

			cntx->col++;
			cntx->inside_token = true;
		}
		else if (token == JX_TOKEN_KEYWORD) {
			if (!jx_push_mode(cntx, JX_MODE_PARSE_KEYWORD)) {
				jx_set_error(cntx, JX_ERROR_LIBC);
				return -1;
			}

			cntx->tok_buf_pos = 0;
			cntx->inside_token = true;
		}
		else if (token == JX_TOKEN_UNICODE) {
			int len = utf8_length((unsigned char *)src + pos);

			if (len == -1) {
				jx_set_error(cntx, JX_ERROR_ILLEGAL_TOKEN, cntx->line, cntx->col, "illegal character");
				return -1;
			}

			cntx->uni_tok_len = len;
			cntx->uni_tok_i = 0;
			cntx->inside_token = true;

			if (!jx_push_mode(cntx, JX_MODE_PARSE_UTF8)) {
				return -1;
			}
		}
	}

	return jx_get_mode(cntx) == JX_MODE_DONE;
}

jx_value * jx_get_result(jx_cntx * cntx)
{
	jx_value * ret;

	if (cntx == NULL) {
		return NULL;
	}

	if (cntx->error != JX_ERROR_NONE) {
		return NULL;
	}

	if (jx_get_mode(cntx) != JX_MODE_DONE) {
		jx_set_error(cntx, JX_ERROR_INCOMPLETE_OBJECT, cntx->line, cntx->col);
		return NULL;
	}

	ret = jx_get_return(cntx);

	jx_set_return(cntx, NULL);

	return ret;
}