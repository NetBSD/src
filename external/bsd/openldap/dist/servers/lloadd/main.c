/*	$NetBSD: main.c,v 1.2 2021/08/14 16:14:58 christos Exp $	*/

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
__RCSID("$NetBSD: main.c,v 1.2 2021/08/14 16:14:58 christos Exp $");

#include "portable.h"

#include <stdio.h>

#include <ac/ctype.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>
#include <ac/unistd.h>
#include <ac/wait.h>
#include <ac/errno.h>

#include <event2/event.h>

#include "lload.h"
#include "lutil.h"
#include "ldif.h"

#ifdef LDAP_SIGCHLD
static void wait4child( evutil_socket_t sig, short what, void *arg );
#endif

#ifdef SIGPIPE
static void sigpipe( evutil_socket_t sig, short what, void *arg );
#endif

#ifdef HAVE_NT_SERVICE_MANAGER
#define MAIN_RETURN(x) return
static struct sockaddr_in bind_addr;

#define SERVICE_EXIT( e, n ) \
    do { \
        if ( is_NT_Service ) { \
            lutil_ServiceStatus.dwWin32ExitCode = (e); \
            lutil_ServiceStatus.dwServiceSpecificExitCode = (n); \
        } \
    } while (0)

#else
#define SERVICE_EXIT( e, n )
#define MAIN_RETURN(x) return (x)
#endif

struct signal_handler {
    int signal;
    event_callback_fn handler;
    struct event *event;
} signal_handlers[] = {
        { LDAP_SIGUSR2, lload_sig_shutdown },

#ifdef SIGPIPE
        { SIGPIPE, sigpipe },
#endif
#ifdef SIGHUP
        { SIGHUP, lload_sig_shutdown },
#endif
        { SIGINT, lload_sig_shutdown },
        { SIGTERM, lload_sig_shutdown },
#ifdef SIGTRAP
        { SIGTRAP, lload_sig_shutdown },
#endif
#ifdef LDAP_SIGCHLD
        { LDAP_SIGCHLD, wait4child },
#endif
#ifdef SIGBREAK
        /* SIGBREAK is generated when Ctrl-Break is pressed. */
        { SIGBREAK, lload_sig_shutdown },
#endif
        { 0, NULL }
};

/*
 * when more than one lloadd is running on one machine, each one might have
 * it's own LOCAL for syslogging and must have its own pid/args files
 */

#ifndef HAVE_MKVERSION
const char Versionstr[] = OPENLDAP_PACKAGE
        " " OPENLDAP_VERSION " LDAP Load Balancer Server (lloadd)";
#endif

#define CHECK_NONE 0x00
#define CHECK_CONFIG 0x01
#define CHECK_LOGLEVEL 0x02
static int check = CHECK_NONE;
static int version = 0;

static int
slapd_opt_slp( const char *val, void *arg )
{
#ifdef HAVE_SLP
    /* NULL is default */
    if ( val == NULL || *val == '(' || strcasecmp( val, "on" ) == 0 ) {
        slapd_register_slp = 1;
        slapd_slp_attrs = ( val != NULL && *val == '(' ) ? val : NULL;

    } else if ( strcasecmp( val, "off" ) == 0 ) {
        slapd_register_slp = 0;

        /* NOTE: add support for URL specification? */

    } else {
        fprintf( stderr, "unrecognized value \"%s\" for SLP option\n", val );
        return -1;
    }

    return 0;

#else
    fputs( "lloadd: SLP support is not available\n", stderr );
    return 0;
#endif
}

/*
 * Option helper structure:
 *
 * oh_nam   is left-hand part of <option>[=<value>]
 * oh_fnc   is handler function
 * oh_arg   is an optional arg to oh_fnc
 * oh_usage is the one-line usage string related to the option,
 *          which is assumed to start with <option>[=<value>]
 *
 * please leave valid options in the structure, and optionally #ifdef
 * their processing inside the helper, so that reasonable and helpful
 * error messages can be generated if a disabled option is requested.
 */
