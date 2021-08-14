/*	$NetBSD: config.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

/* config.c - configuration file handling routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2021 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: config.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/signal.h>
#include <ac/socket.h>
#include <ac/errno.h>
#include <ac/unistd.h>

#include <sys/types.h>
#include <sys/stat.h>

#ifndef S_ISREG
#define S_ISREG(m) ( ((m) & _S_IFMT ) == _S_IFREG )
#endif

#include "lload.h"
#include "lutil.h"
#include "lutil_ldap.h"
#include "lload-config.h"

#ifdef _WIN32
#define LUTIL_ATOULX lutil_atoullx
#define Z "I"
#else
#define LUTIL_ATOULX lutil_atoulx
#define Z "z"
#endif

#define ARGS_STEP 512

/*
 * defaults for various global variables
 */
#ifdef BALANCER_MODULE
char *listeners_list = NULL;
#else /* !BALANCER_MODULE */
slap_mask_t global_allows = 0;
slap_mask_t global_disallows = 0;
int global_gentlehup = 0;
int global_idletimeout = 0;
char *global_host = NULL;

char *slapd_pid_file = NULL;
char *slapd_args_file = NULL;
#endif /* !BALANCER_MODULE */

static FILE *logfile;
static char *logfileName;

static struct timeval timeout_api_tv, timeout_net_tv,
        timeout_write_tv = { 10, 0 };

lload_features_t lload_features;

ber_len_t sockbuf_max_incoming_client = LLOAD_SB_MAX_INCOMING_CLIENT;
ber_len_t sockbuf_max_incoming_upstream = LLOAD_SB_MAX_INCOMING_UPSTREAM;

int lload_conn_max_pdus_per_cycle = LLOAD_CONN_MAX_PDUS_PER_CYCLE_DEFAULT;

struct timeval *lload_timeout_api = NULL;
struct timeval *lload_timeout_net = NULL;
struct timeval *lload_write_timeout = &timeout_write_tv;

static slap_verbmasks tlskey[];

static int fp_getline( FILE *fp, ConfigArgs *c );
static void fp_getline_init( ConfigArgs *c );

static char *strtok_quote(
        char *line,
        char *sep,
        char **quote_ptr,
        int *inquote );

typedef struct ConfigFile {
    struct ConfigFile *c_sibs;
    struct ConfigFile *c_kids;
    struct berval c_file;
    BerVarray c_dseFiles;
} ConfigFile;

static ConfigFile *cfn;

static ConfigDriver config_fname;
static ConfigDriver config_generic;
static ConfigDriver config_backend;
static ConfigDriver config_bindconf;
#ifdef LDAP_TCP_BUFFER
static ConfigDriver config_tcp_buffer;
#endif /* LDAP_TCP_BUFFER */
static ConfigDriver config_restrict;
static ConfigDriver config_loglevel;
static ConfigDriver config_include;
static ConfigDriver config_feature;
#ifdef HAVE_TLS
static ConfigDriver config_tls_option;
static ConfigDriver config_tls_config;
#endif
#ifdef BALANCER_MODULE
static ConfigDriver config_share_tls_ctx;
static ConfigDriver backend_cf_gen;
#endif /* BALANCER_MODULE */

lload_b_head backend = LDAP_CIRCLEQ_HEAD_INITIALIZER(backend);
ldap_pvt_thread_mutex_t backend_mutex;
LloadBackend *current_backend = NULL;

struct slap_bindconf bindconf = {};
struct berval lloadd_identity = BER_BVNULL;

enum {
    CFG_ACL = 1,
    CFG_BACKEND,
    CFG_BINDCONF,
    CFG_LISTEN,
    CFG_LISTEN_URI,
    CFG_TLS_RAND,
    CFG_TLS_CIPHER,
    CFG_TLS_PROTOCOL_MIN,
    CFG_TLS_CERT_FILE,
    CFG_TLS_CERT_KEY,
    CFG_TLS_CA_PATH,
    CFG_TLS_CA_FILE,
    CFG_TLS_DH_FILE,
    CFG_TLS_VERIFY,
    CFG_TLS_CRLCHECK,
    CFG_TLS_CRL_FILE,
    CFG_TLS_SHARE_CTX,
    CFG_CONCUR,
    CFG_THREADS,
    CFG_LOGFILE,
    CFG_MIRRORMODE,
    CFG_IOTHREADS,
    CFG_MAXBUF_CLIENT,
    CFG_MAXBUF_UPSTREAM,
    CFG_FEATURE,
    CFG_THREADQS,
    CFG_TLS_ECNAME,
    CFG_TLS_CACERT,
    CFG_TLS_CERT,
    CFG_TLS_KEY,
    CFG_RESCOUNT,
    CFG_IOTIMEOUT,
    CFG_URI,
    CFG_NUMCONNS,
    CFG_BINDCONNS,
    CFG_RETRY,
    CFG_MAX_PENDING_OPS,
    CFG_MAX_PENDING_CONNS,
    CFG_STARTTLS,
    CFG_CLIENT_PENDING,

    CFG_LAST
};

/* alphabetical ordering */

static ConfigTable config_back_cf_table[] = {
    /* This attr is read-only */
    { "", "", 0, 0, 0,
        ARG_MAGIC,
        &config_fname,
        NULL, NULL, NULL
    },
    { "argsfile", "file", 2, 2, 0,
        ARG_STRING,
        &slapd_args_file,
        NULL, NULL, NULL
    },
    { "concurrency", "level", 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_CONCUR,
        &config_generic,
        NULL, NULL, NULL
    },
    /* conf-file only option */
    { "backend-server", "backend options", 2, 0, 0,
        ARG_MAGIC|CFG_BACKEND,
        &config_backend,
        NULL, NULL, NULL
    },
    { "bindconf", "backend credentials", 2, 0, 0,
        ARG_MAGIC|CFG_BINDCONF,
        &config_bindconf,
        "( OLcfgBkAt:13.2 "
            "NAME 'olcBkLloadBindconf' "
            "DESC 'Backend credentials' "
            /* No EQUALITY since this is a compound attribute (and needs
             * splitting up anyway - which is a TODO) */
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "gentlehup", "on|off", 2, 2, 0,
#ifdef SIGHUP
        ARG_ON_OFF,
        &global_gentlehup,
#else
        ARG_IGNORED,
        NULL,
#endif
        NULL, NULL, NULL
    },
    { "idletimeout", "timeout", 2, 2, 0,
        ARG_UINT,
        &global_idletimeout,
        "( OLcfgBkAt:13.3 "
            "NAME 'olcBkLloadIdleTimeout' "
            "DESC 'Connection idle timeout' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "include", "file", 2, 2, 0,
        ARG_MAGIC,
        &config_include,
        NULL, NULL, NULL
    },
    { "io-threads", "count", 2, 0, 0,
        ARG_UINT|ARG_MAGIC|CFG_IOTHREADS,
        &config_generic,
        "( OLcfgBkAt:13.4 "
            "NAME 'olcBkLloadIOThreads' "
            "DESC 'I/O thread count' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL, NULL
    },
#ifdef BALANCER_MODULE
    { "listen", "uri list", 2, 2, 0,
        ARG_STRING|ARG_MAGIC|CFG_LISTEN,
        &config_generic,
        NULL, NULL, NULL
    },
    { "", "uri", 2, 2, 0,
        ARG_MAGIC|CFG_LISTEN_URI,
        &config_generic,
        "( OLcfgBkAt:13.5 "
            "NAME 'olcBkLloadListen' "
            "DESC 'A listener adress' "
            /* We don't handle adding/removing a value, so no EQUALITY yet */
            "SYNTAX OMsDirectoryString )",
        NULL, NULL
    },
#endif /* BALANCER_MODULE */
    { "logfile", "file", 2, 2, 0,
        ARG_STRING|ARG_MAGIC|CFG_LOGFILE,
        &config_generic,
        NULL, NULL, NULL
    },
    { "loglevel", "level", 2, 0, 0,
        ARG_MAGIC,
        &config_loglevel,
        NULL, NULL, NULL
    },
    { "pidfile", "file", 2, 2, 0,
        ARG_STRING,
        &slapd_pid_file,
        NULL, NULL, NULL
    },
    { "restrict", "op_list", 2, 0, 0,
        ARG_MAGIC,
        &config_restrict,
        NULL, NULL, NULL
    },
    { "sockbuf_max_incoming_client", "max", 2, 2, 0,
        ARG_BER_LEN_T|ARG_MAGIC|CFG_MAXBUF_CLIENT,
        &config_generic,
        "( OLcfgBkAt:13.6 "
            "NAME 'olcBkLloadSockbufMaxClient' "
            "DESC 'The maximum LDAP PDU size accepted coming from clients' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL,
        { .v_ber_t = LLOAD_SB_MAX_INCOMING_CLIENT }
    },
    { "sockbuf_max_incoming_upstream", "max", 2, 2, 0,
        ARG_BER_LEN_T|ARG_MAGIC|CFG_MAXBUF_UPSTREAM,
        &config_generic,
        "( OLcfgBkAt:13.7 "
            "NAME 'olcBkLloadSockbufMaxUpstream' "
            "DESC 'The maximum LDAP PDU size accepted coming from upstream' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL,
        { .v_ber_t = LLOAD_SB_MAX_INCOMING_UPSTREAM }
    },
    { "tcp-buffer", "[listener=<listener>] [{read|write}=]size", 0, 0, 0,
#ifdef LDAP_TCP_BUFFER
        ARG_MAGIC,
        &config_tcp_buffer,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.8 "
            "NAME 'olcBkLloadTcpBuffer' "
            "DESC 'TCP Buffer size' "
            "EQUALITY caseIgnoreMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "threads", "count", 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_THREADS,
        &config_generic,
        NULL, NULL, NULL
    },
    { "threadqueues", "count", 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_THREADQS,
        &config_generic,
        NULL, NULL, NULL
    },
    { "max_pdus_per_cycle", "count", 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_RESCOUNT,
        &config_generic,
        "( OLcfgBkAt:13.9 "
            "NAME 'olcBkLloadMaxPDUPerCycle' "
            "DESC 'Maximum number of PDUs to handle in a single cycle' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "feature", "name", 2, 0, 0,
        ARG_MAGIC|CFG_FEATURE,
        &config_feature,
        "( OLcfgBkAt:13.10 "
            "NAME 'olcBkLloadFeature' "
            "DESC 'Lload features enabled' "
            "EQUALITY caseIgnoreMatch "
            "SYNTAX OMsDirectoryString )",
        NULL, NULL
    },
    { "TLSCACertificate", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_CACERT|ARG_BINARY|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.11 "
            "NAME 'olcBkLloadTLSCACertificate' "
            "DESC 'X.509 certificate, must use ;binary' "
            "EQUALITY certificateExactMatch "
            "SYNTAX 1.3.6.1.4.1.1466.115.121.1.8 "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSCACertificateFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_CA_FILE|ARG_STRING|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.12 "
            "NAME 'olcBkLloadTLSCACertificateFile' "
            "EQUALITY caseExactMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSCACertificatePath", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_CA_PATH|ARG_STRING|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.13 "
            "NAME 'olcBkLloadTLSCACertificatePath' "
            "EQUALITY caseExactMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSCertificate", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_CERT|ARG_BINARY|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.14 "
            "NAME 'olcBkLloadTLSCertificate' "
            "DESC 'X.509 certificate, must use ;binary' "
            "EQUALITY certificateExactMatch "
            "SYNTAX 1.3.6.1.4.1.1466.115.121.1.8 "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSCertificateFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_CERT_FILE|ARG_STRING|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.15 "
            "NAME 'olcBkLloadTLSCertificateFile' "
            "EQUALITY caseExactMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSCertificateKey", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_KEY|ARG_BINARY|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.16 "
            "NAME 'olcBkLloadTLSCertificateKey' "
            "DESC 'X.509 privateKey, must use ;binary' "
            "EQUALITY privateKeyMatch "
            "SYNTAX 1.2.840.113549.1.8.1.1 "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSCertificateKeyFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_CERT_KEY|ARG_STRING|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.17 "
            "NAME 'olcBkLloadTLSCertificateKeyFile' "
            "EQUALITY caseExactMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSCipherSuite", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_CIPHER|ARG_STRING|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.18 "
            "NAME 'olcBkLloadTLSCipherSuite' "
            "EQUALITY caseExactMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSCRLCheck", NULL, 2, 2, 0,
#if defined(HAVE_TLS) && defined(HAVE_OPENSSL)
        CFG_TLS_CRLCHECK|ARG_STRING|ARG_MAGIC,
        &config_tls_config,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.19 "
            "NAME 'olcBkLloadTLSCRLCheck' "
            "EQUALITY caseIgnoreMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSCRLFile", NULL, 2, 2, 0,
#if defined(HAVE_GNUTLS)
        CFG_TLS_CRL_FILE|ARG_STRING|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.20 "
            "NAME 'olcBkLloadTLSCRLFile' "
            "EQUALITY caseExactMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSRandFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_RAND|ARG_STRING|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.21 "
            "NAME 'olcBkLloadTLSRandFile' "
            "EQUALITY caseExactMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSVerifyClient", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_VERIFY|ARG_STRING|ARG_MAGIC,
        &config_tls_config,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.22 "
            "NAME 'olcBkLloadVerifyClient' "
            "EQUALITY caseIgnoreMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSDHParamFile", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_DH_FILE|ARG_STRING|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.23 "
            "NAME 'olcBkLloadTLSDHParamFile' "
            "EQUALITY caseExactMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSECName", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_ECNAME|ARG_STRING|ARG_MAGIC,
        &config_tls_option,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.24 "
            "NAME 'olcBkLloadTLSECName' "
            "EQUALITY caseExactMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSProtocolMin", NULL, 2, 2, 0,
#ifdef HAVE_TLS
        CFG_TLS_PROTOCOL_MIN|ARG_STRING|ARG_MAGIC,
        &config_tls_config,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.25 "
            "NAME 'olcBkLloadTLSProtocolMin' "
            "EQUALITY caseIgnoreMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "TLSShareSlapdCTX", NULL, 2, 2, 0,
#if defined(HAVE_TLS) && defined(BALANCER_MODULE)
        CFG_TLS_SHARE_CTX|ARG_ON_OFF|ARG_MAGIC,
        &config_share_tls_ctx,
#else
        ARG_IGNORED,
        NULL,
#endif
        "( OLcfgBkAt:13.33 "
            "NAME 'olcBkLloadTLSShareSlapdCTX' "
            "DESC 'Share slapd TLS context (all other lloadd TLS options cease to take effect)' "
            "EQUALITY booleanMatch "
            "SYNTAX OMsBoolean "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "iotimeout", "ms timeout", 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_IOTIMEOUT,
        &config_generic,
        "( OLcfgBkAt:13.26 "
            "NAME 'olcBkLloadIOTimeout' "
            "DESC 'I/O timeout threshold in miliseconds' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "client_max_pending", NULL, 2, 2, 0,
        ARG_MAGIC|ARG_UINT|CFG_CLIENT_PENDING,
        &config_generic,
        "( OLcfgBkAt:13.35 "
            "NAME 'olcBkLloadClientMaxPending' "
            "DESC 'Maximum pending operations per client connection' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL,
        { .v_uint = 0 }
    },

    /* cn=config only options */
#ifdef BALANCER_MODULE
    { "", "uri", 2, 2, 0,
        ARG_BERVAL|ARG_MAGIC|CFG_URI,
        &backend_cf_gen,
        "( OLcfgBkAt:13.27 "
            "NAME 'olcBkLloadBackendUri' "
            "DESC 'URI to contact the server on' "
            "EQUALITY caseIgnoreMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "", NULL, 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_NUMCONNS,
        &backend_cf_gen,
        "( OLcfgBkAt:13.28 "
            "NAME 'olcBkLloadNumconns' "
            "DESC 'Number of regular connections to maintain' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "", NULL, 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_BINDCONNS,
        &backend_cf_gen,
        "( OLcfgBkAt:13.29 "
            "NAME 'olcBkLloadBindconns' "
            "DESC 'Number of bind connections to maintain' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "", NULL, 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_RETRY,
        &backend_cf_gen,
        "( OLcfgBkAt:13.30 "
            "NAME 'olcBkLloadRetry' "
            "DESC 'Number of seconds to wait before trying to reconnect' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "", NULL, 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_MAX_PENDING_OPS,
        &backend_cf_gen,
        "( OLcfgBkAt:13.31 "
            "NAME 'olcBkLloadMaxPendingOps' "
            "DESC 'Maximum number of pending operations for this backend' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "", NULL, 2, 2, 0,
        ARG_UINT|ARG_MAGIC|CFG_MAX_PENDING_CONNS,
        &backend_cf_gen,
        "( OLcfgBkAt:13.32 "
            "NAME 'olcBkLloadMaxPendingConns' "
            "DESC 'Maximum number of pending operations on each connection' "
            "EQUALITY integerMatch "
            "SYNTAX OMsInteger "
            "SINGLE-VALUE )",
        NULL, NULL
    },
    { "", NULL, 2, 2, 0,
        ARG_BERVAL|ARG_MAGIC|CFG_STARTTLS,
        &backend_cf_gen,
        "( OLcfgBkAt:13.34 "
            "NAME 'olcBkLloadStartTLS' "
            "DESC 'Whether StartTLS should be attempted on the connection' "
            "EQUALITY caseIgnoreMatch "
            "SYNTAX OMsDirectoryString "
            "SINGLE-VALUE )",
        NULL, NULL
    },
#endif /* BALANCER_MODULE */

    { NULL, NULL, 0, 0, 0, ARG_IGNORED, NULL }
};

