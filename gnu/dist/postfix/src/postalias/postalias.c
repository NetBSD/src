/*++
/* NAME
/*	postalias 1
/* SUMMARY
/*	Postfix alias database maintenance
/* SYNOPSIS
/* .fi
/*	\fBpostalias\fR [\fB-Nfinrvw\fR] [\fB-c \fIconfig_dir\fR]
/*		[\fB-d \fIkey\fR] [\fB-q \fIkey\fR]
/*		[\fIfile_type\fR:]\fIfile_name\fR ...
/* DESCRIPTION
/*	The \fBpostalias\fR command creates or queries one or more Postfix
/*	alias databases, or updates an existing one. The input and output
/*	file formats are expected to be compatible with Sendmail version 8,
/*	and are expected to be suitable for the use as NIS alias maps.
/*
/*	While a database update is in progress, signal delivery is
/*	postponed, and an exclusive, advisory, lock is placed on the
/*	entire database, in order to avoid surprises in spectator
/*	programs.
/*
/*	Options:
/* .IP \fB-N\fR
/*	Include the terminating null character that terminates lookup keys
/*	and values. By default, Postfix does whatever is the default for
/*	the host operating system.
/* .IP "\fB-c \fIconfig_dir\fR"
/*	Read the \fBmain.cf\fR configuration file in the named directory
/*	instead of the default configuration directory.
/* .IP "\fB-d \fIkey\fR"
/*	Search the specified maps for \fIkey\fR and remove one entry per map.
/*	The exit status is zero when the requested information was found.
/*
/*	If a key value of \fB-\fR is specified, the program reads key
/*	values from the standard input stream. The exit status is zero
/*	when at least one of the requested keys was found.
/* .IP \fB-f\fR
/*	Do not fold the lookup key to lower case while creating or querying
/*	a map.
/* .IP \fB-i\fR
/*	Incremental mode. Read entries from standard input and do not
/*	truncate an existing database. By default, \fBpostalias\fR creates
/*	a new database from the entries in \fBfile_name\fR.
/* .IP \fB-n\fR
/*	Don't include the terminating null character that terminates lookup
/*	keys and values. By default, Postfix does whatever is the default for
/*	the host operating system.
/* .IP "\fB-q \fIkey\fR"
/*	Search the specified maps for \fIkey\fR and print the first value
/*	found on the standard output stream. The exit status is zero
/*	when the requested information was found.
/*
/*	If a key value of \fB-\fR is specified, the program reads key
/*	values from the standard input stream and prints one line of
/*	\fIkey: value\fR output for each key that was found. The exit
/*	status is zero when at least one of the requested keys was found.
/* .IP \fB-r\fR
/*	When updating a table, do not warn about duplicate entries; silently
/*	replace them.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* .IP \fB-w\fR
/*	When updating a table, do not warn about duplicate entries; silently
/*	ignore them.
/* .PP
/*	Arguments:
/* .IP \fIfile_type\fR
/*	The type of database to be produced.
/* .RS
/* .IP \fBbtree\fR
/*	The output is a btree file, named \fIfile_name\fB.db\fR.
/*	This is available only on systems with support for \fBdb\fR databases.
/* .IP \fBdbm\fR
/*	The output consists of two files, named \fIfile_name\fB.pag\fR and
/*	\fIfile_name\fB.dir\fR.
/*	This is available only on systems with support for \fBdbm\fR databases.
/* .IP \fBhash\fR
/*	The output is a hashed file, named \fIfile_name\fB.db\fR.
/*	This is available only on systems with support for \fBdb\fR databases.
/* .PP
/*	When no \fIfile_type\fR is specified, the software uses the database
/*	type specified via the \fBdatabase_type\fR configuration parameter.
/*	The default value for this parameter depends on the host environment.
/* .RE
/* .IP \fIfile_name\fR
/*	The name of the alias database source file when rebuilding a database.
/* DIAGNOSTICS
/*	Problems are logged to the standard error stream. No output means
/*	no problems were detected. Duplicate entries are skipped and are
/*	flagged with a warning.
/*
/*	\fBpostalias\fR terminates with zero exit status in case of success
/*	(including successful \fBpostmap -q\fR lookup) and terminates
/*	with non-zero exit status in case of failure.
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP \fBMAIL_CONFIG\fR
/*	Directory with Postfix configuration files.
/* .IP \fBMAIL_VERBOSE\fR
/*	Enable verbose logging for debugging purposes.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program. See the Postfix \fBmain.cf\fR file for syntax details
/*	and for default values.
/* .IP \fBdatabase_type\fR
/*	Default alias database type. On many UNIX systems, the default type
/*	is either \fBdbm\fR or \fBhash\fR.
/* STANDARDS
/*	RFC 822 (ARPA Internet Text Messages)
/* SEE ALSO
/*	aliases(5) format of alias database input file.
/*	sendmail(1) mail posting and compatibility interface.
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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>

/* Utility library. */

