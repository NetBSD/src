/*	$NetBSD: nfs_prot_hpux11.h,v 1.1.1.1 2000/06/07 00:52:21 dogcow Exp $ */
/*
 * Copyright (c) 1997-2000 Erez Zadok
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
 *    must display the following acknowledgment:
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
 * Id: nfs_prot_hpux11.h,v 1.4 2000/01/12 16:44:47 ezk Exp 
 *
 */

#ifndef _AMU_NFS_PROT_H
#define _AMU_NFS_PROT_H

/*
 * NOTE: HPUX 11 is missing many header definitions which had to be
 * guessed and copied over from HPUX 10.20.
 */

/* don't include this file as it isn't needed on hpux */
#ifndef _TIUSER_H
# define _TIUSER_H
#endif /* TIUSER_H */
/* if T_NULL is defined, undefine it due to a conflict with <arpa/nameser.h> */
#ifdef T_NULL
# undef T_NULL
#endif /* T_NULL */

#ifdef HAVE_NFS_EXPORT_H_not
/* don't include this b/c it'll get included twice */
# include <nfs/export.h>
#endif /* HAVE_NFS_EXPORT_H */
#ifdef HAVE_NFS_NFSV2_H
# include <nfs/nfsv2.h>
#endif /* HAVE_NFS_NFSV2_H */
#ifdef HAVE_NFS_RPCV2_H
# include <nfs/rpcv2.h>
#endif /* HAVE_NFS_RPCV2_H */
#ifdef HAVE_NFS_NFS_H
# include <nfs/nfs.h>
#endif /* HAVE_NFS_NFS_H */
#ifdef HAVE_SYS_FS_NFS_H
# include <sys/fs/nfs.h>
#endif /* HAVE_SYS_FS_NFS_H */
#ifdef HAVE_RPCSVC_MOUNT_H
# include <rpcsvc/mount.h>
#endif /* HAVE_RPCSVC_MOUNT_H */


/*
 * MACROS
 */

#define NFS_PORT 2049
#define NFS_MAXDATA 8192
#define NFS_MAXPATHLEN 1024
#define NFS_MAXNAMLEN 255
#define NFS_FHSIZE 32
#define NFS_COOKIESIZE 4
#define	MNTPATHLEN 1024
#define	MNTNAMLEN 255

#define NFSMODE_FMT 0170000
#define NFSMODE_DIR 0040000
#define NFSMODE_CHR 0020000
#define NFSMODE_BLK 0060000
#define NFSMODE_REG 0100000
#define NFSMODE_LNK 0120000
#define NFSMODE_SOCK 0140000
#define NFSMODE_FIFO 0010000

#ifndef NFS_PROGRAM
# define NFS_PROGRAM ((u_long)100003)
#endif /* not NFS_PROGRAM */
#ifndef NFS_VERSION
# define NFS_VERSION ((u_long)2)
#endif /* not NFS_VERSION */

#define NFSPROC_NULL ((u_long)0)
#define NFSPROC_GETATTR ((u_long)1)
#define NFSPROC_SETATTR ((u_long)2)
#define NFSPROC_ROOT ((u_long)3)
#define NFSPROC_LOOKUP ((u_long)4)
#define NFSPROC_READLINK ((u_long)5)
#define NFSPROC_READ ((u_long)6)
#define NFSPROC_WRITECACHE ((u_long)7)
#define NFSPROC_WRITE ((u_long)8)
#define NFSPROC_CREATE ((u_long)9)
#define NFSPROC_REMOVE ((u_long)10)
#define NFSPROC_RENAME ((u_long)11)
#define NFSPROC_LINK ((u_long)12)
#define NFSPROC_SYMLINK ((u_long)13)
#define NFSPROC_MKDIR ((u_long)14)
#define NFSPROC_RMDIR ((u_long)15)
#define NFSPROC_READDIR ((u_long)16)
#define NFSPROC_STATFS ((u_long)17)

/*
 * NFS mount option flags
 */