#ifdef BALANCER_MODULE
static ConfigCfAdd lload_cfadd;
static ConfigLDAPadd lload_backend_ldadd;
#ifdef SLAP_CONFIG_DELETE
static ConfigLDAPdel lload_backend_lddel;
#endif /* SLAP_CONFIG_DELETE */

static ConfigOCs lloadocs[] = {
    { "( OLcfgBkOc:13.1 "
        "NAME 'olcBkLloadConfig' "
        "DESC 'Lload backend configuration' "
        "SUP olcBackendConfig "
        "MUST ( olcBkLloadBindconf "
            "$ olcBkLloadIOThreads "
            "$ olcBkLloadListen "
            "$ olcBkLloadSockbufMaxClient "
            "$ olcBkLloadSockbufMaxUpstream "
            "$ olcBkLloadMaxPDUPerCycle "
            "$ olcBkLloadIOTimeout ) "
        "MAY ( olcBkLloadFeature "
            "$ olcBkLloadTcpBuffer "
            "$ olcBkLloadTLSCACertificateFile "
            "$ olcBkLloadTLSCACertificatePath "
            "$ olcBkLloadTLSCertificateFile "
            "$ olcBkLloadTLSCertificateKeyFile "
            "$ olcBkLloadTLSCipherSuite "
            "$ olcBkLloadTLSCRLCheck "
            "$ olcBkLloadTLSRandFile "
            "$ olcBkLloadVerifyClient "
            "$ olcBkLloadTLSDHParamFile "
            "$ olcBkLloadTLSECName "
            "$ olcBkLloadTLSProtocolMin "
            "$ olcBkLloadTLSCRLFile "
            "$ olcBkLloadTLSShareSlapdCTX "
            "$ olcBkLloadClientMaxPending "
        ") )",
        Cft_Backend, config_back_cf_table,
        NULL,
        lload_cfadd,
    },
    { "( OLcfgBkOc:13.2 "
        "NAME 'olcBkLloadBackendConfig' "
        "DESC 'Lload backend server configuration' "
        "SUP olcConfig STRUCTURAL "
        "MUST ( cn "
            "$ olcBkLloadBackendUri "
            "$ olcBkLloadNumconns "
            "$ olcBkLloadBindconns "
            "$ olcBkLloadRetry "
            "$ olcBkLloadMaxPendingOps "
            "$ olcBkLloadMaxPendingConns ) "
        "MAY ( olcBkLloadStartTLS "
        ") )",
        Cft_Misc, config_back_cf_table,
        lload_backend_ldadd,
        NULL,
#ifdef SLAP_CONFIG_DELETE
        lload_backend_lddel,
#endif /* SLAP_CONFIG_DELETE */
    },
    { NULL, 0, NULL }
};
#endif /* BALANCER_MODULE */

static int
config_generic( ConfigArgs *c )
{
    enum lcf_daemon flag = 0;
    int rc = LDAP_SUCCESS;

    if ( c->op == SLAP_CONFIG_EMIT ) {
        switch ( c->type ) {
            case CFG_IOTHREADS:
                c->value_uint = lload_daemon_threads;
                break;
            case CFG_LISTEN_URI: {
                LloadListener **ll = lloadd_get_listeners();
                struct berval bv = BER_BVNULL;

                for ( ; ll && *ll; ll++ ) {
                    /* The same url could have spawned several consecutive
                     * listeners */
                    if ( !BER_BVISNULL( &bv ) &&
                            !ber_bvcmp( &bv, &(*ll)->sl_url ) ) {
                        continue;
                    }
                    ber_dupbv( &bv, &(*ll)->sl_url );
                    ber_bvarray_add( &c->rvalue_vals, &bv );
                }
            } break;
            case CFG_MAXBUF_CLIENT:
                c->value_uint = sockbuf_max_incoming_client;
                break;
            case CFG_MAXBUF_UPSTREAM:
                c->value_uint = sockbuf_max_incoming_upstream;
                break;
            case CFG_RESCOUNT:
                c->value_uint = lload_conn_max_pdus_per_cycle;
                break;
            case CFG_IOTIMEOUT:
                c->value_uint = 1000 * lload_write_timeout->tv_sec +
                        lload_write_timeout->tv_usec / 1000;
                break;
            case CFG_CLIENT_PENDING:
                c->value_uint = lload_client_max_pending;
                break;
            default:
                rc = 1;
                break;
        }
        return rc;

    } else if ( c->op == LDAP_MOD_DELETE ) {
        /* We only need to worry about deletions to multi-value or MAY
         * attributes that belong to the lloadd module - we don't have any at
         * the moment */
        return rc;
    }

    lload_change.type = LLOAD_CHANGE_MODIFY;
    lload_change.object = LLOAD_DAEMON;

    switch ( c->type ) {
        case CFG_CONCUR:
            ldap_pvt_thread_set_concurrency( c->value_uint );
            break;
        case CFG_LISTEN:
            if ( lloadd_inited ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "listen directive can only be specified once" );
                ch_free( c->value_string );
                return 1;
            }
            if ( lloadd_listeners_init( c->value_string ) ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "could not open one of the listener sockets: %s",
                        c->value_string );
                ch_free( c->value_string );
                return 1;
            }
            ch_free( c->value_string );
            break;
        case CFG_LISTEN_URI: {
            LDAPURLDesc *lud;
            LloadListener *l;

            if ( ldap_url_parse_ext(
                         c->line, &lud, LDAP_PVT_URL_PARSE_DEF_PORT ) ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "string %s could not be parsed as an LDAP URL",
                        c->line );
                goto fail;
            }

            /* A sanity check, although it will not catch everything */
            if ( ( l = lload_config_check_my_url( c->line, lud ) ) ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "Load Balancer already configured to listen on %s "
                        "(while adding %s)",
                        l->sl_url.bv_val, c->line );
                goto fail;
            }

            if ( !lloadd_inited ) {
                if ( lload_open_new_listener( c->line, lud ) ) {
                    snprintf( c->cr_msg, sizeof(c->cr_msg),
                            "could not open a listener for %s", c->line );
                    goto fail;
                }
            } else {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "listener changes will not take effect until restart: "
                        "%s",
                        c->line );
                Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
            }
        } break;
        case CFG_THREADS:
            if ( c->value_uint < 2 ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "threads=%d smaller than minimum value 2",
                        c->value_uint );
                goto fail;

            } else if ( c->value_uint > 2 * SLAP_MAX_WORKER_THREADS ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "warning, threads=%d larger than twice the default "
                        "(2*%d=%d); YMMV",
                        c->value_uint, SLAP_MAX_WORKER_THREADS,
                        2 * SLAP_MAX_WORKER_THREADS );
                Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
            }
            if ( slapMode & SLAP_SERVER_MODE )
                ldap_pvt_thread_pool_maxthreads(
                        &connection_pool, c->value_uint );
            connection_pool_max = c->value_uint; /* save for reference */
            break;

        case CFG_THREADQS:
            if ( c->value_uint < 1 ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "threadqueues=%d smaller than minimum value 1",
                        c->value_uint );
                goto fail;
            }
            if ( slapMode & SLAP_SERVER_MODE )
                ldap_pvt_thread_pool_queues( &connection_pool, c->value_uint );
            connection_pool_queues = c->value_uint; /* save for reference */
            break;

        case CFG_IOTHREADS: {
            int mask = 0;
            /* use a power of two */
            while ( c->value_uint > 1 ) {
                c->value_uint >>= 1;
                mask <<= 1;
                mask |= 1;
            }
            if ( !lloadd_inited ) {
                lload_daemon_mask = mask;
                lload_daemon_threads = mask + 1;
                flag = LLOAD_DAEMON_MOD_THREADS;
            } else {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "io thread changes will not take effect until "
                        "restart" );
                Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
            }
        } break;

        case CFG_LOGFILE: {
            if ( logfileName ) ch_free( logfileName );
            logfileName = c->value_string;
            logfile = fopen( logfileName, "w" );
            if ( logfile ) lutil_debug_file( logfile );
        } break;

        case CFG_RESCOUNT:
            lload_conn_max_pdus_per_cycle = c->value_uint;
            break;

        case CFG_IOTIMEOUT:
            if ( c->value_uint > 0 ) {
                timeout_write_tv.tv_sec = c->value_uint / 1000;
                timeout_write_tv.tv_usec = 1000 * ( c->value_uint % 1000 );
                lload_write_timeout = &timeout_write_tv;
            } else {
                lload_write_timeout = NULL;
            }
            break;
        case CFG_MAXBUF_CLIENT:
            sockbuf_max_incoming_client = c->value_uint;
            break;
        case CFG_MAXBUF_UPSTREAM:
            sockbuf_max_incoming_upstream = c->value_uint;
            break;
        case CFG_CLIENT_PENDING:
            lload_client_max_pending = c->value_uint;
            break;
        default:
            Debug( LDAP_DEBUG_ANY, "%s: unknown CFG_TYPE %d\n",
                    c->log, c->type );
            return 1;
    }

    lload_change.flags.daemon |= flag;

    return 0;

fail:
    if ( lload_change.type == LLOAD_CHANGE_ADD ) {
        /* Abort the ADD */
        lload_change.type = LLOAD_CHANGE_DEL;
    }

    Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
    return 1;
}

static int
lload_backend_finish( ConfigArgs *ca )
{
    LloadBackend *b = ca->ca_private;

    if ( ca->reply.err != LDAP_SUCCESS ) {
        /* Not reached since cleanup is only called on success */
        goto fail;
    }

    if ( b->b_numconns <= 0 || b->b_numbindconns <= 0 ) {
        Debug( LDAP_DEBUG_ANY, "lload_backend_finish: "
                "invalid connection pool configuration\n" );
        goto fail;
    }

    if ( b->b_retry_timeout < 0 ) {
        Debug( LDAP_DEBUG_ANY, "lload_backend_finish: "
                "invalid retry timeout configuration\n" );
        goto fail;
    }

    b->b_retry_tv.tv_sec = b->b_retry_timeout / 1000;
    b->b_retry_tv.tv_usec = ( b->b_retry_timeout % 1000 ) * 1000;

    /* daemon_base is only allocated after initial configuration happens, those
     * events are allocated on startup, we only deal with online Adds */
    if ( !b->b_retry_event && daemon_base ) {
        struct event *event;
        assert( CONFIG_ONLINE_ADD( ca ) );
        event = evtimer_new( daemon_base, backend_connect, b );
        if ( !event ) {
            Debug( LDAP_DEBUG_ANY, "lload_backend_finish: "
                    "failed to allocate retry event\n" );
            goto fail;
        }
        b->b_retry_event = event;
    }

    return LDAP_SUCCESS;

fail:
    if ( lload_change.type == LLOAD_CHANGE_ADD ) {
        /* Abort the ADD */
        lload_change.type = LLOAD_CHANGE_DEL;
    }

    lload_backend_destroy( b );
    return -1;
}

static LloadBackend *
backend_alloc( void )
{
    LloadBackend *b;

    b = ch_calloc( 1, sizeof(LloadBackend) );

    LDAP_CIRCLEQ_INIT( &b->b_conns );
    LDAP_CIRCLEQ_INIT( &b->b_bindconns );
    LDAP_CIRCLEQ_INIT( &b->b_preparing );

    b->b_numconns = 1;
    b->b_numbindconns = 1;

    b->b_retry_timeout = 5000;

    ldap_pvt_thread_mutex_init( &b->b_mutex );

    LDAP_CIRCLEQ_INSERT_TAIL( &backend, b, b_next );
    return b;
}

static int
backend_config_url( LloadBackend *b, struct berval *uri )
{
    LDAPURLDesc *lud = NULL;
    char *host = NULL;
    int rc, proto, tls = b->b_tls_conf;

    /* Effect no changes until we've checked everything */

    rc = ldap_url_parse_ext( uri->bv_val, &lud, LDAP_PVT_URL_PARSE_DEF_PORT );
    if ( rc != LDAP_URL_SUCCESS ) {
        Debug( LDAP_DEBUG_ANY, "backend_config_url: "
                "listen URL \"%s\" parse error=%d\n",
                uri->bv_val, rc );
        return -1;
    }

    if ( ldap_pvt_url_scheme2tls( lud->lud_scheme ) ) {
#ifdef HAVE_TLS
        /* Specifying ldaps:// overrides starttls= settings */
        tls = LLOAD_LDAPS;
#else /* ! HAVE_TLS */

        Debug( LDAP_DEBUG_ANY, "backend_config_url: "
                "TLS not supported (%s)\n",
                uri->bv_val );
        rc = -1;
        goto done;
#endif /* ! HAVE_TLS */
    }

    proto = ldap_pvt_url_scheme2proto( lud->lud_scheme );
    if ( proto == LDAP_PROTO_IPC ) {
#ifdef LDAP_PF_LOCAL
        if ( lud->lud_host == NULL || lud->lud_host[0] == '\0' ) {
            host = LDAPI_SOCK;
        }
#else /* ! LDAP_PF_LOCAL */

        Debug( LDAP_DEBUG_ANY, "backend_config_url: "
                "URL scheme not supported: %s",
                url );
        rc = -1;
        goto done;
#endif /* ! LDAP_PF_LOCAL */
    } else {
        if ( lud->lud_host == NULL || lud->lud_host[0] == '\0' ) {
            Debug( LDAP_DEBUG_ANY, "backend_config_url: "
                    "backend url missing hostname: '%s'\n",
                    uri->bv_val );
            rc = -1;
            goto done;
        }
    }
    if ( !host ) {
        host = lud->lud_host;
    }

    if ( b->b_host ) {
        ch_free( b->b_host );
    }

    b->b_proto = proto;
    b->b_tls = tls;
    b->b_port = lud->lud_port;
    b->b_host = ch_strdup( host );

done:
    ldap_free_urldesc( lud );
    return rc;
}

