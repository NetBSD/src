#	$NetBSD: Makefile,v 1.24.46.1 2009/12/14 06:29:20 mrg Exp $

.include <bsd.own.mk>

.if ${OBJECT_FMT} == "ELF" && exists(${CSU_MACHINE_ARCH}_elf)
SUBDIR=	${CSU_MACHINE_ARCH}_elf
.elif ${OBJECT_FMT} == "ELF" && exists(${MACHINE_ARCH}_elf)
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

.if (${OBJECT_FMT} != "ELF")
SUBDIR+= c++
.endif

.include <bsd.subdir.mk>
