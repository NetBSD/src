/*
 * Start of am-utils-6.x config.h file.
 * Erez Zadok <ezk@cs.columbia.edu>
 *
 * DO NOT EDIT BY HAND.
 * Note: acconfig.h generates config.h.in, which generates config.h.
 */

#ifndef _CONFIG_H
#define _CONFIG_H


/*
 * Check for types of amd filesystems available.
 */

/* Define if have automount filesystem */
#undef HAVE_AMU_FS_AUTO

/* Define if have direct automount filesystem */
#undef HAVE_AMU_FS_DIRECT

/* Define if have "top-level" filesystem */
#undef HAVE_AMU_FS_TOPLVL

/* Define if have error filesystem */
#undef HAVE_AMU_FS_ERROR

/* Define if have inheritance filesystem */
#undef HAVE_AMU_FS_INHERIT

/* Define if have program filesystem */
#undef HAVE_AMU_FS_PROGRAM

/* Define if have symbolic-link filesystem */
#undef HAVE_AMU_FS_LINK

/* Define if have symlink with existence check filesystem */
#undef HAVE_AMU_FS_LINKX

/* Define if have NFS host-tree filesystem */
#undef HAVE_AMU_FS_HOST

/* Define if have nfsl (NFS with local link check) filesystem */
#undef HAVE_AMU_FS_NFSL

/* Define if have multi-NFS filesystem */
#undef HAVE_AMU_FS_NFSX

/* Define if have union filesystem */
#undef HAVE_AMU_FS_UNION


/*
 * Check for types of maps available.
 */

/* Define if have file maps (everyone should have it!) */
#undef HAVE_MAP_FILE

/* Define if have NIS maps */
#undef HAVE_MAP_NIS

/* Define if have NIS+ maps */
#undef HAVE_MAP_NISPLUS

/* Define if have DBM maps */
#undef HAVE_MAP_DBM

/* Define if have NDBM maps */
#undef HAVE_MAP_NDBM

/* Define if have HESIOD maps */
#undef HAVE_MAP_HESIOD

/* Define if have LDAP maps */
#undef HAVE_MAP_LDAP

/* Define if have PASSWD maps */
#undef HAVE_MAP_PASSWD

/* Define if have UNION maps */
#undef HAVE_MAP_UNION

/*
 * Check for filesystem types available.
 */

/* Define if have UFS filesystem */
#undef HAVE_FS_UFS

/* Define if have XFS filesystem (irix) */
#undef HAVE_FS_XFS

/* Define if have EFS filesystem (irix) */
#undef HAVE_FS_EFS

/* Define if have NFS filesystem */
#undef HAVE_FS_NFS

/* Define if have NFS3 filesystem */
#undef HAVE_FS_NFS3

/* Define if have PCFS filesystem */
#undef HAVE_FS_PCFS

/* Define if have LOFS filesystem */
#undef HAVE_FS_LOFS

/* Define if have HSFS filesystem */
#undef HAVE_FS_HSFS

/* Define if have CDFS filesystem */
#undef HAVE_FS_CDFS

/* Define if have TFS filesystem */
#undef HAVE_FS_TFS

/* Define if have TMPFS filesystem */
#undef HAVE_FS_TMPFS

/* Define if have MFS filesystem */
#undef HAVE_FS_MFS

/* Define if have CFS (crypto) filesystem */
#undef HAVE_FS_CFS

/* Define if have AUTOFS filesystem */
#undef HAVE_FS_AUTOFS

/* Define if have CACHEFS filesystem */
#undef HAVE_FS_CACHEFS

/* Define if have NULLFS (loopback on bsd44) filesystem */
#undef HAVE_FS_NULLFS

/* Define if have UNIONFS filesystem */
#undef HAVE_FS_UNIONFS

/* Define if have UMAPFS (uid/gid mapping) filesystem */
#undef HAVE_FS_UMAPFS


/*
 * Check for the type of the mount(2) system name for a filesystem.
 * Normally this is "nfs" (e.g. Solaris) or an integer (older systems)
 */

/* Mount(2) type/name for UFS filesystem */
#undef MOUNT_TYPE_UFS

/* Mount(2) type/name for XFS filesystem (irix) */
#undef MOUNT_TYPE_XFS

/* Mount(2) type/name for EFS filesystem (irix) */
#undef MOUNT_TYPE_EFS

/* Mount(2) type/name for NFS filesystem */
#undef MOUNT_TYPE_NFS

/* Mount(2) type/name for NFS3 filesystem */
#undef MOUNT_TYPE_NFS3

