/*
 * Copyright (C) 1986-2005 The Free Software Foundation, Inc.
 *
 * Portions Copyright (C) 1998-2005 Derek Price, Ximbiot <http://ximbiot.com>,
 *                                  and others.
 *
 * Portions Copyright (C) 1992, Brian Berliner and Jeff Polk
 * Portions Copyright (C) 1989-1992, Brian Berliner
 *
 * You may distribute under the terms of the GNU General Public License as
 * specified in the README file that comes with the CVS kit.
 */

/*
 * basic information used in all source files
 *
 */


#ifdef HAVE_CONFIG_H
# include <config.h>		/* this is stuff found via autoconf */
#endif /* CONFIG_H */

/* Add GNU attribute suppport.  */
#ifndef __attribute__
/* This feature is available in gcc versions 2.5 and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 5) || __STRICT_ANSI__
#  define __attribute__(Spec) /* empty */
# else
#   if __GNUC__ == 2 && __GNUC_MINOR__ < 96
#    define __pure__	/* empty */
#   endif
#   if __GNUC__ < 3
#    define __malloc__	/* empty */
#   endif
# endif
/* The __-protected variants of `format' and `printf' attributes
   are accepted by gcc versions 2.6.4 (effectively 2.7) and later.  */
# if __GNUC__ < 2 || (__GNUC__ == 2 && __GNUC_MINOR__ < 7)
#  define __const__	const
#  define __format__	format
#  define __noreturn__	noreturn
#  define __printf__	printf
# endif
#endif /* __attribute__ */

/* Some GNULIB headers require that we include system headers first.  */
#include "system.h"

/* begin GNULIB headers */
#include "dirname.h"
#include "exit.h"
#include "getdate.h"
#include "minmax.h"
#include "regex.h"
#include "strcase.h"
#include "stat-macros.h"
#include "timespec.h"
#include "unlocked-io.h"
#include "xalloc.h"
#include "xgetcwd.h"
#include "xreadlink.h"
#include "xsize.h"
/* end GNULIB headers */

#if ! STDC_HEADERS
char *getenv();
#endif /* ! STDC_HEADERS */

/* Under OS/2, <stdio.h> doesn't define popen()/pclose(). */
#ifdef USE_OWN_POPEN
#include "popen.h"
#endif

#ifdef SERVER_SUPPORT
/* If the system doesn't provide strerror, it won't be declared in
   string.h.  */
char *strerror (int);
#endif

#include "hash.h"
#include "stack.h"

#include "root.h"

#if defined(SERVER_SUPPORT) || defined(CLIENT_SUPPORT)
# include "client.h"
#endif

#ifdef MY_NDBM
#include "myndbm.h"
#else
#include <ndbm.h>
#endif /* MY_NDBM */

#include "wait.h"

#include "rcs.h"



/* Note that the _ONLY_ reason for PATH_MAX is if various system calls (getwd,
 * getcwd, readlink) require/want us to use it.  All other parts of CVS
 * allocate pathname buffers dynamically, and we want to keep it that way.
 */
#include "pathmax.h"



/* Definitions for the CVS Administrative directory and the files it contains.
   Here as #define's to make changing the names a simple task.  */

#ifdef USE_VMS_FILENAMES
#define CVSADM          getCVSDir("")
#define CVSADM_ENT      getCVSDir("/Entries.")
#define CVSADM_ENTBAK   getCVSDir("/Entries.Backup")
#define CVSADM_ENTLOG   getCVSDir("/Entries.Log")
#define CVSADM_ENTSTAT  getCVSDir("/Entries.Static")
#define CVSADM_REP      getCVSDir("/Repository.")
#define CVSADM_ROOT     getCVSDir("/Root.")
#define CVSADM_CIPROG   getCVSDir("/Checkin.prog")
#define CVSADM_UPROG    getCVSDir("/Update.prog")
#define CVSADM_TAG      getCVSDir("/Tag.")
#define CVSADM_NOTIFY   getCVSDir("/Notify.")
#define CVSADM_NOTIFYTMP getCVSDir("/Notify.tmp")
#define CVSADM_BASE      getCVSDir("/Base")
#define CVSADM_BASEREV   getCVSDir("/Baserev.")
#define CVSADM_BASEREVTMP getCVSDir("/Baserev.tmp")
#define CVSADM_TEMPLATE getCVSDir("/Template.")
#else /* USE_VMS_FILENAMES */
#define	CVSADM		getCVSDir("")
#define	CVSADM_ENT	getCVSDir("/Entries")
#define	CVSADM_ENTBAK	getCVSDir("/Entries.Backup")
#define CVSADM_ENTLOG	getCVSDir("/Entries.Log")
#define	CVSADM_ENTSTAT	getCVSDir("/Entries.Static")
#define	CVSADM_REP	getCVSDir("/Repository")
#define	CVSADM_ROOT	getCVSDir("/Root")
#define	CVSADM_CIPROG	getCVSDir("/Checkin.prog")
#define	CVSADM_UPROG	getCVSDir("/Update.prog")
#define	CVSADM_TAG	getCVSDir("/Tag")
#define CVSADM_NOTIFY	getCVSDir("/Notify")
#define CVSADM_NOTIFYTMP getCVSDir("/Notify.tmp")
/* A directory in which we store base versions of files we currently are
   editing with "cvs edit".  */
