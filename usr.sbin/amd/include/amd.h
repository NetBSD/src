/*
 * Copyright (c) 1997 Erez Zadok
 * Copyright (c) 1990 Jan-Simon Pendry
 * Copyright (c) 1990 Imperial College of Science, Technology & Medicine
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Jan-Simon Pendry at Imperial College, London.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      %W% (Berkeley) %G%
 *
 * $Id: amd.h,v 1.1.1.1 1997/07/24 21:20:15 christos Exp $
 *
 */

#ifndef _AMD_H
#define _AMD_H


/*
 * MACROS:
 */

/* options for amd.conf */
#define CFM_BROWSABLE_DIRS		0x0001
#define CFM_MOUNT_TYPE_AUTOFS		0x0002
#define CFM_ENABLE_DEFAULT_SELECTORS	0x0004
#define CFM_NORMALIZE_HOSTNAMES		0x0008
#define CFM_NOSWAP			0x0010
#define CFM_PRINT_PID			0x0020
#define CFM_RESTART_EXISTING_MOUNTS	0x0040


/* some systems (SunOS 4.x) neglect to define the mount null message */
#ifndef MOUNTPROC_NULL
# define MOUNTPROC_NULL ((u_long)(0))
#endif /* not MOUNTPROC_NULL */

/* Hash table size */
#define NKVHASH (1 << 2)        /* Power of two */


/*
 * TYPEDEFS:
 */

typedef struct cf_map cf_map_t;
typedef struct kv kv;
/*
 * Cache map operations
 */
typedef void add_fn(mnt_map *, char *, char *);
typedef int init_fn(mnt_map *, char *, time_t *);
typedef int mtime_fn(mnt_map *, char *, time_t *);
typedef int reload_fn(mnt_map *, char *, add_fn *);
typedef int search_fn(mnt_map *, char *, char *, char **, time_t *);



/*
 * STRUCTURES:
 */

/* global amd options that are manipulated by conf.c */
struct amu_global_options {
  char *arch;			/* name of current architecture */
  char *auto_dir;		/* automounter temp dir */
  char *cluster;		/* cluster name */
  char *karch;			/* kernel architecture */
  char *logfile;		/* amd log file */
  char *op_sys;			/* operating system name */
  char *op_sys_ver;		/* OS version */
  char *sub_domain;		/* local domain */
  char *map_options;		/* global map options */
  char *map_type;		/* global map type */
  char *search_path;		/* search path for maps */
  char *mount_type;		/* mount type for map */
  u_int flags;			/* various CFM_* flags */
  int afs_retrans;		/* NFS retransmit counter */
  int afs_timeo;		/* NFS retry interval */
  int am_timeo;			/* cache duration */
  int am_timeo_w;		/* dismount interval */
#ifdef HAVE_MAP_LDAP
  char *ldap_base;		/* LDAP base */
  char *ldap_hostports;		/* LDAP host ports */
#endif /* HAVE_MAP_LDAP */
#ifdef HAVE_MAP_NIS
  char *nis_domain;		/* YP domain name */
#endif /* HAVE_MAP_NIS */
};

struct cf_map {
  char *cfm_dir;		/* /home, /u, /src */
  char *cfm_name;		/* amd.home, /etc/amd.home ... */
  char *cfm_type;		/* file, hesiod, ndbm, nis ... */
  char *cfm_opts;		/* -cache:=all, etc. */
  char *cfm_search_path;	/* /etc/local:/etc/amdmaps:/misc/yp */
  char *cfm_tag;		/* optional map tag for amd -T */
  u_int cfm_flags;		/* browsable_dirs? mount_type? */
};

/*
 * Key-value pair
 */
struct kv {
  kv *next;
  char *key;
#ifdef HAVE_REGEXEC
  regex_t re;                   /* store the regexp from regcomp() */
#endif /* HAVE_REGEXEC */
  char *val;
};

