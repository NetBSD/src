/*++
/* NAME
/*	postmap 1
/* SUMMARY
/*	Postfix lookup table management
/* SYNOPSIS
/* .fi
/*	\fBpostmap\fR [\fB-Nfinorvw\fR] [\fB-c \fIconfig_dir\fR]
/*		[\fB-d \fIkey\fR] [\fB-q \fIkey\fR]
/*		[\fIfile_type\fR:]\fIfile_name\fR ...
/* DESCRIPTION
/*	The \fBpostmap\fR command creates or queries one or more Postfix
/*	lookup tables, or updates an existing one. The input and output
/*	file formats are expected to be compatible with:
/*
/* .ti +4
/*	\fBmakemap \fIfile_type\fR \fIfile_name\fR < \fIfile_name\fR
/*
/*	If the result files do not exist they will be created with the
/*	same group and other read permissions as the source file.
/*
/*	While the table update is in progress, signal delivery is
/*	postponed, and an exclusive, advisory, lock is placed on the
/*	entire table, in order to avoid surprises in spectator
/*	programs.
/*
/*	The format of a lookup table input file is as follows:
/* .IP \(bu
/*	A table entry has the form
/* .sp
/* .ti +5
/*	\fIkey\fR whitespace \fIvalue\fR
/* .IP \(bu
/*	Empty lines and whitespace-only lines are ignored, as
/*	are lines whose first non-whitespace character is a `#'.
/* .IP \(bu
/*	A logical line starts with non-whitespace text. A line that
/*	starts with whitespace continues a logical line.
/* .PP
/*	The \fIkey\fR and \fIvalue\fR are processed as is, except that
/*	surrounding white space is stripped off. Unlike with Postfix alias
/*	databases, quotes cannot be used to protect lookup keys that contain
/*	special characters such as `#' or whitespace. The \fIkey\fR is mapped
/*	to lowercase to make mapping lookups case insensitive.
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
/*	truncate an existing database. By default, \fBpostmap\fR creates
/*	a new database from the entries in \fBfile_name\fR.
/* .IP \fB-n\fR
/*	Don't include the terminating null character that terminates lookup
/*	keys and values. By default, Postfix does whatever is the default for
/*	the host operating system.
/* .IP \fB-o\fR
/*	Do not release root privileges when processing a non-root
/*	input file. By default, \fBpostmap\fR drops root privileges
/*	and runs as the source file owner instead.
/* .IP "\fB-q \fIkey\fR"
/*	Search the specified maps for \fIkey\fR and print the first value
/*	found on the standard output stream. The exit status is zero
/*	when the requested information was found.
/*
/*	If a key value of \fB-\fR is specified, the program reads key
/*	values from the standard input stream and prints one line of
/*	\fIkey value\fR output for each key that was found. The exit
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
/*	The output file is a btree file, named \fIfile_name\fB.db\fR.
/*	This is available only on systems with support for \fBdb\fR databases.
/* .IP \fBdbm\fR
/*	The output consists of two files, named \fIfile_name\fB.pag\fR and
/*	\fIfile_name\fB.dir\fR.
/*	This is available only on systems with support for \fBdbm\fR databases.
/* .IP \fBhash\fR
/*	The output file is a hashed file, named \fIfile_name\fB.db\fR.
/*	This is available only on systems with support for \fBdb\fR databases.
/* .PP
/*	Use the command \fBpostconf -m\fR to find out what types of database
/*	your Postfix installation can support.
/*
/*	When no \fIfile_type\fR is specified, the software uses the database
/*	type specified via the \fBdefault_database_type\fR configuration
/*	parameter.
/* .RE
/* .IP \fIfile_name\fR
/*	The name of the lookup table source file when rebuilding a database.
/* DIAGNOSTICS
/*	Problems and transactions are logged to the standard error
/*	stream. No output means no problems. Duplicate entries are
/*	skipped and are flagged with a warning.
/*
/*	\fBpostmap\fR terminates with zero exit status in case of success
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
/* .IP \fBdefault_database_type\fR
/*	Default output database type.
/*	On many UNIX systems, the default database type is either \fBhash\fR
/*	or \fBdbm\fR.
/* .IP \fBberkeley_db_create_buffer_size\fR
/*	Amount of buffer memory to be used when creating a Berkeley DB
/*	\fBhash\fR or \fBbtree\fR lookup table.
/* .IP \fBberkeley_db_read_buffer_size\fR
/*	Amount of buffer memory to be used when reading a Berkeley DB
/*	\fBhash\fR or \fBbtree\fR lookup table.
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
#include <vstring_vstream.h>
#include <set_eugid.h>

/* Global library. */

#include <mail_conf.h>
#include <mail_dict.h>
#include <mail_params.h>
#include <mkmap.h>

/* Application-specific. */

#define STR	vstring_str

#define POSTMAP_FLAG_AS_OWNER	(1<<0)	/* open dest as owner of source */

/* postmap - create or update mapping database */