#define	NFSMNT_SOFT		0x001	/* soft mount (hard is default) */
#define	NFSMNT_WSIZE		0x002	/* set write size */
#define	NFSMNT_RSIZE		0x004	/* set read size */
#define	NFSMNT_TIMEO		0x008	/* set initial timeout */
#define	NFSMNT_RETRANS		0x010	/* set number of request retries */
#define	NFSMNT_HOSTNAME		0x020	/* set hostname for error printf */
#define	NFSMNT_INT		0x040	/* allow interrupts on hard mount */
#define	NFSMNT_NOAC		0x080	/* don't cache attributes */
#define	NFSMNT_ACREGMIN		0x0100	/* set min secs for file attr cache */
#define	NFSMNT_ACREGMAX		0x0200	/* set max secs for file attr cache */
#define	NFSMNT_ACDIRMIN		0x0400	/* set min secs for dir attr cache */
#define	NFSMNT_ACDIRMAX		0x0800	/* set max secs for dir attr cache */
#define	NFSMNT_SECURE		0x1000	/* secure mount */
#define	NFSMNT_NOCTO		0x2000	/* no close-to-open consistency */
#define	NFSMNT_KNCONF		0x4000	/* transport's knetconfig structure */
#define	NFSMNT_GRPID		0x8000	/* System V-style gid inheritance */
#define	NFSMNT_RPCTIMESYNC	0x10000	/* use RPC to do secure NFS time sync */
#define	NFSMNT_KERBEROS		0x20000	/* use kerberos credentials */
#define	NFSMNT_POSIX		0x40000 /* static pathconf kludge info */
#define	NFSMNT_LLOCK		0x80000	/* Local locking (no lock manager) */
#define NFSMNT_FSNAME           0x1000000 /* set f/s name */

#define MS_FSS        0x00            /* fake flag to do nothing */

/* no am-utils support for NFS V.3 yet */
#undef MNTTYPE_NFS3
#undef MNTTAB_TYPE_NFS3
#undef HAVE_FS_NFS3

/*
 * ENUMS:
 */

/*
 * Error status
 * Should include all possible net errors.
 * For now we just cast errno into an enum nfsstat.
 */
enum nfsstat {
  NFS_OK = 0,			/* no error */
  NFSERR_PERM = 1,		/* Not owner */
  NFSERR_NOENT = 2,		/* No such file or directory */
  NFSERR_IO = 5,		/* I/O error */
  NFSERR_NXIO = 6,		/* No such device or address */
  NFSERR_ACCES = 13,		/* Permission denied */
  NFSERR_EXIST = 17,		/* File exists */
  NFSERR_XDEV = 18,		/* Cross-device link */
  NFSERR_NODEV = 19,		/* No such device */
  NFSERR_NOTDIR = 20,		/* Not a directory */
  NFSERR_ISDIR = 21,		/* Is a directory */
  NFSERR_INVAL = 22,		/* Invalid argument */
  NFSERR_FBIG = 27,		/* File too large */
  NFSERR_NOSPC = 28,		/* No space left on device */
  NFSERR_ROFS = 30,		/* Read-only file system */
  NFSERR_OPNOTSUPP = 45,	/* Operation not supported */
  NFSERR_NAMETOOLONG = 63,	/* File name too long */
  NFSERR_NOTEMPTY = 66,		/* Directory not empty */
  NFSERR_DQUOT = 69,		/* Disc quota exceeded */
  NFSERR_STALE = 70,		/* Stale NFS file handle */
  NFSERR_REMOTE = 71,		/* Object is remote */
  NFSERR_WFLUSH			/* write cache flushed */
};

/*
 * File types
 */
enum nfsftype {
  NFNON,
  NFREG,			/* regular file */
  NFDIR,			/* directory */
  NFBLK,			/* block special */
  NFCHR,			/* character special */
  NFLNK,			/* symbolic link */
  NFSOC				/* socket */
};


/*
 * TYPEDEFS:
 */
typedef struct nfs_fh nfs_fh;
typedef char *filename;
typedef char *nfspath;
typedef char nfscookie[NFS_COOKIESIZE];
typedef enum nfsftype nfsftype;
typedef enum nfsstat nfsstat;
typedef struct attrstat nfsattrstat;
typedef struct createargs nfscreateargs;
typedef struct dirlist nfsdirlist;
typedef struct diropargs nfsdiropargs;
typedef struct diropokres nfsdiropokres;
typedef struct diropres nfsdiropres;
typedef struct entry nfsentry;
typedef struct fattr nfsfattr;
typedef struct linkargs nfslinkargs;
typedef struct nfstime nfstime;
typedef struct readargs nfsreadargs;
typedef struct readdirargs nfsreaddirargs;
typedef struct readdirres nfsreaddirres;
typedef struct readlinkres nfsreadlinkres;
typedef struct readokres nfsreadokres;
typedef struct readres nfsreadres;
typedef struct renameargs nfsrenameargs;
typedef struct sattr nfssattr;
typedef struct sattrargs nfssattrargs;
typedef struct statfsokres nfsstatfsokres;
typedef struct statfsres nfsstatfsres;
typedef struct symlinkargs nfssymlinkargs;
typedef struct writeargs nfswriteargs;


