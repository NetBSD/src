#	$NetBSD: Makefile,v 1.14 1999/02/24 15:10:16 drochner Exp $

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF" && exists(${MACHINE_ARCH}_elf) 
SUBDIR=	${MACHINE_ARCH}_elf
.else
.if ${MACHINE_ARCH} == "mipsel" || ${MACHINE_ARCH} == "mipseb"
SUBDIR=	mips
.else
SUBDIR=	${MACHINE_ARCH}
.endif
.endif

.if (${MACHINE_ARCH} != "alpha") && (${MACHINE_ARCH} != "powerpc") && \
    (${MACHINE_ARCH} != "arm32")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
