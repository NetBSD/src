/* $NetBSD: lib.h,v 1.1.1.2.2.2 2007/08/03 13:58:22 joerg Exp $ */

/* from FreeBSD Id: lib.h,v 1.25 1997/10/08 07:48:03 charnier Exp */

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * Jordan K. Hubbard
 * 18 July 1993
 *
 * Include and define various things wanted by the library routines.
 *
 */

#ifndef _INST_LIB_LIB_H_
#define _INST_LIB_LIB_H_

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#if HAVE_SYS_FILE_H
#include <sys/file.h>
#endif
#if HAVE_SYS_QUEUE_H
#include <sys/queue.h>
#endif

#if HAVE_CTYPE_H
#include <ctype.h>
#endif
#if HAVE_DIRENT_H
#include <dirent.h>
#endif
#if HAVE_STDIO_H
#include <stdio.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_STDARG_H
#include <stdarg.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "path.h"

/* Macros */
#define SUCCESS	(0)
#define	FAIL	(-1)

#ifndef TRUE
#define TRUE	(1)
#endif

#ifndef FALSE
#define FALSE	(0)
#endif

#ifndef OPSYS_NAME
#define OPSYS_NAME "NetBSD"
#endif

#ifndef DEF_UMASK
#define DEF_UMASK 022
#endif

/* Usually "rm", but often "echo" during debugging! */
#define REMOVE_CMD	"rm"

/* Usually "rm", but often "echo" during debugging! */
#define RMDIR_CMD	"rmdir"

/* Define tar as a string, in case it's called gtar or something */
#ifndef TAR_CMD
#define TAR_CMD	"tar"
#endif

/* Define pax as a string, used to copy files from staging area */              
#ifndef PAX_CMD        
#define PAX_CMD "pax"
#endif

/* Define gzip and bzip2, used to unpack binary packages */
#ifndef GZIP_CMD
#define GZIP_CMD "gzip"
#endif

#ifndef BZIP2_CMD
#define BZIP2_CMD "bzip2"
#endif

/* Define ftp as a string, in case the ftp client is called something else */
#ifndef FTP_CMD
#define FTP_CMD "ftp"
#endif

#ifndef CHOWN_CMD
#define CHOWN_CMD "chown"
#endif

#ifndef CHMOD_CMD
#define CHMOD_CMD "chmod"
#endif

#ifndef CHGRP_CMD
#define CHGRP_CMD "chgrp"
#endif

/* some operating systems don't have this */
#ifndef MAXPATHLEN
#define MAXPATHLEN	1024
#endif

enum {
	MaxPathSize = MAXPATHLEN
};

/* The names of our "special" files */
#define CONTENTS_FNAME		"+CONTENTS"
#define COMMENT_FNAME		"+COMMENT"
#define DESC_FNAME		"+DESC"
#define INSTALL_FNAME		"+INSTALL"
#define DEINSTALL_FNAME		"+DEINSTALL"
#define REQUIRED_BY_FNAME	"+REQUIRED_BY"
#define DISPLAY_FNAME		"+DISPLAY"
#define MTREE_FNAME		"+MTREE_DIRS"
#define BUILD_VERSION_FNAME	"+BUILD_VERSION"
#define BUILD_INFO_FNAME	"+BUILD_INFO"
#define INSTALLED_INFO_FNAME	"+INSTALLED_INFO"
#define SIZE_PKG_FNAME		"+SIZE_PKG"
#define SIZE_ALL_FNAME		"+SIZE_ALL"
#define PRESERVE_FNAME		"+PRESERVE"
#define VIEWS_FNAME		"+VIEWS"
#define DEPOT_FNAME		"+DEPOT"

/* The names of special variables */
#define AUTOMATIC_VARNAME	"automatic"

/*
 * files which we expect to be in every package, passed to
 * tar --fast-read.
 */
#define ALL_FNAMES              CONTENTS_FNAME" "COMMENT_FNAME" "DESC_FNAME" "MTREE_FNAME" "BUILD_VERSION_FNAME" "BUILD_INFO_FNAME" "SIZE_PKG_FNAME" "SIZE_ALL_FNAME

#define CMD_CHAR		'@'	/* prefix for extended PLIST cmd */

/* The name of the "prefix" environment variable given to scripts */
#define PKG_PREFIX_VNAME	"PKG_PREFIX"

/*
 * The name of the "metadatadir" environment variable given to scripts.
 * This variable holds the location of the +-files for this package.
 */
#define PKG_METADATA_DIR_VNAME	"PKG_METADATA_DIR"

/*
 * The name of the environment variable holding the location to the
 * reference-counts database directory.
 */