static int
config_backend( ConfigArgs *c )
{
    LloadBackend *b;
    int i, rc = 0;

    b = backend_alloc();

    for ( i = 1; i < c->argc; i++ ) {
        if ( lload_backend_parse( c->argv[i], b ) ) {
            Debug( LDAP_DEBUG_ANY, "config_backend: "
                    "error parsing backend configuration item '%s'\n",
                    c->argv[i] );
            return -1;
        }
    }

    if ( BER_BVISNULL( &b->b_uri ) ) {
        Debug( LDAP_DEBUG_ANY, "config_backend: "
                "backend address not specified\n" );
        rc = -1;
        goto done;
    }

    if ( backend_config_url( b, &b->b_uri ) ) {
        rc = -1;
        goto done;
    }

    c->ca_private = b;
    rc = lload_backend_finish( c );
done:
    if ( rc ) {
        ch_free( b );
    }
    return rc;
}

static int
config_bindconf( ConfigArgs *c )
{
    int i;

    if ( c->op == SLAP_CONFIG_EMIT ) {
        struct berval bv;

        lload_bindconf_unparse( &bindconf, &bv );

        for ( i = 0; isspace( (unsigned char)bv.bv_val[i] ); i++ )
            /* count spaces */;

        if ( i ) {
            bv.bv_len -= i;
            AC_MEMCPY( bv.bv_val, &bv.bv_val[i], bv.bv_len + 1 );
        }

        value_add_one( &c->rvalue_vals, &bv );
        ber_memfree( bv.bv_val );
        return LDAP_SUCCESS;
    } else if ( c->op == LDAP_MOD_DELETE ) {
        /* It's a MUST single-valued attribute, noop for now */
        lload_bindconf_free( &bindconf );
        return LDAP_SUCCESS;
    }

    lload_change.type = LLOAD_CHANGE_MODIFY;
    lload_change.object = LLOAD_DAEMON;
    lload_change.flags.daemon |= LLOAD_DAEMON_MOD_BINDCONF;

    for ( i = 1; i < c->argc; i++ ) {
        if ( lload_bindconf_parse( c->argv[i], &bindconf ) ) {
            Debug( LDAP_DEBUG_ANY, "config_bindconf: "
                    "error parsing backend configuration item '%s'\n",
                    c->argv[i] );
            return -1;
        }
    }

    if ( bindconf.sb_method == LDAP_AUTH_SASL ) {
#ifndef HAVE_CYRUS_SASL
        Debug( LDAP_DEBUG_ANY, "config_bindconf: "
                "no sasl support available\n" );
        return -1;
#endif
    }

    if ( !BER_BVISNULL( &bindconf.sb_authzId ) ) {
        ber_dupbv( &lloadd_identity, &bindconf.sb_authzId );
    } else if ( !BER_BVISNULL( &bindconf.sb_authcId ) ) {
        ber_dupbv( &lloadd_identity, &bindconf.sb_authcId );
    } else if ( !BER_BVISNULL( &bindconf.sb_binddn ) ) {
        char *ptr;

        lloadd_identity.bv_len = STRLENOF("dn:") + bindconf.sb_binddn.bv_len;
        lloadd_identity.bv_val = ch_malloc( lloadd_identity.bv_len + 1 );

        ptr = lutil_strcopy( lloadd_identity.bv_val, "dn:" );
        ptr = lutil_strncopy(
                ptr, bindconf.sb_binddn.bv_val, bindconf.sb_binddn.bv_len );
        *ptr = '\0';
    }

    if ( bindconf.sb_timeout_api ) {
        timeout_api_tv.tv_sec = bindconf.sb_timeout_api;
        lload_timeout_api = &timeout_api_tv;
        if ( lload_timeout_event ) {
            event_add( lload_timeout_event, lload_timeout_api );
        }
    } else {
        lload_timeout_api = NULL;
        if ( lload_timeout_event ) {
            event_del( lload_timeout_event );
        }
    }

    if ( bindconf.sb_timeout_net ) {
        timeout_net_tv.tv_sec = bindconf.sb_timeout_net;
        lload_timeout_net = &timeout_net_tv;
    } else {
        lload_timeout_net = NULL;
    }

#ifdef HAVE_TLS
    if ( bindconf.sb_tls_do_init ) {
        lload_bindconf_tls_set( &bindconf, lload_tls_backend_ld );
    }
#endif /* HAVE_TLS */
    return 0;
}

static int
config_fname( ConfigArgs *c )
{
    return 0;
}

/*
 * [listener=<listener>] [{read|write}=]<size>
 */

#ifdef LDAP_TCP_BUFFER
static BerVarray tcp_buffer;
int tcp_buffer_num;

#define SLAP_TCP_RMEM ( 0x1U )
#define SLAP_TCP_WMEM ( 0x2U )

static int
tcp_buffer_parse(
        struct berval *val,
        int argc,
        char **argv,
        int *size,
        int *rw,
        LloadListener **l )
{
    int i, rc = LDAP_SUCCESS;
    LDAPURLDesc *lud = NULL;
    char *ptr;

    if ( val != NULL && argv == NULL ) {
        char *s = val->bv_val;

        argv = ldap_str2charray( s, " \t" );
        if ( argv == NULL ) {
            return LDAP_OTHER;
        }
    }

    i = 0;
    if ( strncasecmp( argv[i], "listener=", STRLENOF("listener=") ) == 0 ) {
        char *url = argv[i] + STRLENOF("listener=");

        if ( ldap_url_parse_ext( url, &lud, LDAP_PVT_URL_PARSE_DEF_PORT ) ) {
            rc = LDAP_INVALID_SYNTAX;
            goto done;
        }

        *l = lload_config_check_my_url( url, lud );
        if ( *l == NULL ) {
            rc = LDAP_NO_SUCH_ATTRIBUTE;
            goto done;
        }

        i++;
    }

    ptr = argv[i];
    if ( strncasecmp( ptr, "read=", STRLENOF("read=") ) == 0 ) {
        *rw |= SLAP_TCP_RMEM;
        ptr += STRLENOF("read=");

    } else if ( strncasecmp( ptr, "write=", STRLENOF("write=") ) == 0 ) {
        *rw |= SLAP_TCP_WMEM;
        ptr += STRLENOF("write=");

    } else {
        *rw |= ( SLAP_TCP_RMEM | SLAP_TCP_WMEM );
    }

    /* accept any base */
    if ( lutil_atoix( size, ptr, 0 ) ) {
        rc = LDAP_INVALID_SYNTAX;
        goto done;
    }

done:;
    if ( val != NULL && argv != NULL ) {
        ldap_charray_free( argv );
    }

    if ( lud != NULL ) {
        ldap_free_urldesc( lud );
    }

    return rc;
}

#ifdef BALANCER_MODULE
static int
tcp_buffer_delete_one( struct berval *val )
{
    int rc = 0;
    int size = -1, rw = 0;
    LloadListener *l = NULL;

    rc = tcp_buffer_parse( val, 0, NULL, &size, &rw, &l );
    if ( rc != 0 ) {
        return rc;
    }

    if ( l != NULL ) {
        int i;
        LloadListener **ll = lloadd_get_listeners();

        for ( i = 0; ll[i] != NULL; i++ ) {
            if ( ll[i] == l ) break;
        }

        if ( ll[i] == NULL ) {
            return LDAP_NO_SUCH_ATTRIBUTE;
        }

        if ( rw & SLAP_TCP_RMEM ) l->sl_tcp_rmem = -1;
        if ( rw & SLAP_TCP_WMEM ) l->sl_tcp_wmem = -1;

        for ( i++; ll[i] != NULL && bvmatch( &l->sl_url, &ll[i]->sl_url );
                i++ ) {
            if ( rw & SLAP_TCP_RMEM ) ll[i]->sl_tcp_rmem = -1;
            if ( rw & SLAP_TCP_WMEM ) ll[i]->sl_tcp_wmem = -1;
        }

    } else {
        /* NOTE: this affects listeners without a specific setting,
         * does not reset all listeners.  If a listener without
         * specific settings was assigned a buffer because of
         * a global setting, it will not be reset.  In any case,
         * buffer changes will only take place at restart. */
        if ( rw & SLAP_TCP_RMEM ) slapd_tcp_rmem = -1;
        if ( rw & SLAP_TCP_WMEM ) slapd_tcp_wmem = -1;
    }

    return rc;
}

static int
tcp_buffer_delete( BerVarray vals )
{
    int i;

    for ( i = 0; !BER_BVISNULL( &vals[i] ); i++ ) {
        tcp_buffer_delete_one( &vals[i] );
    }

    return 0;
}
#endif /* BALANCER_MODULE */

static int
tcp_buffer_unparse( int size, int rw, LloadListener *l, struct berval *val )
{
    char buf[sizeof("2147483648")], *ptr;

    /* unparse for later use */
    val->bv_len = snprintf( buf, sizeof(buf), "%d", size );
    if ( l != NULL ) {
        val->bv_len += STRLENOF( "listener="
                                 " " ) +
                l->sl_url.bv_len;
    }

    if ( rw != ( SLAP_TCP_RMEM | SLAP_TCP_WMEM ) ) {
        if ( rw & SLAP_TCP_RMEM ) {
            val->bv_len += STRLENOF("read=");
        } else if ( rw & SLAP_TCP_WMEM ) {
            val->bv_len += STRLENOF("write=");
        }
    }

    val->bv_val = SLAP_MALLOC( val->bv_len + 1 );

    ptr = val->bv_val;

    if ( l != NULL ) {
        ptr = lutil_strcopy( ptr, "listener=" );
        ptr = lutil_strncopy( ptr, l->sl_url.bv_val, l->sl_url.bv_len );
        *ptr++ = ' ';
    }

    if ( rw != ( SLAP_TCP_RMEM | SLAP_TCP_WMEM ) ) {
        if ( rw & SLAP_TCP_RMEM ) {
            ptr = lutil_strcopy( ptr, "read=" );
        } else if ( rw & SLAP_TCP_WMEM ) {
            ptr = lutil_strcopy( ptr, "write=" );
        }
    }

    ptr = lutil_strcopy( ptr, buf );
    *ptr = '\0';

    assert( val->bv_val + val->bv_len == ptr );

    return LDAP_SUCCESS;
}

static int
tcp_buffer_add_one( int argc, char **argv )
{
    int rc = 0;
    int size = -1, rw = 0;
    LloadListener *l = NULL;

    struct berval val;

    /* parse */
    rc = tcp_buffer_parse( NULL, argc, argv, &size, &rw, &l );
    if ( rc != 0 ) {
        return rc;
    }

    /* unparse for later use */
    rc = tcp_buffer_unparse( size, rw, l, &val );
    if ( rc != LDAP_SUCCESS ) {
        return rc;
    }

    /* use parsed values */
    if ( l != NULL ) {
        int i;
        LloadListener **ll = lloadd_get_listeners();

        for ( i = 0; ll[i] != NULL; i++ ) {
            if ( ll[i] == l ) break;
        }

        if ( ll[i] == NULL ) {
            return LDAP_NO_SUCH_ATTRIBUTE;
        }

        /* buffer only applies to TCP listeners;
         * we do not do any check here, and delegate them
         * to setsockopt(2) */
        if ( rw & SLAP_TCP_RMEM ) l->sl_tcp_rmem = size;
        if ( rw & SLAP_TCP_WMEM ) l->sl_tcp_wmem = size;

        for ( i++; ll[i] != NULL && bvmatch( &l->sl_url, &ll[i]->sl_url );
                i++ ) {
            if ( rw & SLAP_TCP_RMEM ) ll[i]->sl_tcp_rmem = size;
            if ( rw & SLAP_TCP_WMEM ) ll[i]->sl_tcp_wmem = size;
        }

    } else {
        /* NOTE: this affects listeners without a specific setting,
         * does not set all listeners */
        if ( rw & SLAP_TCP_RMEM ) slapd_tcp_rmem = size;
        if ( rw & SLAP_TCP_WMEM ) slapd_tcp_wmem = size;
    }

    tcp_buffer = SLAP_REALLOC(
            tcp_buffer, sizeof(struct berval) * ( tcp_buffer_num + 2 ) );
    /* append */
    tcp_buffer[tcp_buffer_num] = val;

    tcp_buffer_num++;
    BER_BVZERO( &tcp_buffer[tcp_buffer_num] );

    return rc;
}

static int
config_tcp_buffer( ConfigArgs *c )
{
    int rc = LDAP_SUCCESS;

#ifdef BALANCER_MODULE
    if ( c->op == SLAP_CONFIG_EMIT ) {
        if ( tcp_buffer == NULL || BER_BVISNULL( &tcp_buffer[0] ) ) {
            return 1;
        }
        value_add( &c->rvalue_vals, tcp_buffer );
        value_add( &c->rvalue_nvals, tcp_buffer );

        return 0;
    } else if ( c->op == LDAP_MOD_DELETE ) {
        if ( !c->line ) {
            tcp_buffer_delete( tcp_buffer );
            ber_bvarray_free( tcp_buffer );
            tcp_buffer = NULL;
            tcp_buffer_num = 0;

        } else {
            int size = -1, rw = 0;
            LloadListener *l = NULL;

            struct berval val = BER_BVNULL;

            int i;

            if ( tcp_buffer_num == 0 ) {
                return 1;
            }

            /* parse */
            rc = tcp_buffer_parse(
                    NULL, c->argc - 1, &c->argv[1], &size, &rw, &l );
            if ( rc != 0 ) {
                return 1;
            }

            /* unparse for later use */
            rc = tcp_buffer_unparse( size, rw, l, &val );
            if ( rc != LDAP_SUCCESS ) {
                return 1;
            }

            for ( i = 0; !BER_BVISNULL( &tcp_buffer[i] ); i++ ) {
                if ( bvmatch( &tcp_buffer[i], &val ) ) {
                    break;
                }
            }

            if ( BER_BVISNULL( &tcp_buffer[i] ) ) {
                /* not found */
                rc = 1;
                goto done;
            }

            tcp_buffer_delete_one( &tcp_buffer[i] );
            ber_memfree( tcp_buffer[i].bv_val );
            for ( ; i < tcp_buffer_num; i++ ) {
                tcp_buffer[i] = tcp_buffer[i + 1];
            }
            tcp_buffer_num--;

done:;
            if ( !BER_BVISNULL( &val ) ) {
                SLAP_FREE(val.bv_val);
            }
        }

        return rc;
    }
#endif /* BALANCER_MODULE */

    rc = tcp_buffer_add_one( c->argc - 1, &c->argv[1] );
    if ( rc ) {
        snprintf( c->cr_msg, sizeof(c->cr_msg), "<%s> unable to add value #%d",
                c->argv[0], tcp_buffer_num );
        Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
        return 1;
    }

    return 0;
}
#endif /* LDAP_TCP_BUFFER */

