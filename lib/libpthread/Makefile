#	$NetBSD: Makefile,v 1.8.2.2 2001/07/13 02:10:35 nathanw Exp $
#

.if exists(${.CURDIR}/arch/${MACHINE_ARCH})
ARCHSUBDIR=	${MACHINE_ARCH}
.elif exists(${.CURDIR}/arch/${MACHINE_CPU}) 
ARCHSUBDIR=	${MACHINE_CPU}
.else
.BEGIN:
	@echo no ARCHSUBDIR for ${MACHINE_ARCH} nor ${MACHINE_CPU}
	@false
.endif

ARCHDIR=	${.CURDIR}/arch/${ARCHSUBDIR}
AINC=		-I${ARCHDIR}
.PATH:	${ARCHDIR}

CPPFLAGS+=	${AINC} -I${.CURDIR} -I${.OBJDIR}

DPSRCS+=	assym.h

assym.h: genassym.sh ${ARCHDIR}/genassym.cf
	sh ${.CURDIR}/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

LIB=	pthread
SRCS=	pthread.c pthread_lock.c pthread_mutex.c pthread_run.c
SRCS+=	pthread_sa.c pthread_sig.c pthread_stack.c pthread_switch.S
SRCS+=	sched.c

INCS=	pthread.h pthread_types.h pthread_queue.h sched.h
INCSDIR=/usr/include

.include <bsd.lib.mk>
# DO NOT DELETE
