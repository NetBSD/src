/*++
/* NAME
/*	dict_unix 3
/* SUMMARY
/*	dictionary manager interface to UNIX tables
/* SYNOPSIS
/*	#include <dict_unix.h>
/*
/*	DICT	*dict_unix_open(map, dummy, dict_flags)
/*	const char *map;
/*	int	dummy;
/*	int	dict_flags;
/* DESCRIPTION
/*	dict_unix_open() makes the specified UNIX table accessible via
/*	the generic dictionary operations described in dict_open(3).
/*	The \fIdummy\fR argument is not used.
/*
/*	Known map names:
/* .IP passwd.byname
/*	The table is the UNIX password database. The key is a login name.
/*	The result is a password file entry in passwd(5) format.
/* .IP group.byname
/*	The table is the UNIX group database. The key is a group name.
/*	The result is a group file entry in group(5) format.
/* SEE ALSO
/*	dict(3) generic dictionary manager
/* DIAGNOSTICS
/*	Fatal errors: out of memory, unknown map name, attempt to update map.
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

#include "sys_defs.h"
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>

/* Utility library. */

#include "msg.h"
#include "mymalloc.h"
#include "vstring.h"
#include "dict.h"
#include "dict_unix.h"

/* Application-specific. */

typedef struct {
    DICT    dict;			/* generic members */
} DICT_UNIX;

/* dict_unix_getpwnam - find password table entry */

static const char *dict_unix_getpwnam(DICT *unused_dict, const char *key)
{
    struct passwd *pwd;
    static VSTRING *buf;

    dict_errno = 0;

    if ((pwd = getpwnam(key)) == 0) {
	return (0);
    } else {
	if (buf == 0)
	    buf = vstring_alloc(10);
	vstring_sprintf(buf, "%s:%s:%ld:%ld:%s:%s:%s",
			pwd->pw_name, pwd->pw_passwd, (long) pwd->pw_uid,
			(long) pwd->pw_gid, pwd->pw_gecos, pwd->pw_dir,
			pwd->pw_shell);
	return (vstring_str(buf));
    }
}

/* dict_unix_getgrnam - find group table entry */

static const char *dict_unix_getgrnam(DICT *unused_dict, const char *key)
{
    struct group *grp;
    static VSTRING *buf;
    char  **cpp;

    dict_errno = 0;

    if ((grp = getgrnam(key)) == 0) {
	return (0);
    } else {
	if (buf == 0)
	    buf = vstring_alloc(10);
	vstring_sprintf(buf, "%s:%s:%ld:",
			grp->gr_name, grp->gr_passwd, (long) grp->gr_gid);
	for (cpp = grp->gr_mem; *cpp; cpp++) {
	    vstring_strcat(buf, *cpp);
	    if (cpp[1])
		VSTRING_ADDCH(buf, ',');
	}
	VSTRING_TERMINATE(buf);
	return (vstring_str(buf));
    }
}

/* dict_unix_close - close UNIX map */

static void dict_unix_close(DICT *dict)
{
    DICT_UNIX *dict_unix = (DICT_UNIX *) dict;

    dict_free(dict);
}

/* dict_unix_open - open UNIX map */

DICT   *dict_unix_open(const char *map, int unused_flags, int dict_flags)
{
    DICT_UNIX *dict_unix;
    struct dict_unix_lookup {
	char   *name;
	const char *(*lookup) (DICT *, const char *);
    };
    static struct dict_unix_lookup dict_unix_lookup[] = {
	"passwd.byname", dict_unix_getpwnam,
	"group.byname", dict_unix_getgrnam,
	0,
    };
    struct dict_unix_lookup *lp;

    dict_errno = 0;

    dict_unix = (DICT_UNIX *) dict_alloc(DICT_TYPE_UNIX, map,
					 sizeof(*dict_unix));
    for (lp = dict_unix_lookup; /* void */ ; lp++) {
	if (lp->name == 0)
	    msg_fatal("dict_unix_open: unknown map name: %s", map);
	if (strcmp(map, lp->name) == 0)
	    break;
    }
    dict_unix->dict.lookup = lp->lookup;
    dict_unix->dict.close = dict_unix_close;
    dict_unix->dict.flags = dict_flags | DICT_FLAG_FIXED;
    return (DICT_DEBUG(&dict_unix->dict));
}