/* Mount(2) type/name for PCFS filesystem */
/* XXX: conf/trap/trap_hpux.h may override this definition for HPUX 9.0 */
#undef MOUNT_TYPE_PCFS

/* Mount(2) type/name for LOFS filesystem */
#undef MOUNT_TYPE_LOFS

/* Mount(2) type/name for CDFS filesystem */
#undef MOUNT_TYPE_CDFS

/* Mount(2) type/name for TFS filesystem */
#undef MOUNT_TYPE_TFS

/* Mount(2) type/name for TMPFS filesystem */
#undef MOUNT_TYPE_TMPFS

/* Mount(2) type/name for MFS filesystem */
#undef MOUNT_TYPE_MFS

/* Mount(2) type/name for CFS (crypto) filesystem */
#undef MOUNT_TYPE_CFS

/* Mount(2) type/name for AUTOFS filesystem */
#undef MOUNT_TYPE_AUTOFS

/* Mount(2) type/name for CACHEFS filesystem */
#undef MOUNT_TYPE_CACHEFS

/* Mount(2) type/name for IGNORE filesystem (not real just ignore for df) */
#undef MOUNT_TYPE_IGNORE

/* Mount(2) type/name for NULLFS (loopback on bsd44) filesystem */
#undef MOUNT_TYPE_NULLFS

/* Mount(2) type/name for UNIONFS filesystem */
#undef MOUNT_TYPE_UNIONFS

/* Mount(2) type/name for UMAPFS (uid/gid mapping) filesystem */
#undef MOUNT_TYPE_UMAPFS


/*
 * Check for the string name for the mount-table of a filesystem.
 */

/* Mount-table entry name for UFS filesystem */
#undef MNTTAB_TYPE_UFS

/* Mount-table entry name for XFS filesystem (irix) */
#undef MNTTAB_TYPE_XFS

/* Mount-table entry name for EFS filesystem (irix) */
#undef MNTTAB_TYPE_EFS

/* Mount-table entry name for NFS filesystem */
#undef MNTTAB_TYPE_NFS

/* Mount-table entry name for NFS3 filesystem */
#undef MNTTAB_TYPE_NFS3

/* Mount-table entry name for PCFS filesystem */
#undef MNTTAB_TYPE_PCFS

/* Mount-table entry name for LOFS filesystem */
#undef MNTTAB_TYPE_LOFS

/* Mount-table entry name for CDFS filesystem */
#undef MNTTAB_TYPE_CDFS

/* Mount-table entry name for TFS filesystem */
#undef MNTTAB_TYPE_TFS

/* Mount-table entry name for TMPFS filesystem */
#undef MNTTAB_TYPE_TMPFS

/* Mount-table entry name for MFS filesystem */
#undef MNTTAB_TYPE_MFS

/* Mount-table entry name for CFS (crypto) filesystem */
#undef MNTTAB_TYPE_CFS

/* Mount-table entry name for AUTOFS filesystem */
#undef MNTTAB_TYPE_AUTOFS

/* Mount-table entry name for CACHEFS filesystem */
#undef MNTTAB_TYPE_CACHEFS

/* Mount-table entry name for NULLFS (loopback on bsd44) filesystem */
#undef MNTTAB_TYPE_NULLFS

/* Mount-table entry name for UNIONFS filesystem */
#undef MNTTAB_TYPE_UNIONFS

/* Mount-table entry name for UMAPFS (uid/gid mapping) filesystem */
#undef MNTTAB_TYPE_UMAPFS

/*
 * Name of mount table file name.
 */
#undef MNTTAB_FILE_NAME

/* Name of mount type to hide amd mount from df(1) */
#undef HIDE_MOUNT_TYPE

/*
 * Names of various mount table option strings.
 */

/* Mount Table option string: Read only */
#undef MNTTAB_OPT_RO

/* Mount Table option string: Read/write */
#undef MNTTAB_OPT_RW

/* Mount Table option string: Read/write with quotas */
#undef MNTTAB_OPT_RQ

/* Mount Table option string: Check quotas */
#undef MNTTAB_OPT_QUOTA

/* Mount Table option string: Don't check quotas */
#undef MNTTAB_OPT_NOQUOTA

/* Mount Table option string: action to taken on error */
#undef MNTTAB_OPT_ONERROR

/* Mount Table option string: min. time between inconsistencies */
#undef MNTTAB_OPT_TOOSOON

/* Mount Table option string: Soft mount */
#undef MNTTAB_OPT_SOFT

