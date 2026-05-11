/*	$NetBSD: nbdb_safe.c,v 1.2.2.2 2026/05/11 17:13:51 martin Exp $	*/

/*++
/* NAME
/*	nbdb_safe 3
/* SUMMARY
/*	Non-Berkeley-DB migration service
/* SYNOPSIS
/*	#include <nbdb_safe.h>
/*
/*	bool    nbdb_safe_for_uid(
/*	uid_t	uid,
/*	const struct stat *st)
/*
/*	bool    nbdb_safe_to_index_as_legacy_index_owner(
/*	const char *source_path,
/*	const struct stat *source_st,
/*	const char *leg_idx_path,
/*	const struct stat *leg_idx_st,
/*	const char *parent_dir,
/*	const struct stat *parent_dir_st,
/*	VSTRING *why)
/* DESCRIPTION
/*	nbdb_safe_for_uid() determines if the file or directory properties
/*	in "st" are safe for a process that runs with "uid". That is,
/*	the file or directory is owned "root" or by "uid", and does not
/*	allow 'group' or 'other' write access.	This predicate ignores
/*	the safety of other pathname components. It is a good idea to
/*	trust only a limited number of pathname prefixes.
/*
/*	nbdb_safe_to_index_as_legacy_index_owner() determines
/*	if a Berkeley DB source file and parent directory are 'safe'
/*	for the uid of the Berkeley DB indexed file.
/* .PP
/*	Arguments:
/* .IP source_path, source_st
/*	Berkeley DB source file pathname and metadata.
/*  .IP leg_idx_path, leg_idx_st
/*	Berkeley DB indexed file pathname and metadata.
/* .IP parent_dir, parent_dir_st
/*	Parent directory pathname and metadata.
/* .IP why
/*	Storage that is updated with an applicable error description.
/* SEE ALSO
/*	nbdb_sniffer_test(1t), unit test
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
#include <sys/stat.h>

 /*
  * Utility library.
  */
#include <wrap_stat.h>

 /*
  * Global library.
  */
#include <allowed_prefix.h>
#include <mail_conf.h>
#include <mail_params.h>

 /*
  * Application-specific.
  */
#include <nbdb_safe.h>
#include <nbdb_reindexd.h>

/* nbdb_safe_for_uid - owned by the user or root, not group or other writable */

bool    nbdb_safe_for_uid(uid_t uid, const struct stat *st)
{

    /*
     * This "safe for" predicate is weaker than the one used in by Chari et
     * al. in "Where do you want to go today?", which checks every pathname
     * component (including pathname components in symlink targets). For this
     * reason, the root user must have additional constraints on the pathname
     * prefix.
     */
    return ((st->st_uid == 0 || uid == st->st_uid)
	    && (st->st_mode & (S_IWGRP | S_IWOTH)) == 0);
}

/* nbdb_safe_to_index_as_legacy_index_owner - OK to run postmap/postalias */

bool    nbdb_safe_to_index_as_legacy_index_owner(
	              const char *source_path, const struct stat *source_st,
	            const char *leg_idx_path, const struct stat *leg_idx_st,
	           const char *parent_dir, const struct stat *parent_dir_st,
						         VSTRING *why)
{
    uid_t   runas_uid;

    /*
     * Run postmap or postalias as the legacy indexed file owner, if the
     * source file, legacy indexed file, and parent directory are safe for
     * that user, with pathname constraints for 'root' or non-root.
     */
    runas_uid = leg_idx_st->st_uid;

    /*
     * Allow root-privileged execution only for authorized pathnames. This
     * prevents a corrupted Postfix daemon from submitting requests with
     * pathnames that appear to be owned by root, under a directory that is
     * in part under their control, and then run a race attack against us.
     */
    if (runas_uid == 0) {
	if (!allowed_prefix_match(parsed_allow_root_pfxs, source_path)) {
	    vstring_sprintf(why, "table %s has an unexpected pathname; to "
			    "allow automatic indexing as root, append its "
			    "parent directory to the '%s' setting "
			    "(current setting is: \"%s\")",
			    source_path, VAR_NBDB_ALLOW_ROOT_PFXS,
			    var_nbdb_allow_root_pfxs);
	    return (false);
	}
    } else {
	if (!allowed_prefix_match(parsed_allow_user_pfxs, source_path)) {
	    vstring_sprintf(why, "table %s has an unexpected pathname; to "
			 "allow automatic indexing as non-root, append its "
			    "parent directory to the '%s' setting "
			    "(current setting is: \"%s\")",
			    source_path, VAR_NBDB_ALLOW_USER_PFXS,
			    var_nbdb_allow_user_pfxs);
	    return (false);
	}
    }

    /*
     * Allow automated indexing only if the source file and parent directory
     * are not writable by other users.
     */
    if (!nbdb_safe_for_uid(runas_uid, source_st)) {
	vstring_sprintf(why, "legacy indexed file '%s' is owned by "
			"uid '%d', but source file '%s' is owned or "
			"writable by other user; to allow automatic "
			"indexing, correct the ownership or permissions",
			leg_idx_path, (int) runas_uid, source_path);
	return (false);
    }
    if (!nbdb_safe_for_uid(runas_uid, parent_dir_st)) {
	vstring_sprintf(why, "legacy indexed file '%s' is owned by "
			"uid '%d', but parent directory '%s' is "
			"owned or writable by other user; to allow "
			"automatic indexing, correct the ownership or"
			"permissions", leg_idx_path, (int) runas_uid,
			parent_dir);
	return (false);
    }

    /*
     * No objections.
     */
    return (true);
}
