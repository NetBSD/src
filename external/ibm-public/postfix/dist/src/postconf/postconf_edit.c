/*	$NetBSD: postconf_edit.c,v 1.1.1.2.2.1 2014/08/10 07:12:49 tls Exp $	*/

/*++
/* NAME
/*	postconf_edit 3
/* SUMMARY
/*	edit main.cf or master.cf
/* SYNOPSIS
/*	#include <postconf.h>
/*
/*	void	pcf_edit_main(mode, argc, argv)
/*	int	mode;
/*	int	argc;
/*	char	**argv;
/*
/*	void	pcf_edit_master(mode, argc, argv)
/*	int	mode;
/*	int	argc;
/*	char	**argv;
/* DESCRIPTION
/*	pcf_edit_main() edits the \fBmain.cf\fR configuration file.
/*	It replaces or adds parameter settings given as "\fIname=value\fR"
/*	pairs given on the command line, or removes parameter
/*	settings given as "\fIname\fR" on the command line.  The
/*	file is copied to a temporary file then renamed into place.
/*
/*	pcf_edit_master() edits the \fBmaster.cf\fR configuration
/*	file.  The file is copied to a temporary file then renamed
/*	into place. Depending on the flags in \fBmode\fR:
/* .IP PCF_MASTER_ENTRY
/*	With PCF_EDIT_CONF, pcf_edit_master() replaces or adds
/*	entire master.cf entries, specified on the command line as
/*	"\fIname/type = name type private unprivileged chroot wakeup
/*	process_limit command...\fR".
/*
/*	With PCF_EDIT_EXCL or PCF_COMMENT_OUT, pcf_edit_master()
/*	removes or comments out entries specified on the command
/*	line as "\fIname/type\fR.
/* .IP PCF_MASTER_FLD
/*	With PCF_EDIT_CONF, pcf_edit_master() replaces the value
/*	of specific service attributes, specified on the command
/*	line as "\fIname/type/attribute = value\fR".
/* .IP PCF_MASTER_PARAM
/*	With PCF_EDIT_CONF, pcf_edit_master() replaces or adds the
/*	value of service parameters, specified on the command line
/*	as "\fIname/type/parameter = value\fR".
/*
/*	With PCF_EDIT_EXCL, pcf_edit_master() removes service
/*	parameters specified on the command line as "\fIparametername\fR".
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
/* FILES
/*	/etc/postfix/main.cf, Postfix configuration parameters
/*	/etc/postfix/main.cf.tmp, temporary name
/*	/etc/postfix/master.cf, Postfix configuration parameters
/*	/etc/postfix/master.cf.tmp, temporary name
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
#include <split_at.h>

/* Global library. */

#include <mail_params.h>

/* Application-specific. */

#include <postconf.h>

#define STR(x) vstring_str(x)

/* pcf_find_cf_info - pass-through non-content line, return content or null */

static char *pcf_find_cf_info(VSTRING *buf, VSTREAM *dst)
{
    char   *cp;

    for (cp = STR(buf); ISSPACE(*cp) /* including newline */ ; cp++)
	 /* void */ ;
    /* Pass-through comment, all-whitespace, or empty line. */
    if (*cp == '#' || *cp == 0) {
	vstream_fputs(STR(buf), dst);
	return (0);
    } else {
	return (cp);
    }
}

/* pcf_next_cf_line - return next content line, pass non-content */

static char *pcf_next_cf_line(VSTRING *buf, VSTREAM *src, VSTREAM *dst, int *lineno)
{
    char   *cp;

    while (vstring_get(buf, src) != VSTREAM_EOF) {
	if (lineno)
	    *lineno += 1;
	if ((cp = pcf_find_cf_info(buf, dst)) != 0)
	    return (cp);
    }
    return (0);
}

/* pcf_gobble_cf_line - accumulate multi-line content, pass non-content */

static void pcf_gobble_cf_line(VSTRING *full_entry_buf, VSTRING *line_buf,
			            VSTREAM *src, VSTREAM *dst, int *lineno)
{
    int     ch;

    vstring_strcpy(full_entry_buf, STR(line_buf));
    for (;;) {
	if ((ch = VSTREAM_GETC(src)) != VSTREAM_EOF)
	    vstream_ungetc(src, ch);
	if ((ch != '#' && !ISSPACE(ch))
	    || vstring_get(line_buf, src) == VSTREAM_EOF)
	    break;
	lineno += 1;
	if (pcf_find_cf_info(line_buf, dst))
	    vstring_strcat(full_entry_buf, STR(line_buf));
    }
}

/* pcf_edit_main - edit main.cf file */

