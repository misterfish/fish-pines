#include <stdbool.h>

#include <fish-util.h>

#include "arg.h"

/* Your args in 3 places: here, 
 * arg.h (ARG_xxx vals),
 * and options[].
 *
 * Attempts to supply options[] dynamically (since all the info is already
 * there in opts_decl[]) ended in corruption.
 */


/* Num required args (not opts) */
const int NUM_ARGS = 0;

/* Text in usage msg, after args. */
const char *ARGS_DOC = "";

/* Separated by vertical tab. */
const char *DOC = ""; // "Before options \v after options";

struct opt_decl_t opts_decl[] = {
    /* 
     *    -d, --lua-dir=DIR     Path to directory ...
     */
    opt_required(string, "lua_dir", 'd', "lua-dir", "DIR", 0 /* ARG_ flags */, 
        "Path to directory containing .lua files (required)",
    0 /* group */)

        /* test
    opt_required(real, "repetitions", 'r', "repetitions", "repetitions", 0, "Something called repetitions", 0)
    opt_required(integer, "times", 't', "times", "times", 0, "Something called times", 0)
    */


    opt_last
};

/* - - - - - - */

#define opt_set(name, key, valuearg) \
    if (! strcmp("string", opts_decl[ARG_##name].type)) { \
        opts_decl[ARG_##name].got = true; \
        opts[ARG_##name].string = valuearg; \
    } \
    else if (! strcmp("boolean", opts_decl[ARG_##name].type)) { \
        opts_decl[ARG_##name].got = true; \
        if (strcmp(valuearg, "1") && strcmp(valuearg, "0")) { \
            _(); \
            Y(#name); \
            BR(valuearg); \
            warn("Invalid boolean value for key %s (%s)", _s, _t); \
            return false; \
        } \
        opts[ARG_##name].boolean = (bool) valuearg; \
    } \
    else if (! strcmp("real", opts_decl[ARG_##name].type)) { \
        opts_decl[ARG_##name].got = true; \
        double d; \
        if (! f_atod(valuearg, &d)) { \
            _(); \
            Y(#name); \
            BR(valuearg); \
            warn("Invalid double value for key %s (%s)", _s, _t); \
            return false; \
        } \
        opts[ARG_##name].real = d; \
    } \
    else if (! strcmp("integer", opts_decl[ARG_##name].type)) { \
        opts_decl[ARG_##name].got = true; \
        int i; \
        if (! f_atoi(valuearg, &i)) { \
            _(); \
            Y(#name); \
            BR(valuearg); \
            warn("Invalid integer value for key %s (%s)", _s, _t); \
            return false; \
        } \
        opts[ARG_##name].integer = i; \
    } \

union opt_t opts[ARG_numopts];

static struct {
    char *prog_name;
    int num_args;

    int status;
} g;

/* OPTION_ARG_OPTIONAL is poorly named. It means the value is 'optional' --
 * in other words, the opt is a flag, like -s in 'ln -s'. 
 *
 * And added something like --help=abc is an error, which is good. 
 */

static struct argp_option options[] = {
    {
        "help",
        'h', // 0, //'n', // key
        0, // name
        OPTION_ARG_OPTIONAL, // flags
        0, // text
        0
    }, 
    {
        "lua_dir",
        'd',
        "DIR",
        0, // flags
        "Path to directory containing .lua files (required)",
        0
    }, 
    {0}
};

static void arg_state_help(struct argp_state *state);
static void arg_usage(struct argp *argp);
static void arg_state_usage(struct argp_state *state);
static error_t argp_parser(int key, char *arg, struct argp_state *state);

bool arg_args(int argc, char **argv) {
    g.prog_name = argv[0];

    struct argp argp = {0};

    argp.args_doc = ARGS_DOC;
    argp.doc = DOC;

    //struct argp_option *options = f_calloc(ARG_numopts, sizeof(struct argp_option));
    /*
    struct argp_option *options = calloc(ARG_numopts, sizeof(struct argp_option));
    for (int i = 0; i < ARG_numopts; i++) {
        struct opt_decl_t opt = opts_decl[i];
        struct argp_option *ao = f_malloct(struct argp_option);
        memset(ao, 0, sizeof(*ao));
        ao->name = opt.longopt;
        ao->key = opt.shortopt;
        ao->arg = opt.string;
        ao->flags = opt.flags;
        ao->doc = opt.docstr;
        ao->group = opt.group;

        options[i] = *ao;
    }
    struct argp_option *ao_sentinel = f_malloct(struct argp_option);
    memset(ao_sentinel, 0, sizeof(*ao_sentinel));
    options[ARG_numopts] = *ao_sentinel;
    */
    argp.options = options;

    argp.parser = (argp_parser_t) argp_parser;

    int arg_index;

    if (
        argp_parse(&argp, argc, argv, 
            0,
            &arg_index, 
            0 // input = extra data for parsing function
            )
    ) {
        iwarn("Can't parse args.");
        return false;
    }

    if (g.num_args != NUM_ARGS) {
        arg_usage(&argp);
        return false;
    }

    bool usage = false;
    for (int i = 0, l = sizeof(opts_decl) / sizeof opts_decl[0]; i < l - 1; i++) {
        struct opt_decl_t opt_decl = opts_decl[i];

        if (! opt_decl.got) {
            if (opt_decl.required) {
                _();
                BR(opt_decl.longopt);
                warn("Missing value for required option %s", _s);
                usage = true;
            }
            else {
                if (opt_decl.has_default) 
                    opts[i] = opt_decl.dflt;
            }
        }
    }
    if (usage) {
        arg_usage(&argp);
        return false;
    }

    return true;
}

int arg_status() {
    return g.status;
}

static bool try_key(int key, char *arg, struct argp_state *state) {
    if (key == 'd') {
        opt_set(lua_dir, key, arg)
    }
    else if (key == 'h') {
        arg_state_help(state);
        return true;
    }
    else if (key == ARGP_KEY_ARG) { 
        char *the_arg = state->argv[state->next-1];
        (void) the_arg;
        g.num_args++;
        return true;
    }
    return true;
}

static error_t argp_parser(int key, char *arg, struct argp_state *state) {

    if (! try_key(key, arg, state)) 
        arg_state_usage(state);

    return 0;
}

static void arg_state_help(struct argp_state *state) {
    argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
    g.status = ARG_STATUS_HELP;
}

static void arg_state_usage(struct argp_state *state) {
    argp_state_help(state, stdout, ARGP_HELP_STD_HELP);
    g.status = ARG_STATUS_HELP;
}

static void arg_usage(struct argp *args) {
    argp_help(args, stderr, ARGP_HELP_STD_HELP, g.prog_name);
    g.status = ARG_STATUS_INVALID_USAGE;
}


