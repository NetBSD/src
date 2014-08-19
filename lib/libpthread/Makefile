#	$NetBSD: Makefile,v 1.77.2.3 2014/08/20 00:02:20 tls Exp $
#

WARNS?=	5

.include <bsd.own.mk>

.if defined(PTHREAD_MACHINE_ARCH) && !empty(PTHREAD_MACHINE_ARCH) && \
    exists(${.CURDIR}/arch/${PTHREAD_MACHINE_ARCH})
ARCHSUBDIR=	${PTHREAD_MACHINE_ARCH}
.elif defined(PTHREAD_MACHINE_CPU) && !empty(PTHREAD_MACHINE_CPU) && \
    exists(${.CURDIR}/arch/${PTHREAD_MACHINE_CPU})
ARCHSUBDIR=	${PTHREAD_MACHINE_CPU}
.elif exists(${.CURDIR}/arch/${MACHINE_ARCH})
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

CPPFLAGS+=	-I${ARCHDIR} -I${.CURDIR} -I${.OBJDIR} -D_LIBC -D_REENTRANT
CPPFLAGS+=	-I${.CURDIR}/../libc/include
CPPFLAGS+=	-D__LIBPTHREAD_SOURCE__ -D__LIBC_THREAD_STUBS

# XXX: This crappy poke at libc's internals needs to be fixed.
CPPFLAGS+=-I${NETBSDSRCDIR}/sys -I${.CURDIR}/../libc

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
SRCS+=	pthread_once.c
SRCS+=	pthread_rwlock.c
SRCS+=	pthread_specific.c
SRCS+=	pthread_spin.c
SRCS+=	pthread_tsd.c
SRCS+=	res_state.c
SRCS+=	sem.c
# Architecture-dependent files
.if exists(${ARCHDIR}/pthread_md.S)
SRCS+= pthread_md.S
.endif
.if exists(${ARCHDIR}/Makefile.inc)
.include "${ARCHDIR}/Makefile.inc"
.endif

# The PTHREAD__COMPAT flag builds a libpthread that can be dropped
# into a NetBSD 2/3/4 chroot with a NetBSD 5 or later kernel.
# This makes threading work in the chroot, no other modifications
# required.
#
.if defined(PTHREAD__COMPAT)
SRCS+=		pthread_compat.c
.PATH.c:	${.CURDIR}/../../common/lib/libc/arch/${ARCHSUBDIR}/atomic
.PATH.S:	${.CURDIR}/../../common/lib/libc/arch/${ARCHSUBDIR}/atomic
.PATH.c:	${.CURDIR}/../../common/lib/libc/atomic
.PATH.c:	${.CURDIR}/../libc/misc
.include "../../common/lib/libc/arch/${ARCHSUBDIR}/atomic/Makefile.inc"
.endif

ALIGN_FUNCTIONS=	${${ACTIVE_CC} == "gcc":? -falign-functions=32 :}

.if ${MACHINE_CPU} != "m68k" && ${MACHINE_CPU} != "sh3" && ${MACHINE_ARCH} != "vax"
OMIT_FRAME_POINTER=	-fomit-frame-pointer
.else
OMIT_FRAME_POINTER=
.endif

# The TSD routines are used in the implementation of profiling, and so
# can't be profiled themselves.
COPTS.pthread_specific.c+=	${OMIT_FRAME_POINTER} ${ALIGN_FUNCTIONS}
pthread_specific.po: pthread_specific.o
	${_MKTARGET_CREATE}
	cp pthread_specific.o pthread_specific.po

# Internal spinlock routines are performance critical.  Don't profile them,
# it's incompatibile with -fomit-frame-pointer.
COPTS.pthread_lock.c+=	${OMIT_FRAME_POINTER} ${ALIGN_FUNCTIONS}
pthread_lock.po: pthread_lock.o
	${_MKTARGET_CREATE}
	cp pthread_lock.o pthread_lock.po

COPTS.pthread_mutex.c+=	${OMIT_FRAME_POINTER} ${ALIGN_FUNCTIONS}
pthread_mutex.po: pthread_mutex.o
	${_MKTARGET_CREATE}
	cp pthread_mutex.o pthread_mutex.po

COPTS.pthread.c += -Wno-stack-protector -Wno-format-nonliteral
COPTS.pthread_attr.c += -Wno-format-nonliteral

MAN+=	affinity.3 pthread.3 \
	pthread_attr.3 \
	pthread_attr_get_np.3 \
	pthread_attr_getdetachstate.3 \
	pthread_attr_getguardsize.3 \
	pthread_attr_getinheritsched.3 \
	pthread_attr_getname_np.3 \
	pthread_attr_getschedparam.3 \
	pthread_attr_getscope.3 \
	pthread_attr_getstack.3 \
	pthread_attr_setcreatesuspend_np.3 \
	pthread_barrier.3 pthread_barrierattr.3 \
	pthread_cancel.3 pthread_cleanup_push.3 \
	pthread_cond.3 pthread_condattr.3 \
	pthread_create.3 pthread_detach.3 pthread_equal.3 \
	pthread_curcpu_np.3 \
	pthread_exit.3 \
	pthread_getname_np.3 \
	pthread_getspecific.3 pthread_join.3 \
	pthread_key_create.3 pthread_kill.3 \
	pthread_mutex.3 pthread_mutexattr.3 \
	pthread_once.3 pthread_rwlock.3 pthread_rwlockattr.3 \
	pthread_schedparam.3 pthread_self.3 \
	pthread_sigmask.3 pthread_spin.3 \
	pthread_suspend_np.3 pthread_testcancel.3

