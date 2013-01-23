/*	$NetBSD: postconf_other.c,v 1.1.1.1.2.2 2013/01/23 00:05:06 yamt Exp $	*/

/*++
/* NAME
/*	postconf_other 3
/* SUMMARY
/*	support for miscellaneous information categories
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	show_maps()
/*
/*	void	show_locks()
/*
/*	void	show_sasl(mode)
/*	int	mode;
/* DESCRIPTION
/*	show_maps() lists the available map (lookup table) types.
/*
/*	show_locks() lists the available mailbox lock types.
/*
/*	show_sasl() shows the available SASL authentication
/*	plugin types.
/*
/*	Arguments:
/* .IP mode
/*	Show server information if the SHOW_SASL_SERV flag is set,
/*	otherwise show client information.
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
/*--*/

/* System library. */

#include <sys_defs.h>

/* Utility library. */

#include <vstream.h>
#include <argv.h>
#include <dict.h>

/* Global library. */

#include <mbox_conf.h>

/* XSASL library. */

#include <xsasl.h>

/* Application-specific. */

#include <postconf.h>

/* show_maps - show available maps */

void    show_maps(void)
{
    ARGV   *maps_argv;
    int     i;

    maps_argv = dict_mapnames();
    for (i = 0; i < maps_argv->argc; i++)
	vstream_printf("%s\n", maps_argv->argv[i]);
    argv_free(maps_argv);
}

/* show_locks - show available mailbox locking methods */

void    show_locks(void)
{
    ARGV   *locks_argv;
    int     i;

    locks_argv = mbox_lock_names();
    for (i = 0; i < locks_argv->argc; i++)
	vstream_printf("%s\n", locks_argv->argv[i]);
    argv_free(locks_argv);
}

/* show_sasl - show SASL plug-in types */

void    show_sasl(int what)
{
    ARGV   *sasl_argv;
    int     i;

    sasl_argv = (what & SHOW_SASL_SERV) ? xsasl_server_types() :
	xsasl_client_types();
    for (i = 0; i < sasl_argv->argc; i++)
	vstream_printf("%s\n", sasl_argv->argv[i]);
    argv_free(sasl_argv);
}