/* Mount Table option string: spongy mount */
#undef MNTTAB_OPT_SPONGY

/* Mount Table option string: Hard mount */
#undef MNTTAB_OPT_HARD

/* Mount Table option string: Set uid allowed */
#undef MNTTAB_OPT_SUID

/* Mount Table option string: Set uid not allowed */
#undef MNTTAB_OPT_NOSUID

/* Mount Table option string: SysV-compatible gid on create */
#undef MNTTAB_OPT_GRPID

/* Mount Table option string: Change mount options */
#undef MNTTAB_OPT_REMOUNT

/* Mount Table option string: Disallow mounts on subdirs */
#undef MNTTAB_OPT_NOSUB

/* Mount Table option string: Do multi-component lookup */
#undef MNTTAB_OPT_MULTI

/* Mount Table option string: Allow NFS ops to be interrupted */
#undef MNTTAB_OPT_INTR

/* Mount Table option string: Don't allow interrupted ops */
#undef MNTTAB_OPT_NOINTR

/* Mount Table option string: NFS server IP port number */
#undef MNTTAB_OPT_PORT

/* Mount Table option string: Secure (AUTH_DES) mounting */
#undef MNTTAB_OPT_SECURE

/* Mount Table option string: Secure (AUTH_Kerb) mounting */
#undef MNTTAB_OPT_KERB

/* Mount Table option string: Max NFS read size (bytes) */
#undef MNTTAB_OPT_RSIZE

/* Mount Table option string: Max NFS write size (bytes) */
#undef MNTTAB_OPT_WSIZE

/* Mount Table option string: NFS timeout (1/10 sec) */
#undef MNTTAB_OPT_TIMEO

/* Mount Table option string: Max retransmissions (soft mnts) */
#undef MNTTAB_OPT_RETRANS

/* Mount Table option string: Attr cache timeout (sec) */
#undef MNTTAB_OPT_ACTIMEO

/* Mount Table option string: Min attr cache timeout (files) */
#undef MNTTAB_OPT_ACREGMIN

/* Mount Table option string: Max attr cache timeout (files) */
#undef MNTTAB_OPT_ACREGMAX

/* Mount Table option string: Min attr cache timeout (dirs) */
#undef MNTTAB_OPT_ACDIRMIN

/* Mount Table option string: Max attr cache timeout (dirs) */
#undef MNTTAB_OPT_ACDIRMAX

/* Mount Table option string: Don't cache attributes at all */
#undef MNTTAB_OPT_NOAC

/* Mount Table option string: No close-to-open consistency */
#undef MNTTAB_OPT_NOCTO

/* Mount Table option string: Do mount retries in background */
#undef MNTTAB_OPT_BG

/* Mount Table option string: Do mount retries in foreground */
#undef MNTTAB_OPT_FG

/* Mount Table option string: Number of mount retries */
#undef MNTTAB_OPT_RETRY

/* Mount Table option string: Device id of mounted fs */
#undef MNTTAB_OPT_DEV

/* Mount Table option string: Filesystem id of mounted fs */
#undef MNTTAB_OPT_FSID

/* Mount Table option string: Get static pathconf for mount */
#undef MNTTAB_OPT_POSIX

/* Mount Table option string: Automount map */
#undef MNTTAB_OPT_MAP

/* Mount Table option string: Automount   direct map mount */
#undef MNTTAB_OPT_DIRECT

/* Mount Table option string: Automount indirect map mount */
#undef MNTTAB_OPT_INDIRECT

/* Mount Table option string: Local locking (no lock manager) */
#undef MNTTAB_OPT_LLOCK

/* Mount Table option string: Ignore this entry */
#undef MNTTAB_OPT_IGNORE

/* Mount Table option string: No auto (what?) */
#undef MNTTAB_OPT_NOAUTO

/* Mount Table option string: No connection */
#undef MNTTAB_OPT_NOCONN

/* Mount Table option string: protocol version number indicator */
#undef MNTTAB_OPT_VERS

/* Mount Table option string: protocol network_id indicator */
#undef MNTTAB_OPT_PROTO

/* Mount Table option string: Synchronous local directory ops */
#undef MNTTAB_OPT_SYNCDIR

/* Mount Table option string: Do no allow setting sec attrs */
#undef MNTTAB_OPT_NOSETSEC

/* Mount Table option string: set symlink cache time-to-live */
#undef MNTTAB_OPT_SYMTTL

/* Mount Table option string: compress */
#undef MNTTAB_OPT_COMPRESS

/* Mount Table option string: paging threshold */
#undef MNTTAB_OPT_PGTHRESH