struct option_helper {
    struct berval oh_name;
    int (*oh_fnc)( const char *val, void *arg );
    void *oh_arg;
    const char *oh_usage;
} option_helpers[] = {
        { BER_BVC("slp"), slapd_opt_slp, NULL,
                "slp[={on|off|(attrs)}] enable/disable SLP using (attrs)" },
        { BER_BVNULL, 0, NULL, NULL }
};

#if defined(LDAP_DEBUG) && defined(LDAP_SYSLOG)
#ifdef LOG_LOCAL4
int
parse_syslog_user( const char *arg, int *syslogUser )
{
    static slap_verbmasks syslogUsers[] = {
            { BER_BVC("LOCAL0"), LOG_LOCAL0 },
            { BER_BVC("LOCAL1"), LOG_LOCAL1 },
            { BER_BVC("LOCAL2"), LOG_LOCAL2 },
            { BER_BVC("LOCAL3"), LOG_LOCAL3 },
            { BER_BVC("LOCAL4"), LOG_LOCAL4 },
            { BER_BVC("LOCAL5"), LOG_LOCAL5 },
            { BER_BVC("LOCAL6"), LOG_LOCAL6 },
            { BER_BVC("LOCAL7"), LOG_LOCAL7 },
#ifdef LOG_USER
            { BER_BVC("USER"), LOG_USER },
#endif /* LOG_USER */
#ifdef LOG_DAEMON
            { BER_BVC("DAEMON"), LOG_DAEMON },
#endif /* LOG_DAEMON */
            { BER_BVNULL, 0 }
};
    int i = verb_to_mask( arg, syslogUsers );

    if ( BER_BVISNULL( &syslogUsers[i].word ) ) {
        Debug( LDAP_DEBUG_ANY, "unrecognized syslog user \"%s\".\n", arg );
        return 1;
    }

    *syslogUser = syslogUsers[i].mask;

    return 0;
}
#endif /* LOG_LOCAL4 */

int
parse_syslog_level( const char *arg, int *levelp )
{
    static slap_verbmasks str2syslog_level[] = {
            { BER_BVC("EMERG"), LOG_EMERG },
            { BER_BVC("ALERT"), LOG_ALERT },
            { BER_BVC("CRIT"), LOG_CRIT },
            { BER_BVC("ERR"), LOG_ERR },
            { BER_BVC("WARNING"), LOG_WARNING },
            { BER_BVC("NOTICE"), LOG_NOTICE },
            { BER_BVC("INFO"), LOG_INFO },
            { BER_BVC("DEBUG"), LOG_DEBUG },
            { BER_BVNULL, 0 }
};
    int i = verb_to_mask( arg, str2syslog_level );
    if ( BER_BVISNULL( &str2syslog_level[i].word ) ) {
        Debug( LDAP_DEBUG_ANY, "unknown syslog level \"%s\".\n", arg );
        return 1;
    }

    *levelp = str2syslog_level[i].mask;

    return 0;
}
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

int
parse_debug_unknowns( char **unknowns, int *levelp )
{
    int i, level, rc = 0;

    for ( i = 0; unknowns[i] != NULL; i++ ) {
        level = 0;
        if ( str2loglevel( unknowns[i], &level ) ) {
            fprintf( stderr, "unrecognized log level \"%s\"\n", unknowns[i] );
            rc = 1;
        } else {
            *levelp |= level;
        }
    }
    return rc;
}