MLINKS+=	pthread_attr_get_np.3 pthread_getattr_np.3
MLINKS+=	affinity.3 pthread_setaffinity_np.3
MLINKS+=	affinity.3 pthread_getaffinity_np.3

MLINKS+=	pthread_attr.3 pthread_attr_init.3
MLINKS+=	pthread_attr.3 pthread_attr_destroy.3

MLINKS+=	pthread_attr_getdetachstate.3 pthread_attr_setdetachstate.3
MLINKS+=	pthread_attr_getguardsize.3 pthread_attr_setguardsize.3
MLINKS+=	pthread_attr_getinheritsched.3 pthread_attr_setinheritsched.3
MLINKS+=	pthread_attr_getname_np.3 pthread_attr_setname_np.3

MLINKS+=	pthread_attr_getschedparam.3 pthread_attr_setschedparam.3 \
		pthread_attr_getschedparam.3 pthread_attr_getschedpolicy.3 \
		pthread_attr_getschedparam.3 pthread_attr_setschedpolicy.3

MLINKS+=	pthread_attr_getscope.3 pthread_attr_setscope.3

MLINKS+=	pthread_attr_getstack.3 pthread_attr_setstack.3 \
		pthread_attr_getstack.3 pthread_attr_getstacksize.3 \
		pthread_attr_getstack.3 pthread_attr_setstacksize.3 \
		pthread_attr_getstack.3 pthread_attr_getstackaddr.3 \
		pthread_attr_getstack.3 pthread_attr_setstackaddr.3

MLINKS+=	pthread_cleanup_push.3 pthread_cleanup_pop.3

MLINKS+=	pthread_barrier.3 pthread_barrier_init.3
MLINKS+=	pthread_barrier.3 pthread_barrier_destroy.3
MLINKS+=	pthread_barrier.3 pthread_barrier_wait.3

MLINKS+=	pthread_barrierattr.3 pthread_barrierattr_init.3
MLINKS+=	pthread_barrierattr.3 pthread_barrierattr_destroy.3

MLINKS+=	pthread_cond.3 pthread_cond_init.3
MLINKS+=	pthread_cond.3 pthread_cond_destroy.3
MLINKS+=	pthread_cond.3 pthread_cond_broadcast.3
MLINKS+=	pthread_cond.3 pthread_cond_wait.3
MLINKS+=	pthread_cond.3 pthread_cond_signal.3
MLINKS+=	pthread_cond.3 pthread_cond_timedwait.3

MLINKS+=	pthread_condattr.3 pthread_condattr_init.3
MLINKS+=	pthread_condattr.3 pthread_condattr_destroy.3
MLINKS+=	pthread_condattr.3 pthread_condattr_setclock.3

MLINKS+=	pthread_getname_np.3 pthread_setname_np.3
MLINKS+=	pthread_getspecific.3 pthread_setspecific.3
MLINKS+=	pthread_key_create.3 pthread_key_delete.3

MLINKS+=	pthread_mutex.3 pthread_mutex_init.3
MLINKS+=	pthread_mutex.3 pthread_mutex_destroy.3
MLINKS+=	pthread_mutex.3 pthread_mutex_lock.3
MLINKS+=	pthread_mutex.3 pthread_mutex_trylock.3
MLINKS+=	pthread_mutex.3 pthread_mutex_unlock.3

MLINKS+=	pthread_mutexattr.3 pthread_mutexattr_init.3
MLINKS+=	pthread_mutexattr.3 pthread_mutexattr_destroy.3
MLINKS+=	pthread_mutexattr.3 pthread_mutexattr_settype.3
MLINKS+=	pthread_mutexattr.3 pthread_mutexattr_gettype.3

MLINKS+=	pthread_rwlock.3 pthread_rwlock_init.3
MLINKS+=	pthread_rwlock.3 pthread_rwlock_destroy.3
MLINKS+=	pthread_rwlock.3 pthread_rwlock_rdlock.3
MLINKS+=	pthread_rwlock.3 pthread_rwlock_wrlock.3
MLINKS+=	pthread_rwlock.3 pthread_rwlock_unlock.3

MLINKS+=	pthread_rwlock.3 pthread_rwlock_timedrdlock.3
MLINKS+=	pthread_rwlock.3 pthread_rwlock_timedwrlock.3
MLINKS+=	pthread_rwlock.3 pthread_rwlock_tryrdlock.3
MLINKS+=	pthread_rwlock.3 pthread_rwlock_trywrlock.3

MLINKS+=	pthread_rwlockattr.3 pthread_rwlockattr_init.3
MLINKS+=	pthread_rwlockattr.3 pthread_rwlockattr_destroy.3

MLINKS+=	pthread_spin.3 pthread_spin_init.3
MLINKS+=	pthread_spin.3 pthread_spin_destroy.3
MLINKS+=	pthread_spin.3 pthread_spin_lock.3
MLINKS+=	pthread_spin.3 pthread_spin_trylock.3
MLINKS+=	pthread_spin.3 pthread_spin_unlock.3

MLINKS+=	pthread_schedparam.3 pthread_setschedparam.3
MLINKS+=	pthread_schedparam.3 pthread_getschedparam.3
MLINKS+=	pthread_suspend_np.3 pthread_resume_np.3
MLINKS+=	pthread_testcancel.3 pthread_setcancelstate.3
MLINKS+=	pthread_testcancel.3 pthread_setcanceltype.3

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
CWARNFLAGS+= -Wno-cast-qual
