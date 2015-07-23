#	$NetBSD: compatsubdir.mk,v 1.13 2015/07/23 08:03:25 mrg Exp $

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
.if ${MKCOMPATTESTS} != "no"
SUBDIR+= ../../../tests
SUBDIR+= ../../../tests/share		# because MKSHARE=no above
SUBDIR+= ../../../external/bsd/atf/tests
.endif
.if ${MKCOMPATX11} != no && ${MKX11} != no && make(obj)
SUBDIR+= ../../../external/mit/xorg/lib
.endif # } MKX11
.endif # } BOOTSTRAP_SUBDIRS

.include <bsd.subdir.mk>

.endif
.endif
