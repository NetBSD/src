#	$NetBSD: Makefile,v 1.13 1999/02/03 10:55:12 christos Exp $

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF" && exists(${MACHINE_ARCH}_elf) 
SUBDIR=	${MACHINE_ARCH}_elf
.else
SUBDIR=	${MACHINE_ARCH}
.endif

.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "powerpc") && \
    (${MACHINE_ARCH} != "arm32")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
