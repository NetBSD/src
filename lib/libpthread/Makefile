#	$NetBSD: Makefile,v 1.15 2003/02/28 18:37:44 nathanw Exp $
#

WARNS=	2

.include <bsd.own.mk>

.if exists(${.CURDIR}/arch/${MACHINE_ARCH})
ARCHSUBDIR=	${MACHINE_ARCH}
.elif exists(${.CURDIR}/arch/${MACHINE_CPU}) 
ARCHSUBDIR=	${MACHINE_CPU}
.else
.BEGIN:
	@echo "WARNING: no ARCHSUBDIR for ${MACHINE_ARCH}/${MACHINE_CPU}; skipping..."
.endif

INCS=	pthread.h pthread_types.h pthread_queue.h
INCSDIR=/usr/include

.if defined(ARCHSUBDIR)

ARCHDIR=	${.CURDIR}/arch/${ARCHSUBDIR}
.PATH:	${ARCHDIR}

CPPFLAGS+=	-I${ARCHDIR} -I${.CURDIR} -I${.OBJDIR} -D_LIBC

DPSRCS+=	assym.h

assym.h: genassym.sh ${ARCHDIR}/genassym.cf
	sh ${.CURDIR}/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

LIB=	pthread

#
# NOTE: When you create a new file for libpthread, make sure that pthread.c
# gets a reference to a symbol in that file.  Otherwise, Unix's stupid static
# library semantics will end up discarding potentially important objects.
#
SRCS=	pthread.c 
SRCS+=	pthread_alarms.c
SRCS+=	pthread_barrier.c
SRCS+=	pthread_cancelstub.c
SRCS+=	pthread_cond.c
SRCS+=	pthread_lock.c 
SRCS+=	pthread_mutex.c
SRCS+=	pthread_run.c
SRCS+=	pthread_rwlock.c
SRCS+=	pthread_sa.c
SRCS+=	pthread_sig.c
#SRCS+=	pthread_sleep.c
SRCS+=	pthread_specific.c
SRCS+=  pthread_stack.c
SRCS+=  pthread_debug.c
SRCS+=	sched.c
SRCS+=	sem.c
# Architecture-dependent files
SRCS+=	pthread_switch.S _context_u.S
.if exists(${ARCHDIR}/pthread_md.c)
SRCS+= pthread_md.c
.endif

pthread_switch.S _context_u.S: assym.h

debuglog: debuglog.o
	$(CC) -o debuglog debuglog.o -lpthread

.include <bsd.lib.mk>

.else

.include <bsd.man.mk>
.include <bsd.files.mk>
.include <bsd.inc.mk>

.endif

# WARNS=2 sets -Wcast-qual. This causes problems for one of
# pthread_setspecific() and pthread_getspecific(), since the constness
# of the argument to setspecific() has to be discarded *somewhere*
# before returning it from getspecific().
CFLAGS+= -Wno-cast-qual