#define CVSADM_BASE     getCVSDir("/Base")
#define CVSADM_BASEREV  getCVSDir("/Baserev")
#define CVSADM_BASEREVTMP getCVSDir("/Baserev.tmp")
/* File which contains the template for use in log messages.  */
#define CVSADM_TEMPLATE getCVSDir("/Template")
#endif /* USE_VMS_FILENAMES */

/* This is the special directory which we use to store various extra
   per-directory information in the repository.  It must be the same as
   CVSADM to avoid creating a new reserved directory name which users cannot
   use, but is a separate #define because if anyone changes it (which I don't
   recommend), one needs to deal with old, unconverted, repositories.
   
   See fileattr.h for details about file attributes, the only thing stored
   in CVSREP currently.  */
#define CVSREP getCVSDir("")

/*
 * Definitions for the CVSROOT Administrative directory and the files it
 * contains.  This directory is created as a sub-directory of the $CVSROOT
 * environment variable, and holds global administration information for the
 * entire source repository beginning at $CVSROOT.
 */
#define	CVSROOTADM		"CVSROOT"
#define	CVSROOTADM_CHECKOUTLIST "checkoutlist"
#define CVSROOTADM_COMMITINFO	"commitinfo"
#define CVSROOTADM_CONFIG	"config"
#define	CVSROOTADM_HISTORY	"history"
#define	CVSROOTADM_IGNORE	"cvsignore"
#define	CVSROOTADM_LOGINFO	"loginfo"
#define	CVSROOTADM_MODULES	"modules"
#define CVSROOTADM_NOTIFY	"notify"
#define CVSROOTADM_PASSWD	"passwd"
#define CVSROOTADM_POSTADMIN	"postadmin"
#define CVSROOTADM_POSTPROXY	"postproxy"
#define CVSROOTADM_POSTTAG	"posttag"
#define CVSROOTADM_POSTWATCH	"postwatch"
#define CVSROOTADM_PREPROXY	"preproxy"
#define	CVSROOTADM_RCSINFO	"rcsinfo"
#define CVSROOTADM_READERS	"readers"
#define CVSROOTADM_TAGINFO      "taginfo"
#define CVSROOTADM_USERS	"users"
#define CVSROOTADM_VALTAGS	"val-tags"
#define CVSROOTADM_VERIFYMSG    "verifymsg"
#define CVSROOTADM_WRAPPER	"cvswrappers"
#define CVSROOTADM_WRITERS	"writers"

#define CVSNULLREPOS		"Emptydir"	/* an empty directory */

/* Other CVS file names */

/* Files go in the attic if the head main branch revision is dead,
   otherwise they go in the regular repository directories.  The whole
   concept of having an attic is sort of a relic from before death
   support but on the other hand, it probably does help the speed of
   some operations (such as main branch checkouts and updates).  */
#define	CVSATTIC	"Attic"

#define	CVSLCK		"#cvs.lock"
#define	CVSHISTORYLCK	"#cvs.history.lock"
#define	CVSVALTAGSLCK	"#cvs.val-tags.lock"
#define	CVSRFL		"#cvs.rfl"
#define	CVSPFL		"#cvs.pfl"
#define	CVSWFL		"#cvs.wfl"
#define CVSPFLPAT	"#cvs.pfl.*"	/* wildcard expr to match plocks */
#define CVSRFLPAT	"#cvs.rfl.*"	/* wildcard expr to match read locks */
#define	CVSEXT_LOG	",t"
#define	CVSPREFIX	",,"
#define CVSDOTIGNORE	".cvsignore"
#define CVSDOTWRAPPER   ".cvswrappers"

