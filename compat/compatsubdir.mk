#	$NetBSD: compatsubdir.mk,v 1.5 2010/12/03 21:38:48 plunky Exp $

# Build netbsd libraries.

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if !make(includes)

# make sure we get an objdir built early enough
.include <bsd.prog.mk>

# XXX make this use MAKEOBJDIR
MAKEDIRTARGETENV=	MAKEOBJDIRPREFIX=${.OBJDIR} MKOBJDIRS=yes MKSHARE=no BSD_MK_COMPAT_FILE=${BSD_MK_COMPAT_FILE}

.if defined(BOOTSTRAP_SUBDIRS)
SUBDIR=	${BOOTSTRAP_SUBDIRS}
.else
SUBDIR= ../../../lib .WAIT \
	../../../libexec/ld.elf_so
.endif

.include <bsd.subdir.mk>

.endif
.endif
