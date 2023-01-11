/*---------------------------------------------------------------------
| jx_getopt.c
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

#include <jx_getopt.h>

#include <stdlib.h>
#include <string.h>

char *jx_optarg;

static const char *_optstr;
static int _optpos1, _optpos2;

int jx_getopt(int argc, char **argv, const char *optstr)
{
    char opt, *opts, *ptr;

    if (optstr != NULL && _optstr != optstr) {
        _optstr = optstr;
        _optpos1 = 1;
        _optpos2 = 0;
    }

    if (_optstr == NULL) {
        return -1;
    }

    while (1) {
        if (_optpos1 >= argc) {
            return -1;
        }

        opts = argv[_optpos1];

        if (_optpos2 == 0) {
            if (opts[0] != '-' || strlen(opts) < 2) {
                _optpos1 = argc;
                return '?';
            }
        }

        opt = opts[++_optpos2];

        if (opt == '\0') {
            _optpos1++;
            _optpos2 = 0;
            continue;
        }

        ptr = strchr(_optstr, opt);

        if (ptr == NULL) {
            _optpos1 = argc;
            return '?';
        }

        if (ptr[1] == ':') {
            _optpos1++;

            if (_optpos1 >= argc || strlen(opts + _optpos2) > 1) {
                return '?';
            }

            jx_optarg = argv[_optpos1];

            _optpos1++;
            _optpos2 = 0;
        }

        return *ptr;
    }
}