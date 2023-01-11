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