#	$NetBSD: compatsubdir.mk,v 1.5.2.2 2011/01/06 05:19:55 riz Exp $

# Build netbsd libraries.

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if !make(includes)

# make sure we get an objdir built early enough
.include <bsd.prog.mk>

# XXX make this use MAKEOBJDIR
MAKEDIRTARGETENV=	MAKEOBJDIRPREFIX=${.OBJDIR} MKOBJDIRS=yes MKSHARE=no BSD_MK_COMPAT_FILE=${BSD_MK_COMPAT_FILE}

# XXX fix the "library" list to include all 'external' libs?
.if defined(BOOTSTRAP_SUBDIRS)
SUBDIR=	${BOOTSTRAP_SUBDIRS}
.else
SUBDIR= ../../../gnu/lib/crtstuff4 .WAIT \
	../../../lib/csu .WAIT \
	../../../gnu/lib/libgcc4 .WAIT \
	../../../lib/libc .WAIT \
	../../../lib/libutil .WAIT \
	../../../lib .WAIT \
	../../../gnu/lib \
	../../../libexec/ld.elf_so

.if (${MKLDAP} != "no")
SUBDIR+= ../../../external/bsd/openldap/lib
.endif

.endif

.include <bsd.subdir.mk>

.endif
.endif
#	$NetBSD: compatsubdir.mk,v 1.5.2.2 2011/01/06 05:19:55 riz Exp $

# Build netbsd libraries.

.include <bsd.own.mk>

.if ${MKCOMPAT} != "no"
.if !make(includes)

# make sure we get an objdir built early enough
.include <bsd.prog.mk>

# XXX make this use MAKEOBJDIR
MAKEDIRTARGETENV=	MAKEOBJDIRPREFIX=${.OBJDIR} MKOBJDIRS=yes MKSHARE=no BSD_MK_COMPAT_FILE=${BSD_MK_COMPAT_FILE}

# XXX fix the "library" list to include all 'external' libs?
.if defined(BOOTSTRAP_SUBDIRS)
SUBDIR=	${BOOTSTRAP_SUBDIRS}
.else
SUBDIR= ../../../gnu/lib/crtstuff4 .WAIT \
	../../../lib/csu .WAIT \
	../../../gnu/lib/libgcc4 .WAIT \
	../../../lib/libc .WAIT \
	../../../lib/libutil .WAIT \
	../../../lib .WAIT \
	../../../gnu/lib \
	../../../libexec/ld.elf_so

.if (${MKLDAP} != "no")
SUBDIR+= ../../../external/bsd/openldap/lib
.endif

.endif

.include <bsd.subdir.mk>

.endif
.endif
