#	$NetBSD: Makefile,v 1.49 2008/01/15 03:37:14 rmind Exp $
#

WARNS=	4

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
CPPFLAGS+=	-D__LIBPTHREAD_SOURCE__

# XXX: This crappy poke at libc's internals needs to be fixed.
# We need to put this *after our own includes, so that our "assym.h"
# gets picked, instead of the libc one
CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${.CURDIR}/../libc

DPSRCS+=	assym.h
CLEANFILES+=	assym.h

assym.h: ${ARCHDIR}/genassym.cf pthread.h pthread_int.h pthread_md.h
	${_MKTARGET_CREATE}
	${TOOL_GENASSYM} -- ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${ARCHDIR}/genassym.cf > assym.h.tmp && \
	mv -f assym.h.tmp assym.h

LIB=	pthread

#
# NOTE: When you create a new file for libpthread, make sure that pthread.c
# gets a reference to a symbol in that file.  Otherwise, Unix's stupid static
# library semantics will end up discarding potentially important objects.
#
SRCS=	pthread.c 
SRCS+=	pthread_attr.c
SRCS+=	pthread_barrier.c
SRCS+=	pthread_cancelstub.c
SRCS+=	pthread_cond.c
SRCS+=	pthread_lock.c 
SRCS+=	pthread_misc.c
SRCS+=	pthread_mutex.c
SRCS+=	pthread_mutex2.c
SRCS+=	pthread_rwlock.c
SRCS+=	pthread_rwlock2.c
SRCS+=	pthread_specific.c
SRCS+=	pthread_spin.c
SRCS+=	pthread_tsd.c
SRCS+=	res_state.c
SRCS+=	sem.c
# Architecture-dependent files
SRCS+=	_context_u.S
.if exists(${ARCHDIR}/pthread_md.c)
SRCS+= pthread_md.c
.endif

# The TSD routines are used in the implementation of profiling, and so
# can't be profiled themselves.
COPTS.pthread_specific.c+=	-fomit-frame-pointer -falign-functions=32
pthread_specific.po: pthread_specific.o
	${_MKTARGET_CREATE}
	cp pthread_specific.o pthread_specific.po

# Internal spinlock routines are performance critical.  Don't profile them,
# it's incompatibile with -fomit-frame-pointer.
COPTS.pthread_lock.c+=	-fomit-frame-pointer -falign-functions=32
pthread_lock.po: pthread_lock.o
	${_MKTARGET_CREATE}
	cp pthread_lock.o pthread_lock.po

COPTS.pthread_mutex2.c+=	-fomit-frame-pointer -falign-functions=32
pthread_mutex2.po: pthread_mutex2.o
	${_MKTARGET_CREATE}
	cp pthread_mutex2.o pthread_mutex2.po

COPTS.pthread.c += -Wno-stack-protector

_context_u.po: _context_u.o
	${_MKTARGET_CREATE}
	cp _context_u.o _context_u.po

MAN+=	pthread.3 \
	pthread_attr.3 \
	pthread_attr_getname_np.3 \
	pthread_attr_setcreatesuspend_np.3 \
	pthread_attr_setname_np.3 \
	pthread_barrier_destroy.3 pthread_barrier_init.3 \
	pthread_barrier_wait.3 pthread_barrierattr.3 \
	pthread_cancel.3 pthread_cleanup_push.3 \
	pthread_cond_broadcast.3 pthread_cond_destroy.3 pthread_cond_init.3 \
	pthread_cond_wait.3 \
	pthread_condattr.3 pthread_create.3 pthread_detach.3 pthread_equal.3 \
	pthread_exit.3 \
	pthread_getname_np.3 \
	pthread_getspecific.3 pthread_join.3 \
	pthread_key_create.3 pthread_key_delete.3 pthread_kill.3 \
	pthread_mutex_destroy.3 pthread_mutex_init.3 pthread_mutex_lock.3 \
	pthread_mutex_unlock.3 pthread_mutexattr.3 \
	pthread_once.3 pthread_rwlock_destroy.3 pthread_rwlock_init.3 \
	pthread_rwlock_rdlock.3 pthread_rwlock_unlock.3 \
	pthread_rwlock_wrlock.3 pthread_rwlockattr.3 \
	pthread_schedparam.3 pthread_self.3 \
	pthread_setname_np.3 \
	pthread_setspecific.3 pthread_sigmask.3 pthread_spin_destroy.3 \
	pthread_spin_init.3 pthread_spin_lock.3 pthread_spin_unlock.3 \
	pthread_suspend_np.3 pthread_testcancel.3 \
	sched_yield.3

MLINKS+=	pthread_attr.3 pthread_attr_init.3
MLINKS+=	pthread_attr.3 pthread_attr_destroy.3
MLINKS+=	pthread_attr.3 pthread_attr_setdetachstate.3
MLINKS+=	pthread_attr.3 pthread_attr_getdetachstate.3
MLINKS+=	pthread_attr.3 pthread_attr_setschedparam.3
MLINKS+=	pthread_attr.3 pthread_attr_getschedparam.3
MLINKS+=	pthread_barrierattr.3 pthread_barrierattr_init.3
MLINKS+=	pthread_barrierattr.3 pthread_barrierattr_destroy.3
MLINKS+=	pthread_cleanup_push.3 pthread_cleanup_pop.3
MLINKS+=	pthread_cond_broadcast.3 pthread_cond_signal.3
MLINKS+=	pthread_cond_wait.3 pthread_cond_timedwait.3
MLINKS+=	pthread_condattr.3 pthread_condattr_init.3
MLINKS+=	pthread_condattr.3 pthread_condattr_destroy.3
MLINKS+=	pthread_mutex_lock.3 pthread_mutex_trylock.3
MLINKS+=	pthread_mutexattr.3 pthread_mutexattr_init.3
MLINKS+=	pthread_mutexattr.3 pthread_mutexattr_destroy.3
MLINKS+=	pthread_mutexattr.3 pthread_mutexattr_settype.3
MLINKS+=	pthread_mutexattr.3 pthread_mutexattr_gettype.3
MLINKS+=	pthread_rwlock_rdlock.3 pthread_rwlock_timedrdlock.3
MLINKS+=	pthread_rwlock_rdlock.3 pthread_rwlock_tryrdlock.3
MLINKS+=	pthread_rwlock_wrlock.3 pthread_rwlock_timedwrlock.3
MLINKS+=	pthread_rwlock_wrlock.3 pthread_rwlock_trywrlock.3
MLINKS+=	pthread_rwlockattr.3 pthread_rwlockattr_init.3
MLINKS+=	pthread_rwlockattr.3 pthread_rwlockattr_destroy.3
MLINKS+=	pthread_schedparam.3 pthread_setschedparam.3
MLINKS+=	pthread_schedparam.3 pthread_getschedparam.3
MLINKS+=	pthread_spin_lock.3 pthread_spin_trylock.3
MLINKS+=	pthread_suspend_np.3 pthread_resume_np.3
MLINKS+=	pthread_testcancel.3 pthread_setcancelstate.3
MLINKS+=	pthread_testcancel.3 pthread_setcanceltype.3

pthread_switch.S _context_u.S: assym.h

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