#include <msg.h>
#include <mymalloc.h>
#include <vstring.h>
#include <vstream.h>
#include <msg_vstream.h>
#include <readlline.h>
#include <stringops.h>
#include <split_at.h>
#include <get_hostname.h>
#include <vstring_vstream.h>

/* Global library. */

#include <tok822.h>
#include <mail_conf.h>
#include <mail_params.h>
#include <mkmap.h>

/* Application-specific. */

#define STR	vstring_str

/* postalias - create or update alias database */

static void postalias(char *map_type, char *path_name,
		              int open_flags, int dict_flags)
{
    VSTREAM *source_fp;
    VSTRING *line_buffer;
    MKMAP  *mkmap;
    int     lineno;
    VSTRING *key_buffer;
    VSTRING *value_buffer;
    TOK822 *tok_list;
    TOK822 *key_list;
    TOK822 *colon;
    TOK822 *value_list;

    /*
     * Initialize.
     */
    line_buffer = vstring_alloc(100);
    key_buffer = vstring_alloc(100);
    value_buffer = vstring_alloc(100);
    if ((open_flags & O_TRUNC) == 0) {
	source_fp = VSTREAM_IN;
	vstream_control(source_fp, VSTREAM_CTL_PATH, "stdin", VSTREAM_CTL_END);
    } else if ((source_fp = vstream_fopen(path_name, O_RDONLY, 0)) == 0) {
	msg_fatal("open %s: %m", path_name);
    }

    /*
     * Open the database, create it when it does not exist, truncate it when
     * it does exist, and lock out any spectators.
     */
    mkmap = mkmap_open(map_type, path_name, open_flags, dict_flags);

    /*
     * Add records to the database.
     */
    lineno = 0;
    while (readlline(line_buffer, source_fp, &lineno)) {

	/*
	 * Tokenize the input, so that we do the right thing when a quoted
	 * localpart contains special characters such as "@", ":" and so on.
	 */
	if ((tok_list = tok822_scan(STR(line_buffer), (TOK822 **) 0)) == 0)
	    continue;

	/*
	 * Enforce the key:value format. Disallow missing keys, multi-address
	 * keys, or missing values. In order to specify an empty string or
	 * value, enclose it in double quotes.
	 */
	if ((colon = tok822_find_type(tok_list, ':')) == 0
	    || colon->prev == 0 || colon->next == 0
	    || tok822_rfind_type(colon, ',')) {
	    msg_warn("%s, line %d: need name:value pair",
		     VSTREAM_PATH(source_fp), lineno);
	    tok822_free_tree(tok_list);
	    continue;
	}

	/*
	 * Key must be local. XXX We should use the Postfix rewriting and
	 * resolving services to handle all address forms correctly. However,
	 * we can't count on the mail system being up when the alias database
	 * is being built, so we're guessing a bit.
	 */
	if (tok822_rfind_type(colon, '@') || tok822_rfind_type(colon, '%')) {
	    msg_warn("%s, line %d: name must be local",
		     VSTREAM_PATH(source_fp), lineno);
	    tok822_free_tree(tok_list);
	    continue;
	}

	/*
	 * Split the input into key and value parts, and convert from token
	 * representation back to string representation. Convert the key to
	 * internal (unquoted) form, because the resolver produces addresses
	 * in internal form. Convert the value to external (quoted) form,
	 * because it will have to be re-parsed upon lookup. Discard the
	 * token representation when done.
	 */
	key_list = tok_list;
	tok_list = 0;
	value_list = tok822_cut_after(colon);
	tok822_unlink(colon);
	tok822_free(colon);

	tok822_internalize(key_buffer, key_list, TOK822_STR_DEFL);
	tok822_free_tree(key_list);

	tok822_externalize(value_buffer, value_list, TOK822_STR_DEFL);
	tok822_free_tree(value_list);

	/*
	 * Store the value under a case-insensitive key.
	 */
	if (dict_flags & DICT_FLAG_FOLD_KEY)
	    lowercase(STR(key_buffer));
	mkmap_append(mkmap, STR(key_buffer), STR(value_buffer));
    }

    /*
     * Update or append sendmail and NIS signatures.
     */
    if ((open_flags & O_TRUNC) == 0)
	mkmap->dict->flags |= DICT_FLAG_DUP_REPLACE;

    /*
     * Sendmail compatibility: add the @:@ signature to indicate that the
     * database is complete. This might be needed by NIS clients running
     * sendmail.
     */
    mkmap_append(mkmap, "@", "@");

    /*
     * NIS compatibility: add time and master info. Unlike other information,
     * this information MUST be written without a trailing null appended to
     * key or value.
     */
    mkmap->dict->flags &= ~DICT_FLAG_TRY1NULL;
    mkmap->dict->flags |= DICT_FLAG_TRY0NULL;
    vstring_sprintf(value_buffer, "%010ld", (long) time((time_t *) 0));
    mkmap_append(mkmap, "YP_LAST_MODIFIED", STR(value_buffer));
    mkmap_append(mkmap, "YP_MASTER_NAME", get_hostname());

    /*
     * Close the alias database, and release the lock.
     */
    mkmap_close(mkmap);

    /*
     * Cleanup. We're about to terminate, but it is a good sanity check.
     */
    vstring_free(value_buffer);
    vstring_free(key_buffer);
    vstring_free(line_buffer);
    if (source_fp != VSTREAM_IN)
	vstream_fclose(source_fp);
}