static int
config_restrict( ConfigArgs *c )
{
    slap_mask_t restrictops = 0;
    int i;
    slap_verbmasks restrictable_ops[] = {
        { BER_BVC("bind"), SLAP_RESTRICT_OP_BIND },
        { BER_BVC("add"), SLAP_RESTRICT_OP_ADD },
        { BER_BVC("modify"), SLAP_RESTRICT_OP_MODIFY },
        { BER_BVC("rename"), SLAP_RESTRICT_OP_RENAME },
        { BER_BVC("modrdn"), 0 },
        { BER_BVC("delete"), SLAP_RESTRICT_OP_DELETE },
        { BER_BVC("search"), SLAP_RESTRICT_OP_SEARCH },
        { BER_BVC("compare"), SLAP_RESTRICT_OP_COMPARE },
        { BER_BVC("read"), SLAP_RESTRICT_OP_READS },
        { BER_BVC("write"), SLAP_RESTRICT_OP_WRITES },
        { BER_BVC("extended"), SLAP_RESTRICT_OP_EXTENDED },
        { BER_BVC("extended=" LDAP_EXOP_START_TLS), SLAP_RESTRICT_EXOP_START_TLS },
        { BER_BVC("extended=" LDAP_EXOP_MODIFY_PASSWD), SLAP_RESTRICT_EXOP_MODIFY_PASSWD },
        { BER_BVC("extended=" LDAP_EXOP_X_WHO_AM_I), SLAP_RESTRICT_EXOP_WHOAMI },
        { BER_BVC("extended=" LDAP_EXOP_X_CANCEL), SLAP_RESTRICT_EXOP_CANCEL },
        { BER_BVC("all"), SLAP_RESTRICT_OP_ALL },
        { BER_BVNULL, 0 }
    };

    i = verbs_to_mask( c->argc, c->argv, restrictable_ops, &restrictops );
    if ( i ) {
        snprintf( c->cr_msg, sizeof(c->cr_msg), "<%s> unknown operation",
                c->argv[0] );
        Debug( LDAP_DEBUG_ANY, "%s: %s %s\n",
                c->log, c->cr_msg, c->argv[i] );
        return 1;
    }
    if ( restrictops & SLAP_RESTRICT_OP_EXTENDED )
        restrictops &= ~SLAP_RESTRICT_EXOP_MASK;
    return 0;
}

static slap_verbmasks *loglevel_ops;

static int
loglevel_init( void )
{
    slap_verbmasks lo[] = {
        { BER_BVC("Any"), (slap_mask_t)LDAP_DEBUG_ANY },
        { BER_BVC("Trace"), LDAP_DEBUG_TRACE },
        { BER_BVC("Packets"), LDAP_DEBUG_PACKETS },
        { BER_BVC("Args"), LDAP_DEBUG_ARGS },
        { BER_BVC("Conns"), LDAP_DEBUG_CONNS },
        { BER_BVC("BER"), LDAP_DEBUG_BER },
        { BER_BVC("Filter"), LDAP_DEBUG_FILTER },
        { BER_BVC("Config"), LDAP_DEBUG_CONFIG },
        { BER_BVC("ACL"), LDAP_DEBUG_ACL },
        { BER_BVC("Stats"), LDAP_DEBUG_STATS },
        { BER_BVC("Stats2"), LDAP_DEBUG_STATS2 },
        { BER_BVC("Shell"), LDAP_DEBUG_SHELL },
        { BER_BVC("Parse"), LDAP_DEBUG_PARSE },
        { BER_BVC("Sync"), LDAP_DEBUG_SYNC },
        { BER_BVC("None"), LDAP_DEBUG_NONE },
        { BER_BVNULL, 0 }
    };

    return slap_verbmasks_init( &loglevel_ops, lo );
}

static void
loglevel_destroy( void )
{
    if ( loglevel_ops ) {
        (void)slap_verbmasks_destroy( loglevel_ops );
    }
    loglevel_ops = NULL;
}

int
str2loglevel( const char *s, int *l )
{
    int i;

    if ( loglevel_ops == NULL ) {
        loglevel_init();
    }

    i = verb_to_mask( s, loglevel_ops );

    if ( BER_BVISNULL( &loglevel_ops[i].word ) ) {
        return -1;
    }

    *l = loglevel_ops[i].mask;

    return 0;
}

int
loglevel2bvarray( int l, BerVarray *bva )
{
    if ( loglevel_ops == NULL ) {
        loglevel_init();
    }

    if ( l == 0 ) {
        struct berval bv = BER_BVC("0");
        return value_add_one( bva, &bv );
    }

    return mask_to_verbs( loglevel_ops, l, bva );
}

int
loglevel_print( FILE *out )
{
    int i;

    if ( loglevel_ops == NULL ) {
        loglevel_init();
    }

    fprintf( out, "Installed log subsystems:\n\n" );
    for ( i = 0; !BER_BVISNULL( &loglevel_ops[i].word ); i++ ) {
        unsigned mask = loglevel_ops[i].mask & 0xffffffffUL;
        fprintf( out,
                ( mask == ( (slap_mask_t)-1 & 0xffffffffUL ) ?
                                "\t%-30s (-1, 0xffffffff)\n" :
                                "\t%-30s (%u, 0x%x)\n" ),
                loglevel_ops[i].word.bv_val, mask, mask );
    }

    fprintf( out,
            "\nNOTE: custom log subsystems may be later installed "
            "by specific code\n\n" );

    return 0;
}

static int config_syslog;

static int
config_loglevel( ConfigArgs *c )
{
    int i;

    if ( loglevel_ops == NULL ) {
        loglevel_init();
    }

    if ( c->op == SLAP_CONFIG_EMIT ) {
        /* Get default or commandline slapd setting */
        if ( ldap_syslog && !config_syslog ) config_syslog = ldap_syslog;
        return loglevel2bvarray( config_syslog, &c->rvalue_vals );

    } else if ( c->op == LDAP_MOD_DELETE ) {
        if ( !c->line ) {
            config_syslog = 0;
        } else {
            i = verb_to_mask( c->line, loglevel_ops );
            config_syslog &= ~loglevel_ops[i].mask;
        }
        if ( slapMode & SLAP_SERVER_MODE ) {
            ldap_syslog = config_syslog;
        }
        return 0;
    }

    for ( i = 1; i < c->argc; i++ ) {
        int level;

        if ( isdigit( (unsigned char)c->argv[i][0] ) || c->argv[i][0] == '-' ) {
            if ( lutil_atoix( &level, c->argv[i], 0 ) != 0 ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "<%s> unable to parse level",
                        c->argv[0] );
                Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
                        c->log, c->cr_msg, c->argv[i] );
                return 1;
            }
        } else {
            if ( str2loglevel( c->argv[i], &level ) ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg), "<%s> unknown level",
                        c->argv[0] );
                Debug( LDAP_DEBUG_ANY, "%s: %s \"%s\"\n",
                        c->log, c->cr_msg, c->argv[i] );
                return 1;
            }
        }
        /* Explicitly setting a zero clears all the levels */
        if ( level )
            config_syslog |= level;
        else
            config_syslog = 0;
    }
    if ( slapMode & SLAP_SERVER_MODE ) {
        ldap_syslog = config_syslog;
    }
    return 0;
}

static int
config_include( ConfigArgs *c )
{
    int savelineno = c->lineno;
    int rc;
    ConfigFile *cf;
    ConfigFile *cfsave = cfn;
    ConfigFile *cf2 = NULL;

    /* Leftover from RE23. No dynamic config for include files */
    if ( c->op == SLAP_CONFIG_EMIT || c->op == LDAP_MOD_DELETE ) return 1;

    cf = ch_calloc( 1, sizeof(ConfigFile) );
    if ( cfn->c_kids ) {
        for ( cf2 = cfn->c_kids; cf2 && cf2->c_sibs; cf2 = cf2->c_sibs )
            /* empty */;
        cf2->c_sibs = cf;
    } else {
        cfn->c_kids = cf;
    }
    cfn = cf;
    ber_str2bv( c->argv[1], 0, 1, &cf->c_file );
    rc = lload_read_config_file(
            c->argv[1], c->depth + 1, c, config_back_cf_table );
    c->lineno = savelineno - 1;
    cfn = cfsave;
    if ( rc ) {
        if ( cf2 )
            cf2->c_sibs = NULL;
        else
            cfn->c_kids = NULL;
        ch_free( cf->c_file.bv_val );
        ch_free( cf );
    } else {
        c->ca_private = cf;
    }
    return rc;
}

static int
config_feature( ConfigArgs *c )
{
    slap_verbmasks features[] = {
#ifdef LDAP_API_FEATURE_VERIFY_CREDENTIALS
        { BER_BVC("vc"), LLOAD_FEATURE_VC },
#endif /* LDAP_API_FEATURE_VERIFY_CREDENTIALS */
        { BER_BVC("proxyauthz"), LLOAD_FEATURE_PROXYAUTHZ },
        { BER_BVC("read_pause"), LLOAD_FEATURE_PAUSE },
        { BER_BVNULL, 0 }
    };
    slap_mask_t mask = 0;
    int i;

    if ( c->op == SLAP_CONFIG_EMIT ) {
        return mask_to_verbs( features, lload_features, &c->rvalue_vals );
    }

    lload_change.type = LLOAD_CHANGE_MODIFY;
    lload_change.object = LLOAD_DAEMON;
    lload_change.flags.daemon |= LLOAD_DAEMON_MOD_FEATURES;
    if ( !lload_change.target ) {
        lload_change.target = (void *)(uintptr_t)~lload_features;
    }

    if ( c->op == LDAP_MOD_DELETE ) {
        if ( !c->line ) {
            /* Last value has been deleted */
            lload_features = 0;
        } else {
            i = verb_to_mask( c->line, features );
            lload_features &= ~features[i].mask;
        }
        return 0;
    }

    i = verbs_to_mask( c->argc, c->argv, features, &mask );
    if ( i ) {
        Debug( LDAP_DEBUG_ANY, "%s: <%s> unknown feature %s\n", c->log,
                c->argv[0], c->argv[i] );
        return 1;
    }

    if ( mask & ~LLOAD_FEATURE_SUPPORTED_MASK ) {
        for ( i = 1; i < c->argc; i++ ) {
            int j = verb_to_mask( c->argv[i], features );
            if ( features[j].mask & ~LLOAD_FEATURE_SUPPORTED_MASK ) {
                Debug( LDAP_DEBUG_ANY, "%s: <%s> "
                        "experimental feature %s is undocumented, unsupported "
                        "and can change or disappear at any time!\n",
                        c->log, c->argv[0], c->argv[i] );
            }
        }
    }

    lload_features |= mask;
    return 0;
}

#ifdef HAVE_TLS
static int
config_tls_cleanup( ConfigArgs *c )
{
    int rc = 0;

    if ( lload_tls_ld ) {
        int opt = 1;

        ldap_pvt_tls_ctx_free( lload_tls_ctx );
        lload_tls_ctx = NULL;

        /* Force new ctx to be created */
        rc = ldap_pvt_tls_set_option(
                lload_tls_ld, LDAP_OPT_X_TLS_NEWCTX, &opt );
        if ( rc == 0 ) {
            /* The ctx's refcount is bumped up here */
            ldap_pvt_tls_get_option(
                    lload_tls_ld, LDAP_OPT_X_TLS_CTX, &lload_tls_ctx );
        } else {
            if ( rc == LDAP_NOT_SUPPORTED )
                rc = LDAP_UNWILLING_TO_PERFORM;
            else
                rc = LDAP_OTHER;
        }
    }
    return rc;
}

static int
config_tls_option( ConfigArgs *c )
{
    int flag;
    int berval = 0;
    LDAP *ld = lload_tls_ld;

    switch ( c->type ) {
        case CFG_TLS_RAND:
            flag = LDAP_OPT_X_TLS_RANDOM_FILE;
            ld = NULL;
            break;
        case CFG_TLS_CIPHER:
            flag = LDAP_OPT_X_TLS_CIPHER_SUITE;
            break;
        case CFG_TLS_CERT_FILE:
            flag = LDAP_OPT_X_TLS_CERTFILE;
            break;
        case CFG_TLS_CERT_KEY:
            flag = LDAP_OPT_X_TLS_KEYFILE;
            break;
        case CFG_TLS_CA_PATH:
            flag = LDAP_OPT_X_TLS_CACERTDIR;
            break;
        case CFG_TLS_CA_FILE:
            flag = LDAP_OPT_X_TLS_CACERTFILE;
            break;
        case CFG_TLS_DH_FILE:
            flag = LDAP_OPT_X_TLS_DHFILE;
            break;
        case CFG_TLS_ECNAME:
            flag = LDAP_OPT_X_TLS_ECNAME;
            break;
#ifdef HAVE_GNUTLS
        case CFG_TLS_CRL_FILE:
            flag = LDAP_OPT_X_TLS_CRLFILE;
            break;
#endif
        case CFG_TLS_CACERT:
            flag = LDAP_OPT_X_TLS_CACERT;
            berval = 1;
            break;
        case CFG_TLS_CERT:
            flag = LDAP_OPT_X_TLS_CERT;
            berval = 1;
            break;
        case CFG_TLS_KEY:
            flag = LDAP_OPT_X_TLS_KEY;
            berval = 1;
            break;
        default:
            Debug( LDAP_DEBUG_ANY, "%s: "
                    "unknown tls_option <0x%x>\n",
                    c->log, c->type );
            return 1;
    }
    if ( c->op == SLAP_CONFIG_EMIT ) {
        return ldap_pvt_tls_get_option( ld, flag,
                berval ? (void *)&c->value_bv : (void *)&c->value_string );
    }

    lload_change.type = LLOAD_CHANGE_MODIFY;
    lload_change.object = LLOAD_DAEMON;
    lload_change.flags.daemon |= LLOAD_DAEMON_MOD_TLS;

    config_push_cleanup( c, config_tls_cleanup );
    if ( c->op == LDAP_MOD_DELETE ) {
        return ldap_pvt_tls_set_option( ld, flag, NULL );
    }
    if ( !berval ) ch_free( c->value_string );
    return ldap_pvt_tls_set_option(
            ld, flag, berval ? (void *)&c->value_bv : (void *)c->argv[1] );
}

