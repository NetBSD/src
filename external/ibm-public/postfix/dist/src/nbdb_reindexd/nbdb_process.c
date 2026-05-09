/*	$NetBSD: nbdb_process.c,v 1.2 2026/05/09 18:49:17 christos Exp $	*/

/*++
/* NAME
/*	nbdb_process 3
/* SUMMARY
/*	Non-Berkeley-DB migration service
/* SYNOPSIS
/*	#include <nbdb_process.h>
/*
/*	int nbdb_process(
/*	const char *legacy_type,
/*	const char *source_path,
/*	VSTRING *why)
/* DESCRIPTION
/*	nbdb_process() integrates the safety policy and content policy,
/*	and calls spawn_command() to generate an indexed file for
/*	$non_legacy_type:$source_path.
/*
/*	The result value is NBDB_MIGR_OK (success) or NBDB_MIGR_ERROR
/*	(failure).
/*
/*	Arguments:
/* .IP new_type
/*	A non-legacy database type.
/* .P source_path
/*	The pathname for the source text file (no database suffix).
/* .IP why
/*	Storage for descriptive text in case of error.
/* SEE ALSO
/*	nbdb_safe(3), safety policy
/*	nbdb_sniffer(3), content policy
/*	nbdb_process_test(1t), unit test
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	porcupine.org
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <libgen.h>			/* dirname() */

 /*
  * Utility library.
  */
#include <attr.h>
#include <msg.h>
#include <spawn_command.h>
#include <vstring.h>
#include <wrap_stat.h>

 /*
  * Global library.
  */
#include <allowed_prefix.h>
#include <mail_params.h>
#include <nbdb_util.h>

 /*
  * Application-specific.
  */
#include <nbdb_reindexd.h>
#include <nbdb_safe.h>
#include <nbdb_sniffer.h>
#include <nbdb_index_as.h>
#include <nbdb_process.h>

#define STR(x)	vstring_str(x)

/* nbdb_process - main processing flow */

int     nbdb_process(const char *leg_type, const char *source_path,
		             VSTRING *why)
{
    const char *new_type;
    struct stat source_st;
    static VSTRING *new_idx_path;
    struct stat new_idx_st;
    static VSTRING *leg_idx_path;
    struct stat leg_idx_st;
    static VSTRING *parent_dir_buf;
    struct stat parent_dir_st;
    char   *parent_dir;
    const char *new_suffix;
    const char *index_cmd;
    int     status;

    if (new_idx_path == 0) {
	new_idx_path = vstring_alloc(100);
	leg_idx_path = vstring_alloc(100);
	parent_dir_buf = vstring_alloc(100);
    }

    /*
     * First things first.
     */
    if (nbdb_level < NBDB_LEV_CODE_REINDEX) {
	vstring_sprintf(why, "the %s service is disabled", var_servname);
	return (NBDB_STAT_ERROR);
    }

    /*
     * nbdb_map_leg_type() will allow only mapping from an expected Berkeley
     * DB type to an expected non-Berkeley-DB type.
     */
    if ((new_type = nbdb_map_leg_type(leg_type, source_path,
				  (const DICT_OPEN_INFO **) 0, why)) == 0) {
	return (NBDB_STAT_ERROR);
    }

    /*
     * Look up the non-legacy pathname suffix and build the pathname that
     * needs to be created. If no suffix exists then the client is in error.
     */
    if ((new_suffix = nbdb_find_non_leg_suffix(new_type)) == 0) {
	vstring_sprintf(why, "non-Berkeley-DB type '%s' has no known suffix",
			new_type);
	return (NBDB_STAT_ERROR);
    }
    vstring_sprintf(new_idx_path, "%s%s", source_path, new_suffix);

    /*
     * If the target indexed pathname already exists, then do nothing. All
     * indexing requests are handled sequentially (to avoid collisions), and
     * the client may have connected to this service before the file was
     * indexed.
     */
    if (stat (STR(new_idx_path), &new_idx_st) == 0 && new_idx_st.st_size > 0) {
	msg_info("target file '%s' already exists, skipping this request",
		 STR(new_idx_path));
	return (NBDB_STAT_OK);
    }

    /*
     * Require that the legacy indexed file exists. We need this to determine
     * the owner's (uid, gid) to run the postmap or postalias command with.
     */
    vstring_sprintf(leg_idx_path, "%s%s", source_path, NBDB_LEGACY_SUFFIX);
    if (stat (STR(leg_idx_path), &leg_idx_st) < 0) {
	vstring_sprintf(why, "look up status for legacy indexed file '%s': %m",
			STR(leg_idx_path));
	return (NBDB_STAT_ERROR);
    }

    /*
     * Require that the source path exists. We need it as input for indexing,
     * and will reject the request if the source has unsafe permissions for
     * the uid that we would run postmap or postalias with.
     */
    if (stat (source_path, &source_st) < 0) {
	vstring_sprintf(why, "look up status for source file '%s': %m",
			source_path);
	return (NBDB_STAT_ERROR);
    }

    /*
     * Require that the parent directory exists. We will reject the request
     * if the directory has unsafe permissions for the uid that we would run
     * postmap or postalias with.
     */
    vstring_strcpy(parent_dir_buf, source_path);
    parent_dir = dirname(STR(parent_dir_buf));
    if (stat (parent_dir, &parent_dir_st) < 0) {
	vstring_sprintf(why, "look up status for parent directory '%s': %m",
			parent_dir);
	return (NBDB_STAT_ERROR);
    }

    /*
     * Should we run postmap or postalias? Open the source file with the same
     * (uid, gid) as the postmap or postalias commands would, so that we can
     * detect permission errors quickly.
     * 
     * Note: we do this before the file allowlist/owner/permission safety
     * checks, so that we can log the concrete postmap or postalias command
     * if a safety check fails.
     */
    if ((index_cmd = nbdb_get_index_cmd_as(source_path, leg_idx_st.st_uid,
					   leg_idx_st.st_gid, why)) == 0)
	return (NBDB_STAT_ERROR);

    /*
     * Allow indexing as the legacy indexed file owner if it is considered
     * "safe".
     */
    if (!nbdb_safe_to_index_as_legacy_index_owner(source_path, &source_st,
					     STR(leg_idx_path), &leg_idx_st,
					 parent_dir, &parent_dir_st, why)) {
	status = NBDB_STAT_ERROR;
    }

    /*
     * Finally!
     */
    else {
	status = nbdb_index_as(index_cmd, new_type, source_path,
			       leg_idx_st.st_uid, leg_idx_st.st_gid, why);
	if (status != NBDB_STAT_OK)
	    /* Don't leave behind a broken indexed file. */
	    (void) unlink(STR(new_idx_path));
    }

    /*
     * If the command failed, or was not safe to execute automatically, give
     * the user the concrete command so that they could run it by hand.
     */
    if (status == NBDB_STAT_OK) {
	msg_info("successfully executed '%s %s:%s' as uid %d",
		 index_cmd, new_type, source_path, leg_idx_st.st_uid);
    } else {
	vstring_sprintf_prepend(why, "could not execute command '%s %s:%s': ",
				index_cmd, new_type, source_path);
	vstring_strcat(why,
		     "; alternatively, execute the failed command by hand");
    }
    return (status);
}