/* Command attributes -- see function lookup_command_attribute(). */
#define CVS_CMD_IGNORE_ADMROOT        1

/* Set if CVS needs to create a CVS/Root file upon completion of this
   command.  The name may be slightly confusing, because the flag
   isn't really as general purpose as it seems (it is not set for cvs
   release).  */

#define CVS_CMD_USES_WORK_DIR         2

#define CVS_CMD_MODIFIES_REPOSITORY   4

/* miscellaneous CVS defines */

/* This is the string which is at the start of the non-log-message lines
   that we put up for the user when they edit the log message.  */
#define	CVSEDITPREFIX	"CVS: "
/* Number of characters in CVSEDITPREFIX to compare when deciding to strip
   off those lines.  We don't check for the space, to accomodate users who
   have editors which strip trailing spaces.  */
#define CVSEDITPREFIXLEN 4

#define	CVSLCKAGE	(60*60)		/* 1-hour old lock files cleaned up */
#define	CVSLCKSLEEP	30		/* wait 30 seconds before retrying */
#define	CVSBRANCH	"1.1.1"		/* RCS branch used for vendor srcs */

#ifdef USE_VMS_FILENAMES
# define BAKPREFIX	"_$"
#else /* USE_VMS_FILENAMES */
# define BAKPREFIX	".#"		/* when rcsmerge'ing */
#endif /* USE_VMS_FILENAMES */

/*
 * Special tags. -rHEAD	refers to the head of an RCS file, regardless of any
 * sticky tags. -rBASE	refers to the current revision the user has checked
 * out This mimics the behaviour of RCS.
 */
#define	TAG_HEAD	"HEAD"
#define	TAG_BASE	"BASE"

/* Environment variable used by CVS */
#define	CVSREAD_ENV	"CVSREAD"	/* make files read-only */
#define	CVSREAD_DFLT	0		/* writable files by default */

#define	CVSREADONLYFS_ENV "CVSREADONLYFS" /* repository is read-only */

#define	TMPDIR_ENV	"TMPDIR"	/* Temporary directory */
#define	CVS_PID_ENV	"CVS_PID"	/* pid of running cvs */

#define	EDITOR1_ENV	"CVSEDITOR"	/* which editor to use */
#define	EDITOR2_ENV	"VISUAL"	/* which editor to use */
#define	EDITOR3_ENV	"EDITOR"	/* which editor to use */

#define	CVSROOT_ENV	"CVSROOT"	/* source directory root */
/* Define CVSROOT_DFLT to a fallback value for CVSROOT.
 *
#undef	CVSROOT_DFL
 */

#define	IGNORE_ENV	"CVSIGNORE"	/* More files to ignore */
#define WRAPPER_ENV     "CVSWRAPPERS"   /* name of the wrapper file */

#define	CVSUMASK_ENV	"CVSUMASK"	/* Effective umask for repository */

/*
 * If the beginning of the Repository matches the following string, strip it
 * so that the output to the logfile does not contain a full pathname.
 *
 * If the CVSROOT environment variable is set, it overrides this define.
 */
#define	REPOS_STRIP	"/master/"

/* Large enough to hold DATEFORM.  Not an arbitrary limit as long as
   it is used for that purpose, and not to hold a string from the
   command line, the client, etc.  */
#define MAXDATELEN	50

/* The type of an entnode.  */
enum ent_type
{
    ENT_FILE, ENT_SUBDIR
};

/* structure of a entry record */
struct entnode
{
    enum ent_type type;
    char *user;
    char *version;

    /* Timestamp, or "" if none (never NULL).  */
    char *timestamp;

    /* Keyword expansion options, or "" if none (never NULL).  */
    char *options;

    char *tag;
    char *date;
    char *conflict;
};
typedef struct entnode Entnode;

/* The type of request that is being done in do_module() */
enum mtype
{
    CHECKOUT, TAG, PATCH, EXPORT, MISC
};

/*
 * structure used for list-private storage by Entries_Open() and
 * Version_TS() and Find_Directories().
 */
struct stickydirtag
{
    /* These fields pass sticky tag information from Entries_Open() to
       Version_TS().  */
    int aflag;
    char *tag;
    char *date;
    int nonbranch;

    /* This field is set by Entries_Open() if there was subdirectory
       information; Find_Directories() uses it to see whether it needs
       to scan the directory itself.  */
    int subdirs;
};

