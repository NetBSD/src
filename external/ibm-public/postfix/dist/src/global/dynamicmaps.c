/*	$NetBSD: dynamicmaps.c,v 1.3.2.1 2023/12/25 12:43:31 martin Exp $	*/

/*++
/* NAME
/*	dynamicmaps 3
/* SUMMARY
/*	load dictionaries dynamically
/* SYNOPSIS
/*	#include <dynamicmaps.h>
/*
/*	void dymap_init(const char *conf_path, const char *plugin_dir)
/* DESCRIPTION
/*	This module reads the dynamicmaps.cf file and on-demand
/*	loads Postfix dictionaries. Each dynamicmaps.cf entry
/*	specifies the name of a dictionary type, the pathname of a
/*	shared-library object, the name of a "dict_open" function
/*	for access to individual dictionary entries, and optionally
/*	the name of a "mkmap_open" wrapper function for bulk-mode
/*	dictionary creation. Plugins may be specified with a relative
/*	pathname.
/*
/*	A dictionary shared object may be installed on a system
/*	without editing the file dynamicmaps.cf, by placing a
/*	configuration file under the directory dynamicmaps.cf.d,
/*	with the same format as dynamicmaps.cf.
/*
/*	dymap_init() reads the specified configuration file which
/*	must be in dynamicmaps.cf format, appends ".d" to the path
/*	and scans the named directory for other configuration files,
/*	and on the first call hooks itself into the dict_open() and
/*	dict_mapnames() functions. All files are optional, but if
/*	an existing file cannot be opened, that is a fatal error.
/*
/*	dymap_init() may be called multiple times during a process
/*	lifetime, but it will not "unload" dictionaries that have
/*	already been linked into the process address space, nor
/*	will it hide their dictionaries types from later "open"
/*	requests.
/*
/*	Arguments:
/* .IP conf_path
/*	Pathname for the dynamicmaps configuration file. With ".d"
/*	appended,  this becomes the pathname for a directory with
/*	other files in dynamicmaps.cf format.
/* .IP plugin_dir
/*	Default directory for plugins with a relative pathname.
/* SEE ALSO
/*	load_lib(3) low-level run-time linker adapter.
/* DIAGNOSTICS
/*	Warnings: unsupported dictionary type, shared object file
/*	does not exist, shared object permissions are not safe-for-root,
/*	configuration file permissions are not safe-for-root.
/*	Fatal errors: memory allocation problem, error opening an
/*	existing configuration file, bad configuration file syntax.
/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	LaMont Jones
/*	Hewlett-Packard Company
/*	3404 Harmony Road
/*	Fort Collins, CO 80528, USA
/*
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*	Wietse Venema
/*	Google, Inc.
/*	111 8th Avenue
/*	New York, NY 10011, USA
/*--*/

 /*
  * System library.
  */
#include <sys_defs.h>
#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

 /*
  * Utility library.
  */
#include <msg.h>
#include <mymalloc.h>
#include <htable.h>
#include <argv.h>
#include <dict.h>
#include <mkmap.h>
#include <load_lib.h>
#include <vstring.h>
#include <vstream.h>
#include <vstring_vstream.h>
#include <stringops.h>
#include <split_at.h>
#include <scan_dir.h>

 /*
  * Global library.
  */
#include <dynamicmaps.h>

#ifdef USE_DYNAMIC_MAPS

 /*
  * Contents of one dynamicmaps.cf entry.
  */
typedef struct {
    char   *soname;			/* shared-object file name */
    char   *dict_name;			/* dict_xx_open() function name */
    char   *mkmap_name;			/* mkmap_xx_open() function name */
} DYMAP_INFO;

static HTABLE *dymap_info;
static int dymap_hooks_done = 0;
static DICT_OPEN_EXTEND_FN saved_dict_open_hook = 0;
static DICT_MAPNAMES_EXTEND_FN saved_dict_mapnames_hook = 0;

#define STREQ(x, y) (strcmp((x), (y)) == 0)


/* dymap_dict_lookup - look up DICT_OPEN_INFO */

