#	$NetBSD: compatsubdir.mk,v 1.7 2011/01/31 10:58:14 matt Exp $

# Build netbsd libraries.

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if !make(includes)

# make sure we get an objdir built early enough
.include <bsd.prog.mk>

MAKEDIRTARGETENV=
.if defined(MAKEOBJDIRPREFIX)
MAKEDIRTARGETENV+=	unset MAKEOBJDIRPREFIX &&
.endif
MAKEDIRTARGETENV+=	MAKEOBJDIR='$${.CURDIR:C,^${NETBSDSRCDIR},${.OBJDIR},}'
MAKEDIRTARGETENV+=	MKOBJDIRS=yes MKSHARE=no
MAKEDIRTARGETENV+=	BSD_MK_COMPAT_FILE=${BSD_MK_COMPAT_FILE}

.if defined(BOOTSTRAP_SUBDIRS)
SUBDIR=	${BOOTSTRAP_SUBDIRS}
.else
SUBDIR= ../../../lib .WAIT \
	../../../libexec/ld.elf_so
.endif

.include <bsd.subdir.mk>

.endif
.endif
