#include "arguments.h"

#define EXPECTED_ARGUMENTS 1

static struct argp_option argp_options[] = {
    {"hook", 'h', "FILE", 0, "Hooks the matching GOT symbol with the provided FILE's contents. The filename must end with '.text'", 0},
    {"quiet", 'q', NULL, 0, "Surpresses output.", 0},
    {"verbose", 'v', NULL, 0, "Enables verbose output.", 0},
    {0}
};

static const char doc[] = "Xplore remote processes by hooking their GOT functions.";
static const char args_doc[] = "EXECUTABLE";

static error_t parse_option (int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = (struct arguments*) state->input;

    switch (key)
    {
    case 'q':
        arguments->silent = 1;
        break;
    case 'v':
        arguments->verbose = 1;
        break;
    case 'h':
        arguments->hook_filenames[arguments->hook_count] = arg;
        arguments->hook_count++;
        break;
    case ARGP_KEY_ARG:
        if (arguments->executable) {
            return ARGP_ERR_UNKNOWN;
        }
        arguments->executable = arg;
        break;
    case ARGP_KEY_END:
        if (state->arg_num < EXPECTED_ARGUMENTS) {
            /* Not enough arguments. */
            argp_usage(state);
        }
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static struct argp argp = {argp_options, parse_option, args_doc, doc, NULL, NULL};

error_t parse_args(int argc, char** argv, struct arguments* arguments, int* arg_index) {
    return argp_parse(&argp, argc, argv, 0, arg_index, arguments);
}