/* FIXME: this ought to be provided by libldap */
static int
config_tls_config( ConfigArgs *c )
{
    int i, flag;

    switch ( c->type ) {
        case CFG_TLS_CRLCHECK:
            flag = LDAP_OPT_X_TLS_CRLCHECK;
            break;
        case CFG_TLS_VERIFY:
            flag = LDAP_OPT_X_TLS_REQUIRE_CERT;
            break;
        case CFG_TLS_PROTOCOL_MIN:
            flag = LDAP_OPT_X_TLS_PROTOCOL_MIN;
            break;
        default:
            Debug( LDAP_DEBUG_ANY, "%s: "
                    "unknown tls_option <0x%x>\n",
                    c->log, c->type );
            return 1;
    }
    if ( c->op == SLAP_CONFIG_EMIT ) {
        return lload_tls_get_config( lload_tls_ld, flag, &c->value_string );
    }

    lload_change.type = LLOAD_CHANGE_MODIFY;
    lload_change.object = LLOAD_DAEMON;
    lload_change.flags.daemon |= LLOAD_DAEMON_MOD_TLS;

    config_push_cleanup( c, config_tls_cleanup );
    if ( c->op == LDAP_MOD_DELETE ) {
        int i = 0;
        return ldap_pvt_tls_set_option( lload_tls_ld, flag, &i );
    }
    ch_free( c->value_string );
    if ( isdigit( (unsigned char)c->argv[1][0] ) &&
            c->type != CFG_TLS_PROTOCOL_MIN ) {
        if ( lutil_atoi( &i, c->argv[1] ) != 0 ) {
            Debug( LDAP_DEBUG_ANY, "%s: "
                    "unable to parse %s \"%s\"\n",
                    c->log, c->argv[0], c->argv[1] );
            return 1;
        }
        return ldap_pvt_tls_set_option( lload_tls_ld, flag, &i );
    } else {
        return ldap_pvt_tls_config( lload_tls_ld, flag, c->argv[1] );
    }
}
#endif

#ifdef BALANCER_MODULE
static int
config_share_tls_ctx( ConfigArgs *c )
{
    int rc = LDAP_SUCCESS;

    if ( c->op == SLAP_CONFIG_EMIT ) {
        c->value_int = lload_use_slap_tls_ctx;
        return rc;
    }

    lload_change.type = LLOAD_CHANGE_MODIFY;
    lload_change.object = LLOAD_DAEMON;
    lload_change.flags.daemon |= LLOAD_DAEMON_MOD_TLS;

    if ( c->op == LDAP_MOD_DELETE ) {
        lload_use_slap_tls_ctx = 0;
        return rc;
    }

    lload_use_slap_tls_ctx = c->value_int;
    return rc;
}
#endif /* BALANCER_MODULE */

void
lload_init_config_argv( ConfigArgs *c )
{
    c->argv = ch_calloc( ARGS_STEP + 1, sizeof(*c->argv) );
    c->argv_size = ARGS_STEP + 1;
}

ConfigTable *
lload_config_find_keyword( ConfigTable *Conf, ConfigArgs *c )
{
    int i;

    for ( i = 0; Conf[i].name; i++ )
        if ( ( Conf[i].length &&
                     ( !strncasecmp(
                             c->argv[0], Conf[i].name, Conf[i].length ) ) ) ||
                ( !strcasecmp( c->argv[0], Conf[i].name ) ) )
            break;
    if ( !Conf[i].name ) return NULL;
    if ( (Conf[i].arg_type & ARGS_TYPES) == ARG_BINARY ) {
        size_t decode_len = LUTIL_BASE64_DECODE_LEN( c->linelen );
        ch_free( c->tline );
        c->tline = ch_malloc( decode_len + 1 );
        c->linelen = lutil_b64_pton( c->line, c->tline, decode_len );
        if ( c->linelen < 0 ) {
            ch_free( c->tline );
            c->tline = NULL;
            return NULL;
        }
        c->line = c->tline;
    }
    return Conf + i;
}

int
lload_config_check_vals( ConfigTable *Conf, ConfigArgs *c, int check_only )
{
    int arg_user, arg_type, arg_syn, iarg;
    unsigned uiarg;
    long larg;
    unsigned long ularg;
    ber_len_t barg;

    if ( Conf->arg_type == ARG_IGNORED ) {
        Debug( LDAP_DEBUG_CONFIG, "%s: keyword <%s> ignored\n",
                c->log, Conf->name );
        return 0;
    }
    arg_type = Conf->arg_type & ARGS_TYPES;
    arg_user = Conf->arg_type & ARGS_USERLAND;
    arg_syn = Conf->arg_type & ARGS_SYNTAX;

    if ( Conf->min_args && ( c->argc < Conf->min_args ) ) {
        snprintf( c->cr_msg, sizeof(c->cr_msg), "<%s> missing <%s> argument",
                c->argv[0], Conf->what ? Conf->what : "" );
        Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: keyword %s\n",
                c->log, c->cr_msg );
        return ARG_BAD_CONF;
    }
    if ( Conf->max_args && ( c->argc > Conf->max_args ) ) {
        char *ignored = " ignored";

        snprintf( c->cr_msg, sizeof(c->cr_msg), "<%s> extra cruft after <%s>",
                c->argv[0], Conf->what );

        ignored = "";
        Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s%s\n",
                c->log, c->cr_msg, ignored );
        return ARG_BAD_CONF;
    }
    if ( (arg_syn & ARG_PAREN) && *c->argv[1] != '(' /*')'*/ ) {
        snprintf( c->cr_msg, sizeof(c->cr_msg), "<%s> old format not supported",
                c->argv[0] );
        Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
                c->log, c->cr_msg );
        return ARG_BAD_CONF;
    }
    if ( arg_type && !Conf->arg_item && !(arg_syn & ARG_OFFSET) ) {
        snprintf( c->cr_msg, sizeof(c->cr_msg),
                "<%s> invalid config_table, arg_item is NULL",
                c->argv[0] );
        Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
                c->log, c->cr_msg );
        return ARG_BAD_CONF;
    }
    c->type = arg_user;
    memset( &c->values, 0, sizeof(c->values) );
    if ( arg_type == ARG_STRING ) {
        assert( c->argc == 2 );
        if ( !check_only ) c->value_string = ch_strdup( c->argv[1] );
    } else if ( arg_type == ARG_BERVAL ) {
        assert( c->argc == 2 );
        if ( !check_only ) ber_str2bv( c->argv[1], 0, 1, &c->value_bv );
    } else if ( arg_type == ARG_BINARY ) {
        assert( c->argc == 2 );
        if ( !check_only ) {
            c->value_bv.bv_len = c->linelen;
            c->value_bv.bv_val = ch_malloc( c->linelen );
            AC_MEMCPY( c->value_bv.bv_val, c->line, c->linelen );
        }
    } else { /* all numeric */
        int j;
        iarg = 0;
        larg = 0;
        barg = 0;
        switch ( arg_type ) {
            case ARG_INT:
                assert( c->argc == 2 );
                if ( lutil_atoix( &iarg, c->argv[1], 0 ) != 0 ) {
                    snprintf( c->cr_msg, sizeof(c->cr_msg),
                            "<%s> unable to parse \"%s\" as int",
                            c->argv[0], c->argv[1] );
                    Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
                            c->log, c->cr_msg );
                    return ARG_BAD_CONF;
                }
                break;
            case ARG_UINT:
                assert( c->argc == 2 );
                if ( lutil_atoux( &uiarg, c->argv[1], 0 ) != 0 ) {
                    snprintf( c->cr_msg, sizeof(c->cr_msg),
                            "<%s> unable to parse \"%s\" as unsigned int",
                            c->argv[0], c->argv[1] );
                    Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
                            c->log, c->cr_msg );
                    return ARG_BAD_CONF;
                }
                break;
            case ARG_LONG:
                assert( c->argc == 2 );
                if ( lutil_atolx( &larg, c->argv[1], 0 ) != 0 ) {
                    snprintf( c->cr_msg, sizeof(c->cr_msg),
                            "<%s> unable to parse \"%s\" as long",
                            c->argv[0], c->argv[1] );
                    Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
                            c->log, c->cr_msg );
                    return ARG_BAD_CONF;
                }
                break;
            case ARG_ULONG:
                assert( c->argc == 2 );
                if ( LUTIL_ATOULX( &ularg, c->argv[1], 0 ) != 0 ) {
                    snprintf( c->cr_msg, sizeof(c->cr_msg),
                            "<%s> unable to parse \"%s\" as unsigned long",
                            c->argv[0], c->argv[1] );
                    Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
                            c->log, c->cr_msg );
                    return ARG_BAD_CONF;
                }
                break;
            case ARG_BER_LEN_T: {
                unsigned long l;
                assert( c->argc == 2 );
                if ( lutil_atoulx( &l, c->argv[1], 0 ) != 0 ) {
                    snprintf( c->cr_msg, sizeof(c->cr_msg),
                            "<%s> unable to parse \"%s\" as ber_len_t",
                            c->argv[0], c->argv[1] );
                    Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE, "%s: %s\n",
                            c->log, c->cr_msg );
                    return ARG_BAD_CONF;
                }
                barg = (ber_len_t)l;
            } break;
            case ARG_ON_OFF:
                /* note: this is an explicit exception
                 * to the "need exactly 2 args" rule */
                if ( c->argc == 1 ) {
                    iarg = 1;
                } else if ( !strcasecmp( c->argv[1], "on" ) ||
                        !strcasecmp( c->argv[1], "true" ) ||
                        !strcasecmp( c->argv[1], "yes" ) ) {
                    iarg = 1;
                } else if ( !strcasecmp( c->argv[1], "off" ) ||
                        !strcasecmp( c->argv[1], "false" ) ||
                        !strcasecmp( c->argv[1], "no" ) ) {
                    iarg = 0;
                } else {
                    snprintf( c->cr_msg, sizeof(c->cr_msg),
                            "<%s> invalid value",
                            c->argv[0] );
                    Debug( LDAP_DEBUG_ANY|LDAP_DEBUG_NONE, "%s: %s\n",
                            c->log, c->cr_msg );
                    return ARG_BAD_CONF;
                }
                break;
        }
        j = (arg_type & ARG_NONZERO) ? 1 : 0;
        if ( iarg < j && larg < j && barg < (unsigned)j ) {
            larg = larg ? larg : ( barg ? (long)barg : iarg );
            snprintf( c->cr_msg, sizeof(c->cr_msg), "<%s> invalid value",
                    c->argv[0] );
            Debug( LDAP_DEBUG_ANY|LDAP_DEBUG_NONE, "%s: %s\n",
                    c->log, c->cr_msg );
            return ARG_BAD_CONF;
        }
        switch ( arg_type ) {
            case ARG_ON_OFF:
            case ARG_INT:
                c->value_int = iarg;
                break;
            case ARG_UINT:
                c->value_uint = uiarg;
                break;
            case ARG_LONG:
                c->value_long = larg;
                break;
            case ARG_ULONG:
                c->value_ulong = ularg;
                break;
            case ARG_BER_LEN_T:
                c->value_ber_t = barg;
                break;
        }
    }
    return 0;
}

int
lload_config_set_vals( ConfigTable *Conf, ConfigArgs *c )
{
    int rc, arg_type;
    void *ptr = NULL;

    arg_type = Conf->arg_type;
    if ( arg_type & ARG_MAGIC ) {
        c->cr_msg[0] = '\0';
        rc = ( *( (ConfigDriver *)Conf->arg_item ) )( c );
        if ( rc ) {
            if ( !c->cr_msg[0] ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "<%s> handler exited with %d",
                        c->argv[0], rc );
                Debug( LDAP_DEBUG_CONFIG, "%s: %s!\n", c->log, c->cr_msg );
            }
            return ARG_BAD_CONF;
        }
        return 0;
    }
    if ( arg_type & ARG_OFFSET ) {
        {
            snprintf( c->cr_msg, sizeof(c->cr_msg),
                    "<%s> offset is missing base pointer",
                    c->argv[0] );
            Debug( LDAP_DEBUG_CONFIG, "%s: %s!\n", c->log, c->cr_msg );
            return ARG_BAD_CONF;
        }
        ptr = (void *)( (char *)ptr + (long)Conf->arg_item );
    } else if ( arg_type & ARGS_TYPES ) {
        ptr = Conf->arg_item;
    }
    if ( arg_type & ARGS_TYPES ) switch ( arg_type & ARGS_TYPES ) {
            case ARG_ON_OFF:
            case ARG_INT:
                *(int *)ptr = c->value_int;
                break;
            case ARG_UINT:
                *(unsigned *)ptr = c->value_uint;
                break;
            case ARG_LONG:
                *(long *)ptr = c->value_long;
                break;
            case ARG_ULONG:
                *(size_t *)ptr = c->value_ulong;
                break;
            case ARG_BER_LEN_T:
                *(ber_len_t *)ptr = c->value_ber_t;
                break;
            case ARG_STRING: {
                char *cc = *(char **)ptr;
                if ( cc ) {
                    if ( (arg_type & ARG_UNIQUE) &&
                            c->op == SLAP_CONFIG_ADD ) {
                        Debug( LDAP_DEBUG_CONFIG, "%s: already set %s!\n",
                                c->log, Conf->name );
                        return ARG_BAD_CONF;
                    }
                    ch_free( cc );
                }
                *(char **)ptr = c->value_string;
                break;
            }
            case ARG_BERVAL:
            case ARG_BINARY:
                *(struct berval *)ptr = c->value_bv;
                break;
        }
    return 0;
}

int
lload_config_add_vals( ConfigTable *Conf, ConfigArgs *c )
{
    int rc, arg_type;

    arg_type = Conf->arg_type;
    if ( arg_type == ARG_IGNORED ) {
        Debug( LDAP_DEBUG_CONFIG, "%s: keyword <%s> ignored\n",
                c->log, Conf->name );
        return 0;
    }
    rc = lload_config_check_vals( Conf, c, 0 );
    if ( rc ) return rc;
    return lload_config_set_vals( Conf, c );
}

int
lload_read_config_file(
        const char *fname,
        int depth,
        ConfigArgs *cf,
        ConfigTable *cft )
{
    FILE *fp;
    ConfigTable *ct;
    ConfigArgs *c;
    int rc;
    struct stat s;

    c = ch_calloc( 1, sizeof(ConfigArgs) );
    if ( c == NULL ) {
        return 1;
    }

    if ( depth ) {
        memcpy( c, cf, sizeof(ConfigArgs) );
    } else {
        c->depth = depth; /* XXX */
    }

    c->valx = -1;
    c->fname = fname;
    lload_init_config_argv( c );

    if ( stat( fname, &s ) != 0 ) {
        char ebuf[128];
        int saved_errno = errno;
        ldap_syslog = 1;
        Debug( LDAP_DEBUG_ANY, "could not stat config file \"%s\": %s (%d)\n",
                fname, AC_STRERROR_R( saved_errno, ebuf, sizeof(ebuf) ),
                saved_errno );
        ch_free( c->argv );
        ch_free( c );
        return 1;
    }

    if ( !S_ISREG(s.st_mode) ) {
        ldap_syslog = 1;
        Debug( LDAP_DEBUG_ANY, "regular file expected, got \"%s\"\n", fname );
        ch_free( c->argv );
        ch_free( c );
        return 1;
    }

    fp = fopen( fname, "r" );
    if ( fp == NULL ) {
        char ebuf[128];
        int saved_errno = errno;
        ldap_syslog = 1;
        Debug( LDAP_DEBUG_ANY, "could not open config file \"%s\": %s (%d)\n",
                fname, AC_STRERROR_R( saved_errno, ebuf, sizeof(ebuf) ),
                saved_errno );
        ch_free( c->argv );
        ch_free( c );
        return 1;
    }

    Debug( LDAP_DEBUG_CONFIG, "reading config file %s\n", fname );

    fp_getline_init( c );

    c->tline = NULL;

    while ( fp_getline( fp, c ) ) {
        /* skip comments and blank lines */
        if ( c->line[0] == '#' || c->line[0] == '\0' ) {
            continue;
        }

        snprintf( c->log, sizeof(c->log), "%s: line %d",
                c->fname, c->lineno );

        c->argc = 0;
        ch_free( c->tline );
        if ( lload_config_fp_parse_line( c ) ) {
            rc = 1;
            goto done;
        }

        if ( c->argc < 1 ) {
            Debug( LDAP_DEBUG_ANY, "%s: bad config line\n", c->log );
            rc = 1;
            goto done;
        }

        c->op = SLAP_CONFIG_ADD;

        ct = lload_config_find_keyword( cft, c );
        if ( ct ) {
            c->table = Cft_Global;
            rc = lload_config_add_vals( ct, c );
            if ( !rc ) continue;

            if ( rc & ARGS_USERLAND ) {
                /* XXX a usertype would be opaque here */
                Debug( LDAP_DEBUG_CONFIG, "%s: unknown user type <%s>\n",
                        c->log, c->argv[0] );
                rc = 1;
                goto done;

            } else if ( rc == ARG_BAD_CONF ) {
                rc = 1;
                goto done;
            }

        } else {
            Debug( LDAP_DEBUG_ANY, "%s: unknown directive "
                    "<%s> outside backend info and database definitions\n",
                    c->log, *c->argv );
            rc = 1;
            goto done;
        }
    }

    rc = 0;

done:
    ch_free( c->tline );
    fclose( fp );
    ch_free( c->argv );
    ch_free( c );
    return rc;
}

