/*	$NetBSD: postconf_misc.c,v 1.2 2017/02/14 01:16:46 christos Exp $	*/

/*++
/* NAME
/*	postconf_misc 3
/* SUMMARY
/*	miscellaneous low-level code
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	pcf_set_config_dir()
/* DESCRIPTION
/*	pcf_set_config_dir() forcibly overrides the var_config_dir
/*	parameter setting with the value from the environment or
/*	with the default pathname, and updates the mail parameter
/*	dictionary.
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

#include <mymalloc.h>
#include <safe.h>

/* Global library. */

#include <mail_params.h>
#include <mail_conf.h>

/* Application-specific. */

#include <postconf.h>

/* pcf_set_config_dir - forcibly override var_config_dir */

void    pcf_set_config_dir(void)
{
    char   *config_dir;

    if (var_config_dir)
	myfree(var_config_dir);
    if ((config_dir = safe_getenv(CONF_ENV_PATH)) != 0) {
	var_config_dir = mystrdup(config_dir);
	set_mail_conf_str(VAR_CONFIG_DIR, var_config_dir);
    } else {
	var_config_dir = mystrdup(DEF_CONFIG_DIR);
    }
}