/* Mount Table option string: max groups */
#undef MNTTAB_OPT_MAXGROUPS

/* Mount Table option string: support property lists (ACLs) */
#undef MNTTAB_OPT_PROPLIST

/*
 * Generic mount(2) options (hex numbers)
 */

/* asynchronous filesystem access */
#undef MNT2_GEN_OPT_ASYNC

/* automounter filesystem (ignore) flag, used in bsdi-4.1 */
#undef MNT2_GEN_OPT_AUTOMNTFS

/* directory hardlink */
#undef MNT2_GEN_OPT_BIND

/* cache (what?) */
#undef MNT2_GEN_OPT_CACHE

/* 6-argument mount */
#undef MNT2_GEN_OPT_DATA

/* old (4-argument) mount (compatibility) */
#undef MNT2_GEN_OPT_FSS

/* ignore mount entry in df output */
#undef MNT2_GEN_OPT_IGNORE

/* journaling filesystem (AIX's UFS/FFS) */
#undef MNT2_GEN_OPT_JFS

/* old BSD group-id on create */
#undef MNT2_GEN_OPT_GRPID

/* do multi-component lookup on files */
#undef MNT2_GEN_OPT_MULTI

/* use type string instead of int */
#undef MNT2_GEN_OPT_NEWTYPE

/* NFS mount */
#undef MNT2_GEN_OPT_NFS

/* nocache (what?) */
#undef MNT2_GEN_OPT_NOCACHE

/* do not interpret special device files */
#undef MNT2_GEN_OPT_NODEV

/* no exec calls allowed */
#undef MNT2_GEN_OPT_NOEXEC

/* do not interpret special device files */
#undef MNT2_GEN_OPT_NONDEV

/* Disallow mounts beneath this mount */
#undef MNT2_GEN_OPT_NOSUB

/* Setuid programs disallowed */
#undef MNT2_GEN_OPT_NOSUID

/* Return ENAMETOOLONG for long filenames */
#undef MNT2_GEN_OPT_NOTRUNC

/* Pass mount option string to kernel */
#undef MNT2_GEN_OPT_OPTIONSTR

/* allow overlay mounts */
#undef MNT2_GEN_OPT_OVERLAY

/* check quotas */
#undef MNT2_GEN_OPT_QUOTA

/* Read-only */
#undef MNT2_GEN_OPT_RDONLY

/* change options on an existing mount */
#undef MNT2_GEN_OPT_REMOUNT

/* read only */
#undef MNT2_GEN_OPT_RONLY

/* synchronize data immediately to filesystem */
#undef MNT2_GEN_OPT_SYNC

/* synchronous filesystem access (same as SYNC) */
#undef MNT2_GEN_OPT_SYNCHRONOUS

/* Mount with Sys 5-specific semantics */
#undef MNT2_GEN_OPT_SYS5

/* Union mount */
#undef MNT2_GEN_OPT_UNION

/*
 * NFS-specific mount(2) options (hex numbers)
 */

/* hide mount type from df(1) */
#undef MNT2_NFS_OPT_AUTO

/* set max secs for dir attr cache */
#undef MNT2_NFS_OPT_ACDIRMAX

/* set min secs for dir attr cache */
#undef MNT2_NFS_OPT_ACDIRMIN

/* set max secs for file attr cache */
#undef MNT2_NFS_OPT_ACREGMAX

/* set min secs for file attr cache */
#undef MNT2_NFS_OPT_ACREGMIN

/* Authentication error */
#undef MNT2_NFS_OPT_AUTHERR

/* set dead server retry thresh */
#undef MNT2_NFS_OPT_DEADTHRESH

/* Dismount in progress */
#undef MNT2_NFS_OPT_DISMINPROG

/* Dismounted */
#undef MNT2_NFS_OPT_DISMNT

/* Don't estimate rtt dynamically */
#undef MNT2_NFS_OPT_DUMBTIMR

/* System V-style gid inheritance */
#undef MNT2_NFS_OPT_GRPID

/* Has authenticator */
#undef MNT2_NFS_OPT_HASAUTH

/* provide name of server's fs to system */
#undef MNT2_NFS_OPT_FSNAME

/* set hostname for error printf */
#undef MNT2_NFS_OPT_HOSTNAME

/* ignore mount point */
#undef MNT2_NFS_OPT_IGNORE

/* allow interrupts on hard mount */
#undef MNT2_NFS_OPT_INT

/* allow interrupts on hard mount */
#undef MNT2_NFS_OPT_INTR

