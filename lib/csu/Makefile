#	$NetBSD: Makefile,v 1.26 2010/07/05 21:27:55 mrg Exp $

.include <bsd.own.mk>

.if exists(${CSU_MACHINE_ARCH}_elf)
SUBDIR=	${CSU_MACHINE_ARCH}_elf
.elif exists(${MACHINE_ARCH}_elf)
SUBDIR=	${MACHINE_ARCH}_elf
.elif exists(${MACHINE_CPU}_elf)
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

.include <bsd.subdir.mk>
