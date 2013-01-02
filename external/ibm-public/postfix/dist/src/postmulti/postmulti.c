/*	$NetBSD: postmulti.c,v 1.1.1.4 2013/01/02 18:59:04 tron Exp $	*/

/*++
/* NAME
/*	postmulti 1
/* SUMMARY
/*	Postfix multi-instance manager
/* SYNOPSIS
/* .fi
/*	\fBENABLING MULTI-INSTANCE MANAGEMENT:\fR
/*
/*	\fBpostmulti\fR \fB-e init\fR [\fB-v\fR]
/*
/*	\fBITERATOR MODE:\fR
/*
/*	\fBpostmulti\fR \fB-l\fR [\fB-aRv\fR] [\fB-g \fIgroup\fR]
/*	[\fB-i \fIname\fR]
/*
/*	\fBpostmulti\fR \fB-p\fR [\fB-av\fR] [\fB-g \fIgroup\fR]
/*	[\fB-i \fIname\fR] \fIcommand...\fR
/*
/*	\fBpostmulti\fR \fB-x\fR [\fB-aRv\fR] [\fB-g \fIgroup\fR]
/*	[\fB-i \fIname\fR] \fIcommand...\fR
/*
/*	\fBLIFE-CYCLE MANAGEMENT:\fR
/*
/*	\fBpostmulti\fR \fB-e create\fR [\fB-av\fR]
/*	[\fB-g \fIgroup\fR] [\fB-i \fIname\fR] [\fB-G \fIgroup\fR]
/*	[\fB-I \fIname\fR] [\fIparam=value\fR ...]
/*
/*	\fBpostmulti\fR \fB-e import\fR [\fB-av\fR]
/*	[\fB-g \fIgroup\fR] [\fB-i \fIname\fR] [\fB-G \fIgroup\fR]
/*	[\fB-I \fIname\fR] [\fBconfig_directory=\fI/path\fR]
/*
/*	\fBpostmulti\fR \fB-e destroy\fR [\fB-v\fR] \fB-i \fIname\fR
/*
/*	\fBpostmulti\fR \fB-e deport\fR [\fB-v\fR] \fB-i \fIname\fR
/*
/*	\fBpostmulti\fR \fB-e enable\fR [\fB-v\fR] \fB-i \fIname\fR
/*
/*	\fBpostmulti\fR \fB-e disable\fR [\fB-v\fR] \fB-i \fIname\fR
/*
/*	\fBpostmulti\fR \fB-e assign\fR [\fB-v\fR] \fB-i \fIname\fR
/*	[\fB-I \fIname\fR] [-G \fIgroup\fR]
/* DESCRIPTION
/*	The \fBpostmulti\fR(1) command allows a Postfix administrator
/*	to manage multiple Postfix instances on a single host.
/*
/*	\fBpostmulti\fR(1) implements two fundamental modes of
/*	operation.  In \fBiterator\fR mode, it executes the same
/*	command for multiple Postfix instances.  In \fBlife-cycle
/*	management\fR mode, it adds or deletes one instance, or
/*	changes the multi-instance status of one instance.
/*
/*	Each mode of operation has its own command syntax. For this
/*	reason, each mode is documented in separate sections below.
/* BACKGROUND
/* .ad
/* .fi
/*	A multi-instance configuration consists of one primary
/*	Postfix instance, and one or more secondary instances whose
/*	configuration directory pathnames are recorded in the primary
/*	instance's main.cf file. Postfix instances share program
/*	files and documentation, but have their own configuration,
/*	queue and data directories.
/*
/*	Currently, only the default Postfix instance can be used
/*	as primary instance in a multi-instance configuration. The
/*	\fBpostmulti\fR(1) command does not currently support a \fB-c\fR
/*	option to select an alternative primary instance, and exits
/*	with a fatal error if the \fBMAIL_CONFIG\fR environment
/*	variable is set to a non-default configuration directory.
/*
/*	See the MULTI_INSTANCE_README tutorial for a more detailed
/*	discussion of multi-instance management with \fBpostmulti\fR(1).
/* ITERATOR MODE
/* .ad
/* .fi
/*	In iterator mode, \fBpostmulti\fR performs the same operation
/*	on all Postfix instances in turn.
/*
/*	If multi-instance support is not enabled, the requested
/*	command is performed just for the primary instance.
/* .PP
/*	Iterator mode implements the following command options:
/* .SH "Instance selection"
/* .IP \fB-a\fR
/*	Perform the operation on all instances. This is the default.
/* .IP "\fB-g \fIgroup\fR"
/*	Perform the operation only for members of the named \fIgroup\fR.
/* .IP "\fB-i \fIname\fR"
/*	Perform the operation only for the instance with the specified
/*	\fIname\fR.  You can specify either the instance name
/*	or the absolute pathname of the instance's configuration
/*	directory.  Specify "-" to select the primary Postfix instance.
/* .IP \fB-R\fR
/*	Reverse the iteration order. This may be appropriate when
/*	updating a multi-instance system, where "sink" instances
/*	are started before "source" instances.
/* .sp
/*	This option cannot be used with \fB-p\fR.
/* .SH "List mode"
/* .IP \fB-l\fR
/*	List Postfix instances with their instance name, instance
/*	group name, enable/disable status and configuration directory.
/* .SH "Postfix-wrapper mode"
/* .IP \fB-p\fR
/*	Invoke \fBpostfix(1)\fR to execute the specified \fIcommand\fR.
/*	This option implements the \fBpostfix-wrapper\fR(5) interface.
/* .RS
/* .IP \(bu
/*	With "start"-like commands, "postfix check" is executed for
/*	instances that are not enabled. The full list of commands
/*	is specified with the postmulti_start_commands parameter.
/* .IP \(bu
/*	With "stop"-like commands, the iteration order is reversed,
/*	and disabled instances are skipped. The full list of commands
/*	is specified with the postmulti_stop_commands parameter.
/* .IP \(bu
/*	With "reload" and other commands that require a started
/*	instance, disabled instances are skipped. The full list of
/*	commands is specified with the postmulti_control_commands
/*	parameter.
/* .IP \(bu
/*	With "status" and other commands that don't require a started
/*	instance, the command is executed for all instances.
/* .RE
/* .IP
/*	The \fB-p\fR option can also be used interactively to
/*	start/stop/etc.  a named instance or instance group. For
/*	example, to start just the instances in the group "msa",
/*	invoke \fBpostmulti\fR(1) as follows:
/* .RS
/* .IP
/*	# postmulti -g msa -p start
/* .RE
/* .SH "Command mode"
/* .IP \fB-x\fR
/*	Execute the specified \fIcommand\fR for all Postfix instances.
/*	The command runs with appropriate environment settings for
/*	MAIL_CONFIG, command_directory, daemon_directory,
/*	config_directory, queue_directory, data_directory,
/*	multi_instance_name, multi_instance_group and
/*	multi_instance_enable.
/* .SH "Other options"
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple
/*	\fB-v\fR options make the software increasingly verbose.
/* LIFE-CYCLE MANAGEMENT MODE
/* .ad
/* .fi
/*	With the \fB-e\fR option \fBpostmulti\fR(1) can be used to
/*	add or delete a Postfix instance, and to manage the
/*	multi-instance status of an existing instance.
/* .PP
/*	The following options are implemented:
/* .SH "Existing instance selection"
/* .IP \fB-a\fR
/*	When creating or importing an instance, place the new
/*	instance at the front of the secondary instance list.
/* .IP "\fB-g \fIgroup\fR"
/*	When creating or importing an instance, place the new
/*	instance before the first secondary instance that is a
/*	member of the specified group.
/* .IP "\fB-i \fIname\fR"
/*	When creating or importing an instance, place the new
/*	instance before the matching secondary instance.
/* .sp
/*	With other life-cycle operations, apply the operation to
/*	the named existing instance.  Specify "-" to select the
/*	primary Postfix instance.
/* .SH "New or existing instance name assignment"
/* .IP "\fB-I \fIname\fR"
/*	Assign the specified instance \fIname\fR to an existing
/*	instance, newly-created instance, or imported instance.
/*	Instance
/*	names other than "-" (which makes the instance "nameless")
/*	must start with "postfix-".  This restriction reduces the
/*	likelihood of name collisions with system files.
/* .IP "\fB-G \fIgroup\fR"
/*	Assign the specified \fIgroup\fR name to an existing instance
/*	or to a newly created or imported instance.
/* .SH "Instance creation/deletion/status change"
/* .IP "\fB-e \fIaction\fR"
/*	"Edit" managed instances. The following actions are supported:
/* .RS
/* .IP \fBinit\fR
/*	This command is required before \fBpostmulti\fR(1) can be
/*	used to manage Postfix instances.  The "postmulti -e init"
/*	command updates the primary instance's main.cf file by
/*	setting:
/* .RS
/* .IP
/* .nf
/*	multi_instance_wrapper =
/*		${command_directory}/postmulti -p --
/*	multi_instance_enable = yes
/* .fi
/* .RE
/* .IP
/*	You can set these by other means if you prefer.
/* .IP \fBcreate\fR
/*	Create a new Postfix instance and add it to the
/*	multi_instance_directories parameter of the primary instance.
/*	The "\fB-I \fIname\fR" option is recommended to give the
/*	instance a short name that is used to construct default
/*	values for the private directories of the new instance. The
/*	"\fB-G \fIgroup\fR" option may be specified to assign the
/*	instance to a group, otherwise, the new instance is not a
/*	member of any groups.
/* .sp
/*	The new instance main.cf is the stock main.cf with the
/*	parameters that specify the locations of shared files cloned
/*	from the primary instance.  For "nameless" instances, you
/*	should manually adjust "syslog_name" to yield a unique
/*	"logtag" starting with "postfix-" that will uniquely identify
/*	the instance in the mail logs. It is simpler to assign the
/*	instance a short name with the "\fB-I \fIname\fR" option.
/* .sp
/*	Optional "name=value" arguments specify the instance
/*	config_directory, queue_directory and data_directory.
/*	For example:
/* .RS
/* .IP
/* .nf
/*	# postmulti -I postfix-mumble \e
/*		-G mygroup -e create \e
/*		config_directory=/my/config/dir \e
/*		queue_directory=/my/queue/dir \e
/*		data_directory=/my/data/dir
/* .fi
/* .RE
/* .IP
/*	If any of these pathnames is not supplied, the program
/*	attempts to generate the pathname by taking the corresponding
/*	primary instance pathname, and by replacing the last pathname
/*	component by the value of the \fB-I\fR option.
/* .sp
/*	If the instance configuration directory already exists, and
/*	contains both a main.cf and master.cf file, \fBcreate\fR
/*	will "import" the instance as-is. For existing instances,
/*	\fBcreate\fR and \fBimport\fR are identical.
/* .IP \fBimport\fR
/*	Import an existing instance into the list of instances
/*	managed by the \fBpostmulti\fR(1) multi-instance manager.
/*	This adds the instance to the multi_instance_directories
/*	list of the primary instance.  If the "\fB-I \fIname\fR"
/*	option is provided it specifies the new name for the instance
/*	and is used to define a default location for the instance
/*	configuration directory	(as with \fBcreate\fR above).  The
/*	"\fB-G \fIgroup\fR" option may be used to assign the instance
/*	to a group. Add a "\fBconfig_directory=\fI/path\fR" argument
/*	to override a default pathname based on "\fB-I \fIname\fR".
/* .IP \fBdestroy\fR
/*	Destroy a secondary Postfix instance. To be a candidate for
/*	destruction an instance must be disabled, stopped and its
/*	queue must not contain any messages. Attempts to destroy
/*	the primary Postfix instance trigger a fatal error, without
/*	destroying the instance.
/* .sp
/*	The instance is removed from the primary instance main.cf
/*	file's alternate_config_directories parameter and its data,
/*	queue and configuration directories are cleaned of files
/*	and directories created by the Postfix system. The main.cf
/*	and master.cf files are removed from the configuration
/*	directory even if they have been modified since initial
/*	creation. Finally, the instance is "deported" from the list
/*	of managed instances.
/* .sp
/*	If other files are present in instance private directories,
/*	the directories may not be fully removed, a warning is
/*	logged to alert the administrator. It is expected that an
/*	instance built using "fresh" directories via the \fBcreate\fR
/*	action will be fully removed by the \fBdestroy\fR action
/*	(if first disabled). If the instance configuration and queue
/*	directories are populated with additional files	(access and
/*	rewriting tables, chroot jail content, etc.) the instance
/*	directories will not be fully removed.
/* .sp
/*	The \fBdestroy\fR action triggers potentially dangerous
/*	file removal operations. Make sure the instance's data,
/*	queue and configuration directories are set correctly and
/*	do not contain any valuable files.
/* .IP \fBdeport\fR
/*	Deport a secondary instance from the list of managed
/*	instances. This deletes the instance configuration directory
/*	from the primary instance's multi_instance_directories list,
/*	but does not remove any files or directories.
/* .IP \fBassign\fR
/*	Assign a new instance name or a new group name to the
/*	selected instance.  Use "\fB-G -\fR" to specify "no group"
/*	and "\fB-I -\fR" to specify "no name".  If you choose to
/*	make an instance "nameless", set a suitable syslog_name in
/*	the corresponding main.cf file.
/* .IP \fBenable\fR
/*	Mark the selected instance as enabled. This just sets the
/*	multi_instance_enable parameter to "yes" in the instance's
/*	main.cf file.
/* .IP \fBdisable\fR
/*	Mark the selected instance as disabled. This means that
/*	the instance will not be started etc. with "postfix start",
/*	"postmulti -p start" and so on. The instance can still be
/*	started etc. with "postfix -c config-directory start".
/* .SH "Other options"
/* .IP \fB-v\fR
/*	Enable verbose logging for debugging purposes. Multiple
/*	\fB-v\fR options make the software increasingly verbose.
/* .RE
/* ENVIRONMENT
/* .ad
/* .fi
/*	The \fBpostmulti\fR(1) command exports the following environment
/*	variables before executing the requested \fIcommand\fR for a given
/*	instance:
/* .IP \fBMAIL_VERBOSE\fR
/*	This is set when the -v command-line option is present.
/* .IP \fBMAIL_CONFIG\fR
/*	The location of the configuration directory of the instance.
/* CONFIGURATION PARAMETERS
/* .ad
/* .fi
/* .IP "\fBconfig_directory (see 'postconf -d' output)\fR"
/*	The default location of the Postfix main.cf and master.cf
/*	configuration files.
/* .IP "\fBdaemon_directory (see 'postconf -d' output)\fR"
/*	The directory with Postfix support programs and daemon programs.
/* .IP "\fBimport_environment (see 'postconf -d' output)\fR"
/*	The list of environment parameters that a Postfix process will
/*	import from a non-Postfix parent process.
/* .IP "\fBmulti_instance_directories (empty)\fR"
/*	An optional list of non-default Postfix configuration directories;
/*	these directories belong to additional Postfix instances that share
/*	the Postfix executable files and documentation with the default
/*	Postfix instance, and that are started, stopped, etc., together
/*	with the default Postfix instance.
/* .IP "\fBmulti_instance_group (empty)\fR"
/*	The optional instance group name of this Postfix instance.
/* .IP "\fBmulti_instance_name (empty)\fR"
/*	The optional instance name of this Postfix instance.
/* .IP "\fBmulti_instance_enable (no)\fR"
/*	Allow this Postfix instance to be started, stopped, etc., by a
/*	multi-instance manager.
/* .IP "\fBpostmulti_start_commands (start)\fR"
/*	The \fBpostfix\fR(1) commands that the \fBpostmulti\fR(1) instance manager treats
/*	as "start" commands.
/* .IP "\fBpostmulti_stop_commands (see 'postconf -d' output)\fR"
/*	The \fBpostfix\fR(1) commands that the \fBpostmulti\fR(1) instance manager treats
/*	as "stop" commands.
/* .IP "\fBpostmulti_control_commands (reload flush)\fR"
/*	The \fBpostfix\fR(1) commands that the \fBpostmulti\fR(1) instance manager
/*	treats as "control" commands, that operate on running instances.
/* .IP "\fBsyslog_facility (mail)\fR"
/*	The syslog facility of Postfix logging.
/* .IP "\fBsyslog_name (see 'postconf -d' output)\fR"
/*	The mail system name that is prepended to the process name in syslog
/*	records, so that "smtpd" becomes, for example, "postfix/smtpd".
/* FILES
/*	$daemon_directory/main.cf, stock configuration file
/*	$daemon_directory/master.cf, stock configuration file
/*	$daemon_directory/postmulti-script, life-cycle helper program
/* SEE ALSO
/*	postfix(1), Postfix control program
/*	postfix-wrapper(5), Postfix multi-instance API
/* README FILES
/*	Use "\fBpostconf readme_directory\fR" or "\fBpostconf
/*	html_directory\fR" to locate this information.
/*	MULTI_INSTANCE_README, Postfix multi-instance management
/* HISTORY
/* .ad
/* .fi
/*	The \fBpostmulti\fR(1) command was introduced with Postfix
/*	version 2.6.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Victor Duchovni
/*	Morgan Stanley
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

/* System library. */

