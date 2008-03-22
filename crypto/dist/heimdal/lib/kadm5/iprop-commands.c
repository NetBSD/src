#include <stdio.h>
#include <getarg.h>
#include <sl.h>
#include "iprop-commands.h"

static int
dump_wrap(int argc, char **argv)
{
    struct dump_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "config-file", 'c', arg_string, NULL, "configuration file", "file" },
        { "realm", 'r', arg_string, NULL, "realm", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.config_file_string = NULL;
    opt.realm_string = NULL;
    args[0].value = &opt.config_file_string;
    args[1].value = &opt.realm_string;
    args[2].value = &help_flag;
    if(getarg(args, 3, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = iprop_dump(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 3, "dump", "");
    return 0;
}

static int
truncate_wrap(int argc, char **argv)
{
    struct truncate_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "config-file", 'c', arg_string, NULL, "configuration file", "file" },
        { "realm", 'r', arg_string, NULL, "realm", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.config_file_string = NULL;
    opt.realm_string = NULL;
    args[0].value = &opt.config_file_string;
    args[1].value = &opt.realm_string;
    args[2].value = &help_flag;
    if(getarg(args, 3, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = iprop_truncate(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 3, "truncate", "");
    return 0;
}

static int
replay_wrap(int argc, char **argv)
{
    struct replay_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "start-version", 0, arg_integer, NULL, "start replay with this version", "version-number" },
        { "end-version", 0, arg_integer, NULL, "end replay with this version", "version-number" },
        { "config-file", 'c', arg_string, NULL, "configuration file", "file" },
        { "realm", 'r', arg_string, NULL, "realm", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.start_version_integer = -1;
    opt.end_version_integer = -1;
    opt.config_file_string = NULL;
    opt.realm_string = NULL;
    args[0].value = &opt.start_version_integer;
    args[1].value = &opt.end_version_integer;
    args[2].value = &opt.config_file_string;
    args[3].value = &opt.realm_string;
    args[4].value = &help_flag;
    if(getarg(args, 5, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = iprop_replay(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 5, "replay", "");
    return 0;
}

static int
last_version_wrap(int argc, char **argv)
{
    struct last_version_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "config-file", 'c', arg_string, NULL, "configuration file", "file" },
        { "realm", 'r', arg_string, NULL, "realm", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.config_file_string = NULL;
    opt.realm_string = NULL;
    args[0].value = &opt.config_file_string;
    args[1].value = &opt.realm_string;
    args[2].value = &help_flag;
    if(getarg(args, 3, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = last_version(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 3, "last-version", "");
    return 0;
}

static int
help_wrap(int argc, char **argv)
{
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    args[0].value = &help_flag;
    if(getarg(args, 1, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 1) {
        fprintf(stderr, "Arguments given (%u) are more than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = help(NULL, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 1, "help", "command");
    return 0;
}

SL_cmd commands[] = {
        { "dump", dump_wrap, "dump", "Prints the iprop transaction log in text." },

        { "truncate", truncate_wrap, "truncate", "Truncate the log, preserve the version number." },

        { "replay", replay_wrap, "replay", "Replay the log on the database." },

        { "last-version", last_version_wrap, "last-version", "Print the last version of the log-file." },

        { "help", help_wrap, "help command", NULL },

    { NULL }
};
