#	$NetBSD: Makefile,v 1.20 1999/07/02 15:53:54 simonb Exp $

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF" && exists(${MACHINE_ARCH}_elf)
SUBDIR=	${MACHINE_ARCH}_elf
.else
SUBDIR=	${MACHINE_ARCH:C/mipse[bl]/mips/}
.endif

.if (${OBJECT_FMT} != "ELF" && ${MACHINE_ARCH} != "arm32")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