#define PKG_REFCOUNT_DBDIR_VNAME	"PKG_REFCOUNT_DBDIR"

#define	PKG_PATTERN_MAX	MaxPathSize	/* max length of pattern, including nul */
#define	PKG_SUFFIX_MAX	10	/* max length of suffix, including nul */

enum {
	ReadWrite,
	ReadOnly
};


/* Enumerated constants for plist entry types */
typedef enum pl_ent_t {
	PLIST_SHOW_ALL = -1,
	PLIST_FILE,		/*  0 */
	PLIST_CWD,		/*  1 */
	PLIST_CMD,		/*  2 */
	PLIST_CHMOD,		/*  3 */
	PLIST_CHOWN,		/*  4 */
	PLIST_CHGRP,		/*  5 */
	PLIST_COMMENT,		/*  6 */
	PLIST_IGNORE,		/*  7 */
	PLIST_NAME,		/*  8 */
	PLIST_UNEXEC,		/*  9 */
	PLIST_SRC,		/* 10 */
	PLIST_DISPLAY,		/* 11 */
	PLIST_PKGDEP,		/* 12 */
	PLIST_MTREE,		/* 13 */
	PLIST_DIR_RM,		/* 14 */
	PLIST_IGNORE_INST,	/* 15 */
	PLIST_OPTION,		/* 16 */
	PLIST_PKGCFL,		/* 17 */
	PLIST_BLDDEP		/* 18 */
}       pl_ent_t;

/* Enumerated constants for build info */
typedef enum bi_ent_t {
	BI_OPSYS,		/*  0 */
	BI_OS_VERSION,		/*  1 */
	BI_MACHINE_ARCH,	/*  2 */
	BI_IGNORE_RECOMMENDED,	/*  3 */
	BI_USE_ABI_DEPENDS,	/*  4 */
	BI_ENUM_COUNT		/*  5 */
}	bi_ent_t;

/* Types */
typedef unsigned int Boolean;

/* This structure describes a packing list entry */
typedef struct plist_t {
	struct plist_t *prev;	/* previous entry */
	struct plist_t *next;	/* next entry */
	char   *name;		/* name of entry */
	Boolean marked;		/* whether entry has been marked */
	pl_ent_t type;		/* type of entry */
}       plist_t;

/* This structure describes a package's complete packing list */
typedef struct package_t {
	plist_t *head;		/* head of list */
	plist_t *tail;		/* tail of list */
}       package_t;

#define SYMLINK_HEADER	"Symlink:"
#define CHECKSUM_HEADER	"MD5:"

enum {
	ChecksumHeaderLen = 4,	/* strlen(CHECKSUM_HEADER) */
	SymlinkHeaderLen = 8,	/* strlen(SYMLINK_HEADER) */
	ChecksumLen = 16,
	LegibleChecksumLen = 33
};

/* List of files */
typedef struct _lfile_t {
        TAILQ_ENTRY(_lfile_t) lf_link;
        char *lf_name;
} lfile_t;
TAILQ_HEAD(_lfile_head_t, _lfile_t);
typedef struct _lfile_head_t lfile_head_t;
#define	LFILE_ADD(lfhead,lfp,str) do {		\
	lfp = malloc(sizeof(lfile_t));		\
	lfp->lf_name = str;			\
	TAILQ_INSERT_TAIL(lfhead,lfp,lf_link);	\
	} while(0)

/* List of packages */
typedef struct _lpkg_t {
	TAILQ_ENTRY(_lpkg_t) lp_link;
	char   *lp_name;
}       lpkg_t;
TAILQ_HEAD(_lpkg_head_t, _lpkg_t);
typedef struct _lpkg_head_t lpkg_head_t;

/* Type of function to be handed to findmatchingname; return value of this
 * is currently ignored */
typedef int (*matchfn) (const char *, const char *, void *);

/* This structure describes a pipe to a child process */
typedef struct {
	int fds[2];	/* pipe, 0=child stdin, 1=parent output */
	FILE *fp;	/* output from parent process */
	pid_t pid;	/* process id of child process */
	void (*cleanup)(void);	/* called on non-zero child exit status */
} pipe_to_system_t;

/* If URLlength()>0, then there is a ftp:// or http:// in the string,
 * and this must be an URL. Hide this behind a more obvious name. */
#define IS_URL(str)	(URLlength(str) > 0)

#define IS_STDIN(str)	((str) != NULL && !strcmp((str), "-"))
#define IS_FULLPATH(str)	((str) != NULL && (str)[0] == '/')