/* Bits set internally */
#undef MNT2_NFS_OPT_INTERNAL

/* Use Kerberos authentication */
#undef MNT2_NFS_OPT_KERB

/* use kerberos credentials */
#undef MNT2_NFS_OPT_KERBEROS

/* transport's knetconfig structure */
#undef MNT2_NFS_OPT_KNCONF

/* set lease term (nqnfs) */
#undef MNT2_NFS_OPT_LEASETERM

/* Local locking (no lock manager) */
#undef MNT2_NFS_OPT_LLOCK

/* set maximum grouplist size */
#undef MNT2_NFS_OPT_MAXGRPS

/* Mnt server for mnt point */
#undef MNT2_NFS_OPT_MNTD

/* Assume writes were mine */
#undef MNT2_NFS_OPT_MYWRITE

/* mount NFS Version 3 */
#undef MNT2_NFS_OPT_NFSV3

/* don't cache attributes */
#undef MNT2_NFS_OPT_NOAC

/* Don't Connect the socket */
#undef MNT2_NFS_OPT_NOCONN

/* no close-to-open consistency */
#undef MNT2_NFS_OPT_NOCTO

/* disallow interrupts on hard mounts */
#undef MNT2_NFS_OPT_NOINT

/* Get lease for lookup */
#undef MNT2_NFS_OPT_NQLOOKLEASE

/* Don't use locking */
#undef MNT2_NFS_OPT_NONLM

/* Use Nqnfs protocol */
#undef MNT2_NFS_OPT_NQNFS

/* static pathconf kludge info */
#undef MNT2_NFS_OPT_POSIX

/* Rcv socket lock */
#undef MNT2_NFS_OPT_RCVLOCK

/* Do lookup with readdir (nqnfs) */
#undef MNT2_NFS_OPT_RDIRALOOK

/* allow property list operations (ACLs over NFS) */
#undef MNT2_NFS_OPT_PROPLIST

/* Use Readdirplus for NFSv3 */
#undef MNT2_NFS_OPTS_RDIRPLUS

/* set read ahead */
#undef MNT2_NFS_OPT_READAHEAD

/* Set readdir size */
#undef MNT2_NFS_OPT_READDIRSIZE

/* Allocate a reserved port */
#undef MNT2_NFS_OPT_RESVPORT

/* set number of request retries */
#undef MNT2_NFS_OPT_RETRANS

/* read only */
#undef MNT2_NFS_OPT_RONLY

/* use RPC to do secure NFS time sync */
#undef MNT2_NFS_OPT_RPCTIMESYNC

/* set read size */
#undef MNT2_NFS_OPT_RSIZE

/* secure mount */
#undef MNT2_NFS_OPT_SECURE

/* Send socket lock */
#undef MNT2_NFS_OPT_SNDLOCK

/* soft mount (hard is default) */
#undef MNT2_NFS_OPT_SOFT

/* spongy mount */
#undef MNT2_NFS_OPT_SPONGY

/* set initial timeout */
#undef MNT2_NFS_OPT_TIMEO

/* use TCP for mounts */
#undef MNT2_NFS_OPT_TCP

/* linux NFSv3 */
#undef MNT2_NFS_OPT_VER3

/* Wait for authentication */
#undef MNT2_NFS_OPT_WAITAUTH

/* Wants an authenticator */
#undef MNT2_NFS_OPT_WANTAUTH

/* Want receive socket lock */
#undef MNT2_NFS_OPT_WANTRCV

/* Want send socket lock */
#undef MNT2_NFS_OPT_WANTSND

/* set write size */
#undef MNT2_NFS_OPT_WSIZE

/* set symlink cache time-to-live */
#undef MNT2_NFS_OPT_SYMTTL

/* paging threshold */
#undef MNT2_NFS_OPT_PGTHRESH

/* 32<->64 dir cookie translation */
#undef MNT2_NFS_OPT_XLATECOOKIE

/*
 * CDFS-specific mount(2) options (hex numbers)
 */

/* Ignore permission bits */
#undef MNT2_CDFS_OPT_DEFPERM

/* Use on-disk permission bits */
#undef MNT2_CDFS_OPT_NODEFPERM

/* Strip off extension from version string */
#undef MNT2_CDFS_OPT_NOVERSION

/* Use Rock Ridge Interchange Protocol (RRIP) extensions */
#undef MNT2_CDFS_OPT_RRIP

/*
 * Existence of fields in structures.
 */

/* does mntent_t have mnt_cnode field? */
#undef HAVE_FIELD_MNTENT_T_MNT_CNODE

