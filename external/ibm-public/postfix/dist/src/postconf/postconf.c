/*	$NetBSD: postconf.c,v 1.1.1.4.4.1 2013/01/23 00:05:06 yamt Exp $	*/

/*++
/* NAME
/*	postconf 1
/* SUMMARY
/*	Postfix configuration utility
/* SYNOPSIS
/* .fi
/*	\fBManaging main.cf:\fR
/*
/*	\fBpostconf\fR [\fB-dfhnv\fR] [\fB-c \fIconfig_dir\fR]
/*	[\fB-C \fIclass,...\fR] [\fIparameter ...\fR]
/*
/*	\fBpostconf\fR [\fB-ev\fR] [\fB-c \fIconfig_dir\fR]
/*	[\fIparameter=value ...\fR]
/*
/*	\fBpostconf\fR [\fB-#v\fR] [\fB-c \fIconfig_dir\fR]
/*	[\fIparameter ...\fR]
/*
/*	\fBManaging master.cf:\fR
/*
/*	\fBpostconf\fR [\fB-fMv\fR] [\fB-c \fIconfig_dir\fR]
/*	[\fIservice ...\fR]
/*
/*	\fBManaging bounce message templates:\fR
/*
/*	\fBpostconf\fR [\fB-btv\fR] [\fB-c \fIconfig_dir\fR] [\fItemplate_file\fR]
/*
/*	\fBManaging other configuration:\fR
/*
/*	\fBpostconf\fR [\fB-aAlmv\fR] [\fB-c \fIconfig_dir\fR]
/* DESCRIPTION
/*	By default, the \fBpostconf\fR(1) command displays the
/*	values of \fBmain.cf\fR configuration parameters, and warns
/*	about possible mis-typed parameter names (Postfix 2.9 and later).
/*	It can also change \fBmain.cf\fR configuration
/*	parameter values, or display other configuration information
/*	about the Postfix mail system.
/*
/*	Options:
/* .IP \fB-a\fR
/*	List the available SASL server plug-in types.  The SASL
/*	plug-in type is selected with the \fBsmtpd_sasl_type\fR
/*	configuration parameter by specifying one of the names
/*	listed below.
/* .RS
/* .IP \fBcyrus\fR
/*	This server plug-in is available when Postfix is built with
/*	Cyrus SASL support.
/* .IP \fBdovecot\fR
/*	This server plug-in uses the Dovecot authentication server,
/*	and is available when Postfix is built with any form of SASL
/*	support.
/* .RE
/* .IP
/*	This feature is available with Postfix 2.3 and later.
/* .IP \fB-A\fR
/*	List the available SASL client plug-in types.  The SASL
/*	plug-in type is selected with the \fBsmtp_sasl_type\fR or
/*	\fBlmtp_sasl_type\fR configuration parameters by specifying
/*	one of the names listed below.
/* .RS
/* .IP \fBcyrus\fR
/*	This client plug-in is available when Postfix is built with
/*	Cyrus SASL support.
/* .RE
/* .IP
/*	This feature is available with Postfix 2.3 and later.
/* .IP "\fB-b\fR [\fItemplate_file\fR]"
/*	Display the message text that appears at the beginning of
/*	delivery status notification (DSN) messages, replacing
/*	$\fBname\fR expressions with actual values as described in
/*	\fBbounce\fR(5).
/*
/*	To override the built-in templates, specify a template file
/*	name at the end of the \fBpostconf\fR(1) command line, or
/*	specify a file name in \fBmain.cf\fR with the
/*	\fBbounce_template_file\fR parameter.
/*
/*	To force selection of the built-in templates, specify an
/*	empty template file name on the \fBpostconf\fR(1) command
/*	line (in shell language: "").
/*
/*	This feature is available with Postfix 2.3 and later.
/* .IP "\fB-c \fIconfig_dir\fR"
/*	The \fBmain.cf\fR configuration file is in the named directory
/*	instead of the default configuration directory.
/* .IP "\fB-C \fIclass,...\fR"
/*	When displaying \fBmain.cf\fR parameters, select only
/*	parameters from the specified class(es):
/* .RS
/* .IP \fBbuiltin\fR
/*	Parameters with built-in names.
/* .IP \fBservice\fR
/*	Parameters with service-defined names (the first field of
/*	a \fBmaster.cf\fR entry plus a Postfix-defined suffix).
/* .IP \fBuser\fR
/*	Parameters with user-defined names.
/* .IP \fBall\fR
/*	All the above classes.
/* .RE
/* .IP
/*	The default is as if "\fB-C all\fR" is
/*	specified.
/* .IP \fB-d\fR
/*	Print \fBmain.cf\fR default parameter settings instead of
/*	actual settings.
/*	Specify \fB-df\fR to fold long lines for human readability
/*	(Postfix 2.9 and later).
/* .IP \fB-e\fR
/*	Edit the \fBmain.cf\fR configuration file, and update
/*	parameter settings with the "\fIname\fR=\fIvalue\fR" pairs
/*	on the \fBpostconf\fR(1) command line. The file is copied
/*	to a temporary file then renamed into place.
/*	Specify quotes to protect special characters and whitespace
/*	on the \fBpostconf\fR(1) command line.
/*
/*	The \fB-e\fR is no longer needed with Postfix version 2.8
/*	and later.
/* .IP \fB-f\fR
/*	Fold long lines when printing \fBmain.cf\fR or \fBmaster.cf\fR
/*	configuration file entries, for human readability.
/*
/*	This feature is available with Postfix 2.9 and later.
/* .IP \fB-h\fR
/*	Show \fBmain.cf\fR parameter values without the "\fIname\fR
/*	= " label that normally precedes the value.
/* .IP \fB-l\fR
/*	List the names of all supported mailbox locking methods.
/*	Postfix supports the following methods:
/* .RS
/* .IP \fBflock\fR
/*	A kernel-based advisory locking method for local files only.
/*	This locking method is available on systems with a BSD
/*	compatible library.
/* .IP \fBfcntl\fR
/*	A kernel-based advisory locking method for local and remote files.
/* .IP \fBdotlock\fR
/*	An application-level locking method. An application locks a file
/*	named \fIfilename\fR by creating a file named \fIfilename\fB.lock\fR.
/*	The application is expected to remove its own lock file, as well as
/*	stale lock files that were left behind after abnormal termination.
/* .RE
/* .IP \fB-m\fR
/*	List the names of all supported lookup table types. In Postfix
/*	configuration files,
/*	lookup tables are specified as \fItype\fB:\fIname\fR, where
/*	\fItype\fR is one of the types listed below. The table \fIname\fR
/*	syntax depends on the lookup table type as described in the
/*	DATABASE_README document.
/* .RS
/* .IP \fBbtree\fR
/*	A sorted, balanced tree structure.
/*	This is available on systems with support for Berkeley DB
/*	databases.
/* .IP \fBcdb\fR
/*	A read-optimized structure with no support for incremental updates.
/*	This is available on systems with support for CDB databases.
/* .IP \fBcidr\fR
/*	A table that associates values with Classless Inter-Domain Routing
/*	(CIDR) patterns. This is described in \fBcidr_table\fR(5).
/* .IP \fBdbm\fR
/*	An indexed file type based on hashing.
/*	This is available on systems with support for DBM databases.
/* .IP \fBenviron\fR
/*	The UNIX process environment array. The lookup key is the variable
/*	name. Originally implemented for testing, someone may find this
/*	useful someday.
/* .IP \fBfail\fR
/*	A table that reliably fails all requests. The lookup table
/*	name is used for logging. This table exists to simplify
/*	Postfix error tests.
/* .IP \fBhash\fR
/*	An indexed file type based on hashing.
/*	This is available on systems with support for Berkeley DB
/*	databases.
/* .IP \fBinternal\fR
/*	A non-shared, in-memory hash table. Its content are lost
/*	when a process terminates.
/* .IP "\fBldap\fR (read-only)"
/*	Perform lookups using the LDAP protocol. This is described
/*	in \fBldap_table\fR(5).
/* .IP "\fBmemcache\fR"
/*	Perform lookups using the memcache protocol. This is described
/*	in \fBmemcache_table\fR(5).
/* .IP "\fBmysql\fR (read-only)"
/*	Perform lookups using the MYSQL protocol. This is described
/*	in \fBmysql_table\fR(5).
/* .IP "\fBpcre\fR (read-only)"
/*	A lookup table based on Perl Compatible Regular Expressions. The
/*	file format is described in \fBpcre_table\fR(5).
/* .IP "\fBpgsql\fR (read-only)"
/*	Perform lookups using the PostgreSQL protocol. This is described
/*	in \fBpgsql_table\fR(5).
/* .IP "\fBproxy\fR"
/*	A lookup table that is implemented via the Postfix
/*	\fBproxymap\fR(8) service. The table name syntax is
/*	\fItype\fB:\fIname\fR.
/* .IP "\fBregexp\fR (read-only)"
/*	A lookup table based on regular expressions. The file format is
/*	described in \fBregexp_table\fR(5).
/* .IP \fBsdbm\fR
/*	An indexed file type based on hashing.
/*	This is available on systems with support for SDBM databases.
/* .IP "\fBsqlite\fR (read-only)"
/*	Perform lookups from SQLite database files. This is described
/*	in \fBsqlite_table\fR(5).
/* .IP "\fBstatic\fR (read-only)"
/*	A table that always returns its name as lookup result. For example,
/*	\fBstatic:foobar\fR always returns the string \fBfoobar\fR as lookup
/*	result.
/* .IP "\fBtcp\fR (read-only)"
/*	Perform lookups using a simple request-reply protocol that is
/*	described in \fBtcp_table\fR(5).
/* .IP "\fBtexthash\fR (read-only)"
/*	Produces similar results as hash: files, except that you don't
/*	need to run the \fBpostmap\fR(1) command before you can use the file,
/*	and that it does not detect changes after the file is read.
/* .IP "\fBunix\fR (read-only)"
/*	A limited way to query the UNIX authentication database. The
/*	following tables are implemented:
/* .RS
/*. IP \fBunix:passwd.byname\fR
/*	The table is the UNIX password database. The key is a login name.
/*	The result is a password file entry in \fBpasswd\fR(5) format.
/* .IP \fBunix:group.byname\fR
/*	The table is the UNIX group database. The key is a group name.
/*	The result is a group file entry in \fBgroup\fR(5) format.
/* .RE
/* .RE
/* .IP
/*	Other table types may exist depending on how Postfix was built.
/* .IP \fB-M\fR
/*	Show \fBmaster.cf\fR file contents instead of \fBmain.cf\fR
/*	file contents.
/*	Specify \fB-Mf\fR to fold long lines for human readability.
/*
/*	If \fIservice ...\fR is specified, only the matching services
/*	will be output. For example, "\fBpostconf -Mf inet\fR"
/*	will output all services that listen on the network.
/*
/*	Specify zero or more arguments, each with a \fIservice-type\fR
/*	name (\fBinet\fR, \fBunix\fR, \fBfifo\fR, or \fBpass\fR)
/*	or with a \fIservice-name.service-type\fR pair, where
/*	\fIservice-name\fR is the first field of a master.cf entry.
/*
/*	This feature is available with Postfix 2.9 and later.
/* .IP \fB-n\fR
/*	Print \fBmain.cf\fR parameter settings that are explicitly
/*	specified in \fBmain.cf\fR.
/*	Specify \fB-nf\fR to fold long lines for human readability
/*	(Postfix 2.9 and later).
/* .IP "\fB-t\fR [\fItemplate_file\fR]"
/*	Display the templates for text that appears at the beginning
/*	of delivery status notification (DSN) messages, without
/*	expanding $\fBname\fR expressions.
/*
/*	To override the built-in templates, specify a template file
/*	name at the end of the \fBpostconf\fR(1) command line, or
/*	specify a file name in \fBmain.cf\fR with the
/*	\fBbounce_template_file\fR parameter.
/*
/*	To force selection of the built-in templates, specify an
/*	empty template file name on the \fBpostconf\fR(1) command
/*	line (in shell language: "").
/*
/*	This feature is available with Postfix 2.3 and later.
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple \fB-v\fR
/*	options make the software increasingly verbose.
/* .IP \fB-#\fR
/*	Edit the \fBmain.cf\fR configuration file, and comment out
/*	the parameters given on the \fBpostconf\fR(1) command line,
/*	so that those parameters revert to their default values.
/*	The file is copied to a temporary file then renamed into
/*	place.
/*	Specify a list of parameter names, not \fIname\fR=\fIvalue\fR
/*	pairs.  There is no \fBpostconf\fR(1) command to perform
/*	the reverse operation.
/*
/*	This feature is available with Postfix 2.6 and later.
/* DIAGNOSTICS
/*	Problems are reported to the standard error stream.
/* ENVIRONMENT
/* .ad
/* .fi
/* .IP \fBMAIL_CONFIG\fR
/*	Directory with Postfix configuration files.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/*	The following \fBmain.cf\fR parameters are especially relevant to
/*	this program.
/*
/*	The text below provides only a parameter summary. See
/*	\fBpostconf\fR(5) for more details including examples.
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBbounce_template_file (empty)\fR"
/*	Pathname of a configuration file with bounce message templates.
/* FILES
/*	/etc/postfix/main.cf, Postfix configuration parameters
/*	/etc/postfix/master.cf, Postfix master daemon configuraton
/* SEE ALSO
/*	bounce(5), bounce template file format
/*	master(5), master.cf configuration file syntax
/*	postconf(5), main.cf configuration file syntax
/* README FILES
/* .ad
/* .fi
/*	Use "\fBpostconf readme_directory\fR" or
/*	"\fBpostconf html_directory\fR" to locate this information.
/* .na
/* .nf
/*	DATABASE_README, Postfix lookup table overview
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

/* Utility library. */