int
lload_read_config( const char *fname, const char *dir )
{
    if ( !fname ) fname = LLOADD_DEFAULT_CONFIGFILE;

    cfn = ch_calloc( 1, sizeof(ConfigFile) );

    return lload_read_config_file( fname, 0, NULL, config_back_cf_table );
}

/* restrictops, allows, disallows, requires, loglevel */

int
bverb_to_mask( struct berval *bword, slap_verbmasks *v )
{
    int i;
    for ( i = 0; !BER_BVISNULL( &v[i].word ); i++ ) {
        if ( !ber_bvstrcasecmp( bword, &v[i].word ) ) break;
    }
    return i;
}

int
verb_to_mask( const char *word, slap_verbmasks *v )
{
    struct berval bword;
    ber_str2bv( word, 0, 0, &bword );
    return bverb_to_mask( &bword, v );
}

int
verbs_to_mask( int argc, char *argv[], slap_verbmasks *v, slap_mask_t *m )
{
    int i, j;
    for ( i = 1; i < argc; i++ ) {
        j = verb_to_mask( argv[i], v );
        if ( BER_BVISNULL( &v[j].word ) ) return i;
        while ( !v[j].mask )
            j--;
        *m |= v[j].mask;
    }
    return 0;
}

/* Mask keywords that represent multiple bits should occur before single
 * bit keywords in the verbmasks array.
 */
int
mask_to_verbs( slap_verbmasks *v, slap_mask_t m, BerVarray *bva )
{
    int i, rc = 1;

    if ( m ) {
        for ( i = 0; !BER_BVISNULL( &v[i].word ); i++ ) {
            if ( !v[i].mask ) continue;
            if ( (m & v[i].mask) == v[i].mask ) {
                value_add_one( bva, &v[i].word );
                rc = 0;
                m ^= v[i].mask;
                if ( !m ) break;
            }
        }
    }
    return rc;
}

int
slap_verbmasks_init( slap_verbmasks **vp, slap_verbmasks *v )
{
    int i;

    assert( *vp == NULL );

    for ( i = 0; !BER_BVISNULL( &v[i].word ); i++ ) /* EMPTY */;

    *vp = ch_calloc( i + 1, sizeof(slap_verbmasks) );

    for ( i = 0; !BER_BVISNULL( &v[i].word ); i++ ) {
        ber_dupbv( &(*vp)[i].word, &v[i].word );
        *( (slap_mask_t *)&(*vp)[i].mask ) = v[i].mask;
    }

    BER_BVZERO( &(*vp)[i].word );

    return 0;
}

int
slap_verbmasks_destroy( slap_verbmasks *v )
{
    int i;

    assert( v != NULL );

    for ( i = 0; !BER_BVISNULL( &v[i].word ); i++ ) {
        ch_free( v[i].word.bv_val );
    }

    ch_free( v );

    return 0;
}

#ifndef BALANCER_MODULE
int
config_push_cleanup( ConfigArgs *ca, ConfigDriver *cleanup )
{
    /* Stub, cleanups only run in online config */
    return 0;
}
#endif /* !BALANCER_MODULE */

static slap_verbmasks tlskey[] = {
    { BER_BVC("no"), LLOAD_CLEARTEXT },
    { BER_BVC("yes"), LLOAD_STARTTLS_OPTIONAL },
    { BER_BVC("critical"), LLOAD_STARTTLS },
    { BER_BVNULL, 0 }
};

static slap_verbmasks crlkeys[] = {
    { BER_BVC("none"), LDAP_OPT_X_TLS_CRL_NONE },
    { BER_BVC("peer"), LDAP_OPT_X_TLS_CRL_PEER },
    { BER_BVC("all"), LDAP_OPT_X_TLS_CRL_ALL },
    { BER_BVNULL, 0 }
};

static slap_verbmasks vfykeys[] = {
    { BER_BVC("never"), LDAP_OPT_X_TLS_NEVER },
    { BER_BVC("allow"), LDAP_OPT_X_TLS_ALLOW },
    { BER_BVC("try"), LDAP_OPT_X_TLS_TRY },
    { BER_BVC("demand"), LDAP_OPT_X_TLS_DEMAND },
    { BER_BVC("hard"), LDAP_OPT_X_TLS_HARD },
    { BER_BVC("true"), LDAP_OPT_X_TLS_HARD },
    { BER_BVNULL, 0 }
};

static slap_verbmasks methkey[] = {
    { BER_BVC("none"), LDAP_AUTH_NONE },
    { BER_BVC("simple"), LDAP_AUTH_SIMPLE },
#ifdef HAVE_CYRUS_SASL
    { BER_BVC("sasl"), LDAP_AUTH_SASL },
#endif
    { BER_BVNULL, 0 }
};

int
lload_keepalive_parse(
        struct berval *val,
        void *bc,
        slap_cf_aux_table *tab0,
        const char *tabmsg,
        int unparse )
{
    if ( unparse ) {
        slap_keepalive *sk = (slap_keepalive *)bc;
        int rc = snprintf( val->bv_val, val->bv_len, "%d:%d:%d",
                sk->sk_idle, sk->sk_probes, sk->sk_interval );
        if ( rc < 0 ) {
            return -1;
        }

        if ( (unsigned)rc >= val->bv_len ) {
            return -1;
        }

        val->bv_len = rc;

    } else {
        char *s = val->bv_val;
        char *next;
        slap_keepalive *sk = (slap_keepalive *)bc;
        slap_keepalive sk2;

        if ( s[0] == ':' ) {
            sk2.sk_idle = 0;
            s++;

        } else {
            sk2.sk_idle = strtol( s, &next, 10 );
            if ( next == s || next[0] != ':' ) {
                return -1;
            }

            if ( sk2.sk_idle < 0 ) {
                return -1;
            }

            s = ++next;
        }

        if ( s[0] == ':' ) {
            sk2.sk_probes = 0;
            s++;

        } else {
            sk2.sk_probes = strtol( s, &next, 10 );
            if ( next == s || next[0] != ':' ) {
                return -1;
            }

            if ( sk2.sk_probes < 0 ) {
                return -1;
            }

            s = ++next;
        }

        if ( *s == '\0' ) {
            sk2.sk_interval = 0;

        } else {
            sk2.sk_interval = strtol( s, &next, 10 );
            if ( next == s || next[0] != '\0' ) {
                return -1;
            }

            if ( sk2.sk_interval < 0 ) {
                return -1;
            }
        }

        *sk = sk2;

        ber_memfree( val->bv_val );
        BER_BVZERO( val );
    }

    return 0;
}

static slap_cf_aux_table backendkey[] = {
    { BER_BVC("uri="), offsetof(LloadBackend, b_uri), 'b', 1, NULL },

    { BER_BVC("numconns="), offsetof(LloadBackend, b_numconns), 'i', 0, NULL },
    { BER_BVC("bindconns="), offsetof(LloadBackend, b_numbindconns), 'i', 0, NULL },
    { BER_BVC("retry="), offsetof(LloadBackend, b_retry_timeout), 'i', 0, NULL },

    { BER_BVC("max-pending-ops="), offsetof(LloadBackend, b_max_pending), 'i', 0, NULL },
    { BER_BVC("conn-max-pending="), offsetof(LloadBackend, b_max_conn_pending), 'i', 0, NULL },
    { BER_BVC("starttls="), offsetof(LloadBackend, b_tls_conf), 'i', 0, tlskey },
    { BER_BVNULL, 0, 0, 0, NULL }
};

static slap_cf_aux_table bindkey[] = {
    { BER_BVC("bindmethod="), offsetof(slap_bindconf, sb_method), 'i', 0, methkey },
    { BER_BVC("timeout="), offsetof(slap_bindconf, sb_timeout_api), 'i', 0, NULL },
    { BER_BVC("network-timeout="), offsetof(slap_bindconf, sb_timeout_net), 'i', 0, NULL },
    { BER_BVC("binddn="), offsetof(slap_bindconf, sb_binddn), 'b', 1, NULL },
    { BER_BVC("credentials="), offsetof(slap_bindconf, sb_cred), 'b', 1, NULL },
    { BER_BVC("saslmech="), offsetof(slap_bindconf, sb_saslmech), 'b', 0, NULL },
    { BER_BVC("secprops="), offsetof(slap_bindconf, sb_secprops), 's', 0, NULL },
    { BER_BVC("realm="), offsetof(slap_bindconf, sb_realm), 'b', 0, NULL },
    { BER_BVC("authcID="), offsetof(slap_bindconf, sb_authcId), 'b', 1, NULL },
    { BER_BVC("authzID="), offsetof(slap_bindconf, sb_authzId), 'b', 1, NULL },
    { BER_BVC("keepalive="), offsetof(slap_bindconf, sb_keepalive), 'x', 0, (slap_verbmasks *)lload_keepalive_parse },
    { BER_BVC("tcp-user-timeout="), offsetof(slap_bindconf, sb_tcp_user_timeout), 'u', 0, NULL },
#ifdef HAVE_TLS
    /* NOTE: replace "12" with the actual index
     * of the first TLS-related line */
#define aux_TLS (bindkey+12) /* beginning of TLS keywords */

    { BER_BVC("tls_cert="), offsetof(slap_bindconf, sb_tls_cert), 's', 1, NULL },
    { BER_BVC("tls_key="), offsetof(slap_bindconf, sb_tls_key), 's', 1, NULL },
    { BER_BVC("tls_cacert="), offsetof(slap_bindconf, sb_tls_cacert), 's', 1, NULL },
    { BER_BVC("tls_cacertdir="), offsetof(slap_bindconf, sb_tls_cacertdir), 's', 1, NULL },
    { BER_BVC("tls_reqcert="), offsetof(slap_bindconf, sb_tls_reqcert), 's', 0, NULL },
    { BER_BVC("tls_reqsan="), offsetof(slap_bindconf, sb_tls_reqsan), 's', 0, NULL },
    { BER_BVC("tls_cipher_suite="), offsetof(slap_bindconf, sb_tls_cipher_suite), 's', 0, NULL },
    { BER_BVC("tls_protocol_min="), offsetof(slap_bindconf, sb_tls_protocol_min), 's', 0, NULL },
    { BER_BVC("tls_ecname="), offsetof(slap_bindconf, sb_tls_ecname), 's', 0, NULL },
#ifdef HAVE_OPENSSL
    { BER_BVC("tls_crlcheck="), offsetof(slap_bindconf, sb_tls_crlcheck), 's', 0, NULL },
#endif
#endif
    { BER_BVNULL, 0, 0, 0, NULL }
};

/*
 * 's': char *
 * 'b': struct berval
 * 'i': int; if !NULL, compute using ((slap_verbmasks *)aux)
 * 'u': unsigned
 * 'I': long
 * 'U': unsigned long
 */

int
lload_cf_aux_table_parse(
        const char *word,
        void *dst,
        slap_cf_aux_table *tab0,
        LDAP_CONST char *tabmsg )
{
    int rc = SLAP_CONF_UNKNOWN;
    slap_cf_aux_table *tab;

    for ( tab = tab0; !BER_BVISNULL( &tab->key ); tab++ ) {
        if ( !strncasecmp( word, tab->key.bv_val, tab->key.bv_len ) ) {
            char **cptr;
            int *iptr, j;
            unsigned *uptr;
            long *lptr;
            unsigned long *ulptr;
            struct berval *bptr;
            const char *val = word + tab->key.bv_len;

            switch ( tab->type ) {
                case 's':
                    cptr = (char **)( (char *)dst + tab->off );
                    *cptr = ch_strdup( val );
                    rc = 0;
                    break;

                case 'b':
                    bptr = (struct berval *)( (char *)dst + tab->off );
                    assert( tab->aux == NULL );
                    ber_str2bv( val, 0, 1, bptr );
                    rc = 0;
                    break;

                case 'i':
                    iptr = (int *)( (char *)dst + tab->off );

                    if ( tab->aux != NULL ) {
                        slap_verbmasks *aux = (slap_verbmasks *)tab->aux;

                        assert( aux != NULL );

                        rc = 1;
                        for ( j = 0; !BER_BVISNULL( &aux[j].word ); j++ ) {
                            if ( !strcasecmp( val, aux[j].word.bv_val ) ) {
                                *iptr = aux[j].mask;
                                rc = 0;
                                break;
                            }
                        }

                    } else {
                        rc = lutil_atoix( iptr, val, 0 );
                    }
                    break;

                case 'u':
                    uptr = (unsigned *)( (char *)dst + tab->off );

                    rc = lutil_atoux( uptr, val, 0 );
                    break;

                case 'I':
                    lptr = (long *)( (char *)dst + tab->off );

                    rc = lutil_atolx( lptr, val, 0 );
                    break;

                case 'U':
                    ulptr = (unsigned long *)( (char *)dst + tab->off );

                    rc = lutil_atoulx( ulptr, val, 0 );
                    break;

                case 'x':
                    if ( tab->aux != NULL ) {
                        struct berval value;
                        lload_cf_aux_table_parse_x *func =
                                (lload_cf_aux_table_parse_x *)tab->aux;

                        ber_str2bv( val, 0, 1, &value );

                        rc = func( &value, (void *)( (char *)dst + tab->off ),
                                tab, tabmsg, 0 );

                    } else {
                        rc = 1;
                    }
                    break;
            }

            if ( rc ) {
                Debug( LDAP_DEBUG_ANY, "invalid %s value %s\n", tabmsg, word );
            }

            return rc;
        }
    }

    return rc;
}