struct mnt_map {
  qelem hdr;
  int refc;                     /* Reference count */
  short flags;                  /* Allocation flags */
  short alloc;                  /* Allocation mode */
  time_t modify;                /* Modify time of map */
  char *map_name;               /* Name of this map */
  char *wildcard;               /* Wildcard value */
  reload_fn *reload;            /* Function to be used for reloads */
  search_fn *search;            /* Function to be used for searching */
  mtime_fn *mtime;              /* Modify time function */
  kv *kvhash[NKVHASH];          /* Cached data */
  /* options available via amd conf file */
  char *cf_map_type;            /* file, hesiod, ndbm, nis, etc. */
  char *cf_search_path;         /* /etc/local:/etc/amdmaps:/misc/yp */
  u_int cf_flags;               /*  browsable_dirs? mount_type? */
  void *map_data;               /* Map data black box */
};


/*
 * EXTERNALS:
 */

/* amd global functions */
extern amq_mount_info_list *amqproc_getmntfs_1_svc(voidp argp, struct svc_req *rqstp);
extern amq_mount_stats *amqproc_stats_1_svc(voidp argp, struct svc_req *rqstp);
extern amq_mount_tree_list *amqproc_export_1_svc(voidp argp, struct svc_req *rqstp);
extern amq_mount_tree_p *amqproc_mnttree_1_svc(voidp argp, struct svc_req *rqstp);
extern amq_string *amqproc_getvers_1_svc(voidp argp, struct svc_req *rqstp);
extern int *amqproc_mount_1_svc(voidp argp, struct svc_req *rqstp);
extern int *amqproc_setopt_1_svc(voidp argp, struct svc_req *rqstp);
extern voidp amqproc_null_1_svc(voidp argp, struct svc_req *rqstp);
extern voidp amqproc_umnt_1_svc(voidp argp, struct svc_req *rqstp);

extern SVCXPRT *nfs_program_2_transp;	/* For quick_reply() */
extern am_nfs_fh *root_fh(char *dir);
extern bool_t xdr_amq_mount_info_qelem(XDR *xdrs, qelem *qhead);
extern fserver *find_nfs_srvr(mntfs *mf);
extern int mount_nfs_fh(am_nfs_handle_t *fhp, char *dir, char *fs_name, char *opts, mntfs *mf);
extern int process_last_regular_map(void);
extern int set_conf_kv(const char *section, const char *k, const char *v);
extern nfsentry *make_entry_chain(am_node *mp, const nfsentry *current_chain);
extern void flush_srvr_nfs_cache(void);
extern void mf_mounted(mntfs *mf);
extern void quick_reply(am_node *mp, int error);

/* amd global variables */
extern struct amu_global_options gopt;	/* where global options are stored */
extern FILE *yyin;
extern am_ops cdfs_ops;
extern am_ops lofs_ops;
extern am_ops nfs_ops;
extern am_ops pcfs_ops;
extern am_ops ufs_ops;
extern char *conf_tag;
extern int NumChild;
extern int fwd_sock;
extern int select_intr_valid;
extern int usage;
extern int use_conf_file;	/* use amd configuration file */
extern int yyparse (void);
extern jmp_buf select_intr;
extern qelem mfhead;
extern void root_newmap(const char *, const char *, const char *, const cf_map_t *);

#ifdef HAVE_SIGACTION
extern sigset_t masked_sigs;
#endif /* HAVE_SIGACTION */

#if defined(HAVE_AM_FS_SFS) || defined(HAVE_AM_FS_SFSX)
extern char * sfs_match(am_opts *fo);
extern int sfs_fumount(mntfs *mf);
#endif /* defined(HAVE_AM_FS_SFS) || defined(HAVE_AM_FS_SFSX) */

#if defined(HAVE_FS_NFS3) && !defined(HAVE_XDR_MOUNTRES3)
extern bool_t xdr_mountres3(XDR *xdrs, mountres3 *objp);
#endif /* defined(HAVE_FS_NFS3) && !defined(HAVE_XDR_MOUNTRES3) */

#endif /* not _AMD_H */