int
parse_debug_level( const char *arg, int *levelp, char ***unknowns )
{
    int level;

    if ( arg && arg[0] != '-' && !isdigit( (unsigned char)arg[0] ) ) {
        int i;
        char **levels;

        levels = ldap_str2charray( arg, "," );

        for ( i = 0; levels[i] != NULL; i++ ) {
            level = 0;

            if ( str2loglevel( levels[i], &level ) ) {
                /* remember this for later */
                ldap_charray_add( unknowns, levels[i] );
                fprintf( stderr, "unrecognized log level \"%s\" (deferred)\n",
                        levels[i] );
            } else {
                *levelp |= level;
            }
        }

        ldap_charray_free( levels );

    } else {
        int rc;

        if ( arg[0] == '-' ) {
            rc = lutil_atoix( &level, arg, 0 );
        } else {
            unsigned ulevel;

            rc = lutil_atoux( &ulevel, arg, 0 );
            level = (int)ulevel;
        }

        if ( rc ) {
            fprintf( stderr,
                    "unrecognized log level "
                    "\"%s\"\n",
                    arg );
            return 1;
        }

        if ( level == 0 ) {
            *levelp = 0;

        } else {
            *levelp |= level;
        }
    }

    return 0;
}

static void
usage( char *name )
{
    fprintf( stderr, "usage: %s options\n", name );
    fprintf( stderr,
            "\t-4\t\tIPv4 only\n"
            "\t-6\t\tIPv6 only\n"
            "\t-d level\tDebug level"
            "\n"
            "\t-f filename\tConfiguration file\n"
#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
            "\t-g group\tGroup (id or name) to run as\n"
#endif
            "\t-h URLs\t\tList of URLs to serve\n"
#ifdef SLAP_DEFAULT_SYSLOG_USER
            "\t-l facility\tSyslog facility (default: LOCAL4)\n"
#endif
            "\t-n serverName\tService name\n"
            "\t-o <opt>[=val] generic means to specify options" );
    if ( !BER_BVISNULL( &option_helpers[0].oh_name ) ) {
        int i;

        fprintf( stderr, "; supported options:\n" );
        for ( i = 0; !BER_BVISNULL( &option_helpers[i].oh_name ); i++ ) {
            fprintf( stderr, "\t\t%s\n", option_helpers[i].oh_usage );
        }
    } else {
        fprintf( stderr, "\n" );
    }
    fprintf( stderr,
#ifdef HAVE_CHROOT
            "\t-r directory\tSandbox directory to chroot to\n"
#endif
            "\t-s level\tSyslog level\n"
            "\t-t\t\tCheck configuration file\n"
#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
            "\t-u user\t\tUser (id or name) to run as\n"
#endif
            "\t-V\t\tprint version info (-VV exit afterwards)\n" );
}

