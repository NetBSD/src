#	$NetBSD: Makefile,v 1.23 2001/01/10 13:20:17 enami Exp $

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF" && exists(${MACHINE_ARCH}_elf)
SUBDIR=	${MACHINE_ARCH}_elf
.elif ${OBJECT_FMT} == "ELF" && exists(${MACHINE_CPU}_elf)
SUBDIR=	${MACHINE_CPU}_elf
.elif exists(${MACHINE_ARCH})
SUBDIR=	${MACHINE_ARCH}
.elif exists(${MACHINE_CPU})
SUBDIR=	${MACHINE_CPU}
.else
.BEGIN:
	@echo no SUBDIR for ${MACHINE_ARCH}_elf, ${MACHINE_ARCH} nor \
	    ${MACHINE_CPU}
	@false
.endif

.if (${OBJECT_FMT} != "ELF" && ${MACHINE_ARCH} != "arm32")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