/* Flags for find_{names,dirs} routines */
#define W_LOCAL			0x01	/* look for files locally */
#define W_REPOS			0x02	/* look for files in the repository */
#define W_ATTIC			0x04	/* look for files in the attic */

/* Flags for return values of direnter procs for the recursion processor */
enum direnter_type
{
    R_PROCESS = 1,			/* process files and maybe dirs */
    R_SKIP_FILES,			/* don't process files in this dir */
    R_SKIP_DIRS,			/* don't process sub-dirs */
    R_SKIP_ALL				/* don't process files or dirs */
};
#ifdef ENUMS_CAN_BE_TROUBLE
typedef int Dtype;
#else
typedef enum direnter_type Dtype;
#endif

/* Recursion processor lock types */
#define CVS_LOCK_NONE	0
#define CVS_LOCK_READ	1
#define CVS_LOCK_WRITE	2

/* Option flags for Parse_Info() */
#define PIOPT_ALL 1	/* accept "all" keyword */

extern const char *program_name, *program_path, *cvs_cmd_name;
extern char *Editor;
extern int cvsadmin_root;
extern char *CurDir;
extern int really_quiet, quiet;
extern int use_editor;
extern int cvswrite;
extern mode_t cvsumask;

/* Temp dir abstraction.  */
/* From main.c.  */
const char *get_cvs_tmp_dir (void);
/* From filesubr.c.  */
const char *get_system_temp_dir (void);
void push_env_temp_dir (void);


/* This global variable holds the global -d option.  It is NULL if -d
   was not used, which means that we must get the CVSroot information
   from the CVSROOT environment variable or from a CVS/Root file.  */
extern char *CVSroot_cmdline;

/* This variable keeps track of all of the CVSROOT directories that
 * have been seen by the client.
 */
extern List *root_directories;

char *emptydir_name (void);
int safe_location (char *);

extern int trace;		/* Show all commands */
extern int noexec;		/* Don't modify disk anywhere */
extern int nolock;		/* Don't create locks */
extern int readonlyfs;		/* fail on all write locks; succeed all read locks */
extern int logoff;		/* Don't write history entry */



#define LOGMSG_REREAD_NEVER 0	/* do_verify - never  reread message */
#define LOGMSG_REREAD_ALWAYS 1	/* do_verify - always reread message */
#define LOGMSG_REREAD_STAT 2	/* do_verify - reread message if changed */

/* This header needs the LOGMSG_* defns above.  */
#include "parseinfo.h"

/* This structure holds the global configuration data.  */
extern struct config *config;

#ifdef CLIENT_SUPPORT
extern List *dirs_sent_to_server; /* used to decide which "Argument
				     xxx" commands to send to each
				     server in multiroot mode. */
#endif

extern char *hostname;

/* Externs that are included directly in the CVS sources */

int RCS_merge (RCSNode *, const char *, const char *, const char *,
               const char *, const char *);
/* Flags used by RCS_* functions.  See the description of the individual
   functions for which flags mean what for each function.  */
#define RCS_FLAGS_FORCE 1
#define RCS_FLAGS_DEAD 2
#define RCS_FLAGS_QUIET 4
#define RCS_FLAGS_MODTIME 8
#define RCS_FLAGS_KEEPFILE 16
#define RCS_FLAGS_USETIME 32

int RCS_exec_rcsdiff (RCSNode *rcsfile, int diff_argc,
                      char * const *diff_argv, const char *options,
                      const char *rev1, const char *rev1_cache,
                      const char *rev2,
                      const char *label1, const char *label2,
                      const char *workfile);
int diff_exec (const char *file1, const char *file2,
               const char *label1, const char *label2,
               int iargc, char * const *iargv, const char *out);


#include "error.h"

/* If non-zero, error will use the CVS protocol to report error
 * messages.  This will only be set in the CVS server parent process;
 * most other code is run via do_cvs_command, which forks off a child
 * process and packages up its stderr in the protocol.
 *
 * This needs to be here rather than in error.h in order to use an unforked
 * error.h from GNULIB.
 */
extern int error_use_protocol;


DBM *open_module (void);
List *Find_Directories (char *repository, int which, List *entries);
void Entries_Close (List *entries);
List *Entries_Open (int aflag, char *update_dir);
void Subdirs_Known (List *entries);
void Subdir_Register (List *, const char *, const char *);
void Subdir_Deregister (List *, const char *, const char *);
const char *getCVSDir (const char *);

