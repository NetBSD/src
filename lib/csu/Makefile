#	$NetBSD: Makefile,v 1.16 1999/02/27 21:13:50 tv Exp $

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF" && exists(${MACHINE_ARCH}_elf) 
SUBDIR=	${MACHINE_ARCH}_elf
.else
SUBDIR=	${MACHINE_ARCH:C/mipse[bl]/mips/}
.endif

.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "powerpc") && \
    (${MACHINE_ARCH} != "arm32")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