#ifdef HAVE_NT_SERVICE_MANAGER
void WINAPI
ServiceMain( DWORD argc, LPTSTR *argv )
#else
int
main( int argc, char **argv )
#endif
{
    int i, no_detach = 0;
    int rc = 1;
    char *urls = NULL;
#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
    char *username = NULL;
    char *groupname = NULL;
#endif
#if defined(HAVE_CHROOT)
    char *sandbox = NULL;
#endif
#ifdef SLAP_DEFAULT_SYSLOG_USER
    int syslogUser = SLAP_DEFAULT_SYSLOG_USER;
#endif

#ifndef HAVE_WINSOCK
    int pid, waitfds[2];
#endif
    int g_argc = argc;
    char **g_argv = argv;

    char *configfile = NULL;
    char *configdir = NULL;
    char *serverName;
    int serverMode = SLAP_SERVER_MODE;

    char **debug_unknowns = NULL;
    char **syslog_unknowns = NULL;

    int slapd_pid_file_unlink = 0, slapd_args_file_unlink = 0;
    int firstopt = 1;

    slap_sl_mem_init();

    serverName = lutil_progname( "lloadd", argc, argv );

#ifdef HAVE_NT_SERVICE_MANAGER
    {
        int *ip;
        char *newConfigFile;
        char *newConfigDir;
        char *newUrls;
        char *regService = NULL;

        if ( is_NT_Service ) {
            lutil_CommenceStartupProcessing( serverName, lload_sig_shutdown );
            if ( strcmp( serverName, SERVICE_NAME ) ) regService = serverName;
        }

        ip = (int *)lutil_getRegParam( regService, "DebugLevel" );
        if ( ip != NULL ) {
            slap_debug = *ip;
            Debug( LDAP_DEBUG_ANY, "new debug level from registry is: %d\n",
                    slap_debug );
        }

        newUrls = (char *)lutil_getRegParam( regService, "Urls" );
        if ( newUrls ) {
            if ( urls ) ch_free( urls );

            urls = ch_strdup( newUrls );
            Debug( LDAP_DEBUG_ANY, "new urls from registry: %s\n", urls );
        }

        newConfigFile = (char *)lutil_getRegParam( regService, "ConfigFile" );
        if ( newConfigFile != NULL ) {
            configfile = ch_strdup( newConfigFile );
            Debug( LDAP_DEBUG_ANY, "new config file from registry is: %s\n",
                    configfile );
        }

        newConfigDir = (char *)lutil_getRegParam( regService, "ConfigDir" );
        if ( newConfigDir != NULL ) {
            configdir = ch_strdup( newConfigDir );
            Debug( LDAP_DEBUG_ANY, "new config dir from registry is: %s\n",
                    configdir );
        }
    }
#endif

    epoch_init();

    while ( (i = getopt( argc, argv,
                      "c:d:f:F:h:n:o:s:tV"
#ifdef LDAP_PF_INET6
                      "46"
#endif
#ifdef HAVE_CHROOT
                      "r:"
#endif
#if defined(LDAP_DEBUG) && defined(LDAP_SYSLOG)
                      "S:"
#ifdef LOG_LOCAL4
                      "l:"
#endif
#endif
#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
                      "u:g:"
#endif
                      )) != EOF ) {
        switch ( i ) {
#ifdef LDAP_PF_INET6
            case '4':
                slap_inet4or6 = AF_INET;
                break;
            case '6':
                slap_inet4or6 = AF_INET6;
                break;
#endif

            case 'h': /* listen URLs */
                if ( urls != NULL ) free( urls );
                urls = ch_strdup( optarg );
                break;

            case 'd': { /* set debug level and 'do not detach' flag */
                int level = 0;

                if ( strcmp( optarg, "?" ) == 0 ) {
                    check |= CHECK_LOGLEVEL;
                    break;
                }

                no_detach = 1;
                if ( parse_debug_level( optarg, &level, &debug_unknowns ) ) {
                    goto destroy;
                }
#ifdef LDAP_DEBUG
                slap_debug |= level;
#else
                if ( level != 0 )
                    fputs( "must compile with LDAP_DEBUG for debugging\n",
                            stderr );
#endif
            } break;

            case 'f': /* read config file */
                configfile = ch_strdup( optarg );
                break;

            case 'o': {
                char *val = strchr( optarg, '=' );
                struct berval opt;

                opt.bv_val = optarg;

                if ( val ) {
                    opt.bv_len = ( val - optarg );
                    val++;

                } else {
                    opt.bv_len = strlen( optarg );
                }

                for ( i = 0; !BER_BVISNULL( &option_helpers[i].oh_name );
                        i++ ) {
                    if ( ber_bvstrcasecmp( &option_helpers[i].oh_name, &opt ) ==
                            0 ) {
                        assert( option_helpers[i].oh_fnc != NULL );
                        if ( (*option_helpers[i].oh_fnc)(
                                     val, option_helpers[i].oh_arg ) == -1 ) {
                            /* we assume the option parsing helper
                         * issues appropriate and self-explanatory
                         * error messages... */
                            goto stop;
                        }
                        break;
                    }
                }

                if ( BER_BVISNULL( &option_helpers[i].oh_name ) ) {
                    goto unhandled_option;
                }
                break;
            }

            case 's': /* set syslog level */
                if ( strcmp( optarg, "?" ) == 0 ) {
                    check |= CHECK_LOGLEVEL;
                    break;
                }

                if ( parse_debug_level(
                             optarg, &ldap_syslog, &syslog_unknowns ) ) {
                    goto destroy;
                }
                break;

#if defined(LDAP_DEBUG) && defined(LDAP_SYSLOG)
            case 'S':
                if ( parse_syslog_level( optarg, &ldap_syslog_level ) ) {
                    goto destroy;
                }
                break;

#ifdef LOG_LOCAL4
            case 'l': /* set syslog local user */
                if ( parse_syslog_user( optarg, &syslogUser ) ) {
                    goto destroy;
                }
                break;
#endif
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

#ifdef HAVE_CHROOT
            case 'r':
                if ( sandbox ) free( sandbox );
                sandbox = ch_strdup( optarg );
                break;
#endif

#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
            case 'u': /* user name */
                if ( username ) free( username );
                username = ch_strdup( optarg );
                break;

            case 'g': /* group name */
                if ( groupname ) free( groupname );
                groupname = ch_strdup( optarg );
                break;
#endif /* SETUID && GETUID */

            case 'n': /* NT service name */
                serverName = ch_strdup( optarg );
                break;

            case 't':
                check |= CHECK_CONFIG;
                break;

            case 'V':
                version++;
                break;

            default:
unhandled_option:;
                usage( argv[0] );
                rc = 1;
                SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 15 );
                goto stop;
        }

        if ( firstopt ) {
            firstopt = 0;
        }
    }

    if ( optind != argc ) goto unhandled_option;

    ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &slap_debug );
    ldap_set_option( NULL, LDAP_OPT_DEBUG_LEVEL, &slap_debug );
    ldif_debug = slap_debug;

    if ( version ) {
        fprintf( stderr, "%s\n", Versionstr );

        if ( version > 1 ) goto stop;
    }

