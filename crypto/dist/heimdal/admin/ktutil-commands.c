#include <stdio.h>
#include <getarg.h>
#include <sl.h>
#include "ktutil-commands.h"

static int
add_wrap(int argc, char **argv)
{
    struct add_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "principal", 'p', arg_string, NULL, "principal to add", "principal" },
        { "kvno", 'V', arg_integer, NULL, "key version number", NULL },
        { "enctype", 'e', arg_string, NULL, "encryption type", "enctype" },
        { "password", 'w', arg_string, NULL, "password for key", NULL },
        { "salt", 's', arg_negative_flag, NULL, "use unsalted keys", NULL },
        { "random", 'r', arg_flag, NULL, "generate random key", NULL },
        { "hex", 'H', arg_flag, NULL, "password is a hexadecimal string", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.principal_string = "";
    opt.kvno_integer = -1;
    opt.enctype_string = NULL;
    opt.password_string = NULL;
    opt.salt_flag = 1;
    opt.random_flag = 0;
    opt.hex_flag = 0;
    args[0].value = &opt.principal_string;
    args[1].value = &opt.kvno_integer;
    args[2].value = &opt.enctype_string;
    args[3].value = &opt.password_string;
    args[4].value = &opt.salt_flag;
    args[5].value = &opt.random_flag;
    args[6].value = &opt.hex_flag;
    args[7].value = &help_flag;
    if(getarg(args, 8, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = kt_add(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 8, "add", "");
    return 0;
}

static int
change_wrap(int argc, char **argv)
{
    struct change_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "realm", 'r', arg_string, NULL, "realm to use", "realm" },
        { "admin-server", 'a', arg_string, NULL, "server to contact", "host" },
        { "server-port", 's', arg_integer, NULL, "port number on server", "port number" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.realm_string = NULL;
    opt.admin_server_string = NULL;
    opt.server_port_integer = 0;
    args[0].value = &opt.realm_string;
    args[1].value = &opt.admin_server_string;
    args[2].value = &opt.server_port_integer;
    args[3].value = &help_flag;
    if(getarg(args, 4, argc, argv, &optidx))
        goto usage;
    if(help_flag)
        goto usage;
    ret = kt_change(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 4, "change", "[principal...]");
    return 0;
}

static int
copy_wrap(int argc, char **argv)
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
    if(argc - optidx != 2) {
        fprintf(stderr, "Need exactly 2 parameters (%u given).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = kt_copy(NULL, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 1, "copy", "source destination");
    return 0;
}

static int
get_wrap(int argc, char **argv)
{
    struct get_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "principal", 'p', arg_string, NULL, "admin principal", "principal" },
        { "enctypes", 'e', arg_strings, NULL, "encryption types to use", "enctype" },
        { "realm", 'r', arg_string, NULL, "realm to use", "realm" },
        { "admin-server", 'a', arg_string, NULL, "server to contact", "host" },
        { "server-port", 's', arg_integer, NULL, "port number on server", "port number" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.principal_string = NULL;
    opt.enctypes_strings.num_strings = 0;
    opt.enctypes_strings.strings = NULL;
    opt.realm_string = NULL;
    opt.admin_server_string = NULL;
    opt.server_port_integer = 0;
    args[0].value = &opt.principal_string;
    args[1].value = &opt.enctypes_strings;
    args[2].value = &opt.realm_string;
    args[3].value = &opt.admin_server_string;
    args[4].value = &opt.server_port_integer;
    args[5].value = &help_flag;
    if(getarg(args, 6, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 1) {
        fprintf(stderr, "Arguments given (%u) are less than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = kt_get(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.enctypes_strings);
    return ret;
usage:
    arg_printusage (args, 6, "get", "principal...");
    free_getarg_strings (&opt.enctypes_strings);
    return 0;
}

static int
list_wrap(int argc, char **argv)
{
    struct list_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "keys", 0, arg_flag, NULL, "show key values", NULL },
        { "timestamp", 0, arg_flag, NULL, "show timestamps", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.keys_flag = 0;
    opt.timestamp_flag = 0;
    args[0].value = &opt.keys_flag;
    args[1].value = &opt.timestamp_flag;
    args[2].value = &help_flag;
    if(getarg(args, 3, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = kt_list(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 3, "list", "");
    return 0;
}

static int
purge_wrap(int argc, char **argv)
{
    struct purge_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "age", 0, arg_string, NULL, "age to retiere", "time" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.age_string = "1 week";
    args[0].value = &opt.age_string;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = kt_purge(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "purge", "");
    return 0;
}

static int
remove_wrap(int argc, char **argv)
{
    struct remove_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "principal", 'p', arg_string, NULL, "principal to remove", "principal" },
        { "kvno", 'V', arg_integer, NULL, "key version to remove", "enctype" },
        { "enctype", 'e', arg_string, NULL, "enctype to remove", "enctype" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.principal_string = NULL;
    opt.kvno_integer = 0;
    opt.enctype_string = NULL;
    args[0].value = &opt.principal_string;
    args[1].value = &opt.kvno_integer;
    args[2].value = &opt.enctype_string;
    args[3].value = &help_flag;
    if(getarg(args, 4, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = kt_remove(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 4, "remove", "");
    return 0;
}

static int
rename_wrap(int argc, char **argv)
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
    if(argc - optidx != 2) {
        fprintf(stderr, "Need exactly 2 parameters (%u given).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = kt_rename(NULL, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 1, "rename", "from to");
    return 0;
}

static int
srvconvert_wrap(int argc, char **argv)
{
    struct srvconvert_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "srvtab", 's', arg_string, NULL, "name of Kerberos 4 srvtab", "file" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.srvtab_string = "/etc/srvtab";
    args[0].value = &opt.srvtab_string;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = srvconv(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "srvconvert", "");
    return 0;
}

static int
srvcreate_wrap(int argc, char **argv)
{
    struct srvcreate_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "srvtab", 's', arg_string, NULL, "name of Kerberos 4 srvtab", "file" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.srvtab_string = "/etc/srvtab";
    args[0].value = &opt.srvtab_string;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = srvcreate(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "srvcreate", "");
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
        { "add", add_wrap, "add", "Adds a key to a keytab." },

        { "change", change_wrap, "change [principal...]", "Change keys for specified principals (default all)." },

        { "copy", copy_wrap, "copy source destination", "Copies one keytab to another." },

        { "get", get_wrap, "get principal...", "Change keys for specified principals, and add them to the keytab." },

        { "list", list_wrap, "list", "Show contents of keytab." },

        { "purge", purge_wrap, "purge", "Remove superceded keys from keytab." },

        { "remove", remove_wrap, "remove", "Remove keys from keytab." },
        { "delete" },

        { "rename", rename_wrap, "rename from to", "Renames an entry in the keytab." },

        { "srvconvert", srvconvert_wrap, "srvconvert", "Convert a Kerberos 4 srvtab to a keytab." },
        { "srv2keytab" },

        { "srvcreate", srvcreate_wrap, "srvcreate", "Convert a keytab to a Kerberos 4 srvtab." },
        { "key2srvtab" },

        { "help", help_wrap, "help command", NULL },

    { NULL }
};