#include <sys_defs.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <vstream.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <syslog.h>
#include <errno.h>
#include <ctype.h>
#ifdef USE_PATHS_H
#include <paths.h>
#endif
#include <stddef.h>

/* Utility library. */

#include <msg.h>
#include <msg_vstream.h>
#include <msg_syslog.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>
#include <clean_env.h>
#include <argv.h>
#include <safe.h>
#include <mymalloc.h>
#include <htable.h>
#include <name_code.h>
#include <ring.h>
#include <warn_stat.h>

/* Global library. */

#include <mail_version.h>
#include <mail_params.h>
#include <mail_conf.h>

/* Application-specific. */

 /*
  * Configuration parameters, specific to postmulti(1).
  */
char   *var_multi_start_cmds;
char   *var_multi_stop_cmds;
char   *var_multi_cntrl_cmds;

 /*
  * Shared directory pathnames.
  */
typedef struct {
    const char *param_name;
    char  **param_value;
} SHARED_PATH;

static SHARED_PATH shared_dir_table[] = {
    VAR_COMMAND_DIR, &var_command_dir,
    VAR_DAEMON_DIR, &var_daemon_dir,
    0,
};

 /*
  * Actions.
  */
#define ITER_CMD_POSTFIX	(1<<0)	/* postfix(1) iterator mode */
#define ITER_CMD_LIST		(1<<1)	/* listing iterator mode */
#define ITER_CMD_GENERIC	(1<<2)	/* generic command iterator mode */