/* does mntent_t have mnt_time field? */
#undef HAVE_FIELD_MNTENT_T_MNT_TIME

/* does mntent_t have mnt_time field and is of type "char *" ? */
#undef HAVE_FIELD_MNTENT_T_MNT_TIME_STRING

/* does mntent_t have mnt_ro field? */
#undef HAVE_FIELD_MNTENT_T_MNT_RO

/* does cdfs_args_t have flags field? */
#undef HAVE_FIELD_CDFS_ARGS_T_FLAGS

/* does cdfs_args_t have fspec field? */
#undef HAVE_FIELD_CDFS_ARGS_T_FSPEC

/* does cdfs_args_t have iso_flags field? */
#undef HAVE_FIELD_CDFS_ARGS_T_ISO_FLAGS

/* does cdfs_args_t have iso_pgthresh field? */
#undef HAVE_FIELD_CDFS_ARGS_T_ISO_PGTHRESH

/* does cdfs_args_t have norrip field? */
#undef HAVE_FIELD_CDFS_ARGS_T_NORRIP

/* does cdfs_args_t have ssector field? */
#undef HAVE_FIELD_CDFS_ARGS_T_SSECTOR

/* does pcfs_args_t have dsttime field? */
#undef HAVE_FIELD_PCFS_ARGS_T_DSTTIME

/* does pcfs_args_t have fspec field? */
#undef HAVE_FIELD_PCFS_ARGS_T_FSPEC

/* does pcfs_args_t have gid field? */
#undef HAVE_FIELD_PCFS_ARGS_T_GID

/* does pcfs_args_t have mask field? */
#undef HAVE_FIELD_PCFS_ARGS_T_MASK

/* does pcfs_args_t have secondswest field? */
#undef HAVE_FIELD_PCFS_ARGS_T_SECONDSWEST

/* does pcfs_args_t have uid field? */
#undef HAVE_FIELD_PCFS_ARGS_T_UID

/* does ufs_args_t have flags field? */
#undef HAVE_FIELD_UFS_ARGS_T_FLAGS

/* does ufs_args_t have fspec field? */
#undef HAVE_FIELD_UFS_ARGS_T_FSPEC

/* does efs_args_t have flags field? */
#undef HAVE_FIELD_EFS_ARGS_T_FLAGS

/* does efs_args_t have fspec field? */
#undef HAVE_FIELD_EFS_ARGS_T_FSPEC

/* does xfs_args_t have flags field? */
#undef HAVE_FIELD_XFS_ARGS_T_FLAGS

/* does xfs_args_t have fspec field? */
#undef HAVE_FIELD_XFS_ARGS_T_FSPEC

/* does ufs_ars_t have ufs_flags field? */
#undef HAVE_FIELD_UFS_ARGS_T_UFS_FLAGS

/* does ufs_ars_t have ufs_pgthresh field? */
#undef HAVE_FIELD_UFS_ARGS_T_UFS_PGTHRESH

/* does struct fhstatus have a fhs_fh field? */
#undef HAVE_FIELD_STRUCT_FHSTATUS_FHS_FH

/* does struct statfs have an f_fstypename field? */
#undef HAVE_FIELD_STRUCT_STATFS_F_FSTYPENAME

/* does struct nfs_args have an acdirmin field? */
#undef HAVE_FIELD_NFS_ARGS_T_ACDIRMIN

/* does struct nfs_args have an acregmin field? */
#undef HAVE_FIELD_NFS_ARGS_T_ACREGMIN

/* does struct nfs_args have a bsize field? */
#undef HAVE_FIELD_NFS_ARGS_T_BSIZE

/* does struct nfs_args have an fh_len field? */
#undef HAVE_FIELD_NFS_ARGS_T_FH_LEN

/* does struct nfs_args have an fhsize field? */
#undef HAVE_FIELD_NFS_ARGS_T_FHSIZE

/* does struct nfs_args have a gfs_flags field? */
#undef HAVE_FIELD_NFS_ARGS_T_GFS_FLAGS

/* does struct nfs_args have a namlen field? */
#undef HAVE_FIELD_NFS_ARGS_T_NAMLEN

/* does struct nfs_args have an optstr field? */
#undef HAVE_FIELD_NFS_ARGS_T_OPTSTR

/* does struct nfs_args have a proto field? */
#undef HAVE_FIELD_NFS_ARGS_T_PROTO

/* does struct nfs_args have a socket type field? */
#undef HAVE_FIELD_NFS_ARGS_T_SOTYPE

