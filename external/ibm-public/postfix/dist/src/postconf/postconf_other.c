/*	$NetBSD: postconf_other.c,v 1.2 2017/02/14 01:16:46 christos Exp $	*/

/*++
/* NAME
/*	postconf_other 3
/* SUMMARY
/*	support for miscellaneous information categories
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	pcf_show_maps()
/*
/*	void	pcf_show_locks()
/*
/*	void	pcf_show_sasl(mode)
/*	int	mode;
/*
/*	void	pcf_show_tls(what)
/*	const char *what;
/* DESCRIPTION
/*	pcf_show_maps() lists the available map (lookup table)
/*	types.
/*
/*	pcf_show_locks() lists the available mailbox lock types.
/*
/*	pcf_show_sasl() shows the available SASL authentication
/*	plugin types.
/*
/*	pcf_show_tls() reports the "compile-version" or "run-version"
/*	of the TLS library, or the supported public-key algorithms.
/*
/*	Arguments:
/* .IP mode
/*	Show server information if the PCF_SHOW_SASL_SERV flag is
/*	set, otherwise show client information.
/* .IP what
/*	One of the literals "compile-version", "run-version" or
/*	"public-key-algorithms".
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <vstream.h>
#include <argv.h>
#include <dict.h>
#include <msg.h>

/* Global library. */

#include <mbox_conf.h>

/* XSASL library. */

#include <xsasl.h>

/* TLS library. */

#include <tls.h>

/* Application-specific. */

#include <postconf.h>

/* pcf_show_maps - show available maps */

void    pcf_show_maps(void)
{
    ARGV   *maps_argv;
    int     i;

    maps_argv = dict_mapnames();
    for (i = 0; i < maps_argv->argc; i++)
	vstream_printf("%s\n", maps_argv->argv[i]);
    argv_free(maps_argv);
}

/* pcf_show_locks - show available mailbox locking methods */

void    pcf_show_locks(void)
{
    ARGV   *locks_argv;
    int     i;

    locks_argv = mbox_lock_names();
    for (i = 0; i < locks_argv->argc; i++)
	vstream_printf("%s\n", locks_argv->argv[i]);
    argv_free(locks_argv);
}

/* pcf_show_sasl - show SASL plug-in types */

void    pcf_show_sasl(int what)
{
    ARGV   *sasl_argv;
    int     i;

    sasl_argv = (what & PCF_SHOW_SASL_SERV) ? xsasl_server_types() :
	xsasl_client_types();
    for (i = 0; i < sasl_argv->argc; i++)
	vstream_printf("%s\n", sasl_argv->argv[i]);
    argv_free(sasl_argv);
}

/* pcf_show_tls - show TLS support */

void    pcf_show_tls(const char *what)
{
#ifdef USE_TLS
    if (strcmp(what, "compile-version") == 0)
	vstream_printf("%s\n", tls_compile_version());
    else if (strcmp(what, "run-version") == 0)
	vstream_printf("%s\n", tls_run_version());
    else if (strcmp(what, "public-key-algorithms") == 0) {
	const char **cpp;

	for (cpp = tls_pkey_algorithms(); *cpp; cpp++)
	    vstream_printf("%s\n", *cpp);
    } else {
	msg_warn("unknown 'postconf -T' mode: %s", what);
	exit(1);
    }
#endif						/* USE_TLS */
}