/*
 * EXTERNALS:
 */

extern void * nfsproc_null_2_svc(void *, struct svc_req *);
extern nfsattrstat * nfsproc_getattr_2_svc(nfs_fh *, struct svc_req *);
extern nfsattrstat * nfsproc_setattr_2_svc(nfssattrargs *, struct svc_req *);
extern void * nfsproc_root_2_svc(void *, struct svc_req *);
extern nfsdiropres * nfsproc_lookup_2_svc(nfsdiropargs *, struct svc_req *);
extern nfsreadlinkres * nfsproc_readlink_2_svc(nfs_fh *, struct svc_req *);
extern nfsreadres * nfsproc_read_2_svc(nfsreadargs *, struct svc_req *);
extern void * nfsproc_writecache_2_svc(void *, struct svc_req *);
extern nfsattrstat * nfsproc_write_2_svc(nfswriteargs *, struct svc_req *);
extern nfsdiropres * nfsproc_create_2_svc(nfscreateargs *, struct svc_req *);
extern nfsstat * nfsproc_remove_2_svc(nfsdiropargs *, struct svc_req *);
extern nfsstat * nfsproc_rename_2_svc(nfsrenameargs *, struct svc_req *);
extern nfsstat * nfsproc_link_2_svc(nfslinkargs *, struct svc_req *);
extern nfsstat * nfsproc_symlink_2_svc(nfssymlinkargs *, struct svc_req *);
extern nfsdiropres * nfsproc_mkdir_2_svc(nfscreateargs *, struct svc_req *);
extern nfsstat * nfsproc_rmdir_2_svc(nfsdiropargs *, struct svc_req *);
extern nfsreaddirres * nfsproc_readdir_2_svc(nfsreaddirargs *, struct svc_req *);
extern nfsstatfsres * nfsproc_statfs_2_svc(nfs_fh *, struct svc_req *);

extern bool_t xdr_nfsstat(XDR *, nfsstat*);
extern bool_t xdr_ftype(XDR *, nfsftype*);
extern bool_t xdr_nfs_fh(XDR *, nfs_fh*);
extern bool_t xdr_nfstime(XDR *, nfstime*);
extern bool_t xdr_fattr(XDR *, nfsfattr*);
extern bool_t xdr_sattr(XDR *, nfssattr*);
extern bool_t xdr_filename(XDR *, filename*);
extern bool_t xdr_nfspath(XDR *, nfspath*);
extern bool_t xdr_attrstat(XDR *, nfsattrstat*);
extern bool_t xdr_sattrargs(XDR *, nfssattrargs*);
extern bool_t xdr_diropargs(XDR *, nfsdiropargs*);
extern bool_t xdr_diropokres(XDR *, nfsdiropokres*);
extern bool_t xdr_diropres(XDR *, nfsdiropres*);
extern bool_t xdr_readlinkres(XDR *, nfsreadlinkres*);
extern bool_t xdr_readargs(XDR *, nfsreadargs*);
extern bool_t xdr_readokres(XDR *, nfsreadokres*);
extern bool_t xdr_readres(XDR *, nfsreadres*);
extern bool_t xdr_writeargs(XDR *, nfswriteargs*);
extern bool_t xdr_createargs(XDR *, nfscreateargs*);
extern bool_t xdr_renameargs(XDR *, nfsrenameargs*);
extern bool_t xdr_linkargs(XDR *, nfslinkargs*);
extern bool_t xdr_symlinkargs(XDR *, nfssymlinkargs*);
extern bool_t xdr_nfscookie(XDR *, nfscookie);
extern bool_t xdr_readdirargs(XDR *, nfsreaddirargs*);
extern bool_t xdr_entry(XDR *, nfsentry*);
extern bool_t xdr_dirlist(XDR *, nfsdirlist*);
extern bool_t xdr_readdirres(XDR *, nfsreaddirres*);
extern bool_t xdr_statfsokres(XDR *, nfsstatfsokres*);
extern bool_t xdr_statfsres(XDR *, nfsstatfsres*);

/*
 * STRUCTURES:
 */

