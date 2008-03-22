#include <stdio.h>
#include <getarg.h>
#include <sl.h>
#include "hxtool-commands.h"

static int
cms_create_sd_wrap(int argc, char **argv)
{
    struct cms_create_sd_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "certificate", 'c', arg_strings, NULL, "certificate stores to pull certificates from", "certificate-store" },
        { "signer", 's', arg_string, NULL, "certificate to sign with", "signer-friendly-name" },
        { "anchors", 0, arg_strings, NULL, "trust anchors", "certificate-store" },
        { "pool", 0, arg_strings, NULL, "certificate store to pull certificates from", "certificate-pool" },
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "peer-alg", 0, arg_strings, NULL, "oid that the peer support", "oid" },
        { "content-type", 0, arg_string, NULL, "content type oid", "oid" },
        { "content-info", 0, arg_flag, NULL, "wrapped out-data in a ContentInfo", NULL },
        { "pem", 0, arg_flag, NULL, "wrap out-data in PEM armor", NULL },
        { "detached-signature", 0, arg_flag, NULL, "create a detached signature", NULL },
        { "id-by-name", 0, arg_flag, NULL, "use subject name for CMS Identifier", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.certificate_strings.num_strings = 0;
    opt.certificate_strings.strings = NULL;
    opt.signer_string = NULL;
    opt.anchors_strings.num_strings = 0;
    opt.anchors_strings.strings = NULL;
    opt.pool_strings.num_strings = 0;
    opt.pool_strings.strings = NULL;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    opt.peer_alg_strings.num_strings = 0;
    opt.peer_alg_strings.strings = NULL;
    opt.content_type_string = NULL;
    opt.content_info_flag = 0;
    opt.pem_flag = 0;
    opt.detached_signature_flag = 0;
    opt.id_by_name_flag = 0;
    args[0].value = &opt.certificate_strings;
    args[1].value = &opt.signer_string;
    args[2].value = &opt.anchors_strings;
    args[3].value = &opt.pool_strings;
    args[4].value = &opt.pass_strings;
    args[5].value = &opt.peer_alg_strings;
    args[6].value = &opt.content_type_string;
    args[7].value = &opt.content_info_flag;
    args[8].value = &opt.pem_flag;
    args[9].value = &opt.detached_signature_flag;
    args[10].value = &opt.id_by_name_flag;
    args[11].value = &help_flag;
    if(getarg(args, 12, argc, argv, &optidx))
        goto usage;
    if(argc - optidx != 2) {
        fprintf(stderr, "Need exactly 2 parameters (%u given).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = cms_create_sd(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.certificate_strings);
    free_getarg_strings (&opt.anchors_strings);
    free_getarg_strings (&opt.pool_strings);
    free_getarg_strings (&opt.pass_strings);
    free_getarg_strings (&opt.peer_alg_strings);
    return ret;
usage:
    arg_printusage (args, 12, "cms-create-sd", "in-file out-file");
    free_getarg_strings (&opt.certificate_strings);
    free_getarg_strings (&opt.anchors_strings);
    free_getarg_strings (&opt.pool_strings);
    free_getarg_strings (&opt.pass_strings);
    free_getarg_strings (&opt.peer_alg_strings);
    return 0;
}

static int
cms_verify_sd_wrap(int argc, char **argv)
{
    struct cms_verify_sd_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "anchors", 0, arg_strings, NULL, "trust anchors", "certificate-store" },
        { "certificate", 'c', arg_strings, NULL, "certificate store to pull certificates from", "certificate-store" },
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "missing-revoke", 0, arg_flag, NULL, "missing CRL/OCSP is ok", NULL },
        { "content-info", 0, arg_flag, NULL, "unwrap in-data that's in a ContentInfo", NULL },
        { "signed-content", 0, arg_string, NULL, "file containing content", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.anchors_strings.num_strings = 0;
    opt.anchors_strings.strings = NULL;
    opt.certificate_strings.num_strings = 0;
    opt.certificate_strings.strings = NULL;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    opt.missing_revoke_flag = 0;
    opt.content_info_flag = 0;
    opt.signed_content_string = NULL;
    args[0].value = &opt.anchors_strings;
    args[1].value = &opt.certificate_strings;
    args[2].value = &opt.pass_strings;
    args[3].value = &opt.missing_revoke_flag;
    args[4].value = &opt.content_info_flag;
    args[5].value = &opt.signed_content_string;
    args[6].value = &help_flag;
    if(getarg(args, 7, argc, argv, &optidx))
        goto usage;
    if(argc - optidx != 2) {
        fprintf(stderr, "Need exactly 2 parameters (%u given).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = cms_verify_sd(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.anchors_strings);
    free_getarg_strings (&opt.certificate_strings);
    free_getarg_strings (&opt.pass_strings);
    return ret;
usage:
    arg_printusage (args, 7, "cms-verify-sd", "in-file out-file");
    free_getarg_strings (&opt.anchors_strings);
    free_getarg_strings (&opt.certificate_strings);
    free_getarg_strings (&opt.pass_strings);
    return 0;
}

static int
cms_unenvelope_wrap(int argc, char **argv)
{
    struct cms_unenvelope_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "certificate", 'c', arg_strings, NULL, "certificate used to decrypt the data", "certificate-store" },
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "content-info", 0, arg_flag, NULL, "wrapped out-data in a ContentInfo", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.certificate_strings.num_strings = 0;
    opt.certificate_strings.strings = NULL;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    opt.content_info_flag = 0;
    args[0].value = &opt.certificate_strings;
    args[1].value = &opt.pass_strings;
    args[2].value = &opt.content_info_flag;
    args[3].value = &help_flag;
    if(getarg(args, 4, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 2) {
        fprintf(stderr, "Arguments given (%u) are less than expected (2).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = cms_unenvelope(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.certificate_strings);
    free_getarg_strings (&opt.pass_strings);
    return ret;
usage:
    arg_printusage (args, 4, "cms-unenvelope", "in-file out-file");
    free_getarg_strings (&opt.certificate_strings);
    free_getarg_strings (&opt.pass_strings);
    return 0;
}

static int
cms_envelope_wrap(int argc, char **argv)
{
    struct cms_envelope_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "certificate", 'c', arg_strings, NULL, "certificates used to receive the data", "certificate-store" },
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "encryption-type", 0, arg_string, NULL, "enctype", "enctype" },
        { "content-type", 0, arg_string, NULL, "content type oid", "oid" },
        { "content-info", 0, arg_flag, NULL, "wrapped out-data in a ContentInfo", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.certificate_strings.num_strings = 0;
    opt.certificate_strings.strings = NULL;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    opt.encryption_type_string = NULL;
    opt.content_type_string = NULL;
    opt.content_info_flag = 0;
    args[0].value = &opt.certificate_strings;
    args[1].value = &opt.pass_strings;
    args[2].value = &opt.encryption_type_string;
    args[3].value = &opt.content_type_string;
    args[4].value = &opt.content_info_flag;
    args[5].value = &help_flag;
    if(getarg(args, 6, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 2) {
        fprintf(stderr, "Arguments given (%u) are less than expected (2).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = cms_create_enveloped(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.certificate_strings);
    free_getarg_strings (&opt.pass_strings);
    return ret;
usage:
    arg_printusage (args, 6, "cms-envelope", "in-file out-file");
    free_getarg_strings (&opt.certificate_strings);
    free_getarg_strings (&opt.pass_strings);
    return 0;
}

static int
verify_wrap(int argc, char **argv)
{
    struct verify_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "allow-proxy-certificate", 0, arg_flag, NULL, "allow proxy certificates", NULL },
        { "missing-revoke", 0, arg_flag, NULL, "missing CRL/OCSP is ok", NULL },
        { "time", 0, arg_string, NULL, "time when to validate the chain", NULL },
        { "verbose", 'v', arg_flag, NULL, "verbose logging", NULL },
        { "max-depth", 0, arg_integer, NULL, "maximum search length of certificate trust anchor", NULL },
        { "hostname", 0, arg_string, NULL, "match hostname to certificate", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    opt.allow_proxy_certificate_flag = 0;
    opt.missing_revoke_flag = 0;
    opt.time_string = NULL;
    opt.verbose_flag = 0;
    opt.max_depth_integer = 0;
    opt.hostname_string = NULL;
    args[0].value = &opt.pass_strings;
    args[1].value = &opt.allow_proxy_certificate_flag;
    args[2].value = &opt.missing_revoke_flag;
    args[3].value = &opt.time_string;
    args[4].value = &opt.verbose_flag;
    args[5].value = &opt.max_depth_integer;
    args[6].value = &opt.hostname_string;
    args[7].value = &help_flag;
    if(getarg(args, 8, argc, argv, &optidx))
        goto usage;
    if(help_flag)
        goto usage;
    ret = pcert_verify(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.pass_strings);
    return ret;
usage:
    arg_printusage (args, 8, "verify", "cert:foo chain:cert1 chain:cert2 anchor:anchor1 anchor:anchor2");
    free_getarg_strings (&opt.pass_strings);
    return 0;
}

static int
print_wrap(int argc, char **argv)
{
    struct print_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "content", 0, arg_flag, NULL, "print the content of the certificates", NULL },
        { "info", 0, arg_flag, NULL, "print the information about the certificate store", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    opt.content_flag = 0;
    opt.info_flag = 0;
    args[0].value = &opt.pass_strings;
    args[1].value = &opt.content_flag;
    args[2].value = &opt.info_flag;
    args[3].value = &help_flag;
    if(getarg(args, 4, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 1) {
        fprintf(stderr, "Arguments given (%u) are less than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = pcert_print(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.pass_strings);
    return ret;
usage:
    arg_printusage (args, 4, "print", "certificate ...");
    free_getarg_strings (&opt.pass_strings);
    return 0;
}

static int
validate_wrap(int argc, char **argv)
{
    struct validate_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    args[0].value = &opt.pass_strings;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 1) {
        fprintf(stderr, "Arguments given (%u) are less than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = pcert_validate(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.pass_strings);
    return ret;
usage:
    arg_printusage (args, 2, "validate", "certificate ...");
    free_getarg_strings (&opt.pass_strings);
    return 0;
}

static int
certificate_copy_wrap(int argc, char **argv)
{
    struct certificate_copy_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "in-pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "out-pass", 0, arg_string, NULL, "password, prompter, or environment", "password" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.in_pass_strings.num_strings = 0;
    opt.in_pass_strings.strings = NULL;
    opt.out_pass_string = NULL;
    args[0].value = &opt.in_pass_strings;
    args[1].value = &opt.out_pass_string;
    args[2].value = &help_flag;
    if(getarg(args, 3, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 2) {
        fprintf(stderr, "Arguments given (%u) are less than expected (2).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = certificate_copy(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.in_pass_strings);
    return ret;
usage:
    arg_printusage (args, 3, "certificate-copy", "in-certificates-1 ... out-certificate");
    free_getarg_strings (&opt.in_pass_strings);
    return 0;
}

static int
ocsp_fetch_wrap(int argc, char **argv)
{
    struct ocsp_fetch_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "sign", 0, arg_string, NULL, "certificate use to sign the request", "certificate" },
        { "url-path", 0, arg_string, NULL, "part after host in url to put in the request", "url" },
        { "nonce", 0, arg_negative_flag, NULL, "don't include nonce in request", NULL },
        { "pool", 0, arg_strings, NULL, "pool to find parent certificate in", "certificate-store" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    opt.sign_string = NULL;
    opt.url_path_string = NULL;
    opt.nonce_flag = 1;
    opt.pool_strings.num_strings = 0;
    opt.pool_strings.strings = NULL;
    args[0].value = &opt.pass_strings;
    args[1].value = &opt.sign_string;
    args[2].value = &opt.url_path_string;
    args[3].value = &opt.nonce_flag;
    args[4].value = &opt.pool_strings;
    args[5].value = &help_flag;
    if(getarg(args, 6, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 2) {
        fprintf(stderr, "Arguments given (%u) are less than expected (2).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = ocsp_fetch(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.pass_strings);
    free_getarg_strings (&opt.pool_strings);
    return ret;
usage:
    arg_printusage (args, 6, "ocsp-fetch", "outfile certs ...");
    free_getarg_strings (&opt.pass_strings);
    free_getarg_strings (&opt.pool_strings);
    return 0;
}

static int
ocsp_verify_wrap(int argc, char **argv)
{
    struct ocsp_verify_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "ocsp-file", 0, arg_string, NULL, "OCSP file", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.ocsp_file_string = NULL;
    args[0].value = &opt.ocsp_file_string;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 1) {
        fprintf(stderr, "Arguments given (%u) are less than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = ocsp_verify(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "ocsp-verify", "certificates ...");
    return 0;
}

static int
ocsp_print_wrap(int argc, char **argv)
{
    struct ocsp_print_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "verbose", 0, arg_flag, NULL, "verbose", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.verbose_flag = 0;
    args[0].value = &opt.verbose_flag;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 1) {
        fprintf(stderr, "Arguments given (%u) are less than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = ocsp_print(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "ocsp-print", "ocsp-response-file ...");
    return 0;
}

static int
request_create_wrap(int argc, char **argv)
{
    struct request_create_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "subject", 0, arg_string, NULL, "Subject DN", NULL },
        { "email", 0, arg_strings, NULL, "Email address in SubjectAltName", NULL },
        { "dnsname", 0, arg_strings, NULL, "Hostname or domainname in SubjectAltName", NULL },
        { "type", 0, arg_string, NULL, "Type of request CRMF or PKCS10, defaults to PKCS10", NULL },
        { "key", 0, arg_string, NULL, "Key-pair", NULL },
        { "generate-key", 0, arg_string, NULL, "keytype", NULL },
        { "key-bits", 0, arg_integer, NULL, "number of bits in the generated key", NULL },
        { "verbose", 0, arg_flag, NULL, "verbose status", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.subject_string = NULL;
    opt.email_strings.num_strings = 0;
    opt.email_strings.strings = NULL;
    opt.dnsname_strings.num_strings = 0;
    opt.dnsname_strings.strings = NULL;
    opt.type_string = NULL;
    opt.key_string = NULL;
    opt.generate_key_string = NULL;
    opt.key_bits_integer = 0;
    opt.verbose_flag = 0;
    args[0].value = &opt.subject_string;
    args[1].value = &opt.email_strings;
    args[2].value = &opt.dnsname_strings;
    args[3].value = &opt.type_string;
    args[4].value = &opt.key_string;
    args[5].value = &opt.generate_key_string;
    args[6].value = &opt.key_bits_integer;
    args[7].value = &opt.verbose_flag;
    args[8].value = &help_flag;
    if(getarg(args, 9, argc, argv, &optidx))
        goto usage;
    if(argc - optidx != 1) {
        fprintf(stderr, "Need exactly 1 parameters (%u given).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = request_create(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.email_strings);
    free_getarg_strings (&opt.dnsname_strings);
    return ret;
usage:
    arg_printusage (args, 9, "request-create", "output-file");
    free_getarg_strings (&opt.email_strings);
    free_getarg_strings (&opt.dnsname_strings);
    return 0;
}

static int
request_print_wrap(int argc, char **argv)
{
    struct request_print_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "verbose", 0, arg_flag, NULL, "verbose printing", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.verbose_flag = 0;
    args[0].value = &opt.verbose_flag;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 1) {
        fprintf(stderr, "Arguments given (%u) are less than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = request_print(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "request-print", "requests ...");
    return 0;
}

static int
query_wrap(int argc, char **argv)
{
    struct query_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "exact", 0, arg_flag, NULL, "exact match", NULL },
        { "private-key", 0, arg_flag, NULL, "search for private key", NULL },
        { "friendlyname", 0, arg_string, NULL, "match on friendly name", "name" },
        { "keyEncipherment", 0, arg_flag, NULL, "match keyEncipherment certificates", NULL },
        { "digitalSignature", 0, arg_flag, NULL, "match digitalSignature certificates", NULL },
        { "print", 0, arg_flag, NULL, "print matches", NULL },
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.exact_flag = 0;
    opt.private_key_flag = 0;
    opt.friendlyname_string = NULL;
    opt.keyEncipherment_flag = 0;
    opt.digitalSignature_flag = 0;
    opt.print_flag = 0;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    args[0].value = &opt.exact_flag;
    args[1].value = &opt.private_key_flag;
    args[2].value = &opt.friendlyname_string;
    args[3].value = &opt.keyEncipherment_flag;
    args[4].value = &opt.digitalSignature_flag;
    args[5].value = &opt.print_flag;
    args[6].value = &opt.pass_strings;
    args[7].value = &help_flag;
    if(getarg(args, 8, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 1) {
        fprintf(stderr, "Arguments given (%u) are less than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = query(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.pass_strings);
    return ret;
usage:
    arg_printusage (args, 8, "query", "certificates ...");
    free_getarg_strings (&opt.pass_strings);
    return 0;
}

static int
info_wrap(int argc, char **argv)
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
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = info(NULL, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 1, "info", "");
    return 0;
}

static int
random_data_wrap(int argc, char **argv)
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
    if(argc - optidx < 1) {
        fprintf(stderr, "Arguments given (%u) are less than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = random_data(NULL, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 1, "random-data", "bytes");
    return 0;
}

static int
crypto_available_wrap(int argc, char **argv)
{
    struct crypto_available_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "type", 0, arg_string, NULL, "type of CMS algorithm", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.type_string = NULL;
    args[0].value = &opt.type_string;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = crypto_available(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "crypto-available", "");
    return 0;
}

static int
crypto_select_wrap(int argc, char **argv)
{
    struct crypto_select_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "type", 0, arg_string, NULL, "type of CMS algorithm", NULL },
        { "certificate", 0, arg_string, NULL, "source certificate limiting the choices", NULL },
        { "peer-cmstype", 0, arg_strings, NULL, "peer limiting cmstypes", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.type_string = NULL;
    opt.certificate_string = NULL;
    opt.peer_cmstype_strings.num_strings = 0;
    opt.peer_cmstype_strings.strings = NULL;
    args[0].value = &opt.type_string;
    args[1].value = &opt.certificate_string;
    args[2].value = &opt.peer_cmstype_strings;
    args[3].value = &help_flag;
    if(getarg(args, 4, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = crypto_select(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.peer_cmstype_strings);
    return ret;
usage:
    arg_printusage (args, 4, "crypto-select", "");
    free_getarg_strings (&opt.peer_cmstype_strings);
    return 0;
}

static int
hex_wrap(int argc, char **argv)
{
    struct hex_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "decode", 'd', arg_flag, NULL, "decode instead of encode", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.decode_flag = 0;
    args[0].value = &opt.decode_flag;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = hxtool_hex(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "hex", "");
    return 0;
}

static int
certificate_sign_wrap(int argc, char **argv)
{
    struct certificate_sign_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "issue-ca", 0, arg_flag, NULL, "Issue a CA certificate", NULL },
        { "issue-proxy", 0, arg_flag, NULL, "Issue a proxy certificate", NULL },
        { "domain-controller", 0, arg_flag, NULL, "Issue a MS domaincontroller certificate", NULL },
        { "subject", 0, arg_string, NULL, "Subject of issued certificate", NULL },
        { "ca-certificate", 0, arg_string, NULL, "Issuing CA certificate", NULL },
        { "self-signed", 0, arg_flag, NULL, "Issuing a self-signed certificate", NULL },
        { "ca-private-key", 0, arg_string, NULL, "Private key for self-signed certificate", NULL },
        { "certificate", 0, arg_string, NULL, "Issued certificate", NULL },
        { "type", 0, arg_strings, NULL, "Type of certificate to issue", NULL },
        { "lifetime", 0, arg_string, NULL, "Lifetime of certificate", NULL },
        { "serial-number", 0, arg_string, NULL, "serial-number of certificate", NULL },
        { "path-length", 0, arg_integer, NULL, "Maximum path length (CA and proxy certificates), -1 no limit", NULL },
        { "hostname", 0, arg_strings, NULL, "DNS names this certificate is allowed to serve", NULL },
        { "email", 0, arg_strings, NULL, "email addresses assigned to this certificate", NULL },
        { "pk-init-principal", 0, arg_string, NULL, "PK-INIT principal (for SAN)", NULL },
        { "ms-upn", 0, arg_string, NULL, "Microsoft UPN (for SAN)", NULL },
        { "jid", 0, arg_string, NULL, "XMPP jabber id (for SAN)", NULL },
        { "req", 0, arg_string, NULL, "certificate request", NULL },
        { "certificate-private-key", 0, arg_string, NULL, "private-key", NULL },
        { "generate-key", 0, arg_string, NULL, "keytype", NULL },
        { "key-bits", 0, arg_integer, NULL, "number of bits in the generated key", NULL },
        { "crl-uri", 0, arg_string, NULL, "URI to CRL", NULL },
        { "template-certificate", 0, arg_string, NULL, "certificate", NULL },
        { "template-fields", 0, arg_string, NULL, "flag", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.issue_ca_flag = 0;
    opt.issue_proxy_flag = 0;
    opt.domain_controller_flag = 0;
    opt.subject_string = NULL;
    opt.ca_certificate_string = NULL;
    opt.self_signed_flag = 0;
    opt.ca_private_key_string = NULL;
    opt.certificate_string = NULL;
    opt.type_strings.num_strings = 0;
    opt.type_strings.strings = NULL;
    opt.lifetime_string = NULL;
    opt.serial_number_string = NULL;
    opt.path_length_integer = -1;
    opt.hostname_strings.num_strings = 0;
    opt.hostname_strings.strings = NULL;
    opt.email_strings.num_strings = 0;
    opt.email_strings.strings = NULL;
    opt.pk_init_principal_string = NULL;
    opt.ms_upn_string = NULL;
    opt.jid_string = NULL;
    opt.req_string = NULL;
    opt.certificate_private_key_string = NULL;
    opt.generate_key_string = NULL;
    opt.key_bits_integer = 0;
    opt.crl_uri_string = NULL;
    opt.template_certificate_string = NULL;
    opt.template_fields_string = NULL;
    args[0].value = &opt.issue_ca_flag;
    args[1].value = &opt.issue_proxy_flag;
    args[2].value = &opt.domain_controller_flag;
    args[3].value = &opt.subject_string;
    args[4].value = &opt.ca_certificate_string;
    args[5].value = &opt.self_signed_flag;
    args[6].value = &opt.ca_private_key_string;
    args[7].value = &opt.certificate_string;
    args[8].value = &opt.type_strings;
    args[9].value = &opt.lifetime_string;
    args[10].value = &opt.serial_number_string;
    args[11].value = &opt.path_length_integer;
    args[12].value = &opt.hostname_strings;
    args[13].value = &opt.email_strings;
    args[14].value = &opt.pk_init_principal_string;
    args[15].value = &opt.ms_upn_string;
    args[16].value = &opt.jid_string;
    args[17].value = &opt.req_string;
    args[18].value = &opt.certificate_private_key_string;
    args[19].value = &opt.generate_key_string;
    args[20].value = &opt.key_bits_integer;
    args[21].value = &opt.crl_uri_string;
    args[22].value = &opt.template_certificate_string;
    args[23].value = &opt.template_fields_string;
    args[24].value = &help_flag;
    if(getarg(args, 25, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = hxtool_ca(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.type_strings);
    free_getarg_strings (&opt.hostname_strings);
    free_getarg_strings (&opt.email_strings);
    return ret;
usage:
    arg_printusage (args, 25, "certificate-sign", "");
    free_getarg_strings (&opt.type_strings);
    free_getarg_strings (&opt.hostname_strings);
    free_getarg_strings (&opt.email_strings);
    return 0;
}

static int
test_crypto_wrap(int argc, char **argv)
{
    struct test_crypto_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "verbose", 0, arg_flag, NULL, "verbose printing", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    opt.verbose_flag = 0;
    args[0].value = &opt.pass_strings;
    args[1].value = &opt.verbose_flag;
    args[2].value = &help_flag;
    if(getarg(args, 3, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 1) {
        fprintf(stderr, "Arguments given (%u) are less than expected (1).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = test_crypto(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.pass_strings);
    return ret;
usage:
    arg_printusage (args, 3, "test-crypto", "certificates...");
    free_getarg_strings (&opt.pass_strings);
    return 0;
}

static int
statistic_print_wrap(int argc, char **argv)
{
    struct statistic_print_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "type", 0, arg_integer, NULL, "type of statistics", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.type_integer = 0;
    args[0].value = &opt.type_integer;
    args[1].value = &help_flag;
    if(getarg(args, 2, argc, argv, &optidx))
        goto usage;
    if(argc - optidx > 0) {
        fprintf(stderr, "Arguments given (%u) are more than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = statistic_print(&opt, argc - optidx, argv + optidx);
    return ret;
usage:
    arg_printusage (args, 2, "statistic-print", "");
    return 0;
}

static int
crl_sign_wrap(int argc, char **argv)
{
    struct crl_sign_options opt;
    int ret;
    int optidx = 0;
    struct getargs args[] = {
        { "signer", 0, arg_string, NULL, "signer certificate", NULL },
        { "pass", 0, arg_strings, NULL, "password, prompter, or environment", "password" },
        { "crl-file", 0, arg_string, NULL, "CRL output file", NULL },
        { "lifetime", 0, arg_string, NULL, "time the crl will be valid", NULL },
        { "help", 'h', arg_flag, NULL, NULL, NULL }
    };
    int help_flag = 0;
    opt.signer_string = NULL;
    opt.pass_strings.num_strings = 0;
    opt.pass_strings.strings = NULL;
    opt.crl_file_string = NULL;
    opt.lifetime_string = NULL;
    args[0].value = &opt.signer_string;
    args[1].value = &opt.pass_strings;
    args[2].value = &opt.crl_file_string;
    args[3].value = &opt.lifetime_string;
    args[4].value = &help_flag;
    if(getarg(args, 5, argc, argv, &optidx))
        goto usage;
    if(argc - optidx < 0) {
        fprintf(stderr, "Arguments given (%u) are less than expected (0).\n\n", argc - optidx);
        goto usage;
    }
    if(help_flag)
        goto usage;
    ret = crl_sign(&opt, argc - optidx, argv + optidx);
    free_getarg_strings (&opt.pass_strings);
    return ret;
usage:
    arg_printusage (args, 5, "crl-sign", "certificates...");
    free_getarg_strings (&opt.pass_strings);
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
        { "cms-create-sd", cms_create_sd_wrap, "cms-create-sd in-file out-file", "Wrap a file within a SignedData object" },

        { "cms-verify-sd", cms_verify_sd_wrap, "cms-verify-sd in-file out-file", "Verify a file within a SignedData object" },

        { "cms-unenvelope", cms_unenvelope_wrap, "cms-unenvelope in-file out-file", "Unenvelope a file containing a EnvelopedData object" },

        { "cms-envelope", cms_envelope_wrap, "cms-envelope in-file out-file", "Envelope a file containing a EnvelopedData object" },

        { "verify", verify_wrap, "verify cert:foo chain:cert1 chain:cert2 anchor:anchor1 anchor:anchor2", "Verify certificate chain" },

        { "print", print_wrap, "print certificate ...", "Print certificates" },

        { "validate", validate_wrap, "validate certificate ...", "Validate content of certificates" },

        { "certificate-copy", certificate_copy_wrap, "certificate-copy in-certificates-1 ... out-certificate", "Copy in certificates stores into out certificate store" },
        { "cc" },

        { "ocsp-fetch", ocsp_fetch_wrap, "ocsp-fetch outfile certs ...", "Fetch OCSP responses for the following certs" },

        { "ocsp-verify", ocsp_verify_wrap, "ocsp-verify certificates ...", "Check that certificates are in OCSP file and valid" },

        { "ocsp-print", ocsp_print_wrap, "ocsp-print ocsp-response-file ...", "Print the OCSP responses" },

        { "request-create", request_create_wrap, "request-create output-file", "Create a CRMF or PKCS10 request" },

        { "request-print", request_print_wrap, "request-print requests ...", "Print requests" },

        { "query", query_wrap, "query certificates ...", "Query the certificates for a match" },

        { "info", info_wrap, "info", NULL },

        { "random-data", random_data_wrap, "random-data bytes", "Generates random bytes and prints them to standard output" },

        { "crypto-available", crypto_available_wrap, "crypto-available", "Print available CMS crypto types" },

        { "crypto-select", crypto_select_wrap, "crypto-select", "Print selected CMS type" },

        { "hex", hex_wrap, "hex", "Encode input to hex" },

        { "certificate-sign", certificate_sign_wrap, "certificate-sign", "Issue a certificate" },
        { "cert-sign" },
        { "issue-certificate" },
        { "ca" },

        { "test-crypto", test_crypto_wrap, "test-crypto certificates...", "Test crypto system related to the certificates" },

        { "statistic-print", statistic_print_wrap, "statistic-print", "Print statistics" },

        { "crl-sign", crl_sign_wrap, "crl-sign certificates...", "Create a CRL" },

        { "help", help_wrap, "help [command]", "Help! I need somebody" },
        { "?" },

    { NULL }
};
