#	$NetBSD: Makefile,v 1.15 1999/02/27 03:27:01 tv Exp $

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF" && exists(${MACHINE_ARCH}_elf) 
SUBDIR=	${MACHINE_ARCH}_elf
.else
SUBDIR=	${MACHINE_ARCH:S/mipse[bl]/mips/}
.endif

.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "powerpc") && \
    (${MACHINE_ARCH} != "arm32")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