#define ITER_CMD_MASK_ALL \
    (ITER_CMD_POSTFIX | ITER_CMD_LIST | ITER_CMD_GENERIC)

#define EDIT_CMD_CREATE		(1<<4)	/* create new instance */
#define EDIT_CMD_IMPORT		(1<<5)	/* import existing instance */
#define EDIT_CMD_DESTROY	(1<<6)	/* destroy instance */
#define EDIT_CMD_DEPORT		(1<<7)	/* export instance */
#define EDIT_CMD_ENABLE		(1<<8)	/* enable start/stop */
#define EDIT_CMD_DISABLE	(1<<9)	/* disable start/stop */
#define EDIT_CMD_ASSIGN		(1<<10)	/* assign name/group */
#define EDIT_CMD_INIT		(1<<11)	/* hook into main.cf */

#define EDIT_CMD_MASK_ADD	(EDIT_CMD_CREATE | EDIT_CMD_IMPORT)
#define EDIT_CMD_MASK_DEL	(EDIT_CMD_DESTROY | EDIT_CMD_DEPORT)
#define EDIT_CMD_MASK_ASSIGN	(EDIT_CMD_MASK_ADD | EDIT_CMD_ASSIGN)
#define EDIT_CMD_MASK_ENB	(EDIT_CMD_ENABLE | EDIT_CMD_DISABLE)
#define EDIT_CMD_MASK_ALL \
    (EDIT_CMD_MASK_ASSIGN | EDIT_CMD_MASK_DEL | EDIT_CMD_MASK_ENB | \
	EDIT_CMD_INIT)

 /*
  * Edit command to number mapping, and vice versa.
  */
static NAME_CODE edit_command_table[] = {
    "create", EDIT_CMD_CREATE,
    "import", EDIT_CMD_IMPORT,
    "destroy", EDIT_CMD_DESTROY,
    "deport", EDIT_CMD_DEPORT,
    "enable", EDIT_CMD_ENABLE,
    "disable", EDIT_CMD_DISABLE,
    "assign", EDIT_CMD_ASSIGN,
    "init", EDIT_CMD_INIT,
    0, -1,
};

#define EDIT_CMD_CODE(str) \
	name_code(edit_command_table, NAME_CODE_FLAG_STRICT_CASE, (str))
#define EDIT_CMD_STR(code)	str_name_code(edit_command_table, (code))

 /*
  * Mandatory prefix for non-empty instance names.
  */
#ifndef NAME_PREFIX
#define NAME_PREFIX "postfix-"
#endif
#define HAS_NAME_PREFIX(name) \
     (strncmp((name), NAME_PREFIX, sizeof(NAME_PREFIX)-1) == 0)
#define NEED_NAME_PREFIX(name) \
    ((name) != 0 && strcmp((name), "-") != 0 && !HAS_NAME_PREFIX(name))
#define NAME_SUFFIX(name) ((name) + sizeof(NAME_PREFIX) - 1)

 /*
  * In-core instance structure. Only private information is kept here.
  */
typedef struct instance {
    RING    ring;			/* linkage. */
    char   *config_dir;			/* private */
    char   *queue_dir;			/* private */
    char   *data_dir;			/* private */
    char   *name;			/* null or name */
    char   *gname;			/* null or group */
    int     enabled;			/* start/stop enable */
    int     primary;			/* special */
} INSTANCE;

 /*
  * Managed instance list (edit mode and iterator mode).
  */
static RING instance_hd[1];		/* instance list head */

#define RING_TO_INSTANCE(ring_ptr)	RING_TO_APPL(ring_ptr, INSTANCE, ring)
#define RING_PTR_OF(x)			(&((x)->ring))

#define FOREACH_INSTANCE(entry) \
    for ((entry) = instance_hd; \
	 ((entry) = ring_succ(entry)) != instance_hd;)

#define FOREACH_SECONDARY_INSTANCE(entry) \
    for ((entry) = ring_succ(instance_hd); \
	 ((entry) = ring_succ(entry)) != instance_hd;)

#define NEXT_ITERATOR_INSTANCE(flags, entry) \
    (((flags) & ITER_FLAG_REVERSE) ? ring_pred(entry) : ring_succ(entry))

#define FOREACH_ITERATOR_INSTANCE(flags, entry) \
    for ((entry) = instance_hd; \
	((entry) = NEXT_ITERATOR_INSTANCE(flags, (entry))) != instance_hd;)

 /*
  * Instance selection. One can either select all instances, select by
  * instance name, or select by instance group.
  */
typedef struct {
    int     type;			/* see below */
    char   *name;			/* undefined or name */
} INST_SELECTION;

#define INST_SEL_NONE		0	/* default: no selection */
#define INST_SEL_ALL		1	/* select all instances */
#define INST_SEL_NAME		2	/* select instance name */
#define INST_SEL_GROUP		3	/* select instance group */

 /*
  * Instance name assignment. Each instance may be assigned an instance name
  * (this must be globally unique within a multi-instance cluster) or an
  * instance group name (this is intended to be shared). Externally, empty
  * names may be represented as "-". Internally, we use "" only, to simplify
  * the code.
  */
typedef struct {
    char   *name;			/* null or assigned instance name */
    char   *gname;			/* null or assigned group name */
} NAME_ASSIGNMENT;

 /*
  * Iterator controls for non-edit commands. One can reverse the iteration
  * order, or give special treatment to disabled instances.
  */
#define ITER_FLAG_DEFAULT	0	/* default setting */
#define ITER_FLAG_REVERSE	(1<<0)	/* reverse iteration order */
#define ITER_FLAG_CHECK_DISABLED (1<<1)	/* check disabled instances */
#define ITER_FLAG_SKIP_DISABLED	(1<<2)	/* skip disabled instances */

 /*
  * Environment export controls for edit commands. postmulti(1) exports only
  * things that need to be updated.
  */
