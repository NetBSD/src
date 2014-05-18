# $NetBSD: Makefile.inc,v 1.2.2.1 2014/05/18 17:46:19 rmind Exp $

.if ${MACHINE_ARCH} == "powerpc" \
    || (defined(MLIBDIR) && "${MLIBDIR}" == "powerpc")
CPPFLAGS+=	-DARCH_ELFSIZE=32
.else
CPPFLAGS+=	-DARCH_ELFSIZE=64
.endif

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
