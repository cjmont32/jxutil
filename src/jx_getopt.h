#pragma once

extern char *jx_optarg;

int jx_getopt(int argc, char **argv, const char *optstr);

#ifdef WIN32

#define getopt jx_getopt
#define optarg jx_optarg

#endif