int
lload_cf_aux_table_unparse(
        void *src,
        struct berval *bv,
        slap_cf_aux_table *tab0 )
{
    char buf[AC_LINE_MAX], *ptr;
    slap_cf_aux_table *tab;
    struct berval tmp;

    ptr = buf;
    for ( tab = tab0; !BER_BVISNULL( &tab->key ); tab++ ) {
        char **cptr;
        int *iptr, i;
        unsigned *uptr;
        long *lptr;
        unsigned long *ulptr;
        struct berval *bptr;

        cptr = (char **)( (char *)src + tab->off );

        switch ( tab->type ) {
            case 'b':
                bptr = (struct berval *)( (char *)src + tab->off );
                cptr = &bptr->bv_val;

            case 's':
                if ( *cptr ) {
                    *ptr++ = ' ';
                    ptr = lutil_strcopy( ptr, tab->key.bv_val );
                    if ( tab->quote ) *ptr++ = '"';
                    ptr = lutil_strcopy( ptr, *cptr );
                    if ( tab->quote ) *ptr++ = '"';
                }
                break;

            case 'i':
                iptr = (int *)( (char *)src + tab->off );

                if ( tab->aux != NULL ) {
                    slap_verbmasks *aux = (slap_verbmasks *)tab->aux;

                    for ( i = 0; !BER_BVISNULL( &aux[i].word ); i++ ) {
                        if ( *iptr == aux[i].mask ) {
                            *ptr++ = ' ';
                            ptr = lutil_strcopy( ptr, tab->key.bv_val );
                            ptr = lutil_strcopy( ptr, aux[i].word.bv_val );
                            break;
                        }
                    }

                } else {
                    *ptr++ = ' ';
                    ptr = lutil_strcopy( ptr, tab->key.bv_val );
                    ptr += snprintf( ptr, sizeof(buf) - ( ptr - buf ), "%d",
                            *iptr );
                }
                break;

            case 'u':
                uptr = (unsigned *)( (char *)src + tab->off );
                *ptr++ = ' ';
                ptr = lutil_strcopy( ptr, tab->key.bv_val );
                ptr += snprintf( ptr, sizeof(buf) - ( ptr - buf ), "%u",
                        *uptr );
                break;

            case 'I':
                lptr = (long *)( (char *)src + tab->off );
                *ptr++ = ' ';
                ptr = lutil_strcopy( ptr, tab->key.bv_val );
                ptr += snprintf( ptr, sizeof(buf) - ( ptr - buf ), "%ld",
                        *lptr );
                break;

            case 'U':
                ulptr = (unsigned long *)( (char *)src + tab->off );
                *ptr++ = ' ';
                ptr = lutil_strcopy( ptr, tab->key.bv_val );
                ptr += snprintf( ptr, sizeof(buf) - ( ptr - buf ), "%lu",
                        *ulptr );
                break;

            case 'x': {
                char *saveptr = ptr;
                *ptr++ = ' ';
                ptr = lutil_strcopy( ptr, tab->key.bv_val );
                if ( tab->quote ) *ptr++ = '"';
                if ( tab->aux != NULL ) {
                    struct berval value;
                    lload_cf_aux_table_parse_x *func =
                            (lload_cf_aux_table_parse_x *)tab->aux;
                    int rc;

                    value.bv_val = ptr;
                    value.bv_len = buf + sizeof(buf) - ptr;

                    rc = func( &value, (void *)( (char *)src + tab->off ), tab,
                            "(unparse)", 1 );
                    if ( rc == 0 ) {
                        if ( value.bv_len ) {
                            ptr += value.bv_len;
                        } else {
                            ptr = saveptr;
                            break;
                        }
                    }
                }
                if ( tab->quote ) *ptr++ = '"';
            } break;

            default:
                assert(0);
        }
    }
    tmp.bv_val = buf;
    tmp.bv_len = ptr - buf;
    ber_dupbv( bv, &tmp );
    return 0;
}

int
lload_tls_get_config( LDAP *ld, int opt, char **val )
{
#ifdef HAVE_TLS
    slap_verbmasks *keys;
    int i, ival;

    *val = NULL;
    switch ( opt ) {
        case LDAP_OPT_X_TLS_CRLCHECK:
            keys = crlkeys;
            break;
        case LDAP_OPT_X_TLS_REQUIRE_CERT:
            keys = vfykeys;
            break;
        case LDAP_OPT_X_TLS_PROTOCOL_MIN: {
            char buf[8];
            ldap_pvt_tls_get_option( ld, opt, &ival );
            snprintf( buf, sizeof(buf), "%d.%d",
                    ( ival >> 8 ) & 0xff, ival & 0xff );
            *val = ch_strdup( buf );
            return 0;
        }
        default:
            return -1;
    }
    ldap_pvt_tls_get_option( ld, opt, &ival );
    for ( i = 0; !BER_BVISNULL( &keys[i].word ); i++ ) {
        if ( keys[i].mask == ival ) {
            *val = ch_strdup( keys[i].word.bv_val );
            return 0;
        }
    }
#endif
    return -1;
}

#ifdef HAVE_TLS
static struct {
    const char *key;
    size_t offset;
    int opt;
} bindtlsopts[] = {
    { "tls_cert", offsetof(slap_bindconf, sb_tls_cert), LDAP_OPT_X_TLS_CERTFILE },
    { "tls_key", offsetof(slap_bindconf, sb_tls_key), LDAP_OPT_X_TLS_KEYFILE },
    { "tls_cacert", offsetof(slap_bindconf, sb_tls_cacert), LDAP_OPT_X_TLS_CACERTFILE },
    { "tls_cacertdir", offsetof(slap_bindconf, sb_tls_cacertdir), LDAP_OPT_X_TLS_CACERTDIR },
    { "tls_cipher_suite", offsetof(slap_bindconf, sb_tls_cipher_suite), LDAP_OPT_X_TLS_CIPHER_SUITE },
    { "tls_ecname", offsetof(slap_bindconf, sb_tls_ecname), LDAP_OPT_X_TLS_ECNAME },
    { NULL, 0 }
};

int
lload_bindconf_tls_set( slap_bindconf *bc, LDAP *ld )
{
    int i, rc, newctx = 0, res = 0;
    char *ptr = (char *)bc, **word;

    if ( bc->sb_tls_do_init ) {
        for ( i = 0; bindtlsopts[i].opt; i++ ) {
            word = (char **)( ptr + bindtlsopts[i].offset );
            if ( *word ) {
                rc = ldap_set_option( ld, bindtlsopts[i].opt, *word );
                if ( rc ) {
                    Debug( LDAP_DEBUG_ANY, "lload_bindconf_tls_set: "
                            "failed to set %s to %s\n",
                            bindtlsopts[i].key, *word );
                    res = -1;
                } else
                    newctx = 1;
            }
        }
        if ( bc->sb_tls_reqcert ) {
            rc = ldap_pvt_tls_config(
                    ld, LDAP_OPT_X_TLS_REQUIRE_CERT, bc->sb_tls_reqcert );
            if ( rc ) {
                Debug( LDAP_DEBUG_ANY, "lload_bindconf_tls_set: "
                        "failed to set tls_reqcert to %s\n",
                        bc->sb_tls_reqcert );
                res = -1;
            } else {
                newctx = 1;
                /* retrieve the parsed setting for later use */
                ldap_get_option( ld, LDAP_OPT_X_TLS_REQUIRE_CERT,
                        &bc->sb_tls_int_reqcert );
            }
        }
        if ( bc->sb_tls_reqsan ) {
            rc = ldap_pvt_tls_config(
                    ld, LDAP_OPT_X_TLS_REQUIRE_SAN, bc->sb_tls_reqsan );
            if ( rc ) {
                Debug( LDAP_DEBUG_ANY, "lload_bindconf_tls_set: "
                        "failed to set tls_reqsan to %s\n",
                        bc->sb_tls_reqsan );
                res = -1;
            } else {
                newctx = 1;
                /* retrieve the parsed setting for later use */
                ldap_get_option( ld, LDAP_OPT_X_TLS_REQUIRE_SAN,
                        &bc->sb_tls_int_reqsan );
            }
        }
        if ( bc->sb_tls_protocol_min ) {
            rc = ldap_pvt_tls_config(
                    ld, LDAP_OPT_X_TLS_PROTOCOL_MIN, bc->sb_tls_protocol_min );
            if ( rc ) {
                Debug( LDAP_DEBUG_ANY, "lload_bindconf_tls_set: "
                        "failed to set tls_protocol_min to %s\n",
                        bc->sb_tls_protocol_min );
                res = -1;
            } else
                newctx = 1;
        }
#ifdef HAVE_OPENSSL
        if ( bc->sb_tls_crlcheck ) {
            rc = ldap_pvt_tls_config(
                    ld, LDAP_OPT_X_TLS_CRLCHECK, bc->sb_tls_crlcheck );
            if ( rc ) {
                Debug( LDAP_DEBUG_ANY, "lload_bindconf_tls_set: "
                        "failed to set tls_crlcheck to %s\n",
                        bc->sb_tls_crlcheck );
                res = -1;
            } else
                newctx = 1;
        }
#endif
        if ( !res ) bc->sb_tls_do_init = 0;
    }

    if ( newctx ) {
        int opt = 0;

        if ( bc->sb_tls_ctx ) {
            ldap_pvt_tls_ctx_free( bc->sb_tls_ctx );
            bc->sb_tls_ctx = NULL;
        }
        rc = ldap_set_option( ld, LDAP_OPT_X_TLS_NEWCTX, &opt );
        if ( rc )
            res = rc;
        else
            ldap_get_option( ld, LDAP_OPT_X_TLS_CTX, &bc->sb_tls_ctx );
    } else if ( bc->sb_tls_ctx ) {
        rc = ldap_set_option( ld, LDAP_OPT_X_TLS_CTX, bc->sb_tls_ctx );
        if ( rc == LDAP_SUCCESS ) {
            /* these options aren't actually inside the ctx, so have to be set again */
            ldap_set_option(
                    ld, LDAP_OPT_X_TLS_REQUIRE_CERT, &bc->sb_tls_int_reqcert );
            ldap_set_option(
                    ld, LDAP_OPT_X_TLS_REQUIRE_SAN, &bc->sb_tls_int_reqsan );
        } else
            res = rc;
    }

    return res;
}
#endif

int
lload_bindconf_tls_parse( const char *word, slap_bindconf *bc )
{
#ifdef HAVE_TLS
    if ( lload_cf_aux_table_parse( word, bc, aux_TLS, "tls config" ) == 0 ) {
        bc->sb_tls_do_init = 1;
        return 0;
    }
#endif
    return -1;
}

int
lload_backend_parse( const char *word, LloadBackend *b )
{
    return lload_cf_aux_table_parse( word, b, backendkey, "backend config" );
}

int
lload_bindconf_parse( const char *word, slap_bindconf *bc )
{
#ifdef HAVE_TLS
    /* Detect TLS config changes explicitly */
    if ( lload_bindconf_tls_parse( word, bc ) == 0 ) {
        return 0;
    }
#endif
    return lload_cf_aux_table_parse( word, bc, bindkey, "bind config" );
}

int
lload_bindconf_unparse( slap_bindconf *bc, struct berval *bv )
{
    return lload_cf_aux_table_unparse( bc, bv, bindkey );
}

void
lload_bindconf_free( slap_bindconf *bc )
{
    if ( !BER_BVISNULL( &bc->sb_uri ) ) {
        ch_free( bc->sb_uri.bv_val );
        BER_BVZERO( &bc->sb_uri );
    }
    if ( !BER_BVISNULL( &bc->sb_binddn ) ) {
        ch_free( bc->sb_binddn.bv_val );
        BER_BVZERO( &bc->sb_binddn );
    }
    if ( !BER_BVISNULL( &bc->sb_cred ) ) {
        ch_free( bc->sb_cred.bv_val );
        BER_BVZERO( &bc->sb_cred );
    }
    if ( !BER_BVISNULL( &bc->sb_saslmech ) ) {
        ch_free( bc->sb_saslmech.bv_val );
        BER_BVZERO( &bc->sb_saslmech );
    }
    if ( bc->sb_secprops ) {
        ch_free( bc->sb_secprops );
        bc->sb_secprops = NULL;
    }
    if ( !BER_BVISNULL( &bc->sb_realm ) ) {
        ch_free( bc->sb_realm.bv_val );
        BER_BVZERO( &bc->sb_realm );
    }
    if ( !BER_BVISNULL( &bc->sb_authcId ) ) {
        ch_free( bc->sb_authcId.bv_val );
        BER_BVZERO( &bc->sb_authcId );
    }
    if ( !BER_BVISNULL( &bc->sb_authzId ) ) {
        ch_free( bc->sb_authzId.bv_val );
        BER_BVZERO( &bc->sb_authzId );
    }
#ifdef HAVE_TLS
    if ( bc->sb_tls_cert ) {
        ch_free( bc->sb_tls_cert );
        bc->sb_tls_cert = NULL;
    }
    if ( bc->sb_tls_key ) {
        ch_free( bc->sb_tls_key );
        bc->sb_tls_key = NULL;
    }
    if ( bc->sb_tls_cacert ) {
        ch_free( bc->sb_tls_cacert );
        bc->sb_tls_cacert = NULL;
    }
    if ( bc->sb_tls_cacertdir ) {
        ch_free( bc->sb_tls_cacertdir );
        bc->sb_tls_cacertdir = NULL;
    }
    if ( bc->sb_tls_reqcert ) {
        ch_free( bc->sb_tls_reqcert );
        bc->sb_tls_reqcert = NULL;
    }
    if ( bc->sb_tls_cipher_suite ) {
        ch_free( bc->sb_tls_cipher_suite );
        bc->sb_tls_cipher_suite = NULL;
    }
    if ( bc->sb_tls_protocol_min ) {
        ch_free( bc->sb_tls_protocol_min );
        bc->sb_tls_protocol_min = NULL;
    }
#ifdef HAVE_OPENSSL_CRL
    if ( bc->sb_tls_crlcheck ) {
        ch_free( bc->sb_tls_crlcheck );
        bc->sb_tls_crlcheck = NULL;
    }
#endif
    if ( bc->sb_tls_ctx ) {
        ldap_pvt_tls_ctx_free( bc->sb_tls_ctx );
        bc->sb_tls_ctx = NULL;
    }
#endif
}

void
lload_bindconf_tls_defaults( slap_bindconf *bc )
{
#ifdef HAVE_TLS
    if ( bc->sb_tls_do_init ) {
        if ( !bc->sb_tls_cacert )
            ldap_pvt_tls_get_option( lload_tls_ld, LDAP_OPT_X_TLS_CACERTFILE,
                    &bc->sb_tls_cacert );
        if ( !bc->sb_tls_cacertdir )
            ldap_pvt_tls_get_option( lload_tls_ld, LDAP_OPT_X_TLS_CACERTDIR,
                    &bc->sb_tls_cacertdir );
        if ( !bc->sb_tls_cert )
            ldap_pvt_tls_get_option(
                    lload_tls_ld, LDAP_OPT_X_TLS_CERTFILE, &bc->sb_tls_cert );
        if ( !bc->sb_tls_key )
            ldap_pvt_tls_get_option(
                    lload_tls_ld, LDAP_OPT_X_TLS_KEYFILE, &bc->sb_tls_key );
        if ( !bc->sb_tls_cipher_suite )
            ldap_pvt_tls_get_option( lload_tls_ld, LDAP_OPT_X_TLS_CIPHER_SUITE,
                    &bc->sb_tls_cipher_suite );
        if ( !bc->sb_tls_reqcert ) bc->sb_tls_reqcert = ch_strdup( "demand" );
#ifdef HAVE_OPENSSL_CRL
        if ( !bc->sb_tls_crlcheck )
            lload_tls_get_config( lload_tls_ld, LDAP_OPT_X_TLS_CRLCHECK,
                    &bc->sb_tls_crlcheck );
#endif
    }
#endif
}