#define EXP_FLAG_MULTI_DIRS	(1<<0)	/* export multi_instance_directories */
#define EXP_FLAG_MULTI_NAME	(1<<1)	/* export multi_instance_name */
#define EXP_FLAG_MULTI_GROUP	(1<<2)	/* export multi_instance_group */

 /*
  * To detect conflicts, each instance name and each shared or private
  * pathname is registered in one place, with its owner. Everyone must
  * register their claims when they join, and will be rejected in case of
  * conlict.
  * 
  * Each claim value involves a parameter value (either a directory name or an
  * instance name). Each claim owner is the config_directory pathname plus
  * the parameter name.
  * 
  * XXX: No multi.cf lock file, so this is not race-free.
  */
static HTABLE *claim_table;

#define IS_CLAIMED_BY(name) \
    (claim_table ? htable_find(claim_table, (name)) : 0)

 /*
  * Forward references.
  */
static int iterate_command(int, int, char **, INST_SELECTION *);
static int match_instance_selection(INSTANCE *, INST_SELECTION *);

 /*
  * Convenience.
  */
#define INSTANCE_NAME(i) ((i)->name ? (i)->name : (i)->config_dir)
#define STR(buf)	vstring_str(buf)

/* register_claim - register claim or bust */

static void register_claim(const char *instance_path, const char *param_name,
			           const char *param_value)
{
    const char *myname = "register_claim";
    char   *requestor;
    const char *owner;

    /*
     * Sanity checks.
     */
    if (instance_path == 0 || *instance_path == 0)
	msg_panic("%s: no or empty instance pathname", myname);
    if (param_name == 0 || *param_name == 0)
	msg_panic("%s: no or empty parameter name", myname);
    if (param_value == 0)
	msg_panic("%s: no parameter value", myname);

    /*
     * Make a claim or report a conflict.
     */
    if (claim_table == 0)
	claim_table = htable_create(100);
    requestor = concatenate(instance_path, ", ", param_name, (char *) 0);
    if ((owner = htable_find(claim_table, param_value)) == 0) {
	(void) htable_enter(claim_table, param_value, requestor);
    } else if (strcmp(owner, requestor) == 0) {
	myfree(requestor);
    } else {
	msg_fatal("instance %s, %s=%s conflicts with instance %s=%s",
		instance_path, param_name, param_value, owner, param_value);
    }
}

/* claim_instance_attributes - claim multiple private instance attributes */

static void claim_instance_attributes(INSTANCE *ip)
{

    /*
     * Detect instance name or pathname conflicts between this instance and
     * other instances. XXX: No multi.cf lock file, so this is not race-free.
     */
    if (ip->name)
	register_claim(ip->config_dir, VAR_MULTI_NAME, ip->name);
    register_claim(ip->config_dir, VAR_CONFIG_DIR, ip->config_dir);
    register_claim(ip->config_dir, VAR_QUEUE_DIR, ip->queue_dir);
    register_claim(ip->config_dir, VAR_DATA_DIR, ip->data_dir);
}

/* alloc_instance - allocate a single instance object */

static INSTANCE *alloc_instance(const char *config_dir)
{
    INSTANCE *ip = (INSTANCE *) mymalloc(sizeof(INSTANCE));

    ring_init(RING_PTR_OF(ip));
    ip->config_dir = config_dir ? mystrdup(config_dir) : 0;
    ip->queue_dir = 0;
    ip->data_dir = 0;
    ip->name = 0;
    ip->gname = 0;
    ip->enabled = 0;
    ip->primary = 0;

    return (ip);
}

#if 0

/* free_instance - free a single instance object */

static void free_instance(INSTANCE *ip)
{

    /*
     * If we continue after secondary main.cf file read error, we must be
     * prepared for the case that some parameters may be missing.
     */
    if (ip->name)
	myfree(ip->name);
    if (ip->gname)
	myfree(ip->gname);
    if (ip->config_dir)
	myfree(ip->config_dir);
    if (ip->queue_dir)
	myfree(ip->queue_dir);
    if (ip->data_dir)
	myfree(ip->data_dir);
    myfree((char *) ip);
}

#endif

/* insert_instance - insert instance before selected location, claim names */

static void insert_instance(INSTANCE *ip, INST_SELECTION *selection)
{
    RING   *old;

#define append_instance(ip) insert_instance((ip), (INST_SELECTION *) 0)

    /*
     * Insert instance before the selected site.
     */
    claim_instance_attributes(ip);
    if (ring_succ(instance_hd) == 0)
	ring_init(instance_hd);
    if (selection && selection->type != INST_SEL_NONE) {
	FOREACH_SECONDARY_INSTANCE(old) {
	    if (match_instance_selection(RING_TO_INSTANCE(old), selection)) {
		ring_prepend(old, RING_PTR_OF(ip));
		return;
	    }
	}
	if (selection->type != INST_SEL_ALL)
	    msg_fatal("No matching secondary instances");
    }
    ring_prepend(instance_hd, RING_PTR_OF(ip));
}

/* create_primary_instance - synthetic entry for primary instance */

static INSTANCE *create_primary_instance(void)
{
    INSTANCE *ip = alloc_instance(var_config_dir);

    /*
     * There is no need to load primary instance paramater settings from
     * file. We already have the main.cf parameters of interest in memory.
     */
#define SAVE_INSTANCE_NAME(val) (*(val) ? mystrdup(val) : 0)

    ip->name = SAVE_INSTANCE_NAME(var_multi_name);
    ip->gname = SAVE_INSTANCE_NAME(var_multi_group);
    ip->enabled = var_multi_enable;
    ip->queue_dir = mystrdup(var_queue_dir);
    ip->data_dir = mystrdup(var_data_dir);
    ip->primary = 1;
    return (ip);
}

/* load_instance - read instance parameters from config_dir/main.cf */

static INSTANCE *load_instance(INSTANCE *ip)
{
    VSTREAM *pipe;
    VSTRING *buf;
    char   *name;
    char   *value;
    ARGV   *cmd;
    int     count = 0;
    static NAME_CODE bool_code[] = {
	CONFIG_BOOL_YES, 1,
	CONFIG_BOOL_NO, 0,
	0, -1,
    };

    /*
     * XXX: We could really use a "postconf -E" to expand values in the
     * context of the target main.cf!
     */
#define REQUEST_PARAM_COUNT 5			/* # of requested parameters */

    cmd = argv_alloc(REQUEST_PARAM_COUNT + 3);
    name = concatenate(var_command_dir, "/", "postconf", (char *) 0);
    argv_add(cmd, name, "-c", ip->config_dir,
	     VAR_QUEUE_DIR, VAR_DATA_DIR,
	     VAR_MULTI_NAME, VAR_MULTI_GROUP, VAR_MULTI_ENABLE,
	     (char *) 0);
    myfree(name);
    pipe = vstream_popen(O_RDONLY, VSTREAM_POPEN_ARGV, cmd->argv,
			 VSTREAM_POPEN_END);
    argv_free(cmd);
    if (pipe == 0)
	msg_fatal("Cannot parse %s/main.cf file: %m", ip->config_dir);

    /*
     * Read parameter settings from postconf. See also comments below on
     * whether we should continue or skip groups after error instead of
     * bailing out immediately.
     */
    buf = vstring_alloc(100);
    while (vstring_get_nonl(buf, pipe) != VSTREAM_EOF) {
	if (split_nameval(STR(buf), &name, &value))
	    msg_fatal("Invalid %s/main.cf parameter: %s",
		      ip->config_dir, STR(buf));
	if (strcmp(name, VAR_QUEUE_DIR) == 0 && ++count)
	    ip->queue_dir = mystrdup(value);
	else if (strcmp(name, VAR_DATA_DIR) == 0 && ++count)
	    ip->data_dir = mystrdup(value);
	else if (strcmp(name, VAR_MULTI_NAME) == 0 && ++count)
	    ip->name = SAVE_INSTANCE_NAME(value);
	else if (strcmp(name, VAR_MULTI_GROUP) == 0 && ++count)
	    ip->gname = SAVE_INSTANCE_NAME(value);
	else if (strcmp(name, VAR_MULTI_ENABLE) == 0 && ++count) {
	    /* mail_conf_bool(3) is case insensitive! */
	    ip->enabled = name_code(bool_code, NAME_CODE_FLAG_NONE, value);
	    if (ip->enabled < 0)
		msg_fatal("Unexpected %s/main.cf entry: %s = %s",
			  ip->config_dir, VAR_MULTI_ENABLE, value);
	}
    }
    vstring_free(buf);

    /*
     * XXX We should not bail out while reading a bad secondary main.cf file.
     * When we manage dozens or more instances, the likelihood increases that
     * some file will be damaged or missing after a system crash. That is not
     * a good reason to prevent undamaged Postfix instances from starting.
     */
    if (count != REQUEST_PARAM_COUNT)
	msg_fatal("Failed to obtain all required %s/main.cf parameters",
		  ip->config_dir);

    if (vstream_pclose(pipe))
	msg_fatal("Cannot parse %s/main.cf file", ip->config_dir);
    return (ip);
}

