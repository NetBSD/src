# $NetBSD: Makefile.inc,v 1.1.12.3 2017/12/03 11:39:16 jdolecek Exp $

.if ${MACHINE_ARCH} == "powerpc" \
    || (defined(MLIBDIR) && "${MLIBDIR}" == "powerpc")
CPPFLAGS+=	-DARCH_ELFSIZE=32
.else
CPPFLAGS+=	-DARCH_ELFSIZE=64
.endif

.PATH:	${RUMPTOP}/librump/rumpkern/arch/generic
SRCS+=	rump_generic_abi.c
SRCS+=	rump_generic_cpu.c rump_generic_kobj.c rump_generic_pmap.c