void    pcf_edit_main(int mode, int argc, char **argv)
{
    char   *path;
    EDIT_FILE *ep;
    VSTREAM *src;
    VSTREAM *dst;
    VSTRING *buf = vstring_alloc(100);
    VSTRING *key = vstring_alloc(10);
    char   *cp;
    char   *pattern;
    char   *edit_value;
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
	    msg_fatal("-e, -X, or -# accepts no multi-line input");
	while (ISSPACE(*cp))
	    cp++;
	if (*cp == '#')
	    msg_fatal("-e, -X, or -# accepts no comment input");
	if (mode & PCF_EDIT_CONF) {
	    if ((err = split_nameval(cp, &pattern, &edit_value)) != 0)
		msg_fatal("%s: \"%s\"", err, cp);
	} else if (mode & (PCF_COMMENT_OUT | PCF_EDIT_EXCL)) {
	    if (*cp == 0)
		msg_fatal("-X or -# requires non-blank parameter names");
	    if (strchr(cp, '=') != 0)
		msg_fatal("-X or -# requires parameter names without value");
	    pattern = cp;
	    trimblanks(pattern, 0);
	    edit_value = 0;
	} else {
	    msg_panic("pcf_edit_main: unknown mode %d", mode);
	}
	cvalue = (struct cvalue *) mymalloc(sizeof(*cvalue));
	cvalue->value = edit_value;
	cvalue->found = 0;
	htable_enter(table, pattern, (char *) cvalue);
    }

    /*
     * Open a temp file for the result. This uses a deterministic name so we
     * don't leave behind thrash with random names.
     */
    pcf_set_config_dir();
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
    while ((cp = pcf_next_cf_line(buf, src, dst, (int *) 0)) != 0) {
	/* Copy, skip or replace continued text. */
	if (cp > STR(buf)) {
	    if (interesting == 0)
		vstream_fputs(STR(buf), dst);
	    else if (mode & PCF_COMMENT_OUT)
		vstream_fprintf(dst, "#%s", STR(buf));
	}
	/* Copy or replace start of logical line. */
	else {
	    vstring_strncpy(key, cp, strcspn(cp, " \t\r\n="));
	    cvalue = (struct cvalue *) htable_find(table, STR(key));
	    if ((interesting = !!cvalue) != 0) {
		if (cvalue->found++ == 1)
		    msg_warn("%s: multiple entries for \"%s\"", path, STR(key));
		if (mode & PCF_EDIT_CONF)
		    vstream_fprintf(dst, "%s = %s\n", STR(key), cvalue->value);
		else if (mode & PCF_COMMENT_OUT)
		    vstream_fprintf(dst, "#%s", cp);
	    } else {
		vstream_fputs(STR(buf), dst);
	    }
	}
    }

    /*
     * Generate new entries for parameters that were not found.
     */
    if (mode & PCF_EDIT_CONF) {
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

 /*
  * Data structure to hold a master.cf edit request.
  */
typedef struct {
    int     match_count;		/* hit count */
    const char *raw_text;		/* unparsed command-line argument */
    char   *parsed_text;		/* destructive parse */
    ARGV   *service_pattern;		/* service name, type, ... */
    int     field_number;		/* attribute field number */
    const char *param_pattern;		/* parameter name */
    char   *edit_value;			/* value substring */
} PCF_MASTER_EDIT_REQ;

/* pcf_edit_master - edit master.cf file */

void    pcf_edit_master(int mode, int argc, char **argv)
{
    const char *myname = "pcf_edit_master";
    char   *path;
    EDIT_FILE *ep;
    VSTREAM *src;
    VSTREAM *dst;
    VSTRING *line_buf = vstring_alloc(100);
    VSTRING *parse_buf = vstring_alloc(100);
    int     lineno;
    PCF_MASTER_ENT *new_entry;
    VSTRING *full_entry_buf = vstring_alloc(100);
    char   *cp;
    char   *pattern;
    int     service_name_type_matched;
    const char *err;
    PCF_MASTER_EDIT_REQ *edit_reqs;
    PCF_MASTER_EDIT_REQ *req;
    int     num_reqs = argc;
    const char *edit_opts = "-Me, -Fe, -Pe, -X, or -#";
    char   *service_name;
    char   *service_type;

    /*
     * Sanity check.
     */
    if (num_reqs <= 0)
	msg_panic("%s: empty argument list", myname);

    /*
     * Preprocessing: split pattern=value, then split the pattern components.
     */
    edit_reqs = (PCF_MASTER_EDIT_REQ *) mymalloc(sizeof(*edit_reqs) * num_reqs);
    for (req = edit_reqs; *argv != 0; req++, argv++) {
	req->match_count = 0;
	req->raw_text = *argv;
	cp = req->parsed_text = mystrdup(req->raw_text);
	if (strchr(cp, '\n') != 0)
	    msg_fatal("%s accept no multi-line input", edit_opts);
	while (ISSPACE(*cp))
	    cp++;
	if (*cp == '#')
	    msg_fatal("%s accept no comment input", edit_opts);
	/* Separate the pattern from the value. */
	if (mode & PCF_EDIT_CONF) {
	    if ((err = split_nameval(cp, &pattern, &req->edit_value)) != 0)
		msg_fatal("%s: \"%s\"", err, req->raw_text);
	    if ((mode & PCF_MASTER_PARAM)
	    && req->edit_value[strcspn(req->edit_value, PCF_MASTER_BLANKS)])
		msg_fatal("whitespace in parameter value: \"%s\"",
			  req->raw_text);
	} else if (mode & (PCF_COMMENT_OUT | PCF_EDIT_EXCL)) {
	    if (strchr(cp, '=') != 0)
		msg_fatal("-X or -# requires names without value");
	    pattern = cp;
	    trimblanks(pattern, 0);
	    req->edit_value = 0;
	} else {
	    msg_panic("%s: unknown mode %d", myname, mode);
	}

#define PCF_MASTER_MASK (PCF_MASTER_ENTRY | PCF_MASTER_FLD | PCF_MASTER_PARAM)

	/*
	 * Split name/type or name/type/whatever pattern into components.
	 */
	switch (mode & PCF_MASTER_MASK) {
	case PCF_MASTER_ENTRY:
	    if ((req->service_pattern =
		 pcf_parse_service_pattern(pattern, 2, 2)) == 0)
		msg_fatal("-Me, -MX or -M# requires service_name/type");
	    break;
	case PCF_MASTER_FLD:
	    if ((req->service_pattern =
		 pcf_parse_service_pattern(pattern, 3, 3)) == 0)
		msg_fatal("-Fe or -FX requires service_name/type/field_name");
	    req->field_number =
		pcf_parse_field_pattern(req->service_pattern->argv[2]);
	    if (pcf_is_magic_field_pattern(req->field_number))
		msg_fatal("-Fe does not accept wild-card field name");
	    if ((mode & PCF_EDIT_CONF)
		&& req->field_number < PCF_MASTER_FLD_CMD
	    && req->edit_value[strcspn(req->edit_value, PCF_MASTER_BLANKS)])
		msg_fatal("-Fe does not accept whitespace in non-command field");
	    break;
	case PCF_MASTER_PARAM:
	    if ((req->service_pattern =
		 pcf_parse_service_pattern(pattern, 3, 3)) == 0)
		msg_fatal("-Pe or -PX requires service_name/type/parameter");
	    req->param_pattern = req->service_pattern->argv[2];
	    if (PCF_IS_MAGIC_PARAM_PATTERN(req->param_pattern))
		msg_fatal("-Pe does not accept wild-card parameter name");
	    if ((mode & PCF_EDIT_CONF)
	    && req->edit_value[strcspn(req->edit_value, PCF_MASTER_BLANKS)])
		msg_fatal("-Pe does not accept whitespace in parameter value");
	    break;
	default:
	    msg_panic("%s: unknown edit mode %d", myname, mode);
	}
    }

    /*
     * Open a temp file for the result. This uses a deterministic name so we
     * don't leave behind thrash with random names.
     */
    pcf_set_config_dir();
    path = concatenate(var_config_dir, "/", MASTER_CONF_FILE, (char *) 0);
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
     * Copy original file to temp file, while replacing service entries on
     * the fly.
     */
    service_name_type_matched = 0;
    new_entry = 0;
    lineno = 0;
    while ((cp = pcf_next_cf_line(parse_buf, src, dst, &lineno)) != 0) {
	vstring_strcpy(line_buf, STR(parse_buf));

	/*
	 * Copy, skip or replace continued text.
	 */
	if (cp > STR(parse_buf)) {
	    if (service_name_type_matched == 0)
		vstream_fputs(STR(line_buf), dst);
	    else if (mode & PCF_COMMENT_OUT)
		vstream_fprintf(dst, "#%s", STR(line_buf));
	}

	/*
	 * Copy or replace (start of) logical line.
	 */
	else {
	    service_name_type_matched = 0;

	    /*
	     * Parse out the service name and type.
	     */
	    if ((service_name = mystrtok(&cp, PCF_MASTER_BLANKS)) == 0
		|| (service_type = mystrtok(&cp, PCF_MASTER_BLANKS)) == 0)
		msg_fatal("file %s: line %d: specify service name and type "
			  "on the same line", path, lineno);
	    if (strchr(service_name, '='))
		msg_fatal("file %s: line %d: service name syntax \"%s\" is "
			  "unsupported with %s", path, lineno, service_name,
			  edit_opts);
	    if (service_type[strcspn(service_type, "=/")] != 0)
		msg_fatal("file %s: line %d: "
			"service type syntax \"%s\" is unsupported with %s",
			  path, lineno, service_type, edit_opts);

	    /*
	     * Match each service pattern.
	     */
	    for (req = edit_reqs; req < edit_reqs + num_reqs; req++) {
		if (PCF_MATCH_SERVICE_PATTERN(req->service_pattern,
					      service_name,
					      service_type)) {
		    service_name_type_matched = 1;	/* Sticky flag */
		    req->match_count += 1;

		    /*
		     * Generate replacement master.cf entries.
		     */
		    if ((mode & PCF_EDIT_CONF)
			|| ((mode & PCF_MASTER_PARAM) && (mode & PCF_EDIT_EXCL))) {
			switch (mode & PCF_MASTER_MASK) {

			    /*
			     * Replace master.cf entry field or parameter
			     * value.
			     */
			case PCF_MASTER_FLD:
			case PCF_MASTER_PARAM:
			    if (new_entry == 0) {
				/* Gobble up any continuation lines. */
				pcf_gobble_cf_line(full_entry_buf, line_buf,
						   src, dst, &lineno);
				new_entry = (PCF_MASTER_ENT *)
				    mymalloc(sizeof(*new_entry));
				if ((err = pcf_parse_master_entry(new_entry,
						 STR(full_entry_buf))) != 0)
				    msg_fatal("file %s: line %d: %s",
					      path, lineno, err);
			    }
			    if (mode & PCF_MASTER_FLD) {
				pcf_edit_master_field(new_entry,
						      req->field_number,
						      req->edit_value);
			    } else {
				pcf_edit_master_param(new_entry, mode,
						      req->param_pattern,
						      req->edit_value);
			    }
			    break;

			    /*
			     * Replace entire master.cf entry.
			     */
			case PCF_MASTER_ENTRY:
			    if (new_entry != 0)
				pcf_free_master_entry(new_entry);
			    new_entry = (PCF_MASTER_ENT *)
				mymalloc(sizeof(*new_entry));
			    if ((err = pcf_parse_master_entry(new_entry,
						     req->edit_value)) != 0)
				msg_fatal("%s: \"%s\"", err, req->raw_text);
			    break;
			default:
			    msg_panic("%s: unknown edit mode %d", myname, mode);
			}
		    }
		}
	    }

	    /*
	     * Pass through or replace the current input line.
	     */
	    if (new_entry) {
		pcf_print_master_entry(dst, PCF_FOLD_LINE, new_entry);
		pcf_free_master_entry(new_entry);
		new_entry = 0;
	    } else if (service_name_type_matched == 0) {
		vstream_fputs(STR(line_buf), dst);
	    } else if (mode & PCF_COMMENT_OUT) {
		vstream_fprintf(dst, "#%s", STR(line_buf));
	    }
	}
    }

    /*
     * Postprocessing: when editing entire service entries, generate new
     * entries for services not found. Otherwise (editing fields or
     * parameters), "service not found" is a fatal error.
     */
    for (req = edit_reqs; req < edit_reqs + num_reqs; req++) {
	if (req->match_count == 0) {
	    if ((mode & PCF_MASTER_ENTRY) && (mode & PCF_EDIT_CONF)) {
		new_entry = (PCF_MASTER_ENT *) mymalloc(sizeof(*new_entry));
		if ((err = pcf_parse_master_entry(new_entry, req->edit_value)) != 0)
		    msg_fatal("%s: \"%s\"", err, req->raw_text);
		pcf_print_master_entry(dst, PCF_FOLD_LINE, new_entry);
		pcf_free_master_entry(new_entry);
	    } else if ((mode & PCF_MASTER_ENTRY) == 0) {
		msg_warn("unmatched service_name/type: \"%s\"", req->raw_text);
	    }
	}
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
    vstring_free(line_buf);
    vstring_free(parse_buf);
    vstring_free(full_entry_buf);
    for (req = edit_reqs; req < edit_reqs + num_reqs; req++) {
	argv_free(req->service_pattern);
	myfree(req->parsed_text);
    }
    myfree((char *) edit_reqs);
}
