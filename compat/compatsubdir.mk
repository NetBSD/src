#	$NetBSD: compatsubdir.mk,v 1.8 2012/08/17 16:22:27 joerg Exp $

# Build netbsd libraries.

.include <bsd.own.mk>

TARGETS+=	build_install

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
