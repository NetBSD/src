#	$NetBSD: archdirs.mk,v 1.1.2.2 2011/01/06 05:19:55 riz Exp $

# list of subdirs used per-platform

.if ${MACHINE} == "sparc64"
ARCHDIR_SUBDIR=	sparc64/sparc
.endif

.if ${MACHINE} == "amd64"
ARCHDIR_SUBDIR=	amd64/i386
.endif
#	$NetBSD: archdirs.mk,v 1.1.2.2 2011/01/06 05:19:55 riz Exp $

# list of subdirs used per-platform

.if ${MACHINE} == "sparc64"
ARCHDIR_SUBDIR=	sparc64/sparc
.endif

.if ${MACHINE} == "amd64"
ARCHDIR_SUBDIR=	amd64/i386
.endif
