/* $NetBSD: aux_conf.h,v 1.6 2003/03/09 17:57:50 wiz Exp $ */

/*
 * aux_conf.h:
 * This file gets "filled in" for each architecture.
 * aux_conf.h.  Generated from aux_conf.h.in by configure.
 */

#ifndef _AUX_CONF_H
#define _AUX_CONF_H

/*
 * The next line is a literal inclusion of a file which includes a
 * definition for the MOUNT_TRAP macro for a particular architecture.
 * If it defines the wrong entry, check the AC_CHECK_MOUNT_TRAP m4 macro
 * in $srcdir/m4/macros.
 */

/* $srcdir/conf/trap/trap_default.h */
#define MOUNT_TRAP(type, mnt, flags, mnt_data) mount(type, mnt->mnt_dir, flags, mnt_data)
/* End of included MOUNT_TRAP macro definition file */

/*
 * The next line is a literal replacement of a variable which defines the
 * the UNMOUNT_TRAP macro for a particular architecture.
 * If it defines the wrong entry, check the AC_CHECK_UNMOUNT_CALL m4 macro
 * in $srcdir/aclocal.m4.  If the arguments are being defined wrong, check
 * the macro AC_CHECK_UNMOUNT_ARGS in $srcdir/m4/macros.
 */
#define UNMOUNT_TRAP(mnt)	unmount(mnt->mnt_dir)
/* End of replaced UNMOUNT_TRAP macro definition */

/*
 * The next line is a literal inclusion of a file which includes a
 * definition for the NFS_FH_DREF macro for a particular architecture.
 * If it defines the wrong entry, check the AC_CHECK_NFS_FH_DREF m4 macro
 * in $srcdir/m4/macros.
 */

/* $srcdir/conf/fh_dref/fh_dref_freebsd22.h */
#define	NFS_FH_DREF(dst, src) (dst) = (u_char *) (src)
/* End of included NFS_FH_DREF macro definition file */

/*
 * The next line is a literal inclusion of a file which includes a
 * definition for the NFS_SA_DREF macro for a particular architecture.
 * If it defines the wrong entry, check the AC_CHECK_NFS_SA_DREF m4 macro
 * in $srcdir/m4/macros.
 */

/* $srcdir/conf/sa_dref/sa_dref_bsd44.h */
#define	NFS_SA_DREF(dst, src) { \
		(dst)->addr = (struct sockaddr *) (src); \
		(dst)->addrlen = sizeof(*src); \
		(dst)->sotype = SOCK_DGRAM; \
		(dst)->proto = 0; \
	}
/* End of included NFS_SA_DREF macro definition file */

/*
 * The next line is a literal inclusion of a file which includes a
 * definition for the NFS_HN_DREF macro for a particular architecture.
 * If it defines the wrong entry, check the AC_CHECK_NFS_HN_DREF m4 macro
 * in $srcdir/m4/macros.
 */

/* $srcdir/conf/hn_dref/hn_dref_default.h */
#define NFS_HN_DREF(dst, src) (dst) = (src)
/* End of included NFS_HN_DREF macro definition file */

/*
 * The next line is a literal inclusion of a file which defines
 * necessary structures and functions for this system's flavor
 * of kernel-based autofs.
 * If it defines the wrong entry, check the AC_CHECK_AUTOFS_STYLE m4 macro
 * in $srcdir/m4/macros.
 */

/*
 * Autofs is not supported on this platform,
 * so disable it if it gets detected.
 */

#ifdef MNTTYPE_AUTOFS
# undef MNTTYPE_AUTOFS
#endif /* MNTTYPE_AUTOFS */
#ifdef MNTTAB_TYPE_AUTOFS
# undef MNTTAB_TYPE_AUTOFS
#endif /* MNTTAB_TYPE_AUTOFS */
#ifdef HAVE_FS_AUTOFS
# undef HAVE_FS_AUTOFS
#endif /* HAVE_FS_AUTOFS */
/* End of included AUTOFS_STYLE macro definition file */

#endif /* not _AUX_CONF_H */