/* postalias_queries - apply multiple requests from stdin */

static int postalias_queries(VSTREAM *in, char **maps, const int map_count,
			             const int dict_flags)
{
    int     found = 0;
    VSTRING *keybuf = vstring_alloc(100);
    DICT  **dicts;
    const char *map_name;
    const char *value;
    int     n;

    /*
     * Sanity check.
     */
    if (map_count <= 0)
	msg_panic("postalias_queries: bad map count");

    /*
     * Prepare to open maps lazily.
     */
    dicts = (DICT **) mymalloc(sizeof(*dicts) * map_count);
    for (n = 0; n < map_count; n++)
	dicts[n] = 0;

    /*
     * Perform all queries. Open maps on the fly, to avoid opening unecessary
     * maps.
     */
    while (vstring_get_nonl(keybuf, in) != VSTREAM_EOF) {
	if (dict_flags & DICT_FLAG_FOLD_KEY)
	    lowercase(STR(keybuf));
	for (n = 0; n < map_count; n++) {
	    if (dicts[n] == 0)
		dicts[n] = ((map_name = split_at(maps[n], ':')) != 0 ?
		   dict_open3(maps[n], map_name, O_RDONLY, DICT_FLAG_LOCK) :
		dict_open3(var_db_type, maps[n], O_RDONLY, DICT_FLAG_LOCK));
	    if ((value = dict_get(dicts[n], STR(keybuf))) != 0) {
		vstream_printf("%s:	%s\n", STR(keybuf), value);
		found = 1;
		break;
	    }
	}
    }
    if (found)
	vstream_fflush(VSTREAM_OUT);

    /*
     * Cleanup.
     */
    for (n = 0; n < map_count; n++)
	if (dicts[n])
	    dict_close(dicts[n]);
    myfree((char *) dicts);
    vstring_free(keybuf);

    return (found);
}