/* load_all_instances - compute list of Postfix instances */

static void load_all_instances(void)
{
    INSTANCE *primary_instance;
    char  **cpp;
    ARGV   *secondary_names;

    /*
     * Avoid unexpected behavior when $multi_instance_directories contains
     * only comma characters. Count the actual number of elements, before we
     * decide that the list is empty.
     */
    secondary_names = argv_split(var_multi_conf_dirs, "\t\n\r, ");

    /*
     * First, the primary instance.  This is synthesized out of thin air.
     */
    primary_instance = create_primary_instance();
    if (secondary_names->argc == 0)
	primary_instance->enabled = 1;		/* Single-instance mode */
    append_instance(primary_instance);

    /*
     * Next, instances defined in $multi_instance_directories. Note:
     * load_instance() has side effects on the global config dictionary, but
     * this does not affect the values that have already been extracted into
     * C variables.
     */
    for (cpp = secondary_names->argv; *cpp != 0; cpp++)
	append_instance(load_instance(alloc_instance(*cpp)));

    argv_free(secondary_names);
}

/* match_instance_selection - match all/name/group constraints */

static int match_instance_selection(INSTANCE *ip, INST_SELECTION *selection)
{
    char   *iname;
    char   *name;

    /*
     * When selecting (rather than assigning names) an instance, we match by
     * the instance name, config_directory path, or the instance name suffix
     * (name without mandatory prefix). Selecting "-" selects the primary
     * instance.
     */
    switch (selection->type) {
    case INST_SEL_NONE:
	return (0);
    case INST_SEL_ALL:
	return (1);
    case INST_SEL_GROUP:
	return (ip->gname != 0 && strcmp(selection->name, ip->gname) == 0);
    case INST_SEL_NAME:
	name = selection->name;
	if (*name == '/' || ip->name == 0)
	    iname = ip->config_dir;
	else if (!HAS_NAME_PREFIX(name) && HAS_NAME_PREFIX(ip->name))
	    iname = NAME_SUFFIX(ip->name);
	else
	    iname = ip->name;
	return (strcmp(name, iname) == 0
		|| (ip->primary && strcmp(name, "-") == 0));
    default:
	msg_panic("match_instance_selection: unknown selection type: %d",
		  selection->type);
    }
}

/* check_setenv - setenv() with extreme prejudice */

static void check_setenv(const char *name, const char *value)
{
#define CLOBBER 1
    if (setenv(name, value, CLOBBER) < 0)
	msg_fatal("setenv: %m");
}

/* prepend_command_path - prepend command_directory to PATH */

static void prepend_command_path(void)
{
    char   *cmd_path;

    /*
     * Carefully prepend "$command_directory:" to PATH. We can free the
     * buffer after check_setenv(), since the value is copied there.
     */
    cmd_path = safe_getenv("PATH");
    cmd_path = concatenate(var_command_dir, ":", (cmd_path && *cmd_path) ?
			   cmd_path : ROOT_PATH, (char *) 0);
    check_setenv("PATH", cmd_path);
    myfree(cmd_path);
}

/* check_shared_dir_status - check and claim shared directories */

static void check_shared_dir_status(void)
{
    struct stat st;
    const SHARED_PATH *sp;

    for (sp = shared_dir_table; sp->param_name; ++sp) {
	if (stat(sp->param_value[0], &st) < 0)
	    msg_fatal("%s = '%s': directory not found: %m",
		      sp->param_name, sp->param_value[0]);
	if (!S_ISDIR(st.st_mode))
	    msg_fatal("%s = '%s' is not a directory",
		      sp->param_name, sp->param_value[0]);
	register_claim(var_config_dir, sp->param_name, sp->param_value[0]);
    }
}

/* check_safe_name - allow instance or group name with only "safe" characters */

static int check_safe_name(const char *s)
{
#define SAFE_PUNCT	"!@%-_=+:./"
    if (*s == 0)
	return (0);
    for (; *s; ++s) {
	if (!ISALNUM(*s) && !strchr(SAFE_PUNCT, *s))
	    return (0);
    }
    return (1);
}

/* check_name_assignments - Check validity of assigned instance or group name */

static void check_name_assignments(NAME_ASSIGNMENT *assignment)
{

    /*
     * Syntax check the assigned instance name. This name is also used to
     * generate directory pathnames, so we must not allow "/" characters.
     * 
     * The value "" will clear the name and is always valid. The command-line
     * parser has already converted "-" into "", to simplify implementation.
     */
    if (assignment->name && *assignment->name) {
	if (!check_safe_name(assignment->name))
	    msg_fatal("Unsafe characters in new instance name: '%s'",
		      assignment->name);
	if (strchr(assignment->name, '/'))
	    msg_fatal("Illegal '/' character in new instance name: '%s'",
		      assignment->name);
	if (NEED_NAME_PREFIX(assignment->name))
	    msg_fatal("New instance name must start with '%s'",
		      NAME_PREFIX);
    }

    /*
     * Syntax check the assigned group name.
     */
    if (assignment->gname && *assignment->gname) {
	if (!check_safe_name(assignment->gname))
	    msg_fatal("Unsafe characters in '-G %s'", assignment->gname);
    }
}

/* do_name_assignments - assign instance/group names */

static int do_name_assignments(INSTANCE *target, NAME_ASSIGNMENT *assignment)
{
    int     export_flags = 0;

    /*
     * The command-line parser has already converted "-" into "", to simplify
     * implementation.
     */
    if (assignment->name
	&& strcmp(assignment->name, target->name ? target->name : "")) {
	register_claim(target->config_dir, VAR_MULTI_NAME, assignment->name);
	if (target->name)
	    myfree(target->name);
	target->name = SAVE_INSTANCE_NAME(assignment->name);
	export_flags |= EXP_FLAG_MULTI_NAME;
    }
    if (assignment->gname
	&& strcmp(assignment->gname, target->gname ? target->gname : "")) {
	if (target->gname)
	    myfree(target->gname);
	target->gname = SAVE_INSTANCE_NAME(assignment->gname);
	export_flags |= EXP_FLAG_MULTI_GROUP;
    }
    return (export_flags);
}

/* make_private_path - generate secondary pathname using primary as template */

static char *make_private_path(const char *param_name,
			               const char *primary_value,
			               NAME_ASSIGNMENT *assignment)
{
    char   *path;
    char   *base;
    char   *end;

    /*
     * The command-line parser has already converted "-" into "", to simplify
     * implementation.
     */
    if (assignment->name == 0 || *assignment->name == 0)
	msg_fatal("Missing %s parameter value", param_name);

    if (*primary_value != '/')
	msg_fatal("Invalid default %s parameter value: '%s': "
		  "specify an absolute pathname",
		  param_name, primary_value);

    base = mystrdup(primary_value);
    if ((end = strrchr(base, '/')) != 0) {
	/* Drop trailing slashes */
	if (end[1] == '\0') {
	    while (--end > base && *end == '/')
		*end = '\0';
	    end = strrchr(base, '/');
	}
	/* Drop last path component */
	while (end > base && *end == '/')
	    *end-- = '\0';
    }
    path = concatenate(base[1] ? base : "", "/",
		       assignment->name, (char *) 0);
    myfree(base);
    return (path);
}

/* assign_new_parameter - assign new instance private name=value */

