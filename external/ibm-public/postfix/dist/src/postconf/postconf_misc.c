/*	$NetBSD: postconf_misc.c,v 1.2.22.1 2023/12/25 12:43:33 martin Exp $	*/

/*++
/* NAME
/*	postconf_misc 3
/* SUMMARY
/*	miscellaneous low-level code
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	pcf_set_config_dir()
/*
/*	const char *pcf_get_main_path()
/*
/*	const char *pcf_get_master_path()
/* DESCRIPTION
/*	pcf_set_config_dir() forcibly overrides the var_config_dir
/*	parameter setting with the value from the environment or
/*	with the default pathname, and updates the mail parameter
/*	dictionary.
/*
/*	pcf_get_main_path() and pcf_get_master_path() return a
/*	pointer to a cached main.cf or master.cf full pathname,
/*	based on the current var_config_dir setting. The functions
/*	call pcf_set_config_dir() when no full pathname is cached.
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
#include <stringops.h>

/* Global library. */

#include <mail_params.h>
#include <mail_conf.h>

/* Application-specific. */

#include <postconf.h>

 /*
  * Pathname cache, based on current var_config_dir.
  */
static char *pcf_main_path = 0;
static char *pcf_master_path = 0;

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

    /*
     * Populate the full pathname cache.
     */
    if (pcf_main_path)
	myfree(pcf_main_path);
    pcf_main_path =
	concatenate(var_config_dir, "/", MAIN_CONF_FILE, (char *) 0);
    if (pcf_master_path)
	myfree(pcf_master_path);
    pcf_master_path =
	concatenate(var_config_dir, "/", MASTER_CONF_FILE, (char *) 0);
}

/* pcf_get_main_path - generate and return full main.cf pathname */

const char *pcf_get_main_path(void)
{
    if (pcf_main_path == 0)
	pcf_set_config_dir();
    return (pcf_main_path);
}

/* pcf_get_master_path - generate and return full master.cf pathname */

const char *pcf_get_master_path(void)
{
    if (pcf_master_path == 0)
	pcf_set_config_dir();
    return (pcf_master_path);
}