/* postalias_query - query a map and print the result to stdout */

static int postalias_query(const char *map_type, const char *map_name,
			           const char *key)
{
    DICT   *dict;
    const char *value;

    dict = dict_open3(map_type, map_name, O_RDONLY, DICT_FLAG_LOCK);
    if ((value = dict_get(dict, key)) != 0) {
	vstream_printf("%s\n", value);
	vstream_fflush(VSTREAM_OUT);
    }
    dict_close(dict);
    return (value != 0);
}

/* postalias_deletes - apply multiple requests from stdin */

static int postalias_deletes(VSTREAM *in, char **maps, const int map_count)
{
    int     found = 0;
    VSTRING *keybuf = vstring_alloc(100);
    DICT  **dicts;
    const char *map_name;
    int     n;

    /*
     * Sanity check.
     */
    if (map_count <= 0)
	msg_panic("postalias_deletes: bad map count");

    /*
     * Open maps ahead of time.
     */
    dicts = (DICT **) mymalloc(sizeof(*dicts) * map_count);
    for (n = 0; n < map_count; n++)
	dicts[n] = ((map_name = split_at(maps[n], ':')) != 0 ?
		    dict_open3(maps[n], map_name, O_RDWR, DICT_FLAG_LOCK) :
		  dict_open3(var_db_type, maps[n], O_RDWR, DICT_FLAG_LOCK));

    /*
     * Perform all requests.
     */
    while (vstring_get_nonl(keybuf, in) != VSTREAM_EOF)
	for (n = 0; n < map_count; n++)
	    found |= (dict_del(dicts[n], STR(keybuf)) == 0);

    /*
     * Cleanup.
     */
    for (n = 0; n < map_count; n++)
	if (dicts[n])
	    dict_close(dicts[n]);
    myfree((char *) dicts);
    vstring_free(keybuf);

    return (found);
}

/* postalias_delete - delete a key value pair from a map */

static int postalias_delete(const char *map_type, const char *map_name,
			            const char *key)
{
    DICT   *dict;
    int     status;

    dict = dict_open3(map_type, map_name, O_RDWR, DICT_FLAG_LOCK);
    status = dict_del(dict, key);
    dict_close(dict);
    return (status == 0);
}

/* usage - explain */

static NORETURN usage(char *myname)
{
    msg_fatal("usage: %s [-Nfinrvw] [-c config_dir] [-d key] [-q key] [map_type:]file...",
	      myname);
}