static void assign_new_parameter(INSTANCE *new, int edit_cmd,
				         const char *arg)
{
    char   *saved_arg;
    char   *name;
    char   *value;
    char   *end;
    char  **target = 0;

    /*
     * With "import", only config_directory is specified on the command line
     * (either explicitly as config_directory=/path/name, or implicitly as
     * instance name). The other private directory pathnames are taken from
     * the existing instance's main.cf file.
     * 
     * With "create", all private pathname parameters are specified on the
     * command line, or generated from an instance name.
     */
    saved_arg = mystrdup(arg);
    if (split_nameval(saved_arg, &name, &value))
	msg_fatal("Malformed parameter setting '%s'", arg);

    if (strcmp(VAR_CONFIG_DIR, name) == 0) {
	target = &new->config_dir;
    } else if (edit_cmd != EDIT_CMD_IMPORT) {
	if (strcmp(VAR_QUEUE_DIR, name) == 0) {
	    target = &new->queue_dir;
	} else if (strcmp(VAR_DATA_DIR, name) == 0) {
	    target = &new->data_dir;
	}
    }
    if (target == 0)
	msg_fatal("Parameter '%s' not valid with action %s",
		  name, EDIT_CMD_STR(edit_cmd));

    /*
     * Extract and assign the parameter value. We do a limited number of
     * checks here. Conflicts between instances are checked by the caller.
     * More checks may be implemented in the helper script if inspired.
     */
    if (*value != '/')
	msg_fatal("Parameter setting '%s' is not an absolute path", name);

    /* Tolerate+trim trailing "/" from readline completion */
    for (end = value + strlen(value) - 1; end > value && *end == '/'; --end)
	*end = 0;

    /* No checks here for "/." or other shoot-foot silliness. */
    if (end == value)
	msg_fatal("Parameter setting '%s' is the root directory", name);

    if (*target)
	myfree(*target);
    *target = mystrdup(value);

    /*
     * Cleanup.
     */
    myfree(saved_arg);
}

/* assign_new_parameters - initialize new instance private parameters */

static void assign_new_parameters(INSTANCE *new, int edit_cmd,
			           char **argv, NAME_ASSIGNMENT *assignment)
{
    const char *owner;

    /*
     * Sanity check the explicit parameter settings. More stringent checks
     * may take place in the helper script.
     */
    while (*argv)
	assign_new_parameter(new, edit_cmd, *argv++);

    /*
     * Initialize any missing private directory pathnames, using the primary
     * configuration directory parameter values as a template, and using the
     * assigned instance name to fill in the blanks.
     * 
     * When importing an existing instance, load private directory pathnames
     * from its main.cf file.
     */
    if (new->config_dir == 0)
	new->config_dir =
	    make_private_path(VAR_CONFIG_DIR, var_config_dir, assignment);
    /* Needed for better-quality error message. */
    if ((owner = IS_CLAIMED_BY(new->config_dir)) != 0)
	msg_fatal("new %s=%s is already in use by instance %s=%s",
		  VAR_CONFIG_DIR, new->config_dir, owner, new->config_dir);
    if (edit_cmd != EDIT_CMD_IMPORT) {
	if (new->queue_dir == 0)
	    new->queue_dir =
		make_private_path(VAR_QUEUE_DIR, var_queue_dir, assignment);
	if (new->data_dir == 0)
	    new->data_dir =
		make_private_path(VAR_DATA_DIR, var_data_dir, assignment);
    } else {
	load_instance(new);
    }
}

/* export_helper_environment - update environment settings for helper command */

static void export_helper_environment(INSTANCE *target, int export_flags)
{
    ARGV   *import_env;
    VSTRING *multi_dirs;
    const SHARED_PATH *sp;
    RING   *entry;

    /*
     * Environment import filter, to enforce consistent behavior whether this
     * command is started by hand, or at system boot time. This is necessary
     * because some shell scripts use environment settings to override
     * main.cf settings.
     */
    import_env = argv_split(var_import_environ, ", \t\r\n");
    clean_env(import_env->argv);
    argv_free(import_env);

    /*
     * Prepend $command_directory: to PATH. This supposedly ensures that
     * naive programs will execute commands from the right Postfix version.
     */
    prepend_command_path();

    /*
     * The following ensures that Postfix's own programs will target the
     * primary instance.
     */
    check_setenv(CONF_ENV_PATH, var_config_dir);

    /*
     * Export the parameter settings that are shared between instances.
     */
    for (sp = shared_dir_table; sp->param_name; ++sp)
	check_setenv(sp->param_name, sp->param_value[0]);

    /*
     * Export the target instance's private directory locations.
     */
    check_setenv(VAR_CONFIG_DIR, target->config_dir);
    check_setenv(VAR_QUEUE_DIR, target->queue_dir);
    check_setenv(VAR_DATA_DIR, target->data_dir);

    /*
     * With operations that add or delete a secondary instance, we export the
     * modified multi_instance_directories parameter value for the primary
     * Postfix instance.
     */
    if (export_flags & EXP_FLAG_MULTI_DIRS) {
	multi_dirs = vstring_alloc(100);
	FOREACH_SECONDARY_INSTANCE(entry) {
	    if (VSTRING_LEN(multi_dirs) > 0)
		VSTRING_ADDCH(multi_dirs, ' ');
	    vstring_strcat(multi_dirs, RING_TO_INSTANCE(entry)->config_dir);
	}
	check_setenv(VAR_MULTI_CONF_DIRS, STR(multi_dirs));
	vstring_free(multi_dirs);
    }

    /*
     * Export updates for the instance name and group. Empty value (or no
     * export) means don't update, "-" means clear.
     */
    if (export_flags & EXP_FLAG_MULTI_NAME)
	check_setenv(VAR_MULTI_NAME, target->name && *target->name ?
		     target->name : "-");

    if (export_flags & EXP_FLAG_MULTI_GROUP)
	check_setenv(VAR_MULTI_GROUP, target->gname && *target->gname ?
		     target->gname : "-");

    /*
     * If we would implement enable/disable commands by exporting the updated
     * parameter value, then we could skip commands that have no effect, just
     * like we can skip "assign" commands that make no change.
     */
}

/* install_new_instance - install and return newly created instance */

static INSTANCE *install_new_instance(int edit_cmd, char **argv,
				              INST_SELECTION *selection,
				              NAME_ASSIGNMENT *assignment,
				              int *export_flags)
{
    INSTANCE *new;

    new = alloc_instance((char *) 0);
    check_name_assignments(assignment);
    assign_new_parameters(new, edit_cmd, argv, assignment);
    *export_flags |=
	(do_name_assignments(new, assignment) | EXP_FLAG_MULTI_DIRS);
    insert_instance(new, selection);
    return (new);
}

/* update_instance - update existing instance, return export flags */

static int update_instance(INSTANCE *target, NAME_ASSIGNMENT *assignment)
{
    int     export_flags;

    check_name_assignments(assignment);
    export_flags = do_name_assignments(target, assignment);
    return (export_flags);
}

/* select_existing_instance - return instance selected for management */

static INSTANCE *select_existing_instance(INST_SELECTION *selection,
					          int unlink_flag,
					          int *export_flags)
{
    INSTANCE *selected = 0;
    RING   *entry;
    INSTANCE *ip;

#define DONT_UNLINK	0
#define DO_UNLINK	1

    if (selection->type != INST_SEL_NAME)
	msg_fatal("Select an instance via '-i name'");

    /* Find the selected instance and its predecessor */
    FOREACH_INSTANCE(entry) {
	if (match_instance_selection(ip = RING_TO_INSTANCE(entry), selection)) {
	    selected = ip;
	    break;
	}
    }

    if (selected == 0)
	msg_fatal("No instance named %s", selection->name);

    if (unlink_flag) {
	/* Splice the target instance out of the list */
	if (ring_pred(entry) == instance_hd)
	    msg_fatal("Cannot remove the primary instance");
	if (selected->enabled)
	    msg_fatal("Cannot remove enabled instances");
	ring_detach(entry);
	if (export_flags == 0)
	    msg_panic("select_existing_instance: no export flags");
	*export_flags |= EXP_FLAG_MULTI_DIRS;
    }
    return (selected);
}

/* manage - create/destroy/... manage instances */

