# $NetBSD: Makefile,v 1.19.2.1 2025/11/20 18:46:45 martin Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libpthread
LDADD+=		-lpthread
LDADD.t_fpu+=	-lm

.include "${.PARSEDIR}/../csu/Makefile.check_stack"

SRCS.t_join=	t_join.c ${SRCS_CHECK_STACK}
CPPFLAGS.t_join.c+=	${CPPFLAGS_CHECK_STACK}
CPPFLAGS.t_condwait.c+=	-I${.CURDIR}/../libc/gen

TESTS_SH+=	t_atexit
TESTS_C+=	t_barrier
TESTS_SH+=	t_cancel
TESTS_C+=	t_cancellation
TESTS_C+=	t_compat_cancel
TESTS_C+=	t_cond
TESTS_C+=	t_condwait
TESTS_C+=	t_detach
TESTS_C+=	t_equal
TESTS_SH+=	t_exit
TESTS_C+=	t_fork
TESTS_C+=	t_fpu
TESTS_C+=	t_join
TESTS_C+=	t_kill
TESTS_C+=	t_mutex
TESTS_C+=	t_name
TESTS_C+=	t_once
TESTS_C+=	t_preempt
TESTS_SH+=	t_resolv
TESTS_C+=	t_rwlock
TESTS_C+=	t_sem
TESTS_C+=	t_sigmask
TESTS_C+=	t_sigsuspend
TESTS_C+=	t_siglongjmp
TESTS_C+=	t_sleep
TESTS_C+=	t_stack
TESTS_C+=	t_swapcontext
TESTS_SH+=	t_thread_local_dtor
TESTS_C+=	t_timedmutex

LDADD.t_sem+=	-lrt
LDADD.t_cancellation+=	-lrt
LDADD.t_compat_cancel+=	-lrt
CPPFLAGS.t_compat_cancel.c+=	-I${NETBSDSRCDIR}/lib/libc
CPPFLAGS.t_compat_cancel.c+=	-I${NETBSDSRCDIR}/sys
LDFLAGS.t_compat_cancel+=	-Wl,--no-fatal-warnings # intend compat use

BINDIR=		${TESTSDIR}
PROGS=		h_atexit
PROGS+=		h_cancel
PROGS+=		h_exit
PROGS+=		h_resolv
PROGS_CXX+=	h_thread_local_dtor

TESTS_C+=	t_call_once t_cnd t_mtx t_thrd t_tss # C11 threads(3)

COPTS.h_thread_local_dtor.cpp+=	-std=c++11
# Deal with questionable warning and header quality in libstdc++.
COPTS.h_thread_local_dtor.cpp+=	 ${${ACTIVE_CC} == "gcc" :?  -Wno-ctor-dtor-privacy -Wno-sign-compare -Wno-shadow :}
SRCS.h_thread_local_dtor= h_thread_local_dtor.cpp

FILESDIR=	${TESTSDIR}
FILES=		d_mach

TESTS_SUBDIRS+=	dlopen
TESTS_SUBDIRS+=	weak

.include <bsd.test.mk>