/* Prototypes */
/* Misc */
void    cleanup(int);
char   *make_playpen(char *, size_t, size_t);
char   *where_playpen(void);
void    leave_playpen(char *);
uint64_t min_free(const char *);
void    save_dirs(char **, char **);
void    restore_dirs(char *, char *);
void    show_version(void);
int	fexec(const char *, ...);
int	fexec_skipempty(const char *, ...);
int	fcexec(const char *, const char *, ...);
int	pfcexec(const char *, const char *, const char **);
pipe_to_system_t	*pipe_to_system_begin(const char *, char *const *, void (*)(void));
int	pipe_to_system_end(pipe_to_system_t *);

/* variables file handling */

char   *var_get(const char *, const char *);
int	var_set(const char *, const char *, const char *);
int     var_copy_list(const char *, const char **);

/* automatically installed as dependency */

Boolean	is_automatic_installed(const char *);
int	mark_as_automatic_installed(const char *, int);

/* String */
char   *get_dash_string(char **);
void    str_lowercase(unsigned char *);
const char *basename_of(const char *);
const char *dirname_of(const char *);
const char *suffix_of(const char *);
int     pkg_match(const char *, const char *);
int	pkg_order(const char *, const char *, const char *);
int     findmatchingname(const char *, const char *, matchfn, void *); /* doesn't really belong to "strings" */
char   *findbestmatchingname(const char *, const char *);	/* neither */
int     ispkgpattern(const char *);
void	strip_txz(char *, char *, const char *);

/* callback functions for findmatchingname */
int     findbestmatchingname_fn(const char *, const char *, void *);	/* neither */
int     note_whats_installed(const char *, const char *, void *);
int     add_to_list_fn(const char *, const char *, void *);


/* File */
Boolean fexists(const char *);
Boolean isdir(const char *);
Boolean islinktodir(const char *);
Boolean isemptydir(const char *);
Boolean isemptyfile(const char *);
Boolean isfile(const char *);
Boolean isempty(const char *);
int     URLlength(const char *);
char   *fileGetURL(const char *);
const char *fileURLFilename(const char *, char *, int);
const char *fileURLHost(const char *, char *, int);
char   *fileFindByPath(const char *);
char   *fileGetContents(char *);
Boolean make_preserve_name(char *, size_t, char *, char *);
void    write_file(char *, char *);
void    copy_file(char *, char *, char *);
void    move_file(char *, char *, char *);
void    move_files(const char *, const char *, const char *);
void    remove_files(const char *, const char *);
int     delete_hierarchy(char *, Boolean, Boolean);
int     unpack(const char *, const lfile_head_t *);
void    format_cmd(char *, size_t, char *, char *, char *);

/* ftpio.c: FTP handling */
int	expandURL(char *, const char *);
int	unpackURL(const char *, const char *);
int	ftp_cmd(const char *, const char *);
int	ftp_start(const char *);
void	ftp_stop(void);

/* Packing list */
plist_t *new_plist_entry(void);
plist_t *last_plist(package_t *);
plist_t *find_plist(package_t *, pl_ent_t);
char   *find_plist_option(package_t *, char *);
void    plist_delete(package_t *, Boolean, pl_ent_t, char *);
void    free_plist(package_t *);
void    mark_plist(package_t *);
void    csum_plist_entry(char *, plist_t *);
void    add_plist(package_t *, pl_ent_t, const char *);
void    add_plist_top(package_t *, pl_ent_t, const char *);
void    delete_plist(package_t *, Boolean, pl_ent_t, char *);
void    write_plist(package_t *, FILE *, char *);
void	stringify_plist(package_t *, char **, size_t *, char *);
void    read_plist(package_t *, FILE *);
int     plist_cmd(unsigned char *, char **);
int     delete_package(Boolean, Boolean, package_t *, Boolean);

/* Package Database */
int     pkgdb_open(int);
void    pkgdb_close(void);
int     pkgdb_store(const char *, const char *);
char   *pkgdb_retrieve(const char *);
void	pkgdb_dump(void);
int     pkgdb_remove(const char *);
int	pkgdb_remove_pkg(const char *);
char   *pkgdb_refcount_dir(void);
char   *_pkgdb_getPKGDB_FILE(char *, unsigned);
char   *_pkgdb_getPKGDB_DIR(void);
void	_pkgdb_setPKGDB_DIR(const char *);

/* List of packages functions */
lpkg_t *alloc_lpkg(const char *);
lpkg_t *find_on_queue(lpkg_head_t *, const char *);
void    free_lpkg(lpkg_t *);

/* Externs */
extern Boolean Verbose;
extern Boolean Fake;
extern Boolean Force;

#endif				/* _INST_LIB_LIB_H_ */
