#	$NetBSD: Makefile,v 1.18 1999/03/19 22:42:16 thorpej Exp $

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF" && exists(${MACHINE_ARCH}_elf) 
SUBDIR=	${MACHINE_ARCH}_elf
.else
SUBDIR=	${MACHINE_ARCH:C/mipse[bl]/mips/}
.endif

.if (${OBJECT_FMT} != "ELF")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
