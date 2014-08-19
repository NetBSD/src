#	$NetBSD: archdirs.mk,v 1.1.12.1 2014/08/20 00:04:31 tls Exp $

# list of subdirs used per-platform

.if ${MACHINE} == "amd64"
ARCHDIR_SUBDIR=	x86/amd64-xen
.endif

.if ${MACHINE} == "i386"
ARCHDIR_SUBDIR=	x86/i386-xen x86/i386pae-xen
.endif

.if ${MACHINE_ARCH} == "powerpc"
ARCHDIR_SUBDIR=	powerpc/powerpc-4xx powerpc/powerpc-booke
.endif