#include <msg.h>
#include <msg_vstream.h>
#include <dict.h>
#include <htable.h>
#include <vstring.h>
#include <vstream.h>
#include <stringops.h>
#include <name_mask.h>
#include <warn_stat.h>

/* Global library. */

#include <mail_params.h>
#include <mail_conf.h>
#include <mail_version.h>
#include <mail_run.h>
#include <mail_dict.h>

/* Application-specific. */

#include <postconf.h>

 /*
  * Global storage. See postconf.h for description.
  */
PC_PARAM_TABLE *param_table;
PC_MASTER_ENT *master_table;
int     cmd_mode = DEF_MODE;

 /*
  * Application fingerprinting.
  */
MAIL_VERSION_STAMP_DECLARE;

/* main */

int     main(int argc, char **argv)
{
    int     ch;
    int     fd;
    struct stat st;
    int     junk;
    ARGV   *ext_argv = 0;
    int     param_class = PC_PARAM_MASK_CLASS;
    static const NAME_MASK param_class_table[] = {
	"builtin", PC_PARAM_FLAG_BUILTIN,
	"service", PC_PARAM_FLAG_SERVICE,
	"user", PC_PARAM_FLAG_USER,
	"all", PC_PARAM_MASK_CLASS,
	0,
    };

    /*
     * Fingerprint executables and core dumps.
     */
    MAIL_VERSION_STAMP_ALLOCATE;

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
     * Set up logging.
     */
    msg_vstream_init(argv[0], VSTREAM_ERR);

    /*
     * Parse JCL.
     */
    while ((ch = GETOPT(argc, argv, "aAbc:C:deEf#hlmMntv")) > 0) {
	switch (ch) {
	case 'a':
	    cmd_mode |= SHOW_SASL_SERV;
	    break;
	case 'A':
	    cmd_mode |= SHOW_SASL_CLNT;
	    break;
	case 'b':
	    if (ext_argv)
		msg_fatal("specify one of -b and -t");
	    ext_argv = argv_alloc(2);
	    argv_add(ext_argv, "bounce", "-SVnexpand_templates", (char *) 0);
	    break;
	case 'c':
	    if (setenv(CONF_ENV_PATH, optarg, 1) < 0)
		msg_fatal("out of memory");
	    break;
	case 'C':
	    param_class = name_mask_opt("-C option", param_class_table,
				    optarg, NAME_MASK_ANY_CASE | NAME_MASK_FATAL);
	    break;
	case 'd':
	    cmd_mode |= SHOW_DEFS;
	    break;
	case 'e':
	    cmd_mode |= EDIT_MAIN;
	    break;
	case 'f':
	    cmd_mode |= FOLD_LINE;
	    break;

	    /*
	     * People, this does not work unless you properly handle default
	     * settings. For example, fast_flush_domains = $relay_domains
	     * must not evaluate to the empty string when relay_domains is
	     * left at its default setting of $mydestination.
	     */
#if 0
	case 'E':
	    cmd_mode |= SHOW_EVAL;
	    break;
#endif
	case '#':
	    cmd_mode = COMMENT_OUT;
	    break;

	case 'h':
	    cmd_mode &= ~SHOW_NAME;
	    break;
	case 'l':
	    cmd_mode |= SHOW_LOCKS;
	    break;
	case 'm':
	    cmd_mode |= SHOW_MAPS;
	    break;
	case 'M':
	    cmd_mode |= SHOW_MASTER;
	    break;
	case 'n':
	    cmd_mode |= SHOW_NONDEF;
	    break;
	case 't':
	    if (ext_argv)
		msg_fatal("specify one of -b and -t");
	    ext_argv = argv_alloc(2);
	    argv_add(ext_argv, "bounce", "-SVndump_templates", (char *) 0);
	    break;
	case 'v':
	    msg_verbose++;
	    break;
	default:
	    msg_fatal("usage: %s [-a (server SASL types)] [-A (client SASL types)] [-b (bounce templates)] [-c config_dir] [-C param_class] [-d (defaults)] [-e (edit)] [-f (fold lines)] [-# (comment-out)] [-h (no names)] [-l (lock types)] [-m (map types)] [-M (master.cf)] [-n (non-defaults)] [-v] [name...]", argv[0]);
	}
    }

    /*
     * Sanity check.
     */
    junk = (cmd_mode & (SHOW_DEFS | SHOW_NONDEF | SHOW_MAPS | SHOW_LOCKS | EDIT_MAIN | SHOW_SASL_SERV | SHOW_SASL_CLNT | COMMENT_OUT | SHOW_MASTER));
    if (junk != 0 && ((junk != SHOW_DEFS && junk != SHOW_NONDEF
	     && junk != SHOW_MAPS && junk != SHOW_LOCKS && junk != EDIT_MAIN
		       && junk != SHOW_SASL_SERV && junk != SHOW_SASL_CLNT
		       && junk != COMMENT_OUT && junk != SHOW_MASTER)
		      || ext_argv != 0))
	msg_fatal("specify one of -a, -A, -b, -d, -e, -#, -l, -m, -M and -n");

    /*
     * Display bounce template information and exit.
     */
    if (ext_argv) {
	if (argv[optind]) {
	    if (argv[optind + 1])
		msg_fatal("options -b and -t require at most one template file");
	    argv_add(ext_argv, "-o",
		     concatenate(VAR_BOUNCE_TMPL, "=",
				 argv[optind], (char *) 0),
		     (char *) 0);
	}
	/* Grr... */
	argv_add(ext_argv, "-o",
		 concatenate(VAR_QUEUE_DIR, "=", ".", (char *) 0),
		 (char *) 0);
	mail_conf_read();
	mail_run_replace(var_daemon_dir, ext_argv->argv);
	/* NOTREACHED */
    }

    /*
     * If showing map types, show them and exit
     */
    if (cmd_mode & SHOW_MAPS) {
	mail_dict_init();
	show_maps();
    }

    /*
     * If showing locking methods, show them and exit
     */
    else if (cmd_mode & SHOW_LOCKS) {
	show_locks();
    }

    /*
     * If showing master.cf entries, show them and exit
     */
    else if (cmd_mode & SHOW_MASTER) {
	read_master(FAIL_ON_OPEN_ERROR);
	show_master(cmd_mode, argv + optind);
    }

    /*
     * If showing SASL plug-in types, show them and exit
     */
    else if (cmd_mode & SHOW_SASL_SERV) {
	show_sasl(SHOW_SASL_SERV);
    } else if (cmd_mode & SHOW_SASL_CLNT) {
	show_sasl(SHOW_SASL_CLNT);
    }

    /*
     * Edit main.cf.
     */
    else if (cmd_mode & (EDIT_MAIN | COMMENT_OUT)) {
	edit_parameters(cmd_mode, argc - optind, argv + optind);
    } else if (cmd_mode == DEF_MODE
	       && argv[optind] && strchr(argv[optind], '=')) {
	edit_parameters(cmd_mode | EDIT_MAIN, argc - optind, argv + optind);
    }

    /*
     * If showing non-default values, read main.cf.
     */
    else {
	if ((cmd_mode & SHOW_DEFS) == 0) {
	    read_parameters();
	    set_parameters();
	}
	register_builtin_parameters();

	/*
	 * Add service-dependent parameters (service names from master.cf)
	 * and user-defined parameters ($name macros in parameter values in
	 * main.cf and master.cf, but only if those names have a name=value
	 * in main.cf or master.cf).
	 */
	read_master(WARN_ON_OPEN_ERROR);
	register_service_parameters();
	if ((cmd_mode & SHOW_DEFS) == 0)
	    register_user_parameters();

	/*
	 * Show the requested values.
	 */
	show_parameters(cmd_mode, param_class, argv + optind);

	/*
	 * Flag unused parameters. This makes no sense with "postconf -d",
	 * because that ignores all the user-specified parameters and
	 * user-specified macro expansions in main.cf.
	 */
	if ((cmd_mode & SHOW_DEFS) == 0) {
	    flag_unused_main_parameters();
	    flag_unused_master_parameters();
	}
    }
    vstream_fflush(VSTREAM_OUT);
    exit(0);
}