static NORETURN manage(int edit_cmd, int argc, char **argv,
		               INST_SELECTION *selection,
		               NAME_ASSIGNMENT *assignment)
{
    char   *cmd;
    INSTANCE *target;
    int     export_flags;

    /*
     * Edit mode is not subject to iterator controls.
     */
#define NO_EXPORT_FLAGS		((int *) 0)
    export_flags = 0;

    switch (edit_cmd) {
    case EDIT_CMD_INIT:
	target = create_primary_instance();
	break;

    case EDIT_CMD_CREATE:
    case EDIT_CMD_IMPORT:
	load_all_instances();
	target = install_new_instance(edit_cmd, argv, selection,
				      assignment, &export_flags);
	break;

    case EDIT_CMD_ASSIGN:
	load_all_instances();
	target =
	    select_existing_instance(selection, DONT_UNLINK, NO_EXPORT_FLAGS);
	export_flags |= update_instance(target, assignment);
	if (export_flags == 0)
	    exit(0);
	break;

    case EDIT_CMD_DESTROY:
    case EDIT_CMD_DEPORT:
	load_all_instances();
	target = select_existing_instance(selection, DO_UNLINK, &export_flags);
	break;

    default:
	load_all_instances();
	target =
	    select_existing_instance(selection, DONT_UNLINK, NO_EXPORT_FLAGS);
	break;
    }

    /*
     * Set up the helper script's process environment, and execute the helper
     * script.
     */
#define HELPER "postmulti-script"

    export_helper_environment(target, export_flags);
    cmd = concatenate(var_daemon_dir, "/" HELPER, (char *) 0);
    execl(cmd, cmd, "-e", EDIT_CMD_STR(edit_cmd), (char *) 0);
    msg_fatal("%s: %m", cmd);
}

/* run_user_command - execute external command with requested MAIL_CONFIG env */

static int run_user_command(INSTANCE *ip, int iter_cmd, int iter_flags,
			            char **argv)
{
    WAIT_STATUS_T status;
    int     pid;
    int     wpid;

    /*
     * Set up a process environment. The postfix(1) command needs MAIL_CONFIG
     * (or the equivalent command-line option); it overrides everything else.
     * 
     * postmulti(1) typically runs various Postfix utilities (postsuper, ...) in
     * the context of one or more instances. It can also run various scripts
     * on the users PATH. So we can't clobber the user's PATH, but do want to
     * make sure that the utilities in $command_directory are always found in
     * the right place (or at all).
     */
    switch (pid = fork()) {
    case -1:
	msg_warn("fork %s: %m", argv[0]);
	return -1;
    case 0:
	check_setenv(CONF_ENV_PATH, ip->config_dir);
	if (iter_cmd != ITER_CMD_POSTFIX) {
	    check_setenv(VAR_DAEMON_DIR, var_daemon_dir);
	    check_setenv(VAR_COMMAND_DIR, var_command_dir);
	    check_setenv(VAR_CONFIG_DIR, ip->config_dir);
	    check_setenv(VAR_QUEUE_DIR, ip->queue_dir);
	    check_setenv(VAR_DATA_DIR, ip->data_dir);
	    check_setenv(VAR_MULTI_NAME, ip->name ? ip->name : "");
	    check_setenv(VAR_MULTI_GROUP, ip->gname ? ip->gname : "");
	    check_setenv(VAR_MULTI_ENABLE, ip->enabled ?
			 CONFIG_BOOL_YES : CONFIG_BOOL_NO);
	    prepend_command_path();
	}

	/*
	 * Replace: postfix -- start ... With: postfix -- check ...
	 */
	if (iter_cmd == ITER_CMD_POSTFIX
	    && (iter_flags & ITER_FLAG_CHECK_DISABLED) && !ip->enabled)
	    argv[2] = "check";

	execvp(argv[0], argv);
	msg_fatal("execvp %s: %m", argv[0]);
    default:
	do {
	    wpid = waitpid(pid, &status, 0);
	} while (wpid == -1 && errno == EINTR);
	return (wpid == -1 ? -1 :
		WIFEXITED(status) ? WEXITSTATUS(status) : 1);
    }
}

/* word_in_list - look up command in start, stop, or control list */

static int word_in_list(char *cmdlist, const char *cmd)
{
    char   *saved;
    char   *cp;
    char   *elem;

    cp = saved = mystrdup(cmdlist);
    while ((elem = mystrtok(&cp, "\t\n\r, ")) != 0 && strcmp(elem, cmd) != 0)
	 /* void */ ;
    myfree(saved);
    return (elem != 0);
}

/* iterate_postfix_command - execute postfix(1) command */

static int iterate_postfix_command(int iter_cmd, int argc, char **argv,
				           INST_SELECTION *selection)
{
    int     exit_status;
    char   *cmd;
    ARGV   *my_argv;
    int     iter_flags;

    /*
     * Override the iterator controls.
     */
    if (word_in_list(var_multi_start_cmds, argv[0])) {
	iter_flags = ITER_FLAG_CHECK_DISABLED;
    } else if (word_in_list(var_multi_stop_cmds, argv[0])) {
	iter_flags = ITER_FLAG_SKIP_DISABLED | ITER_FLAG_REVERSE;
    } else if (word_in_list(var_multi_cntrl_cmds, argv[0])) {
	iter_flags = ITER_FLAG_SKIP_DISABLED;
    } else {
	iter_flags = 0;
    }

    /*
     * Override the command line in a straightforward manner: prepend
     * "postfix --" to the command arguments. Other overrides (environment,
     * start -> check) are implemented below the iterator.
     */
#define POSTFIX_CMD	"postfix"

    my_argv = argv_alloc(argc + 2);
    cmd = concatenate(var_command_dir, "/" POSTFIX_CMD, (char *) 0);
    argv_add(my_argv, cmd, "--", (char *) 0);
    myfree(cmd);
    while (*argv)
	argv_add(my_argv, *argv++, (char *) 0);

    /*
     * Execute the command for all applicable Postfix instances.
     */
    exit_status =
	iterate_command(iter_cmd, iter_flags, my_argv->argv, selection);

    argv_free(my_argv);
    return (exit_status);
}

/* list_instances - list all selected instances */

static void list_instances(int iter_flags, INST_SELECTION *selection)
{
    RING   *entry;
    INSTANCE *ip;

    /*
     * Iterate over the selected instances.
     */
    FOREACH_ITERATOR_INSTANCE(iter_flags, entry) {
	ip = RING_TO_INSTANCE(entry);
	if (match_instance_selection(ip, selection))
	    vstream_printf("%-15s %-15s %-9s %s\n",
			   ip->name ? ip->name : "-",
			   ip->gname ? ip->gname : "-",
			   ip->enabled ? "y" : "n",
			   ip->config_dir);
    }
    if (vstream_fflush(VSTREAM_OUT))
	msg_fatal("error writing output: %m");
}

/* iterate_command - execute command for selected instances */

static int iterate_command(int iter_cmd, int iter_flags, char **argv,
			           INST_SELECTION *selection)
{
    int     exit_status = 0;
    int     matched = 0;
    RING   *entry;
    INSTANCE *ip;

    /*
     * Iterate over the selected instances.
     */
    FOREACH_ITERATOR_INSTANCE(iter_flags, entry) {
	ip = RING_TO_INSTANCE(entry);
	if ((iter_flags & ITER_FLAG_SKIP_DISABLED) && !ip->enabled)
	    continue;
	if (!match_instance_selection(ip, selection))
	    continue;
	matched = 1;

	/* Run the requested command */
	if (run_user_command(ip, iter_cmd, iter_flags, argv) != 0)
	    exit_status = 1;
    }
    if (matched == 0)
	msg_fatal("No matching instances");

    return (exit_status);
}

/* iterate - Iterate over all or selected instances */

static NORETURN iterate(int iter_cmd, int iter_flags, int argc, char **argv,
			        INST_SELECTION *selection)
{
    int     exit_status;

    /*
     * In iterator mode, no selection means wild-card selection.
     */
    if (selection->type == INST_SEL_NONE)
	selection->type = INST_SEL_ALL;

    /*
     * Load the in-memory instance table from main.cf files.
     */
    load_all_instances();

    /*
     * Iterate over the selected instances.
     */
    switch (iter_cmd) {
    case ITER_CMD_POSTFIX:
	exit_status = iterate_postfix_command(iter_cmd, argc, argv, selection);
	break;
    case ITER_CMD_LIST:
	list_instances(iter_flags, selection);
	exit_status = 0;
	break;
    case ITER_CMD_GENERIC:
	exit_status = iterate_command(iter_cmd, iter_flags, argv, selection);
	break;
    default:
	msg_panic("iterate: unknown mode: %d", iter_cmd);
    }
    exit(exit_status);
}

