#include <stdio.h>
#include <getarg.h>
#include <sl.h>
#include "kdigest-commands.h"

static int
digest_probe_wrap(int argc, char **argv)
{
    struct digest_probe_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "realm", 0, arg_string, NULL, "Kerberos realm to communicate with", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.realm_string = NULL;
    args[0].value = &opt.realm_string;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = digest_probe(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "digest-probe", "");
    return 0;
}

static int
digest_server_init_wrap(int argc, char **argv)
{
    struct digest_server_init_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "type", 0, arg_string, NULL, "digest type", NULL },
        { "kerberos-realm", 0, arg_string, NULL, "", "realm" },
        { "digest", 0, arg_string, NULL, "digest type to use in the algorithm", "digest-type" },
        { "cb-type", 0, arg_string, NULL, "type of channel bindings", "type" },
        { "cb-value", 0, arg_string, NULL, "value of channel bindings", "value" },
        { "hostname", 0, arg_string, NULL, "hostname of the server", "hostname" },
        { "realm", 0, arg_string, NULL, "Kerberos realm to communicate with", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.type_string = "sasl";
    opt.kerberos_realm_string = NULL;
    opt.digest_string = NULL;
    opt.cb_type_string = NULL;
    opt.cb_value_string = NULL;
    opt.hostname_string = NULL;
    opt.realm_string = NULL;
    args[0].value = &opt.type_string;
    args[1].value = &opt.kerberos_realm_string;
    args[2].value = &opt.digest_string;
    args[3].value = &opt.cb_type_string;
    args[4].value = &opt.cb_value_string;
    args[5].value = &opt.hostname_string;
    args[6].value = &opt.realm_string;
    args[7].value = &help_flag;
    if(getarg(args, 8, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = digest_server_init(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 8, "digest-server-init", "");
    return 0;
}

static int
digest_server_request_wrap(int argc, char **argv)
{
    struct digest_server_request_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "type", 0, arg_string, NULL, "digest type", NULL },
        { "kerberos-realm", 0, arg_string, NULL, "", "realm" },
        { "username", 0, arg_string, NULL, "digest type", "name" },
        { "server-nonce", 0, arg_string, NULL, "", "nonce" },
        { "server-identifier", 0, arg_string, NULL, "", "nonce" },
        { "client-nonce", 0, arg_string, NULL, "", "nonce" },
        { "client-response", 0, arg_string, NULL, "", "response" },
        { "opaque", 0, arg_string, NULL, "", "string" },
        { "authentication-name", 0, arg_string, NULL, "", "name" },
        { "realm", 0, arg_string, NULL, "", "realm" },
        { "method", 0, arg_string, NULL, "", "method" },
        { "uri", 0, arg_string, NULL, "", "uri" },
        { "nounce-count", 0, arg_string, NULL, "", "count" },
        { "qop", 0, arg_string, NULL, "", "qop" },
        { "ccache", 0, arg_string, NULL, "Where the the credential cache is created when the KDC returns tickets", "ccache" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.type_string = "sasl";
    opt.kerberos_realm_string = NULL;
    opt.username_string = NULL;
    opt.server_nonce_string = NULL;
    opt.server_identifier_string = NULL;
    opt.client_nonce_string = NULL;
    opt.client_response_string = NULL;
    opt.opaque_string = NULL;
    opt.authentication_name_string = NULL;
    opt.realm_string = NULL;
    opt.method_string = NULL;
    opt.uri_string = NULL;
    opt.nounce_count_string = NULL;
    opt.qop_string = NULL;
    opt.ccache_string = NULL;
    args[0].value = &opt.type_string;
    args[1].value = &opt.kerberos_realm_string;
    args[2].value = &opt.username_string;
    args[3].value = &opt.server_nonce_string;
    args[4].value = &opt.server_identifier_string;
    args[5].value = &opt.client_nonce_string;
    args[6].value = &opt.client_response_string;
    args[7].value = &opt.opaque_string;
    args[8].value = &opt.authentication_name_string;
    args[9].value = &opt.realm_string;
    args[10].value = &opt.method_string;
    args[11].value = &opt.uri_string;
    args[12].value = &opt.nounce_count_string;
    args[13].value = &opt.qop_string;
    args[14].value = &opt.ccache_string;
    args[15].value = &help_flag;
    if(getarg(args, 16, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = digest_server_request(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 16, "digest-server-request", "");
    return 0;
}

static int
digest_client_request_wrap(int argc, char **argv)
{
    struct digest_client_request_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "type", 0, arg_string, NULL, "digest type", NULL },
        { "username", 0, arg_string, NULL, "digest type", "name" },
        { "password", 0, arg_string, NULL, NULL, "password" },
        { "server-nonce", 0, arg_string, NULL, "", "nonce" },
        { "server-identifier", 0, arg_string, NULL, "", "nonce" },
        { "client-nonce", 0, arg_string, NULL, "", "nonce" },
        { "opaque", 0, arg_string, NULL, "", "string" },
        { "realm", 0, arg_string, NULL, "", "realm" },
        { "method", 0, arg_string, NULL, "", "method" },
        { "uri", 0, arg_string, NULL, "", "uri" },
        { "nounce-count", 0, arg_string, NULL, "", "count" },
        { "qop", 0, arg_string, NULL, "", "qop" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.type_string = "sasl";
    opt.username_string = NULL;
    opt.password_string = NULL;
    opt.server_nonce_string = NULL;
    opt.server_identifier_string = NULL;
    opt.client_nonce_string = NULL;
    opt.opaque_string = NULL;
    opt.realm_string = NULL;
    opt.method_string = NULL;
    opt.uri_string = NULL;
    opt.nounce_count_string = NULL;
    opt.qop_string = NULL;
    args[0].value = &opt.type_string;
    args[1].value = &opt.username_string;
    args[2].value = &opt.password_string;
    args[3].value = &opt.server_nonce_string;
    args[4].value = &opt.server_identifier_string;
    args[5].value = &opt.client_nonce_string;
    args[6].value = &opt.opaque_string;
    args[7].value = &opt.realm_string;
    args[8].value = &opt.method_string;
    args[9].value = &opt.uri_string;
    args[10].value = &opt.nounce_count_string;
    args[11].value = &opt.qop_string;
    args[12].value = &help_flag;
    if(getarg(args, 13, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = digest_client_request(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 13, "digest-client-request", "");
    return 0;
}

static int
ntlm_server_init_wrap(int argc, char **argv)
{
    struct ntlm_server_init_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "version", 0, arg_integer, NULL, "ntlm version", NULL },
        { "kerberos-realm", 0, arg_string, NULL, "Kerberos realm to communicate with", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.version_integer = 1;
    opt.kerberos_realm_string = NULL;
    args[0].value = &opt.version_integer;
    args[1].value = &opt.kerberos_realm_string;
    args[2].value = &help_flag;
    if(getarg(args, 3, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = ntlm_server_init(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 3, "ntlm-server-init", "");
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
    if(argc - optidx < 0) {
        fprintf(stderr, "Arguments given (%u) are less than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = help(NULL, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 1, "help", "[command]");
    return 0;
}

SL_cmd commands[] = {
        { "digest-probe", digest_probe_wrap, "digest-probe", "probe what mech is allowed/supported for this server" },

        { "digest-server-init", digest_server_init_wrap, "digest-server-init", "Sets up a digest context and return initial parameters" },

        { "digest-server-request", digest_server_request_wrap, "digest-server-request", "Completes digest negotiation and return final parameters" },

        { "digest-client-request", digest_client_request_wrap, "digest-client-request", "Client part of a digest exchange" },

        { "ntlm-server-init", ntlm_server_init_wrap, "ntlm-server-init", "Sets up a digest context and return initial parameters" },

        { "help", help_wrap, "help [command]", "Help! I need somebody." },
        { "?" },

    { NULL }
};
