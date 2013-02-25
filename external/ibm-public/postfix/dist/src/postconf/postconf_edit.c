/*	$NetBSD: postconf_edit.c,v 1.1.1.1.6.2 2013/02/25 00:27:22 tls Exp $	*/

/*++
/* NAME
/*	postconf_edit 3
/* SUMMARY
/*	edit main.cf
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	edit_parameters(mode, argc, argv)
/*	int	mode;
/*	int	argc;
/*	char	**argv;
/* DESCRIPTION
/*	Edit the \fBmain.cf\fR configuration file, and update
/*	parameter settings with the "\fIname\fR=\fIvalue\fR" pairs
/*	given on the command line. The file is copied to a temporary
/*	file then renamed into place.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
/* FILES
/*	/etc/postfix/main.cf, Postfix configuration parameters
/*	/etc/postfix/main.cf.tmp, temporary name
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
#include <ctype.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <vstring.h>
#include <vstring_vstream.h>
#include <edit_file.h>
#include <readlline.h>
#include <stringops.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include <postconf.h>

#define STR(x) vstring_str(x)

/* edit_parameters - edit parameter file */

void    edit_parameters(int mode, int argc, char **argv)
{
    char   *path;
    EDIT_FILE *ep;
    VSTREAM *src;
    VSTREAM *dst;
    VSTRING *buf = vstring_alloc(100);
    VSTRING *key = vstring_alloc(10);
    char   *cp;
    char   *edit_key;
    char   *edit_val;
    HTABLE *table;
    struct cvalue {
	char   *value;
	int     found;
    };
    struct cvalue *cvalue;
    HTABLE_INFO **ht_info;
    HTABLE_INFO **ht;
    int     interesting;
    const char *err;

    /*
     * Store command-line parameters for quick lookup.
     */
    table = htable_create(argc);
    while ((cp = *argv++) != 0) {
	if (strchr(cp, '\n') != 0)
	    msg_fatal("-e or -# accepts no multi-line input");
	while (ISSPACE(*cp))
	    cp++;
	if (*cp == '#')
	    msg_fatal("-e or -# accepts no comment input");
	if (mode & EDIT_MAIN) {
	    if ((err = split_nameval(cp, &edit_key, &edit_val)) != 0)
		msg_fatal("%s: \"%s\"", err, cp);
	} else if (mode & COMMENT_OUT) {
	    if (*cp == 0)
		msg_fatal("-# requires non-blank parameter names");
	    if (strchr(cp, '=') != 0)
		msg_fatal("-# requires parameter names only");
	    edit_key = mystrdup(cp);
	    trimblanks(edit_key, 0);
	    edit_val = 0;
	} else {
	    msg_panic("edit_parameters: unknown mode %d", mode);
	}
	cvalue = (struct cvalue *) mymalloc(sizeof(*cvalue));
	cvalue->value = edit_val;
	cvalue->found = 0;
	htable_enter(table, edit_key, (char *) cvalue);
    }

    /*
     * Open a temp file for the result. This uses a deterministic name so we
     * don't leave behind thrash with random names.
     */
    set_config_dir();
    path = concatenate(var_config_dir, "/", MAIN_CONF_FILE, (char *) 0);
    if ((ep = edit_file_open(path, O_CREAT | O_WRONLY, 0644)) == 0)
	msg_fatal("open %s%s: %m", path, EDIT_FILE_SUFFIX);
    dst = ep->tmp_fp;

    /*
     * Open the original file for input.
     */
    if ((src = vstream_fopen(path, O_RDONLY, 0)) == 0) {
	/* OK to delete, since we control the temp file name exclusively. */
	(void) unlink(ep->tmp_path);
	msg_fatal("open %s for reading: %m", path);
    }

    /*
     * Copy original file to temp file, while replacing parameters on the
     * fly. Issue warnings for names found multiple times.
     */
#define STR(x) vstring_str(x)

    interesting = 0;
    while (vstring_get(buf, src) != VSTREAM_EOF) {
	for (cp = STR(buf); ISSPACE(*cp) /* including newline */ ; cp++)
	     /* void */ ;
	/* Copy comment, all-whitespace, or empty line. */
	if (*cp == '#' || *cp == 0) {
	    vstream_fputs(STR(buf), dst);
	}
	/* Copy, skip or replace continued text. */
	else if (cp > STR(buf)) {
	    if (interesting == 0)
		vstream_fputs(STR(buf), dst);
	    else if (mode & COMMENT_OUT)
		vstream_fprintf(dst, "#%s", STR(buf));
	}
	/* Copy or replace start of logical line. */
	else {
	    vstring_strncpy(key, cp, strcspn(cp, " \t\r\n="));
	    cvalue = (struct cvalue *) htable_find(table, STR(key));
	    if ((interesting = !!cvalue) != 0) {
		if (cvalue->found++ == 1)
		    msg_warn("%s: multiple entries for \"%s\"", path, STR(key));
		if (mode & EDIT_MAIN)
		    vstream_fprintf(dst, "%s = %s\n", STR(key), cvalue->value);
		else if (mode & COMMENT_OUT)
		    vstream_fprintf(dst, "#%s", cp);
		else
		    msg_panic("edit_parameters: unknown mode %d", mode);
	    } else {
		vstream_fputs(STR(buf), dst);
	    }
	}
    }

    /*
     * Generate new entries for parameters that were not found.
     */
    if (mode & EDIT_MAIN) {
	for (ht_info = ht = htable_list(table); *ht; ht++) {
	    cvalue = (struct cvalue *) ht[0]->value;
	    if (cvalue->found == 0)
		vstream_fprintf(dst, "%s = %s\n", ht[0]->key, cvalue->value);
	}
	myfree((char *) ht_info);
    }

    /*
     * When all is well, rename the temp file to the original one.
     */
    if (vstream_fclose(src))
	msg_fatal("read %s: %m", path);
    if (edit_file_close(ep) != 0)
	msg_fatal("close %s%s: %m", path, EDIT_FILE_SUFFIX);

    /*
     * Cleanup.
     */
    myfree(path);
    vstring_free(buf);
    vstring_free(key);
    htable_free(table, myfree);
}
