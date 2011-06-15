#	$NetBSD: archdirs.mk,v 1.1 2011/06/15 09:45:59 mrg Exp $

# list of subdirs used per-platform

.if ${MACHINE} == "amd64" || ${MACHINE} == "i386"
# not yet
#ARCHDIR_SUBDIR=	x86/x86-xen
.endif

.if ${MACHINE_ARCH} == "powerpc"
ARCHDIR_SUBDIR=	powerpc/powerpc-4xx powerpc/powerpc-booke
.endif
