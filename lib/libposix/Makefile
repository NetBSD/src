#	$NetBSD: Makefile,v 1.10 2002/08/19 09:41:33 lukem Exp $

LIB=	posix

.include <bsd.own.mk>

CPPFLAGS+=-D_REENTRANT -I${NETBSDSRCDIR}/lib/libc/include
.if exists(${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_ARCH})
ARCHDIR=	${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_ARCH}
.elif exists(${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_CPU})
ARCHDIR=	${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_CPU}
.else
.BEGIN:
	@echo no ARCHDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

AINC=	-I${ARCHDIR}

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