static const DICT_OPEN_INFO *dymap_dict_lookup(const char *dict_type)
{
    const char myname[] = "dymap_dict_lookup";
    struct stat st;
    LIB_FN  fn[3];
    DYMAP_INFO *dp;
    const DICT_OPEN_INFO *op;
    DICT_OPEN_INFO *np;

    if (msg_verbose > 1)
	msg_info("%s: %s", myname, dict_type);

    /*
     * Respect the hook nesting order.
     */
    if (saved_dict_open_hook != 0
	&& (op = saved_dict_open_hook(dict_type)) != 0)
	return (op);

    /*
     * Allow for graceful degradation when a database is unavailable. This
     * allows Postfix daemon processes to continue handling email with
     * reduced functionality.
     */
    if ((dp = (DYMAP_INFO *) htable_find(dymap_info, dict_type)) == 0) {
	msg_warn("unsupported dictionary type: %s. "
		 "Is the postfix-%s package installed?",
		 dict_type, dict_type);
	return (0);
    }
    if (stat(dp->soname, &st) < 0) {
	msg_warn("unsupported dictionary type: %s (%s: %m)",
		 dict_type, dp->soname);
	return (0);
    }
    if (st.st_uid != 0 || (st.st_mode & (S_IWGRP | S_IWOTH)) != 0) {
	msg_warn("unsupported dictionary type: %s "
		 "(%s: file is owned or writable by non-root users)",
		 dict_type, dp->soname);
	return (0);
    }
    fn[0].name = dp->dict_name;			/* not null */
    fn[1].name = dp->mkmap_name;		/* may be null */
    fn[2].name = 0;
    load_library_symbols(dp->soname, fn, (LIB_DP *) 0);
    np = (DICT_OPEN_INFO *) mymalloc(sizeof(*op));
    np->type = mystrdup(dict_type);
    np->dict_fn = (DICT_OPEN_FN) fn[0].fptr;
    np->mkmap_fn = (MKMAP_OPEN_FN) (dp->mkmap_name ? fn[1].fptr : 0);
    return (np);
}

/* dymap_list - enumerate dynamically-linked database type names */

static void dymap_list(ARGV *map_names)
{
    HTABLE_INFO **ht_list, **ht;

    /*
     * Respect the hook nesting order.
     */
    if (saved_dict_mapnames_hook != 0)
	saved_dict_mapnames_hook(map_names);

    for (ht_list = ht = htable_list(dymap_info); *ht != 0; ht++)
	argv_add(map_names, ht[0]->key, ARGV_END);
    myfree((void *) ht_list);
}

/* dymap_entry_alloc - allocate dynamicmaps.cf entry */

static DYMAP_INFO *dymap_entry_alloc(char **argv)
{
    DYMAP_INFO *dp;

    dp = (DYMAP_INFO *) mymalloc(sizeof(*dp));
    dp->soname = mystrdup(argv[0]);
    dp->dict_name = mystrdup(argv[1]);
    dp->mkmap_name = argv[2] ? mystrdup(argv[2]) : 0;
    return (dp);
}

/* dymap_entry_free - htable(3) call-back to destroy dynamicmaps.cf entry */

static void dymap_entry_free(void *ptr)
{
    DYMAP_INFO *dp = (DYMAP_INFO *) ptr;

    myfree(dp->soname);
    myfree(dp->dict_name);
    if (dp->mkmap_name)
	myfree(dp->mkmap_name);
    myfree((void *) dp);
}

/* dymap_read_conf - read dynamicmaps.cf-like file */