void parse_tagdate (char **tag, char **date, const char *input);
char *Make_Date (const char *rawdate);
char *date_from_time_t (time_t);
void date_to_internet (char *, const char *);
void date_to_tm (struct tm *, const char *);
void tm_to_internet (char *, const struct tm *);
char *gmformat_time_t (time_t unixtime);
char *format_date_alloc (char *text);

char *Name_Repository (const char *dir, const char *update_dir);
const char *Short_Repository (const char *repository);
void Sanitize_Repository_Name (char *repository);

char *entries_time (time_t unixtime);
time_t unix_time_stamp (const char *file);
char *time_stamp (const char *file);

typedef	int (*CALLPROC)	(const char *repository, const char *value,
                         void *closure);
int Parse_Info (const char *infofile, const char *repository,
                CALLPROC callproc, int opt, void *closure);

typedef	RETSIGTYPE (*SIGCLEANUPPROC)	(int);
int SIG_register (int sig, SIGCLEANUPPROC sigcleanup);
bool isdir (const char *file);
bool isfile (const char *file);
ssize_t islink (const char *file);
bool isdevice (const char *file);
bool isreadable (const char *file);
bool iswritable (const char *file);
bool isaccessible (const char *file, const int mode);
const char *last_component (const char *path);
char *get_homedir (void);
char *strcat_filename_onto_homedir (const char *, const char *);
char *cvs_temp_name (void);
FILE *cvs_temp_file (char **filename);

int ls (int argc, char *argv[]);
int unlink_file (const char *f);
int unlink_file_dir (const char *f);

/* This is the structure that the recursion processor passes to the
   fileproc to tell it about a particular file.  */
struct file_info
{
    /* Name of the file, without any directory component.  */
    const char *file;

    /* Name of the directory we are in, relative to the directory in
       which this command was issued.  We have cd'd to this directory
       (either in the working directory or in the repository, depending
       on which sort of recursion we are doing).  If we are in the directory
       in which the command was issued, this is "".  */
    const char *update_dir;

    /* update_dir and file put together, with a slash between them as
       necessary.  This is the proper way to refer to the file in user
       messages.  */
    const char *fullname;

    /* Name of the directory corresponding to the repository which contains
       this file.  */
    const char *repository;

    /* The pre-parsed entries for this directory.  */
    List *entries;

    RCSNode *rcs;
};

/* This needs to be included after the struct file_info definition since some
 * of the functions subr.h defines refer to struct file_info.
 */
#include "subr.h"

int update (int argc, char *argv[]);
/* The only place this is currently used outside of update.c is add.c.
 * Restricting its use to update.c seems to be in the best interest of
 * modularity, but I can't think of a good way to get an update of a
 * resurrected file done and print the fact otherwise.
 */
void write_letter (struct file_info *finfo, int letter);
int xcmp (const char *file1, const char *file2);
void *valloc (size_t bytes);

int Create_Admin (const char *dir, const char *update_dir,
                  const char *repository, const char *tag, const char *date,
                  int nonbranch, int warn, int dotemplate);
int expand_at_signs (const char *, size_t, FILE *);

/* Locking subsystem (implemented in lock.c).  */

int Reader_Lock (char *xrepository);
void Simple_Lock_Cleanup (void);
void Lock_Cleanup (void);

/* Writelock an entire subtree, well the part specified by ARGC, ARGV, LOCAL,
   and AFLAG, anyway.  */
void lock_tree_promotably (int argc, char **argv, int local, int which,
			   int aflag);

/* See lock.c for description.  */
void lock_dir_for_write (const char *);

/* Get a write lock for the history file.  */
int history_lock (const char *);
void clear_history_lock (void);

/* Get a write lock for the val-tags file.  */
int val_tags_lock (const char *);
void clear_val_tags_lock (void);

void Scratch_Entry (List * list, const char *fname);
void ParseTag (char **tagp, char **datep, int *nonbranchp);
void WriteTag (const char *dir, const char *tag, const char *date,
               int nonbranch, const char *update_dir, const char *repository);
void WriteTemplate (const char *update_dir, int dotemplate,
                    const char *repository);
void cat_module (int status);
void check_entries (char *dir);
void close_module (DBM * db);
void copy_file (const char *from, const char *to);
void fperrmsg (FILE * fp, int status, int errnum, char *message,...);