#if defined(LDAP_DEBUG) && defined(LDAP_SYSLOG)
    {
        char *logName;
#ifdef HAVE_EBCDIC
        logName = ch_strdup( serverName );
        __atoe( logName );
#else
        logName = serverName;
#endif

#ifdef LOG_LOCAL4
        openlog( logName, OPENLOG_OPTIONS, syslogUser );
#elif defined LOG_DEBUG
        openlog( logName, OPENLOG_OPTIONS );
#endif
#ifdef HAVE_EBCDIC
        free( logName );
#endif
    }
#endif /* LDAP_DEBUG && LDAP_SYSLOG */

    Debug( LDAP_DEBUG_ANY, "%s", Versionstr );

    global_host = ldap_pvt_get_fqdn( NULL );

    if ( check == CHECK_NONE && lloadd_listeners_init( urls ) != 0 ) {
        rc = 1;
        SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 16 );
        goto stop;
    }

#if defined(HAVE_CHROOT)
    if ( sandbox ) {
        if ( chdir( sandbox ) ) {
            perror( "chdir" );
            rc = 1;
            goto stop;
        }
        if ( chroot( sandbox ) ) {
            perror( "chroot" );
            rc = 1;
            goto stop;
        }
        if ( chdir( "/" ) ) {
            perror( "chdir" );
            rc = 1;
            goto stop;
        }
    }
#endif

#if defined(HAVE_SETUID) && defined(HAVE_SETGID)
    if ( username != NULL || groupname != NULL ) {
        slap_init_user( username, groupname );
    }
