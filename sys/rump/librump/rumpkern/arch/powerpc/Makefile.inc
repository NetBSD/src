# $NetBSD: Makefile.inc,v 1.1.12.2 2014/08/20 00:04:41 tls Exp $

.if ${MACHINE_ARCH} == "powerpc" \
    || (defined(MLIBDIR) && "${MLIBDIR}" == "powerpc")
CPPFLAGS+=	-DARCH_ELFSIZE=32
.else
CPPFLAGS+=	-DARCH_ELFSIZE=64
.endif

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