int ign_name (char *name);
void ign_add (char *ign, int hold);
void ign_add_file (char *file, int hold);
void ign_setup (void);
void ign_dir_add (char *name);
int ignore_directory (const char *name);
typedef void (*Ignore_proc) (const char *, const char *);
void ignore_files (List *, List *, const char *, Ignore_proc);
extern int ign_inhibit_server;

#include "update.h"

void make_directories (const char *name);
void make_directory (const char *name);
int mkdir_if_needed (const char *name);
void rename_file (const char *from, const char *to);
/* Expand wildcards in each element of (ARGC,ARGV).  This is according to the
   files which exist in the current directory, and accordingly to OS-specific
   conventions regarding wildcard syntax.  It might be desirable to change the
   former in the future (e.g. "cvs status *.h" including files which don't exist
   in the working directory).  The result is placed in *PARGC and *PARGV;
   the *PARGV array itself and all the strings it contains are newly
   malloc'd.  It is OK to call it with PARGC == &ARGC or PARGV == &ARGV.  */
void expand_wild (int argc, char **argv, 
                  int *pargc, char ***pargv);

/* exithandle.c */
void signals_register (RETSIGTYPE (*handler)(int));
void cleanup_register (void (*handler) (void));

void update_delproc (Node * p);
void usage (const char *const *cpp);
void xchmod (const char *fname, int writable);
List *Find_Names (char *repository, int which, int aflag,
		  List ** optentries);
void Register (List * list, const char *fname, const char *vn, const char *ts,
               const char *options, const char *tag, const char *date,
               const char *ts_conflict);
void Update_Logfile (const char *repository, const char *xmessage,
                     FILE *xlogfp, List *xchanges);
void do_editor (const char *dir, char **messagep,
                const char *repository, List *changes);

void do_verify (char **messagep, const char *repository, List *changes);

typedef	int (*CALLBACKPROC)	(int argc, char *argv[], char *where,
	char *mwhere, char *mfile, int shorten, int local_specified,
	char *omodule, char *msg);


typedef	int (*FILEPROC) (void *callerdat, struct file_info *finfo);
typedef	int (*FILESDONEPROC) (void *callerdat, int err,
                              const char *repository, const char *update_dir,
                              List *entries);
typedef	Dtype (*DIRENTPROC) (void *callerdat, const char *dir,
                             const char *repos, const char *update_dir,
                             List *entries);
typedef	int (*DIRLEAVEPROC) (void *callerdat, const char *dir, int err,
                             const char *update_dir, List *entries);

int mkmodules (char *dir);
int init (int argc, char **argv);

int do_module (DBM * db, char *mname, enum mtype m_type, char *msg,
		CALLBACKPROC callback_proc, char *where, int shorten,
		int local_specified, int run_module_prog, int build_dirs,
		char *extra_arg);
void history_write (int type, const char *update_dir, const char *revs,
                    const char *name, const char *repository);
int start_recursion (FILEPROC fileproc, FILESDONEPROC filesdoneproc,
		     DIRENTPROC direntproc, DIRLEAVEPROC dirleaveproc,
		     void *callerdat,
		     int argc, char *argv[], int local, int which,
		     int aflag, int locktype, char *update_preload,
		     int dosrcs, char *repository);
void SIG_beginCrSect (void);
void SIG_endCrSect (void);
int SIG_inCrSect (void);
void read_cvsrc (int *argc, char ***argv, const char *cmdname);

/* flags for run_exec(), the fast system() for CVS */
#define	RUN_NORMAL            0x0000    /* no special behaviour */
#define	RUN_COMBINED          0x0001    /* stdout is duped to stderr */
#define	RUN_REALLY            0x0002    /* do the exec, even if noexec is on */
#define	RUN_STDOUT_APPEND     0x0004    /* append to stdout, don't truncate */
#define	RUN_STDERR_APPEND     0x0008    /* append to stderr, don't truncate */
#define	RUN_SIGIGNORE         0x0010    /* ignore interrupts for command */
#define	RUN_UNSETXID          0x0020	/* undo setxid in child */
#define	RUN_TTY               (char *)0 /* for the benefit of lint */

void run_add_arg_p (int *, size_t *, char ***, const char *s);
void run_arg_free_p (int, char **);
void run_add_arg (const char *s);
void run_print (FILE * fp);
void run_setup (const char *prog);
int run_exec (const char *stin, const char *stout, const char *sterr,
              int flags);
int run_piped (int *, int *);

