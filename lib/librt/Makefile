#	$NetBSD: Makefile,v 1.2 2007/04/30 14:44:29 rmind Exp $
#

WARNS=	2

LIB=	rt
SRCS=	sem.c

MAN+=	sem_destroy.3 sem_getvalue.3 sem_init.3 sem_open.3 sem_post.3 \
	sem_wait.3

MLINKS+=	sem_open.3 sem_close.3
MLINKS+=	sem_open.3 sem_unlink.3
MLINKS+=	sem_wait.3 sem_trywait.3

.include <bsd.own.mk>

.if exists(${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_ARCH})
ARCHDIR=	${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_ARCH}
.elif exists(${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_CPU})
ARCHDIR=	${NETBSDSRCDIR}/lib/libc/arch/${MACHINE_CPU}
.else
.BEGIN:
	@echo no ARCHDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

AFLAGS+=-I${ARCHDIR}

.include "${.CURDIR}/sys/Makefile.inc"

.include <bsd.lib.mk>