static NORETURN usage(const char *progname)
{
    msg_fatal("Usage:"
	      "%s -l [-v] [-a] [-g group] [-i instance] | "
	      "%s -p [-v] [-a] [-g group] [-i instance] command... | "
	      "%s -x [-v] [-a] [-i name] [-g group] command... | "
	      "%s -e action [-v] [-a] [-i name] [-g group] [-I name] "
	      "[-G group] [param=value ...]",
	      progname, progname, progname, progname);
}

MAIL_VERSION_STAMP_DECLARE;

/* main - iterate commands over multiple instance or manage instances */

int     main(int argc, char **argv)
{
    int     fd;
    struct stat st;
    char   *slash;
    char   *config_dir;
    int     ch;
    static const CONFIG_STR_TABLE str_table[] = {
	VAR_MULTI_START_CMDS, DEF_MULTI_START_CMDS, &var_multi_start_cmds, 0, 0,
	VAR_MULTI_STOP_CMDS, DEF_MULTI_STOP_CMDS, &var_multi_stop_cmds, 0, 0,
	VAR_MULTI_CNTRL_CMDS, DEF_MULTI_CNTRL_CMDS, &var_multi_cntrl_cmds, 0, 0,
	0,
    };
    int     instance_select_count = 0;
    int     command_mode_count = 0;
    INST_SELECTION selection;
    NAME_ASSIGNMENT assignment;
    int     iter_flags = ITER_FLAG_DEFAULT;
    int     cmd_mode = 0;
    int     code;

    selection.type = INST_SEL_NONE;
    assignment.name = assignment.gname = 0;

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
     * Set up diagnostics. XXX What if stdin is the system console during
     * boot time? It seems a bad idea to log startup errors to the console.
     * This is UNIX, a system that can run without hand holding.
     */
    if ((slash = strrchr(argv[0], '/')) != 0 && slash[1])
	argv[0] = slash + 1;
    if (isatty(STDERR_FILENO))
	msg_vstream_init(argv[0], VSTREAM_ERR);
    msg_syslog_init(argv[0], LOG_PID, LOG_FACILITY);

    /*
     * Check the Postfix library version as soon as we enable logging.
     */
    MAIL_VERSION_CHECK;

    if ((config_dir = getenv(CONF_ENV_PATH)) != 0
	&& strcmp(config_dir, DEF_CONFIG_DIR) != 0)
	msg_fatal("Non-default configuration directory: %s=%s",
		  CONF_ENV_PATH, config_dir);

    /*
     * Parse switches.
     */
    while ((ch = GETOPT(argc, argv, "ae:g:i:G:I:lpRvx")) > 0) {
	switch (ch) {
	default:
	    usage(argv[0]);
	    /* NOTREACHED */
	case 'a':
	    if (selection.type != INST_SEL_ALL)
		instance_select_count++;
	    selection.type = INST_SEL_ALL;
	    break;
	case 'e':
	    if ((code = EDIT_CMD_CODE(optarg)) < 0)
		msg_fatal("Invalid '-e' edit action '%s'. Specify '%s', "
			  "'%s', '%s', '%s', '%s', '%s', '%s', '%s' or '%s'",
			  optarg,
			  EDIT_CMD_STR(EDIT_CMD_CREATE),
			  EDIT_CMD_STR(EDIT_CMD_DESTROY),
			  EDIT_CMD_STR(EDIT_CMD_IMPORT),
			  EDIT_CMD_STR(EDIT_CMD_DEPORT),
			  EDIT_CMD_STR(EDIT_CMD_ENABLE),
			  EDIT_CMD_STR(EDIT_CMD_DISABLE),
			  EDIT_CMD_STR(EDIT_CMD_ASSIGN),
			  EDIT_CMD_STR(EDIT_CMD_INIT),
			  optarg);
	    if (cmd_mode != code)
		command_mode_count++;
	    cmd_mode = code;
	    break;
	case 'g':
	    instance_select_count++;
	    selection.type = INST_SEL_GROUP;
	    selection.name = optarg;
	    break;
	case 'i':
	    instance_select_count++;
	    selection.type = INST_SEL_NAME;
	    selection.name = optarg;
	    break;
	case 'G':
	    if (assignment.gname != 0)
		msg_fatal("Specify at most one '-G' option");
	    assignment.gname = strcmp(optarg, "-") == 0 ? "" : optarg;
	    break;
	case 'I':
	    if (assignment.name != 0)
		msg_fatal("Specify at most one '-I' option");
	    assignment.name = strcmp(optarg, "-") == 0 ? "" : optarg;
	    break;
	case 'l':
	    if (cmd_mode != ITER_CMD_LIST)
		command_mode_count++;
	    cmd_mode = ITER_CMD_LIST;
	    break;
	case 'p':
	    if (cmd_mode != ITER_CMD_POSTFIX)
		command_mode_count++;
	    cmd_mode = ITER_CMD_POSTFIX;
	    break;
	case 'R':
	    iter_flags ^= ITER_FLAG_REVERSE;
	    break;
	case 'v':
	    msg_verbose++;
	    check_setenv(CONF_ENV_VERB, "");
	    break;
	case 'x':
	    if (cmd_mode != ITER_CMD_GENERIC)
		command_mode_count++;
	    cmd_mode = ITER_CMD_GENERIC;
	    break;
	}
    }

    /*
     * Report missing arguments, or wrong arguments in the wrong context.
     */
    if (instance_select_count > 1)
	msg_fatal("Specity no more than one of '-a', '-g', '-i'");

    if (command_mode_count != 1)
	msg_fatal("Specify exactly one of '-e', '-l', '-p', '-x'");

    if (cmd_mode == ITER_CMD_LIST && argc > optind)
	msg_fatal("Command not allowed with '-l'");

    if (cmd_mode == ITER_CMD_POSTFIX || cmd_mode == ITER_CMD_GENERIC)
	if (argc == optind)
	    msg_fatal("Command required with '-p' or '-x' option");

    if (cmd_mode == ITER_CMD_POSTFIX || (cmd_mode & EDIT_CMD_MASK_ALL))
	if (iter_flags != ITER_FLAG_DEFAULT)
	    msg_fatal("The '-p' and '-e' options preclude the use of '-R'");

    if ((cmd_mode & EDIT_CMD_MASK_ASSIGN) == 0
	&& (assignment.name || assignment.gname)) {
	if ((cmd_mode & EDIT_CMD_MASK_ALL) == 0)
	    msg_fatal("Cannot assign instance name or group without '-e %s'",
		      EDIT_CMD_STR(EDIT_CMD_ASSIGN));
	else
	    msg_fatal("Cannot assign instance name or group with '-e %s'",
		      EDIT_CMD_STR(cmd_mode));
    }
    if (cmd_mode & EDIT_CMD_MASK_ALL) {
	if (cmd_mode == EDIT_CMD_ASSIGN
	    && (assignment.name == 0 && assignment.gname == 0))
	    msg_fatal("Specify new instance name or group with '-e %s'",
		      EDIT_CMD_STR(cmd_mode));

	if ((cmd_mode & ~EDIT_CMD_MASK_ADD) != 0 && argc > optind)
	    msg_fatal("Parameter overrides not valid with '-e %s'",
		      EDIT_CMD_STR(cmd_mode));
    }

    /*
     * Proces main.cf parameters.
     */
    mail_conf_read();
    get_mail_conf_str_table(str_table);

    /*
     * Sanity checks.
     */
    check_shared_dir_status();

    /*
     * Iterate over selected instances, or manipulate one instance.
     */
    if (cmd_mode & ITER_CMD_MASK_ALL)
	iterate(cmd_mode, iter_flags, argc - optind, argv + optind, &selection);
    else
	manage(cmd_mode, argc - optind, argv + optind, &selection, &assignment);
}