/* other similar-minded stuff from run.c.  */
FILE *run_popen (const char *, const char *);
int piped_child (char *const *, int *, int *, bool);
void close_on_exec (int);

pid_t waitpid (pid_t, int *, int);

/*
 * a struct vers_ts contains all the information about a file including the
 * user and rcs file names, and the version checked out and the head.
 *
 * this is usually obtained from a call to Version_TS which takes a
 * tag argument for the RCS file if desired
 */
struct vers_ts
{
    /* rcs version user file derives from, from CVS/Entries.
       It can have the following special values:

       NULL = file is not mentioned in Entries (this is also used for a
	      directory).
       "" = INVALID!  The comment used to say that it meant "no user file"
	    but as far as I know CVS didn't actually use it that way.
	    Note that according to cvs.texinfo, "" is not valid in the
	    Entries file.
       0 = user file is new
       -vers = user file to be removed.  */
    char *vn_user;

    /* Numeric revision number corresponding to ->vn_tag (->vn_tag
       will often be symbolic).  */
    char *vn_rcs;
    /* If ->tag is a simple tag in the RCS file--a tag which really
       exists which is not a magic revision--and if ->date is NULL,
       then this is a copy of ->tag.  Otherwise, it is a copy of
       ->vn_rcs.  */
    char *vn_tag;

    /* This is the timestamp from stating the file in the working directory.
       It is NULL if there is no file in the working directory.  It is
       "Is-modified" if we know the file is modified but don't have its
       contents.  */
    char *ts_user;
    /* Timestamp from CVS/Entries.  For the server, ts_user and ts_rcs
       are computed in a slightly different way, but the fact remains that
       if they are equal the file in the working directory is unmodified
       and if they differ it is modified.  */
    char *ts_rcs;

    /* Options from CVS/Entries (keyword expansion), malloc'd.  If none,
       then it is an empty string (never NULL).  */
    char *options;

    /* If non-NULL, there was a conflict (or merely a merge?  See merge_file)
       and the time stamp in this field is the time stamp of the working
       directory file which was created with the conflict markers in it.
       This is from CVS/Entries.  */
    char *ts_conflict;

    /* Tag specified on the command line, or if none, tag stored in
       CVS/Entries.  */
    char *tag;
    /* Date specified on the command line, or if none, date stored in
       CVS/Entries.  */
    char *date;
    /* If this is 1, then tag is not a branch tag.  If this is 0, then
       tag may or may not be a branch tag.  */
    int nonbranch;

    /* Pointer to entries file node  */
    Entnode *entdata;

    /* Pointer to parsed src file info */
    RCSNode *srcfile;
};
typedef struct vers_ts Vers_TS;

Vers_TS *Version_TS (struct file_info *finfo, char *options, char *tag,
			    char *date, int force_tag_match,
			    int set_time);
void freevers_ts (Vers_TS ** versp);

/* Miscellaneous CVS infrastructure which layers on top of the recursion
   processor (for example, needs struct file_info).  */

int Checkin (int type, struct file_info *finfo, char *rev,
	     char *tag, char *options, char *message);
int No_Difference (struct file_info *finfo, Vers_TS *vers);
/* TODO: can the finfo argument to special_file_mismatch be changed? -twp */
int special_file_mismatch (struct file_info *finfo,
				  char *rev1, char *rev2);

/* CVSADM_BASEREV stuff, from entries.c.  */
char *base_get (struct file_info *);
void base_register (struct file_info *, char *);
void base_deregister (struct file_info *);

/*
 * defines for Classify_File() to determine the current state of a file.
 * These are also used as types in the data field for the list we make for
 * Update_Logfile in commit, import, and add.
 */
enum classify_type
{
    T_UNKNOWN = 1,			/* no old-style analog existed	 */
    T_CONFLICT,				/* C (conflict) list		 */
    T_NEEDS_MERGE,			/* G (needs merging) list	 */
    T_MODIFIED,				/* M (needs checked in) list 	 */
    T_CHECKOUT,				/* O (needs checkout) list	 */
    T_ADDED,				/* A (added file) list		 */
    T_REMOVED,				/* R (removed file) list	 */
    T_REMOVE_ENTRY,			/* W (removed entry) list	 */
    T_UPTODATE,				/* File is up-to-date		 */
    T_PATCH,				/* P Like C, but can patch	 */
    T_TITLE				/* title for node type 		 */
};
typedef enum classify_type Ctype;

