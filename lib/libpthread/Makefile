#	$NetBSD: Makefile,v 1.8.2.11 2002/04/04 00:39:41 nathanw Exp $
#

WARNS=	2

.include <bsd.own.mk>

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
.PATH:	${ARCHDIR}

CPPFLAGS+=	-I${ARCHDIR} -I${.CURDIR} -I${.OBJDIR}

DPSRCS+=	assym.h

assym.h: genassym.sh ${ARCHDIR}/genassym.cf
	sh ${.CURDIR}/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

LIB=	pthread

SRCS=	pthread.c 
SRCS+=	pthread_alarms.c
SRCS+=	pthread_cancelstub.c
SRCS+=	pthread_cond.c
SRCS+=	pthread_lock.c 
SRCS+=	pthread_mutex.c
SRCS+=	pthread_run.c
SRCS+=	pthread_sa.c
SRCS+=	pthread_sig.c
SRCS+=	pthread_specific.c
SRCS+=  pthread_stack.c
SRCS+=  pthread_debug.c
SRCS+=	sched.c
# Architecture-dependent files
SRCS+=	pthread_switch.S _context_u.S

pthread_switch.S _context_u.S: assym.h

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
