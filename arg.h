#ifndef __INCL_ARG_H
#define __INCL_ARG_H

#include <argp.h>

#define ARG_STATUS_UNKNOWN          0x00
#define ARG_STATUS_HELP             0x01
#define ARG_STATUS_INVALID_USAGE    0x02

#define opt_full(typename, nameval, requiredval, shortval, longval, stringval, flagsval, docstrval, groupval) { \
    .type = #typename, \
    .name = nameval, \
    .required = requiredval, \
    .shortopt = shortval, \
    .longopt = longval, \
    .string = stringval, \
    .flags = flagsval, \
    .docstr = docstrval, \
    .group = groupval, \
},

#define opt_required(typename, nameval, shortval, longval, stringval, flagsval, docstrval, groupval) \
    opt_full(typename, nameval, true, shortval, longval, stringval, flagsval, docstrval, groupval) 

/*
#define opt_optional(typename, name) { \
    .key = name, \
    .required = false, \
    .type = #typename, \
},

#define opt_default(typename, name, dflt_val) { \
    .key = name, \
    .required = false, \
    .type = #typename, \
    .has_default = true, \
    .dflt.typename = dflt_val \
},
*/

#define opt_last {0}

union opt_t {
    char *string;
    int integer;
    double real;
    bool boolean;
};

struct opt_decl_t {
    char *type;
    char *name; // lua_dir, for ARG_lua_dir
    bool required;
    char shortopt;
    char *longopt;
    char *string; // DIR, for usage string
    int flags;
    char *docstr;
    int group;
        
    bool got;
    bool has_default;
    union opt_t dflt;
};

#define opt_get_s(key) opts[ARG_##key].value.string
#define opt_get_i(key) opts[ARG_##key].value.integer
#define opt_get_r(key) opts[ARG_##key].value.real
#define opt_get_b(key) opts[ARG_##key].value.boolean

extern union opt_t opts[];

/* Your args here - - - - - -*/
#define ARG_lua_dir         0x00
#define ARG_repetitions     0x01
#define ARG_times           0x02

#define ARG_numopts         0x03

/* - - - - - - - - - - - - - */

//bool opt_args(int argc, char **argv, union opt_t opts[]);
bool arg_args(int argc, char **argv);
int arg_status();


#endif