int     main(int argc, char **argv)
{
    char   *path_name;
    int     ch;
    int     fd;
    char   *slash;
    struct stat st;
    int     open_flags = O_RDWR | O_CREAT | O_TRUNC;
    int     dict_flags = DICT_FLAG_DUP_WARN | DICT_FLAG_FOLD_KEY;
    char   *query = 0;
    char   *delkey = 0;
    int     found;

    /*
     * Be consistent with file permissions.
     */
    umask(022);

    /*
     * To minimize confusion, make sure that the standard file descriptors
     * are open before opening anything else. XXX Work around for 44BSD where
     * fstat can return EBADF on an open file descriptor.
     */
    for (fd = 0; fd < 3; fd++)
	if (fstat(fd, &st) == -1
	    && (close(fd), open("/dev/null", O_RDWR, 0)) != fd)
	    msg_fatal("open /dev/null: %m");

    /*
     * Process environment options as early as we can. We are not set-uid,
     * and we are supposed to be running in a controlled environment.
     */
    if (getenv(CONF_ENV_VERB))
	msg_verbose = 1;

    /*
     * Initialize. Set up logging, read the global configuration file and
     * extract configuration information.
     */
    if ((slash = strrchr(argv[0], '/')) != 0)
	argv[0] = slash + 1;
    msg_vstream_init(argv[0], VSTREAM_ERR);

    /*
     * Parse JCL.
     */
    while ((ch = GETOPT(argc, argv, "Nc:d:finq:rvw")) > 0) {
	switch (ch) {
	default:
	    usage(argv[0]);
	    break;
	case 'N':
	    dict_flags |= DICT_FLAG_TRY1NULL;
	    dict_flags &= ~DICT_FLAG_TRY0NULL;
	    break;
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'd':
	    if (query || delkey)
		msg_fatal("specify only one of -q or -d");
	    delkey = optarg;
	    break;
	case 'f':
	    dict_flags &= ~DICT_FLAG_FOLD_KEY;
	    break;
	case 'i':
	    open_flags &= ~O_TRUNC;
	    break;
	case 'n':
	    dict_flags |= DICT_FLAG_TRY0NULL;
	    dict_flags &= ~DICT_FLAG_TRY1NULL;
	    break;
	case 'q':
	    if (query || delkey)
		msg_fatal("specify only one of -q or -d");
	    query = optarg;
	    break;
	case 'r':
	    dict_flags &= ~(DICT_FLAG_DUP_WARN | DICT_FLAG_DUP_IGNORE);
	    dict_flags |= DICT_FLAG_DUP_REPLACE;
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	case 'w':
	    dict_flags &= ~(DICT_FLAG_DUP_WARN | DICT_FLAG_DUP_REPLACE);
	    dict_flags |= DICT_FLAG_DUP_IGNORE;
	    break;
	}
    }
    mail_conf_read();

    /*
     * Use the map type specified by the user, or fall back to a default
     * database type.
     */
    if (delkey) {				/* remove entry */
	if (optind + 1 > argc)
	    usage(argv[0]);
	if (strcmp(delkey, "-") == 0)
	    exit(postalias_deletes(VSTREAM_IN, argv + optind, argc - optind) == 0);
	found = 0;
	while (optind < argc) {
	    if ((path_name = split_at(argv[optind], ':')) != 0) {
		found |= postalias_delete(argv[optind], path_name, delkey);
	    } else {
		found |= postalias_delete(var_db_type, argv[optind], delkey);
	    }
	    optind++;
	}
	exit(found ? 0 : 1);
    } else if (query) {				/* query map(s) */
	if (optind + 1 > argc)
	    usage(argv[0]);
	if (strcmp(query, "-") == 0)
	    exit(postalias_queries(VSTREAM_IN, argv + optind, argc - optind,
				   dict_flags) == 0);
	if (dict_flags & DICT_FLAG_FOLD_KEY)
	    lowercase(query);
	while (optind < argc) {
	    if ((path_name = split_at(argv[optind], ':')) != 0) {
		found = postalias_query(argv[optind], path_name, query);
	    } else {
		found = postalias_query(var_db_type, argv[optind], query);
	    }
	    if (found)
		exit(0);
	    optind++;
	}
	exit(1);
    } else {					/* create/update map(s) */
	if (optind + 1 > argc)
	    usage(argv[0]);
	while (optind < argc) {
	    if ((path_name = split_at(argv[optind], ':')) != 0) {
		postalias(argv[optind], path_name, open_flags, dict_flags);
	    } else {
		postalias(var_db_type, argv[optind], open_flags, dict_flags);
	    }
	    optind++;
	}
	exit(0);
    }
}