static void dymap_read_conf(const char *path, const char *path_base)
{
    const char myname[] = "dymap_read_conf";
    VSTREAM *fp;
    VSTRING *buf;
    char   *cp;
    ARGV   *argv;
    int     linenum = 0;
    struct stat st;

    /*
     * Silently ignore a missing dynamicmaps.cf file, but be explicit about
     * problems when the file does exist.
     */
    if (msg_verbose > 1)
	msg_info("%s: opening %s", myname, path);
    if ((fp = vstream_fopen(path, O_RDONLY, 0)) != 0) {
	if (fstat(vstream_fileno(fp), &st) < 0)
	    msg_fatal("%s: fstat failed; %m", path);
	if (st.st_uid != 0 || (st.st_mode & (S_IWGRP | S_IWOTH)) != 0) {
	    msg_warn("%s: file is owned or writable by non-root users"
		     " -- skipping this file", path);
	} else {
	    buf = vstring_alloc(100);
	    while (vstring_get_nonl(buf, fp) != VSTREAM_EOF) {
		cp = vstring_str(buf);
		if (msg_verbose > 1)
		    msg_info("%s: read: %s", myname, cp);
		linenum++;
		if (*cp == '#' || *cp == '\0')
		    continue;
		argv = argv_split(cp, " \t");
		if (argv->argc != 3 && argv->argc != 4)
		    msg_fatal("%s, line %d: Expected \"dict-type .so-name dict"
			      "-function [mkmap-function]\"", path, linenum);
		if (!ISALNUM(argv->argv[0][0]))
		    msg_fatal("%s, line %d: unsupported syntax \"%s\"",
			      path, linenum, argv->argv[0]);
		if (argv->argv[1][0] != '/') {
		    cp = concatenate(path_base, "/", argv->argv[1], (char *) 0);
		    argv_replace_one(argv, 1, cp);
		    myfree(cp);
		}
		if (htable_locate(dymap_info, argv->argv[0]) != 0)
		    msg_warn("%s: ignoring duplicate entry for \"%s\"",
			     path, argv->argv[0]);
		else
		    htable_enter(dymap_info, argv->argv[0],
				 (void *) dymap_entry_alloc(argv->argv + 1));
		argv_free(argv);
	    }
	    vstring_free(buf);

	    /*
	     * Once-only: hook into the dict_open(3) infrastructure.
	     */
	    if (dymap_hooks_done == 0) {
		dymap_hooks_done = 1;
		saved_dict_open_hook = dict_open_extend(dymap_dict_lookup);
		saved_dict_mapnames_hook = dict_mapnames_extend(dymap_list);
	    }
	}
	vstream_fclose(fp);
    } else if (errno != ENOENT) {
	msg_fatal("%s: file open failed: %m", path);
    }
}

/* dymap_init - initialize dictionary type to soname etc. mapping */

void    dymap_init(const char *conf_path, const char *plugin_dir)
{
    static const char myname[] = "dymap_init";
    SCAN_DIR *dir;
    char   *conf_path_d;
    const char *conf_name;
    VSTRING *sub_conf_path;

    if (msg_verbose > 1)
	msg_info("%s: %s %s", myname, conf_path, plugin_dir);

    /*
     * Reload dynamicmaps.cf, but don't reload already-loaded plugins.
     */
    if (dymap_info != 0)
	htable_free(dymap_info, dymap_entry_free);
    dymap_info = htable_create(3);

    /*
     * Read dynamicmaps.cf.
     */
    dymap_read_conf(conf_path, plugin_dir);

    /*
     * Read dynamicmaps.cf.d/filename entries.
     */
    conf_path_d = concatenate(conf_path, ".d", (char *) 0);
    if (access(conf_path_d, R_OK | X_OK) == 0
	&& (dir = scan_dir_open(conf_path_d)) != 0) {
	sub_conf_path = vstring_alloc(100);
	while ((conf_name = scan_dir_next(dir)) != 0) {
	    vstring_sprintf(sub_conf_path, "%s/%s", conf_path_d, conf_name);
	    dymap_read_conf(vstring_str(sub_conf_path), plugin_dir);
	}
	if (errno != 0)
	    /* Don't crash all programs - degrade gracefully. */
	    msg_warn("%s: directory read error: %m", conf_path_d);
	scan_dir_close(dir);
	vstring_free(sub_conf_path);
    } else if (errno != ENOENT) {
	/* Don't crash all programs - degrade gracefully. */
	msg_warn("%s: directory open failed: %m", conf_path_d);
    }
    myfree(conf_path_d);

    /*
     * Future proofing, in case someone "improves" the code. We can't hook
     * into other functions without initializing our private lookup table.
     */
    if (dymap_hooks_done != 0 && dymap_info == 0)
	msg_panic("%s: post-condition botch", myname);
}

#endif
