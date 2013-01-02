/*	$NetBSD: mail_addr_map.c,v 1.1.1.2 2013/01/02 18:58:58 tron Exp $	*/

/*++
/* NAME
/*	mail_addr_map 3
/* SUMMARY
/*	generic address mapping
/* SYNOPSIS
/*	#include <mail_addr_map.h>
/*
/*	ARGV	*mail_addr_map(path, address, propagate)
/*	MAPS	*path;
/*	const char *address;
/*	int	propagate;
/* DESCRIPTION
/*	mail_addr_map() returns the translation for the named address,
/*	or a null pointer if none is found.  The result is in canonical
/*	external (quoted) form.  The search is case insensitive.
/*
/*	When the \fBpropagate\fR argument is non-zero,
/*	address extensions that aren't explicitly matched in the lookup
/*	table are propagated to the result addresses. The caller is
/*	expected to pass the result to argv_free().
/*
/*	Lookups are performed by mail_addr_find(). When the result has the
/*	form \fI@otherdomain\fR, the result is the original user in
/*	\fIotherdomain\fR.
/*
/*	Arguments:
/* .IP path
/*	Dictionary search path (see maps(3)).
/* .IP address
/*	The address to be looked up.
/* DIAGNOSTICS
/*	Warnings: map lookup returns a non-address result.
/*
/*	The path->error value is non-zero when the lookup
/*	should be tried again.
/* SEE ALSO
/*	mail_addr_find(3), mail address matching
/*	mail_addr_crunch(3), mail address parsing and rewriting
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
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <vstring.h>
#include <dict.h>
#include <argv.h>
#include <mymalloc.h>

/* Global library. */

#include <mail_addr_find.h>
#include <mail_addr_crunch.h>
#include <mail_addr_map.h>

/* Application-specific. */

#define STR	vstring_str
#define LEN	VSTRING_LEN

/* mail_addr_map - map a canonical address */

ARGV   *mail_addr_map(MAPS *path, const char *address, int propagate)
{
    VSTRING *buffer = 0;
    const char *myname = "mail_addr_map";
    const char *string;
    char   *ratsign;
    char   *extension = 0;
    ARGV   *argv = 0;
    int     i;

    /*
     * Look up the full address; if no match is found, look up the address
     * with the extension stripped off, and remember the unmatched extension.
     */
    if ((string = mail_addr_find(path, address, &extension)) != 0) {

	/*
	 * Prepend the original user to @otherdomain, but do not propagate
	 * the unmatched address extension.
	 */
	if (*string == '@') {
	    buffer = vstring_alloc(100);
	    if ((ratsign = strrchr(address, '@')) != 0)
		vstring_strncpy(buffer, address, ratsign - address);
	    else
		vstring_strcpy(buffer, address);
	    if (extension)
		vstring_truncate(buffer, LEN(buffer) - strlen(extension));
	    vstring_strcat(buffer, string);
	    string = STR(buffer);
	}

	/*
	 * Canonicalize and externalize the result, and propagate the
	 * unmatched extension to each address found.
	 */
	argv = mail_addr_crunch(string, propagate ? extension : 0);
	if (buffer)
	    vstring_free(buffer);
	if (msg_verbose)
	    for (i = 0; i < argv->argc; i++)
		msg_info("%s: %s -> %d: %s", myname, address, i, argv->argv[i]);
	if (argv->argc == 0) {
	    msg_warn("%s lookup of %s returns non-address result \"%s\"",
		     path->title, address, string);
	    argv = argv_free(argv);
	    path->error = DICT_ERR_RETRY;
	}
    }

    /*
     * No match found.
     */
    else {
	if (msg_verbose)
	    msg_info("%s: %s -> %s", myname, address,
		     path->error ? "(try again)" : "(not found)");
    }

    /*
     * Cleanup.
     */
    if (extension)
	myfree(extension);

    return (argv);
}

#ifdef TEST

 /*
  * Proof-of-concept test program. Read an address from stdin, and spit out
  * the lookup result.
  */
#include <unistd.h>
#include <mail_conf.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <mail_params.h>

int     main(int argc, char **argv)
{
    VSTRING *buffer = vstring_alloc(100);
    MAPS   *path;
    ARGV   *result;

    /*
     * Parse JCL.
     */
    if (argc != 2)
	msg_fatal("usage: %s database", argv[0]);

    /*
     * Initialize.
     */
#define UPDATE(dst, src) { myfree(dst); dst = mystrdup(src); }

    mail_conf_read();
    msg_verbose = 1;
    if (chdir(var_queue_dir) < 0)
	msg_fatal("chdir %s: %m", var_queue_dir);
    path = maps_create(argv[0], argv[1], DICT_FLAG_LOCK | DICT_FLAG_FOLD_FIX);
    while (vstring_fgets_nonl(buffer, VSTREAM_IN)) {
	msg_info("=== Address extension on, extension propagation on ===");
	UPDATE(var_rcpt_delim, "+");
	if ((result = mail_addr_map(path, STR(buffer), 1)) != 0)
	    argv_free(result);
	msg_info("=== Address extension on, extension propagation off ===");
	if ((result = mail_addr_map(path, STR(buffer), 0)) != 0)
	    argv_free(result);
	msg_info("=== Address extension off ===");
	UPDATE(var_rcpt_delim, "");
	if ((result = mail_addr_map(path, STR(buffer), 1)) != 0)
	    argv_free(result);
    }
    vstring_free(buffer);
    maps_free(path);
    return (0);
}

#endif