#endif

    rc = lload_init( serverMode, serverName );
    if ( rc ) {
        SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 18 );
        goto destroy;
    }

    if ( lload_read_config( configfile, configdir ) != 0 ) {
        rc = 1;
        SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 19 );

        if ( check & CHECK_CONFIG ) {
            fprintf( stderr, "config check failed\n" );
        }

        goto destroy;
    }

    if ( debug_unknowns ) {
        rc = parse_debug_unknowns( debug_unknowns, &slap_debug );
        ldap_charray_free( debug_unknowns );
        debug_unknowns = NULL;
        if ( rc ) goto destroy;
    }
    if ( syslog_unknowns ) {
        rc = parse_debug_unknowns( syslog_unknowns, &ldap_syslog );
        ldap_charray_free( syslog_unknowns );
        syslog_unknowns = NULL;
        if ( rc ) goto destroy;
    }

    if ( check & CHECK_LOGLEVEL ) {
        rc = 0;
        goto destroy;
    }

    if ( check & CHECK_CONFIG ) {
        fprintf( stderr, "config check succeeded\n" );

        check &= ~CHECK_CONFIG;
        if ( check == CHECK_NONE ) {
            rc = 0;
            goto destroy;
        }
    }

#ifdef HAVE_TLS
    rc = ldap_pvt_tls_init( 1 );
    if ( rc != 0 ) {
        Debug( LDAP_DEBUG_ANY, "main: "
                "TLS init failed: %d\n",
                rc );
        rc = 1;
        SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 20 );
        goto destroy;
    }

    if ( lload_tls_init() ) {
        rc = 1;
        SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 20 );
        goto destroy;
    }
#endif

    daemon_base = event_base_new();
    if ( !daemon_base ) {
        Debug( LDAP_DEBUG_ANY, "main: "
                "main event base allocation failed\n" );
        rc = 1;
        SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 21 );
        goto destroy;
    }

    for ( i = 0; signal_handlers[i].signal; i++ ) {
        struct event *event;
        event = evsignal_new( daemon_base, signal_handlers[i].signal,
                signal_handlers[i].handler, daemon_base );
        if ( !event || event_add( event, NULL ) ) {
            Debug( LDAP_DEBUG_ANY, "main: "
                    "failed to register a handler for signal %d\n",
                    signal_handlers[i].signal );
            rc = 1;
            SERVICE_EXIT( ERROR_SERVICE_SPECIFIC_ERROR, 21 );
            goto destroy;
        }
        signal_handlers[i].event = event;
    }

#ifndef HAVE_WINSOCK
    if ( !no_detach ) {
        if ( lutil_pair( waitfds ) < 0 ) {
            Debug( LDAP_DEBUG_ANY, "main: "
                    "lutil_pair failed\n" );
            rc = 1;
            goto destroy;
        }
        pid = lutil_detach( no_detach, 0 );
        if ( pid ) {
            char buf[4];
            rc = EXIT_SUCCESS;
            close( waitfds[1] );
            if ( read( waitfds[0], buf, 1 ) != 1 ) rc = EXIT_FAILURE;
            _exit( rc );
        } else {
            close( waitfds[0] );
        }
    }
#endif /* HAVE_WINSOCK */

    if ( slapd_pid_file != NULL ) {
        FILE *fp = fopen( slapd_pid_file, "w" );

        if ( fp == NULL ) {
            char ebuf[128];
            int save_errno = errno;

            Debug( LDAP_DEBUG_ANY, "unable to open pid file "
                    "\"%s\": %d (%s)\n",
                    slapd_pid_file, save_errno,
                    AC_STRERROR_R( save_errno, ebuf, sizeof(ebuf) ) );

            free( slapd_pid_file );
            slapd_pid_file = NULL;

            rc = 1;
            goto destroy;
        }
        fprintf( fp, "%d\n", (int)getpid() );
        fclose( fp );
        slapd_pid_file_unlink = 1;
    }

    if ( slapd_args_file != NULL ) {
        FILE *fp = fopen( slapd_args_file, "w" );

        if ( fp == NULL ) {
            char ebuf[128];
            int save_errno = errno;

            Debug( LDAP_DEBUG_ANY, "unable to open args file "
                    "\"%s\": %d (%s)\n",
                    slapd_args_file, save_errno,
                    AC_STRERROR_R( save_errno, ebuf, sizeof(ebuf) ) );

            free( slapd_args_file );
            slapd_args_file = NULL;

            rc = 1;
            goto destroy;
        }

        for ( i = 0; i < g_argc; i++ ) {
            fprintf( fp, "%s ", g_argv[i] );
        }
        fprintf( fp, "\n" );
        fclose( fp );
        slapd_args_file_unlink = 1;
    }

    /*
     * FIXME: moved here from lloadd_daemon_task()
     * because back-monitor db_open() needs it
     */
    time( &starttime );

    Debug( LDAP_DEBUG_ANY, "lloadd starting\n" );

