#	$NetBSD: Makefile,v 1.21 2001/01/04 08:01:49 itojun Exp $

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF" && exists(${MACHINE_ARCH}_elf)
SUBDIR=	${MACHINE_ARCH}_elf
.else
SUBDIR=	${MACHINE_ARCH:C/mipse[bl]/mips/:C/sh3e[bl]/sh3/}
.endif

.if (${OBJECT_FMT} != "ELF" && ${MACHINE_ARCH} != "arm32")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
