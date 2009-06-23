/*	$NetBSD: data_redirect.c,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

/*++
/* NAME
/*	data_redirect 3
/* SUMMARY
/*	redirect legacy writes to Postfix-owned data directory
/* SYNOPSIS
/*	#include <data_redirect.h>
/*
/*	char	*data_redirect_file(result, path)
/*	VSTRING	*result;
/*	const char *path;
/*
/*	char	*data_redirect_map(result, map)
/*	VSTRING	*result;
/*	const char *map;
/* DESCRIPTION
/*	With Postfix version 2.5 and later, the tlsmgr(8) and
/*	verify(8) servers no longer open cache files with root
/*	privilege. This avoids a potential security loophole where
/*	the ownership of a file (or directory) does not match the
/*	trust level of the content of that file (or directory).
/*
/*	This module implements a migration aid that allows a
/*	transition without disruption of service.
/*
/*	data_redirect_file() detects a request to open a file in a
/*	non-Postfix directory, logs a warning, and redirects the
/*	request to the Postfix-owned data_directory.
/*
/*	data_redirect_map() performs the same function for a limited
/*	subset of file-based lookup tables.
/*
/*	Arguments:
/* .IP result
/*	A possibly redirected copy of the input.
/* .IP path
/*	The pathname that may be redirected.
/* .IP map
/*	The "mapname" or "maptype:mapname" that may be redirected.
/*	The result is always in "maptype:mapname" form.
/* BUGS
/*	Only a few map types are redirected. This is acceptable for
/*	a temporary migration tool.
/* DIAGNOSTICS
/*	Fatal errors: memory allocation failure.
/* CONFIGURATION PARAMETERS
/*	data_directory, location of Postfix-writable files
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
#include <sys/stat.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <stringops.h>
#include <split_at.h>
#include <name_code.h>
#include <dict_db.h>
#include <dict_dbm.h>
#include <dict_cdb.h>

/* Global directory. */

#include <mail_params.h>
#include <dict_proxy.h>
#include <data_redirect.h>

/* Application-specific. */

#define STR(x) vstring_str(x)
#define LEN(x) VSTRING_LEN(x)

 /*
  * Redirect only these map types, so that we don't try stupid things with
  * NIS, *SQL or LDAP. This is a transition feature for legacy TLS and verify
  * configurations, so it does not have to cover every possible map type.
  * 
  * XXX In this same spirit of imperfection we also use hard-coded map names,
  * because maintainers may add map types that the official release doesn't
  * even know about, because map types may be added dynamically on some
  * platforms.
  */
static const NAME_CODE data_redirect_map_types[] = {
    DICT_TYPE_HASH, 1,
    DICT_TYPE_BTREE, 1,
    DICT_TYPE_DBM, 1,
    DICT_TYPE_CDB, 1,			/* not a read-write map type */
    "sdbm", 1,				/* legacy 3rd-party TLS */
    "dbz", 1,				/* just in case */
    0, 0,
};

/* data_redirect_path - redirect path to Postfix-owned directory */

static char *data_redirect_path(VSTRING *result, const char *path,
			         const char *log_type, const char *log_name)
{
    struct stat st;

#define PATH_DELIMITER "/"

    (void) sane_dirname(result, path);
    if (stat(STR(result), &st) != 0 || st.st_uid == var_owner_uid) {
	vstring_strcpy(result, path);
    } else {
	msg_warn("request to update %s %s in non-%s directory %s",
		 log_type, log_name, var_mail_owner, STR(result));
	msg_warn("redirecting the request to %s-owned %s %s",
		 var_mail_owner, VAR_DATA_DIR, var_data_dir);
	(void) sane_basename(result, path);
	vstring_prepend(result, PATH_DELIMITER, sizeof(PATH_DELIMITER) - 1);
	vstring_prepend(result, var_data_dir, strlen(var_data_dir));
    }
    return (STR(result));
}

/* data_redirect_file - redirect file to Postfix-owned directory */

char   *data_redirect_file(VSTRING *result, const char *path)
{

    /*
     * Sanity check.
     */
    if (path == STR(result))
	msg_panic("data_redirect_file: result clobbers input");

    return (data_redirect_path(result, path, "file", path));
}

char   *data_redirect_map(VSTRING *result, const char *map)
{
    const char *path;
    const char *map_type;
    size_t  map_type_len;

#define MAP_DELIMITER ":"

    /*
     * Sanity check.
     */
    if (map == STR(result))
	msg_panic("data_redirect_map: result clobbers input");

    /*
     * Parse the input into map type and map name.
     */
    path = strchr(map, MAP_DELIMITER[0]);
    if (path != 0) {
	map_type = map;
	map_type_len = path - map;
	path += 1;
    } else {
	map_type = var_db_type;
	map_type_len = strlen(map_type);
	path = map;
    }

    /*
     * Redirect the pathname.
     */
    vstring_strncpy(result, map_type, map_type_len);
    if (name_code(data_redirect_map_types, NAME_CODE_FLAG_NONE, STR(result))) {
	data_redirect_path(result, path, "table", map);
    } else {
	vstring_strcpy(result, path);
    }

    /*
     * (Re)combine the map type with the map name.
     */
    vstring_prepend(result, MAP_DELIMITER, sizeof(MAP_DELIMITER) - 1);
    vstring_prepend(result, map_type, map_type_len);
    return (STR(result));
}

 /*
  * Proof-of-concept test program. This can't be run as automated regression
  * test, because the result depends on main.cf information (mail_owner UID
  * and data_directory pathname) and on local file system details.
  */
#ifdef TEST

#include <unistd.h>
#include <stdlib.h>
#include <vstring_vstream.h>
#include <mail_conf.h>

int     main(int argc, char **argv)
{
    VSTRING *inbuf = vstring_alloc(100);
    VSTRING *result = vstring_alloc(100);
    char   *bufp;
    char   *cmd;
    char   *target;
    char   *junk;

    mail_conf_read();

    while (vstring_get_nonl(inbuf, VSTREAM_IN) != VSTREAM_EOF) {
	bufp = STR(inbuf);
	if (!isatty(0)) {
	    vstream_printf("> %s\n", bufp);
	    vstream_fflush(VSTREAM_OUT);
	}
	if (*bufp == '#')
	    continue;
	if ((cmd = mystrtok(&bufp, " \t")) == 0) {
	    vstream_printf("usage: file path|map maptype:mapname\n");
	    vstream_fflush(VSTREAM_OUT);
	    continue;
	}
	target = mystrtok(&bufp, " \t");
	junk = mystrtok(&bufp, " \t");
	if (strcmp(cmd, "file") == 0 && target && !junk) {
	    data_redirect_file(result, target);
	    vstream_printf("%s -> %s\n", target, STR(result));
	} else if (strcmp(cmd, "map") == 0 && target && !junk) {
	    data_redirect_map(result, target);
	    vstream_printf("%s -> %s\n", target, STR(result));
	} else {
	    vstream_printf("usage: file path|map maptype:mapname\n");
	}
	vstream_fflush(VSTREAM_OUT);
    }
    vstring_free(inbuf);
    return (0);
}

#endif