#ifndef HAVE_WINSOCK
    if ( !no_detach ) {
        write( waitfds[1], "1", 1 );
        close( waitfds[1] );
    }
#endif

#ifdef HAVE_NT_EVENT_LOG
    if ( is_NT_Service )
        lutil_LogStartedEvent( serverName, slap_debug,
                configfile ? configfile : LLOADD_DEFAULT_CONFIGFILE, urls );
#endif

    rc = lloadd_daemon( daemon_base );

#ifdef HAVE_NT_SERVICE_MANAGER
    /* Throw away the event that we used during the startup process. */
    if ( is_NT_Service ) ldap_pvt_thread_cond_destroy( &started_event );
#endif

destroy:
    if ( daemon_base ) {
        for ( i = 0; signal_handlers[i].signal; i++ ) {
            if ( signal_handlers[i].event ) {
                event_del( signal_handlers[i].event );
                event_free( signal_handlers[i].event );
            }
        }
        event_base_free( daemon_base );
    }

    if ( check & CHECK_LOGLEVEL ) {
        (void)loglevel_print( stdout );
    }
    /* remember an error during destroy */
    rc |= lload_destroy();

stop:
#ifdef HAVE_NT_EVENT_LOG
    if ( is_NT_Service ) lutil_LogStoppedEvent( serverName );
#endif

    Debug( LDAP_DEBUG_ANY, "lloadd stopped.\n" );

#ifdef HAVE_NT_SERVICE_MANAGER
    lutil_ReportShutdownComplete();
#endif

#ifdef LOG_DEBUG
    closelog();
#endif
    lloadd_daemon_destroy();

#ifdef HAVE_TLS
    if ( lload_tls_ld ) {
        ldap_pvt_tls_ctx_free( lload_tls_ctx );
        ldap_unbind_ext( lload_tls_ld, NULL, NULL );
    }
    ldap_pvt_tls_destroy();
#endif

    if ( slapd_pid_file_unlink ) {
        unlink( slapd_pid_file );
    }
    if ( slapd_args_file_unlink ) {
        unlink( slapd_args_file );
    }

    lload_config_destroy();

    if ( configfile ) ch_free( configfile );
    if ( configdir ) ch_free( configdir );
    if ( urls ) ch_free( urls );
    if ( global_host ) ch_free( global_host );

    /* kludge, get symbols referenced */
    ldap_tavl_free( NULL, NULL );

    MAIN_RETURN(rc);
}

#ifdef SIGPIPE

/*
 *  Catch and discard terminated child processes, to avoid zombies.
 */

static void
sigpipe( evutil_socket_t sig, short what, void *arg )
{
}

#endif /* SIGPIPE */

#ifdef LDAP_SIGCHLD

/*
 *  Catch and discard terminated child processes, to avoid zombies.
 */

static void
wait4child( evutil_socket_t sig, short what, void *arg )
{
    int save_errno = errno;

#ifdef WNOHANG
    do
        errno = 0;
#ifdef HAVE_WAITPID
    while ( waitpid( (pid_t)-1, NULL, WNOHANG ) > 0 || errno == EINTR );
#else
    while ( wait3( NULL, WNOHANG, NULL ) > 0 || errno == EINTR );
#endif
#else
    (void)wait( NULL );
#endif
    errno = save_errno;
}

#endif /* LDAP_SIGCHLD */