/* does struct nfs_args have a version field? */
#undef HAVE_FIELD_NFS_ARGS_T_VERSION

/* does struct ifreq have field ifr_addr? */
#undef HAVE_FIELD_STRUCT_IFREQ_IFR_ADDR

/* does struct ifaddrs have field ifa_next? */
#undef HAVE_FIELD_STRUCT_IFADDRS_IFA_NEXT

/* does struct sockaddr have field sa_len? */
#undef HAVE_FIELD_STRUCT_SOCKADDR_SA_LEN

/* does struct autofs_args have an addr field? */
#undef HAVE_FIELD_AUTOFS_ARGS_T_ADDR

/* does umntrequest have an rdevid field? */
#undef HAVE_FIELD_UMNTREQUEST_RDEVID


/* should signal handlers be reinstalled? */
#undef REINSTALL_SIGNAL_HANDLER

/*
 * More definitions that depend on configure options.
 */

/* Turn off general debugging by default */
#undef DEBUG

/* Turn off memory debugging by default */
#undef DEBUG_MEM

/* Define package name (must be defined by configure.in) */
#undef PACKAGE

/* Define version of package (must be defined by configure.in) */
#undef VERSION

/* Define name of host machine's cpu (eg. sparc) */
#undef HOST_CPU

/* Define name of host machine's architecture (eg. sun4) */
#undef HOST_ARCH

/* Define name of host machine's vendor (eg. sun) */
#undef HOST_VENDOR

/* Define name and version of host machine (eg. solaris2.5.1) */
#undef HOST_OS

/* Define only name of host machine OS (eg. solaris2) */
#undef HOST_OS_NAME

/* Define only version of host machine (eg. 2.5.1) */
#undef HOST_OS_VERSION

/* Define the header version of (linux) hosts (eg. 2.2.10) */
#undef HOST_HEADER_VERSION

/* Define name of host */
#undef HOST_NAME

/* Define user name */
#undef USER_NAME

/* Define configuration date */
#undef CONFIG_DATE

/* what type of network transport type is in use?  TLI or sockets? */
#undef HAVE_TRANSPORT_TYPE_TLI

/* Define to `long' if <sys/types.h> doesn't define time_t */
#undef time_t

/* Define to "void *" if compiler can handle, otherwise "char *" */
#undef voidp

/* Define a type/structure for an NFS V2 filehandle */
#undef am_nfs_fh

/* Define a type/structure for an NFS V3 filehandle */
#undef am_nfs_fh3

/* define if the host has NFS protocol headers in system headers */
#undef HAVE_NFS_PROT_HEADERS

/* define name of am-utils' NFS protocol header */
#undef AMU_NFS_PROTOCOL_HEADER

/* Define a type for the nfs_args structure */
#undef nfs_args_t

/* Define the field name for the filehandle within nfs_args_t */
#undef NFS_FH_FIELD

/* Define if plain fhandle type exists */
#undef HAVE_FHANDLE

/* Define the type of the 3rd argument ('in') to svc_getargs() */
#undef SVC_IN_ARG_TYPE

/* Define to the type of xdr procedure type */
#undef XDRPROC_T_TYPE

/* Define if mount table is on file, undefine if in kernel */
#undef MOUNT_TABLE_ON_FILE

/* Define if have struct mntent in one of the standard headers */
#undef HAVE_STRUCT_MNTENT

/* Define if have struct mnttab in one of the standard headers */
#undef HAVE_STRUCT_MNTTAB

/* Define if have struct nfs_args in one of the standard nfs headers */
#undef HAVE_STRUCT_NFS_ARGS

/* Define if have struct nfs_gfs_mount in one of the standard nfs headers */
#undef HAVE_STRUCT_NFS_GFS_MOUNT

/* Type of the 3rd argument to yp_order() */
#undef YP_ORDER_OUTORDER_TYPE

/* Type of the 6th argument to recvfrom() */
#undef RECVFROM_FROMLEN_TYPE

/* Type of the 5rd argument to authunix_create() */
#undef AUTH_CREATE_GIDLIST_TYPE

/* The string used in printf to print the mount-type field of mount(2) */
#undef MTYPE_PRINTF_TYPE

/* Type of the mount-type field in the mount() system call */
#undef MTYPE_TYPE

/* Define a type for the pcfs_args structure */
#undef pcfs_args_t

/* Define a type for the autofs_args structure */
#undef autofs_args_t

/* Define a type for the cachefs_args structure */
#undef cachefs_args_t

/* Define a type for the tmpfs_args structure */
#undef tmpfs_args_t