/* HP shamelessly stole this from Solaris 2.5.1 */
struct nfs_args {
  struct netbuf		*addr;		/* file server address */
  struct netbuf		*syncaddr;	/* secure NFS time sync addr */
  struct knetconfig	*knconf;	/* transport netconfig struct */
  char			*hostname;	/* server's hostname */
  char			*netname;	/* server's netname */
  caddr_t		fh;		/* File handle to be mounted */
  int			flags;		/* flags */
  int			wsize;		/* write size in bytes */
  int			rsize;		/* read size in bytes */
  int			timeo;		/* initial timeout in .1 secs */
  int			retrans;	/* times to retry send */
  int			acregmin;	/* attr cache file min secs */
  int			acregmax;	/* attr cache file max secs */
  int			acdirmin;	/* attr cache dir min secs */
  int			acdirmax;	/* attr cache dir max secs */
  char                  *fsname;        /* F/S name */
#if 0
  struct pathcnf	*pathconf;	/* static pathconf kludge */
#endif
};

struct nfs_fh {
  char fh_data[NFS_FHSIZE];
};

struct nfstime {
  u_int nt_seconds;
  u_int nt_useconds;
};

struct fattr {
  nfsftype na_type;
  u_int na_mode;
  u_int na_nlink;
  u_int na_uid;
  u_int na_gid;
  u_int na_size;
  u_int na_blocksize;
  u_int na_rdev;
  u_int na_blocks;
  u_int na_fsid;
  u_int na_fileid;
  nfstime na_atime;
  nfstime na_mtime;
  nfstime na_ctime;
};

struct sattr {
  u_int sa_mode;
  u_int sa_uid;
  u_int sa_gid;
  u_int sa_size;
  nfstime sa_atime;
  nfstime sa_mtime;
};

struct attrstat {
  nfsstat ns_status;
  union {
    nfsfattr ns_attr_u;
  } ns_u;
};

struct sattrargs {
  nfs_fh sag_fhandle;
  nfssattr sag_attributes;
};

struct diropargs {
  nfs_fh da_fhandle;		/* was dir */
  filename da_name;
};

struct diropokres {
  nfs_fh drok_fhandle;
  nfsfattr drok_attributes;
};

struct diropres {
  nfsstat dr_status;		/* was status */
  union {
    nfsdiropokres dr_drok_u;	/* was diropres */
  } dr_u;			/* was diropres_u */
};

struct readlinkres {
  nfsstat rlr_status;
  union {
    nfspath rlr_data_u;
  } rlr_u;
};

struct readargs {
  nfs_fh ra_fhandle;
  u_int ra_offset;
  u_int ra_count;
  u_int ra_totalcount;
};

struct readokres {
  nfsfattr raok_attributes;
  struct {
    u_int raok_len_u;
    char *raok_val_u;
  } raok_u;
};

struct readres {
  nfsstat rr_status;
  union {
    nfsreadokres rr_reply_u;
  } rr_u;
};

struct writeargs {
  nfs_fh wra_fhandle;
  u_int wra_beginoffset;
  u_int wra_offset;
  u_int wra_totalcount;
  struct {
    u_int wra_len_u;
    char *wra_val_u;
  } wra_u;
};

struct createargs {
  nfsdiropargs ca_where;
  nfssattr ca_attributes;
};

struct renameargs {
  nfsdiropargs rna_from;
  nfsdiropargs rna_to;
};

struct linkargs {
  nfs_fh la_fhandle;
  nfsdiropargs la_to;
};

struct symlinkargs {
  nfsdiropargs sla_from;
  nfspath sla_to;
  nfssattr sla_attributes;
};

struct readdirargs {
  nfs_fh rda_fhandle;
  nfscookie rda_cookie;
  u_int rda_count;
};

struct entry {
  u_int ne_fileid;
  filename ne_name;
  nfscookie ne_cookie;
  nfsentry *ne_nextentry;
};

struct dirlist {
  nfsentry *dl_entries;
  bool_t dl_eof;
};

struct readdirres {
  nfsstat rdr_status;
  union {
    nfsdirlist rdr_reply_u;
  } rdr_u;
};

struct statfsokres {
  u_int sfrok_tsize;
  u_int sfrok_bsize;
  u_int sfrok_blocks;
  u_int sfrok_bfree;
  u_int sfrok_bavail;
};

struct statfsres {
  nfsstat sfr_status;
  union {
    nfsstatfsokres sfr_reply_u;
  } sfr_u;
};

#endif /* not _AMU_NFS_PROT_H */
