/*---------------------------------------------------------------------
| jx_util.h
----------------------------------------------------------------------
| jxutil project
|
| Created by Cory Montgomery
| cory.james.montgomery@gmail.com
|
| https://cjmont32.dev/jxutil
| https://github.com/cjmont32/jxutil
|
----------------------------------------------------------------------
| MIT License
|
| Copyright (c) 2018-2023, Cory Montgomery
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions
| are met:
|
|    1. Redistributions of source code must retain the above
|       copyright notice, this list of conditions and the following
|       disclaimer.
|    2. Redistributions in binary form must reproduce the above
|       copyright notice, this list of conditions and the following
|       disclaimer in the documentation and/or other materials
|       provided with the distribution.
|
| THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
| "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
| LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
| FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
| COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
| INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
| BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
| CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
| LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
| WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
| POSSIBILITY OF SUCH DAMAGE.
*/

#include <jx.h>
#include <jx_json.h>

ssize_t jx_read(jx_cntx *cntx, int fd, size_t n_bytes);
ssize_t jx_read_block(jx_cntx *cntx, int fd, ssize_t n_bytes);
jx_value *jx_obj_from_file(jx_cntx *cntx, const char *filename);
void jx_set_read_buffer_size(jx_cntx *cntx, size_t sz);
