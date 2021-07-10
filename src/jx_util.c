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

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define JX_RB_SIZE cntx->read_buffer_size

#define JX_MAX_READ_BUF_SIZE 8192

ssize_t jx_read(jx_cntx * cntx, int fd, size_t n_bytes)
{
    long n_read;
    char * buf;

    if (n_bytes == 0) {
        return -1;
    }

    if (n_bytes > JX_MAX_READ_BUF_SIZE) {
        n_bytes = JX_MAX_READ_BUF_SIZE;
    }

    buf = alloca(n_bytes);

    n_read = read(fd, buf, n_bytes);

    if (n_read == -1) {
        if (errno != EAGAIN && errno != EINTR) {
            jx_set_error(cntx, JX_ERROR_LIBC);
            return -2;
        }

        return -1;
    }

    if (n_read == 0)
        return 0;

    if (jx_parse_json(cntx, buf, n_read) == -1) {
        return -2;
    }

    return n_read;
}

ssize_t jx_read_block(jx_cntx * cntx, int fd, ssize_t n_bytes)
{
    ssize_t n_remaining, n_read;

    if (n_bytes <= 0)
        return -1;

    n_remaining = n_bytes;

    do {
        n_read = jx_read(cntx, fd, (n_remaining < JX_RB_SIZE) ? n_remaining : JX_RB_SIZE);

        if (n_read > 0) {
            n_remaining -= n_read;

            if (n_remaining == 0) {
                break;
            }
        }
    } while (n_read > 0);

    return n_bytes - n_remaining;
}

jx_value * jx_obj_from_file(jx_cntx * cntx, const char * filename)
{
    int fd, n_read;

    if ((fd = open(filename, O_RDONLY)) == -1) {
        jx_set_error(cntx, JX_ERROR_LIBC);
        return NULL;
    }

    do {
        n_read = jx_read(cntx, fd, JX_RB_SIZE);
    } while (n_read > 0);

    close(fd);

    if (n_read < 0) {
        return NULL;
    }

    return jx_get_result(cntx);
}

void jx_set_read_buffer_size(jx_cntx * cntx, size_t sz)
{
    cntx->read_buffer_size = sz;
}