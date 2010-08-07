#	$NetBSD: Makefile,v 1.27 2010/08/07 18:01:32 joerg Exp $

.include <bsd.own.mk>

CSU_MACHINE_ARCH?=	${MACHINE_ARCH}

.if ${CSU_MACHINE_ARCH} == "i386" || ${CSU_MACHINE_ARCH} == "x86_64"
ARCHDIR:=	${.PARSEDIR}/arch/${CSU_MACHINE_ARCH}
.PATH:	${ARCHDIR}
.  include "${ARCHDIR}/Makefile.inc"

.  include "${.PARSEDIR}/common/Makefile.inc"
.else

.  if exists(${CSU_MACHINE_ARCH}_elf/Makefile)
SUBDIR=	${CSU_MACHINE_ARCH}_elf
.  elif exists(${CSU_MACHINE_ARCH}/Makefile)
SUBDIR=	${CSU_MACHINE_ARCH}
.  elif exists(${MACHINE_CPU}_elf/Makefile)
SUBDIR=	${MACHINE_CPU}_elf
.  elif exists(${MACHINE_CPU}/Makefile)
SUBDIR=	${MACHINE_CPU}
.  else
.BEGIN:
	@echo no SUBDIR for ${MACHINE_ARCH}_elf, ${MACHINE_ARCH} nor \
	    ${MACHINE_CPU}
	@false
.  endif

.  include <bsd.subdir.mk>
.endif