Ctype Classify_File (struct file_info *finfo, char *tag, char *date, char *options,
      int force_tag_match, int aflag, Vers_TS **versp, int pipeout);

/*
 * structure used for list nodes passed to Update_Logfile() and
 * do_editor().
 */
struct logfile_info
{
  enum classify_type type;
  char *tag;
  char *rev_old;		/* rev number before a commit/modify,
				   NULL for add or import */
  char *rev_new;		/* rev number after a commit/modify,
				   add, or import, NULL for remove */
};

/* Wrappers.  */

typedef enum { WRAP_MERGE, WRAP_COPY } WrapMergeMethod;
typedef enum {
    /* -t and -f wrapper options.  Treating directories as single files.  */
    WRAP_TOCVS,
    WRAP_FROMCVS,
    /* -k wrapper option.  Default keyword expansion options.  */
    WRAP_RCSOPTION
} WrapMergeHas;

void  wrap_setup (void);
int   wrap_name_has (const char *name,WrapMergeHas has);
char *wrap_rcsoption (const char *fileName, int asFlag);
char *wrap_tocvs_process_file (const char *fileName);
int   wrap_merge_is_copy (const char *fileName);
void wrap_fromcvs_process_file (const char *fileName);
void wrap_add_file (const char *file,int temp);
void wrap_add (char *line,int temp);
void wrap_send (void);
#if defined(SERVER_SUPPORT) || defined(CLIENT_SUPPORT)
void wrap_unparse_rcs_options (char **, int);
#endif /* SERVER_SUPPORT || CLIENT_SUPPORT */

/* Pathname expansion */
char *expand_path (const char *name, const char *cvsroot, bool formatsafe,
		   const char *file, int line);

/* User variables.  */
extern List *variable_list;

void variable_set (char *nameval);

int watch (int argc, char **argv);
int edit (int argc, char **argv);
int unedit (int argc, char **argv);
int editors (int argc, char **argv);
int watchers (int argc, char **argv);
int annotate (int argc, char **argv);
int add (int argc, char **argv);
int admin (int argc, char **argv);
int checkout (int argc, char **argv);
int commit (int argc, char **argv);
int diff (int argc, char **argv);
int history (int argc, char **argv);
int import (int argc, char **argv);
int cvslog (int argc, char **argv);
#ifdef AUTH_CLIENT_SUPPORT
/* Some systems (namely Mac OS X) have conflicting definitions for these
 * functions.  Avoid them.
 */
#ifdef HAVE_LOGIN
# define login		cvs_login
#endif /* HAVE_LOGIN */
#ifdef HAVE_LOGOUT
# define logout		cvs_logout
#endif /* HAVE_LOGOUT */
int login (int argc, char **argv);
int logout (int argc, char **argv);
#endif /* AUTH_CLIENT_SUPPORT */
int patch (int argc, char **argv);
int release (int argc, char **argv);
int cvsremove (int argc, char **argv);
int rtag (int argc, char **argv);
int cvsstatus (int argc, char **argv);
int cvstag (int argc, char **argv);
int version (int argc, char **argv);
int admin_group_member (void);
void free_cvs_password (char *);
void getoptreset (void);

unsigned long int lookup_command_attribute (const char *);

#if defined(AUTH_CLIENT_SUPPORT) || defined(AUTH_SERVER_SUPPORT)
char *scramble (char *str);
char *descramble (char *str);
#endif /* AUTH_CLIENT_SUPPORT || AUTH_SERVER_SUPPORT */

#ifdef AUTH_CLIENT_SUPPORT
char *get_cvs_password (void);
/* get_cvs_port_number() is not pure since the /etc/services file could change
 * between calls.  */
int get_cvs_port_number (const cvsroot_t *root);
/* normalize_cvsroot() is not pure since it calls get_cvs_port_number.  */
char *normalize_cvsroot (const cvsroot_t *root)
	__attribute__ ((__malloc__));
#endif /* AUTH_CLIENT_SUPPORT */

void tag_check_valid (const char *, int, char **, int, int, char *, bool);

#include "server.h"

/* From server.c and documented there.  */
void cvs_output (const char *, size_t);
void cvs_output_binary (char *, size_t);
void cvs_outerr (const char *, size_t);
void cvs_flusherr (void);
void cvs_flushout (void);
void cvs_output_tagged (const char *, const char *);

extern const char *global_session_id;

/* From find_names.c.  */
List *find_files (const char *dir, const char *pat);
