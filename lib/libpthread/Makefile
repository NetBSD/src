#	$NetBSD: Makefile,v 1.8.2.7 2001/08/08 16:39:33 nathanw Exp $
#

WARNS=	2

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
SRCS=	pthread.c pthread_cond.c pthread_lock.c pthread_mutex.c
SRCS+=	pthread_run.c pthread_sa.c pthread_sig.c pthread_specific.c
SRCS+=  pthread_stack.c
SRCS+=  pthread_debug.c
SRCS+=	sched.c
# Architecture-dependent files
SRCS+=	pthread_switch.S _context_u.S


INCS=	pthread.h pthread_types.h pthread_queue.h sched.h
INCSDIR=/usr/include

debuglog: debuglog.o
	$(CC) -o debuglog debuglog.o -lpthread

.include <bsd.lib.mk>

# WARNS=2 sets -Wcast-qual. This causes problems for one of
# pthread_setspecific() and pthread_getspecific(), since the constness
# of the argument to setspecific() has to be discarded *somewhere*
# before returning it from getspecific().
CFLAGS+= -Wno-cast-qual


# DO NOT DELETE