/* -------------------------------------- */

static char *
strtok_quote( char *line, char *sep, char **quote_ptr, int *iqp )
{
    int inquote;
    char *tmp;
    static char *next;

    *quote_ptr = NULL;
    if ( line != NULL ) {
        next = line;
    }
    while ( *next && strchr( sep, *next ) ) {
        next++;
    }

    if ( *next == '\0' ) {
        next = NULL;
        return NULL;
    }
    tmp = next;

    for ( inquote = 0; *next; ) {
        switch ( *next ) {
            case '"':
                if ( inquote ) {
                    inquote = 0;
                } else {
                    inquote = 1;
                }
                AC_MEMCPY( next, next + 1, strlen( next + 1 ) + 1 );
                break;

            case '\\':
                if ( next[1] )
                    AC_MEMCPY( next, next + 1, strlen( next + 1 ) + 1 );
                next++; /* dont parse the escaped character */
                break;

            default:
                if ( !inquote ) {
                    if ( strchr( sep, *next ) != NULL ) {
                        *quote_ptr = next;
                        *next++ = '\0';
                        return tmp;
                    }
                }
                next++;
                break;
        }
    }
    *iqp = inquote;

    return tmp;
}

static char buf[AC_LINE_MAX];
static char *line;
static size_t lmax, lcur;

#define CATLINE( buf ) \
    do { \
        size_t len = strlen( buf ); \
        while ( lcur + len + 1 > lmax ) { \
            lmax += AC_LINE_MAX; \
            line = (char *)ch_realloc( line, lmax ); \
        } \
        strcpy( line + lcur, buf ); \
        lcur += len; \
    } while (0)

static void
fp_getline_init( ConfigArgs *c )
{
    c->lineno = -1;
    buf[0] = '\0';
}

static int
fp_getline( FILE *fp, ConfigArgs *c )
{
    char *p;

    lcur = 0;
    CATLINE( buf );
    c->lineno++;

    /* avoid stack of bufs */
    if ( strncasecmp( line, "include", STRLENOF("include") ) == 0 ) {
        buf[0] = '\0';
        c->line = line;
        return 1;
    }

    while ( fgets( buf, sizeof(buf), fp ) ) {
        p = strchr( buf, '\n' );
        if ( p ) {
            if ( p > buf && p[-1] == '\r' ) {
                --p;
            }
            *p = '\0';
        }
        /* XXX ugly */
        c->line = line;
        if ( line[0] && ( p = line + strlen( line ) - 1 )[0] == '\\' &&
                p[-1] != '\\' ) {
            p[0] = '\0';
            lcur--;

        } else {
            if ( !isspace( (unsigned char)buf[0] ) ) {
                return 1;
            }
            buf[0] = ' ';
        }
        CATLINE( buf );
        c->lineno++;
    }

    buf[0] = '\0';
    c->line = line;
    return ( line[0] ? 1 : 0 );
}

int
lload_config_fp_parse_line( ConfigArgs *c )
{
    char *token;
    static char *const hide[] = { "bindconf", NULL };
    static char *const raw[] = { NULL };
    char *quote_ptr;
    int i = (int)( sizeof(hide) / sizeof(hide[0]) ) - 1;
    int inquote = 0;

    c->tline = ch_strdup( c->line );
    c->linelen = strlen( c->line );
    token = strtok_quote( c->tline, " \t", &quote_ptr, &inquote );

    if ( token )
        for ( i = 0; hide[i]; i++ )
            if ( !strcasecmp( token, hide[i] ) ) break;
    if ( quote_ptr ) *quote_ptr = ' ';
    Debug( LDAP_DEBUG_CONFIG, "%s (%s%s)\n",
            c->log, hide[i] ? hide[i] : c->line, hide[i] ? " ***" : "" );
    if ( quote_ptr ) *quote_ptr = '\0';

    for ( ;; token = strtok_quote( NULL, " \t", &quote_ptr, &inquote ) ) {
        if ( c->argc >= c->argv_size ) {
            char **tmp;
            tmp = ch_realloc( c->argv,
                    ( c->argv_size + ARGS_STEP ) * sizeof(*c->argv) );
            if ( !tmp ) {
                Debug( LDAP_DEBUG_ANY, "%s: out of memory\n", c->log );
                return -1;
            }
            c->argv = tmp;
            c->argv_size += ARGS_STEP;
        }
        if ( token == NULL ) break;
        c->argv[c->argc++] = token;
    }
    c->argv[c->argc] = NULL;
    if ( inquote ) {
        /* these directives parse c->line independently of argv tokenizing */
        for ( i = 0; raw[i]; i++ )
            if ( !strcasecmp( c->argv[0], raw[i] ) ) return 0;

        Debug( LDAP_DEBUG_ANY, "%s: unterminated quoted string \"%s\"\n",
                c->log, c->argv[c->argc - 1] );
        return -1;
    }
    return 0;
}

void
lload_config_destroy( void )
{
    free( line );
    if ( slapd_args_file ) free( slapd_args_file );
    if ( slapd_pid_file ) free( slapd_pid_file );
    loglevel_destroy();
}

/* See if the given URL (in plain and parsed form) matches
 * any of the server's listener addresses. Return matching
 * LloadListener or NULL for no match.
 */
LloadListener *
lload_config_check_my_url( const char *url, LDAPURLDesc *lud )
{
    LloadListener **l = lloadd_get_listeners();
    int i, isMe;

    /* Try a straight compare with LloadListener strings */
    for ( i = 0; l && l[i]; i++ ) {
        if ( !strcasecmp( url, l[i]->sl_url.bv_val ) ) {
            return l[i];
        }
    }

    isMe = 0;
    /* If hostname is empty, or is localhost, or matches
     * our hostname, this url refers to this host.
     * Compare it against listeners and ports.
     */
    if ( !lud->lud_host || !lud->lud_host[0] ||
            !strncasecmp(
                    "localhost", lud->lud_host, STRLENOF("localhost") ) ||
            !strcasecmp( global_host, lud->lud_host ) ) {
        for ( i = 0; l && l[i]; i++ ) {
            LDAPURLDesc *lu2;
            ldap_url_parse_ext(
                    l[i]->sl_url.bv_val, &lu2, LDAP_PVT_URL_PARSE_DEF_PORT );
            do {
                if ( strcasecmp( lud->lud_scheme, lu2->lud_scheme ) ) break;
                if ( lud->lud_port != lu2->lud_port ) break;
                /* Listener on ANY address */
                if ( !lu2->lud_host || !lu2->lud_host[0] ) {
                    isMe = 1;
                    break;
                }
                /* URL on ANY address */
                if ( !lud->lud_host || !lud->lud_host[0] ) {
                    isMe = 1;
                    break;
                }
                /* Listener has specific host, must
                 * match it
                 */
                if ( !strcasecmp( lud->lud_host, lu2->lud_host ) ) {
                    isMe = 1;
                    break;
                }
            } while (0);
            ldap_free_urldesc( lu2 );
            if ( isMe ) {
                return l[i];
            }
        }
    }
    return NULL;
}

#ifdef BALANCER_MODULE
static int
backend_cf_gen( ConfigArgs *c )
{
    LloadBackend *b = c->ca_private;
    enum lcf_backend flag = 0;
    int rc = LDAP_SUCCESS;

    assert( b != NULL );

    if ( c->op == SLAP_CONFIG_EMIT ) {
        switch ( c->type ) {
            case CFG_URI:
                c->value_bv = b->b_uri;
                break;
            case CFG_NUMCONNS:
                c->value_uint = b->b_numconns;
                break;
            case CFG_BINDCONNS:
                c->value_uint = b->b_numbindconns;
                break;
            case CFG_RETRY:
                c->value_uint = b->b_retry_timeout;
                break;
            case CFG_MAX_PENDING_CONNS:
                c->value_uint = b->b_max_conn_pending;
                break;
            case CFG_MAX_PENDING_OPS:
                c->value_uint = b->b_max_pending;
                break;
            case CFG_STARTTLS:
                enum_to_verb( tlskey, b->b_tls_conf, &c->value_bv );
                break;
            default:
                rc = 1;
                break;
        }

        return rc;
    } else if ( c->op == LDAP_MOD_DELETE ) {
        /* We only need to worry about deletions to multi-value or MAY
         * attributes */
        switch ( c->type ) {
            case CFG_STARTTLS:
                b->b_tls_conf = LLOAD_CLEARTEXT;
                break;
            default:
                break;
        }
        return rc;
    }

    switch ( c->type ) {
        case CFG_URI:
            rc = backend_config_url( b, &c->value_bv );
            if ( rc ) {
                backend_config_url( b, &b->b_uri );
                goto fail;
            }
            if ( !BER_BVISNULL( &b->b_uri ) ) {
                ch_free( b->b_uri.bv_val );
            }
            b->b_uri = c->value_bv;
            flag = LLOAD_BACKEND_MOD_OTHER;
            break;
        case CFG_NUMCONNS:
            if ( !c->value_uint ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "invalid connection pool configuration" );
                goto fail;
            }
            b->b_numconns = c->value_uint;
            flag = LLOAD_BACKEND_MOD_CONNS;
            break;
        case CFG_BINDCONNS:
            if ( !c->value_uint ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "invalid connection pool configuration" );
                goto fail;
            }
            b->b_numbindconns = c->value_uint;
            flag = LLOAD_BACKEND_MOD_CONNS;
            break;
        case CFG_RETRY:
            b->b_retry_timeout = c->value_uint;
            break;
        case CFG_MAX_PENDING_CONNS:
            b->b_max_conn_pending = c->value_uint;
            break;
        case CFG_MAX_PENDING_OPS:
            b->b_max_pending = c->value_uint;
            break;
        case CFG_STARTTLS: {
            int i = bverb_to_mask( &c->value_bv, tlskey );
            if ( BER_BVISNULL( &tlskey[i].word ) ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "invalid starttls configuration" );
                goto fail;
            }
#ifndef HAVE_TLS
            if ( tlskey[i].mask == LLOAD_STARTTLS_OPTIONAL ) {
                Debug( LDAP_DEBUG_ANY, "%s: "
                        "lloadd compiled without TLS but starttls specified, "
                        "it will be ignored\n",
                        c->log );
            } else if ( tlskey[i].mask != LLOAD_CLEARTEXT ) {
                snprintf( c->cr_msg, sizeof(c->cr_msg),
                        "invalid starttls configuration when compiled without "
                        "TLS support" );
                goto fail;
            }
#endif /* ! HAVE_TLS */
            b->b_tls_conf = tlskey[i].mask;
        } break;
        default:
            rc = 1;
            break;
    }

    /* do not set this if it has already been set by another callback, e.g.
     * lload_backend_ldadd */
    if ( lload_change.type == LLOAD_CHANGE_UNDEFINED ) {
        lload_change.type = LLOAD_CHANGE_MODIFY;
    }
    lload_change.object = LLOAD_BACKEND;
    lload_change.target = b;
    lload_change.flags.backend |= flag;

    config_push_cleanup( c, lload_backend_finish );
    return rc;

fail:
    if ( lload_change.type == LLOAD_CHANGE_ADD ) {
        /* Abort the ADD */
        lload_change.type = LLOAD_CHANGE_DEL;
    }

    Debug( LDAP_DEBUG_ANY, "%s: %s\n", c->log, c->cr_msg );
    return 1;
}

int
lload_back_init_cf( BackendInfo *bi )
{
    /* Make sure we don't exceed the bits reserved for userland */
    config_check_userland( CFG_LAST );

    bi->bi_cf_ocs = lloadocs;

    return config_register_schema( config_back_cf_table, lloadocs );
}

static int
lload_backend_ldadd( CfEntryInfo *p, Entry *e, ConfigArgs *ca )
{
    LloadBackend *b;
    Attribute *a;
    AttributeDescription *ad = NULL;
    struct berval bv, type, rdn;
    const char *text;
    char *name;

    Debug( LDAP_DEBUG_TRACE, "lload_backend_ldadd: "
            "a new backend-server is being added\n" );

    if ( p->ce_type != Cft_Backend || !p->ce_bi ||
            p->ce_bi->bi_cf_ocs != lloadocs )
        return LDAP_CONSTRAINT_VIOLATION;

    dnRdn( &e->e_name, &rdn );
    type.bv_len = strchr( rdn.bv_val, '=' ) - rdn.bv_val;
    type.bv_val = rdn.bv_val;

    /* Find attr */
    slap_bv2ad( &type, &ad, &text );
    if ( ad != slap_schema.si_ad_cn ) return LDAP_NAMING_VIOLATION;

    a = attr_find( e->e_attrs, ad );
    if ( !a || a->a_numvals != 1 ) return LDAP_NAMING_VIOLATION;
    bv = a->a_vals[0];

    if ( bv.bv_val[0] == '{' && ( name = strchr( bv.bv_val, '}' ) ) ) {
        name++;
        bv.bv_len -= name - bv.bv_val;
        bv.bv_val = name;
    }

    b = backend_alloc();
    ber_dupbv( &b->b_name, &bv );

    ca->bi = p->ce_bi;
    ca->ca_private = b;
    config_push_cleanup( ca, lload_backend_finish );

    /* ca cleanups are only run in the case of online config but we use it to
     * save the new config when done with the entry */
    ca->lineno = 0;

    lload_change.type = LLOAD_CHANGE_ADD;
    lload_change.object = LLOAD_BACKEND;
    lload_change.target = b;

    return LDAP_SUCCESS;
}

#ifdef SLAP_CONFIG_DELETE
static int
lload_backend_lddel( CfEntryInfo *ce, Operation *op )
{
    LloadBackend *b = ce->ce_private;

    lload_change.type = LLOAD_CHANGE_DEL;
    lload_change.object = LLOAD_BACKEND;
    lload_change.target = b;

    return LDAP_SUCCESS;
}
#endif /* SLAP_CONFIG_DELETE */

static int
lload_cfadd( Operation *op, SlapReply *rs, Entry *p, ConfigArgs *c )
{
    struct berval bv;
    LloadBackend *b;
    int i = 0;

    bv.bv_val = c->cr_msg;
    LDAP_CIRCLEQ_FOREACH ( b, &backend, b_next ) {
        char buf[STRLENOF( "server 4294967295" ) + 1] = { 0 };

        bv.bv_len = snprintf( c->cr_msg, sizeof(c->cr_msg),
                "cn=" SLAP_X_ORDERED_FMT "server %d", i, i + 1 );

        snprintf( buf, sizeof(buf), "server %d", i + 1 );
        ber_str2bv( buf, 0, 1, &b->b_name );

        c->ca_private = b;
        c->valx = i;

        config_build_entry( op, rs, p->e_private, c, &bv, &lloadocs[1], NULL );

        i++;
    }
    return LDAP_SUCCESS;
}
#endif /* BALANCER_MODULE */