static void postmap(char *map_type, char *path_name, int postmap_flags,
		            int open_flags, int dict_flags)
{
    VSTREAM *source_fp;
    VSTRING *line_buffer;
    MKMAP  *mkmap;
    int     lineno;
    char   *key;
    char   *value;
    struct stat st;
    mode_t  saved_mask;

    /*
     * Initialize.
     */
    line_buffer = vstring_alloc(100);
    if ((open_flags & O_TRUNC) == 0) {
	source_fp = VSTREAM_IN;
	vstream_control(source_fp, VSTREAM_CTL_PATH, "stdin", VSTREAM_CTL_END);
    } else if ((source_fp = vstream_fopen(path_name, O_RDONLY, 0)) == 0) {
	msg_fatal("open %s: %m", path_name);
    }
    if (fstat(vstream_fileno(source_fp), &st) < 0)
	msg_fatal("fstat %s: %m", path_name);

    /*
     * Turn off group/other read permissions as indicated in the source file.
     */
    if (S_ISREG(st.st_mode))
	saved_mask = umask(022 | (~st.st_mode & 077));

    /*
     * If running as root, run as the owner of the source file, so that the
     * result shows proper ownership, and so that a bug in postmap does not
     * allow privilege escalation.
     */
    if ((postmap_flags & POSTMAP_FLAG_AS_OWNER) && getuid() == 0
	&& (st.st_uid != geteuid() || st.st_gid != getegid()))
	set_eugid(st.st_uid, st.st_gid);

    /*
     * Open the database, optionally create it when it does not exist,
     * optionally truncate it when it does exist, and lock out any
     * spectators.
     */
    mkmap = mkmap_open(map_type, path_name, open_flags, dict_flags);

    /*
     * And restore the umask, in case it matters.
     */
    if (S_ISREG(st.st_mode))
	umask(saved_mask);

    /*
     * Add records to the database.
     */
    lineno = 0;
    while (readlline(line_buffer, source_fp, &lineno)) {

	/*
	 * Split on the first whitespace character, then trim leading and
	 * trailing whitespace from key and value.
	 */
	key = STR(line_buffer);
	value = key + strcspn(key, " \t\r\n");
	if (*value)
	    *value++ = 0;
	while (ISSPACE(*value))
	    value++;
	trimblanks(key, 0)[0] = 0;
	trimblanks(value, 0)[0] = 0;

	/*
	 * Enforce the "key whitespace value" format. Disallow missing keys
	 * or missing values.
	 */
	if (*key == 0 || *value == 0) {
	    msg_warn("%s, line %d: expected format: key whitespace value",
		     VSTREAM_PATH(source_fp), lineno);
	    continue;
	}
	if (key[strlen(key) - 1] == ':')
	    msg_warn("%s, line %d: record is in \"key: value\" format; is this an alias file?",
		     VSTREAM_PATH(source_fp), lineno);

	/*
	 * Store the value under a case-insensitive key.
	 */
	if (dict_flags & DICT_FLAG_FOLD_KEY)
	    lowercase(key);
	mkmap_append(mkmap, key, value);
    }

    /*
     * Close the mapping database, and release the lock.
     */
    mkmap_close(mkmap);

    /*
     * Cleanup. We're about to terminate, but it is a good sanity check.
     */
    vstring_free(line_buffer);
    if (source_fp != VSTREAM_IN)
	vstream_fclose(source_fp);
}

/* postmap_queries - apply multiple requests from stdin */

static int postmap_queries(VSTREAM *in, char **maps, const int map_count,
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
	msg_panic("postmap_queries: bad map count");

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
		vstream_printf("%s	%s\n", STR(keybuf), value);
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

/* postmap_query - query a map and print the result to stdout */

static int postmap_query(const char *map_type, const char *map_name,
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

/* postmap_deletes - apply multiple requests from stdin */

static int postmap_deletes(VSTREAM *in, char **maps, const int map_count)
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
	msg_panic("postmap_deletes: bad map count");

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

/* postmap_delete - delete a (key, value) pair from a map */

static int postmap_delete(const char *map_type, const char *map_name,
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
    msg_fatal("usage: %s [-Nfinorvw] [-c config_dir] [-d key] [-q key] [map_type:]file...",
	      myname);
}

int     main(int argc, char **argv)
{
    char   *path_name;
    int     ch;
    int     fd;
    char   *slash;
    struct stat st;
    int     postmap_flags = POSTMAP_FLAG_AS_OWNER;
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
    while ((ch = GETOPT(argc, argv, "Nc:d:finoq:rvw")) > 0) {
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
	case 'o':
	    postmap_flags &= ~POSTMAP_FLAG_AS_OWNER;
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
    mail_dict_init();

    /*
     * Use the map type specified by the user, or fall back to a default
     * database type.
     */
    if (delkey) {				/* remove entry */
	if (optind + 1 > argc)
	    usage(argv[0]);
	if (strcmp(delkey, "-") == 0)
	    exit(postmap_deletes(VSTREAM_IN, argv + optind, argc - optind) == 0);
	found = 0;
	while (optind < argc) {
	    if ((path_name = split_at(argv[optind], ':')) != 0) {
		found |= postmap_delete(argv[optind], path_name, delkey);
	    } else {
		found |= postmap_delete(var_db_type, argv[optind], delkey);
	    }
	    optind++;
	}
	exit(found ? 0 : 1);
    } else if (query) {				/* query map(s) */
	if (optind + 1 > argc)
	    usage(argv[0]);
	if (strcmp(query, "-") == 0)
	    exit(postmap_queries(VSTREAM_IN, argv + optind, argc - optind,
				 dict_flags) == 0);
	if (dict_flags & DICT_FLAG_FOLD_KEY)
	    lowercase(query);
	while (optind < argc) {
	    if ((path_name = split_at(argv[optind], ':')) != 0) {
		found = postmap_query(argv[optind], path_name, query);
	    } else {
		found = postmap_query(var_db_type, argv[optind], query);
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
		postmap(argv[optind], path_name, postmap_flags,
			open_flags, dict_flags);
	    } else {
		postmap(var_db_type, argv[optind], postmap_flags,
			open_flags, dict_flags);
	    }
	    optind++;
	}
	exit(0);
    }
}