/* Define a type for the ufs_args structure */
#undef ufs_args_t

/* Define a type for the efs_args structure */
#undef efs_args_t

/* Define a type for the xfs_args structure */
#undef xfs_args_t

/* Define a type for the lofs_args structure */
#undef lofs_args_t

/* Define a type for the cdfs_args structure */
#undef cdfs_args_t

/* Define a type for the mfs_args structure */
#undef mfs_args_t

/* Define a type for the rfs_args structure */
#undef rfs_args_t

/* define if have a bad version of memcmp() */
#undef HAVE_BAD_MEMCMP

/* define if have a bad version of yp_all() */
#undef HAVE_BAD_YP_ALL

/* define if must use NFS "noconn" option */
#undef USE_UNCONNECTED_NFS_SOCKETS
/* define if must NOT use NFS "noconn" option */
#undef USE_CONNECTED_NFS_SOCKETS

/**************************************************************************/
/*** Everything above this line is part of the "TOP" of acconfig.h.	***/
/**************************************************************************/

@TOP@

/**************************************************************************/
/*** Everything below this line is above "BOTTOM" and right below "TOP" ***/
/**************************************************************************/

/**************************************************************************/
/*** Everything below this line is right above "BOTTOM" but below "TOP" ***/
/**************************************************************************/

@BOTTOM@

/**************************************************************************/
/*** Everything below this line is part of the "BOTTOM" of acconfig.h.	***/
/**************************************************************************/

/*
 * Existence of external definitions.
 */

/* does extern definition for sys_errlist[] exist? */
#undef HAVE_EXTERN_SYS_ERRLIST

/* does extern definition for optarg exist? */
#undef HAVE_EXTERN_OPTARG

/* does extern definition for clnt_spcreateerror() exist? */
#undef HAVE_EXTERN_CLNT_SPCREATEERROR

/* does extern definition for clnt_sperrno() exist? */
#undef HAVE_EXTERN_CLNT_SPERRNO

/* does extern definition for free() exist? */
#undef HAVE_EXTERN_FREE

/* does extern definition for get_myaddress() exist? */
#undef HAVE_EXTERN_GET_MYADDRESS

/* does extern definition for getccent() (hpux) exist? */
#undef HAVE_EXTERN_GETCCENT

/* does extern definition for getdomainname() exist? */
#undef HAVE_EXTERN_GETDOMAINNAME

/* does extern definition for gethostname() exist? */
#undef HAVE_EXTERN_GETHOSTNAME

/* does extern definition for getlogin() exist? */
#undef HAVE_EXTERN_GETLOGIN

/* does extern definition for gettablesize() exist? */
#undef HAVE_EXTERN_GETTABLESIZE

/* does extern definition for getpagesize() exist? */
#undef HAVE_EXTERN_GETPAGESIZE

/* does extern definition for innetgr() exist? */
#undef HAVE_EXTERN_INNETGR

/* does extern definition for mkstemp() exist? */
#undef HAVE_EXTERN_MKSTEMP

/* does extern definition for sbrk() exist? */
#undef HAVE_EXTERN_SBRK

/* does extern definition for seteuid() exist? */
#undef HAVE_EXTERN_SETEUID

/* does extern definition for setitimer() exist? */
#undef HAVE_EXTERN_SETITIMER

/* does extern definition for strcasecmp() exist? */
#undef HAVE_EXTERN_STRCASECMP

/* does extern definition for strdup() exist? */
#undef HAVE_EXTERN_STRDUP

/* does extern definition for strstr() exist? */
#undef HAVE_EXTERN_STRSTR

/* does extern definition for usleep() exist? */
#undef HAVE_EXTERN_USLEEP

/* does extern definition for wait3() exist? */
#undef HAVE_EXTERN_WAIT3

/* does extern definition for vsnprintf() exist? */
#undef HAVE_EXTERN_VSNPRINTF

/* does extern definition for xdr_opaque_auth() exist? */
#undef HAVE_EXTERN_XDR_OPAQUE_AUTH

/****************************************************************************/
/*** INCLUDE localconfig.h if it exists, to allow users to make some      ***/
/*** compile time configuration changes.                                  ***/
/****************************************************************************/
/* does a local configuration file exist? */
#undef HAVE_LOCALCONFIG_H
#ifdef HAVE_LOCALCONFIG_H
# include <localconfig.h>
#endif /* HAVE_LOCALCONFIG_H */

#endif /* not _CONFIG_H */

/*
 * Local Variables:
 * mode: c
 * End:
 */

/* End of am-utils-6.x config.h file */
