# $NetBSD: Makefile,v 1.70.2.1 2024/11/17 16:16:10 martin Exp $

NOMAN=		# defined

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/kernel

TESTS_SUBDIRS+=	kqueue
TESTS_C+=	t_cloexec
TESTS_C+=	t_fcntl
TESTS_C+=	t_lock
TESTS_C+=	t_lockf
TESTS_C+=	t_pty
TESTS_C+=	t_mqueue
TESTS_C+=	t_proccwd
TESTS_C+=	t_sysv
TESTS_C+=	t_subr_prf
TESTS_C+=	t_kauth_pr_47598
TESTS_C+=	t_ksem
TESTS_C+=	t_sysctl
TESTS_C+=	t_timeleft
TESTS_C+=	t_zombie

TESTS_SH=	t_umount
TESTS_SH+=	t_umountstress
TESTS_SH+=	t_ps_strings
TESTS_SH+=	t_trapsignal
TESTS_SH+=	t_interp
TESTS_SH+=	t_magic_symlinks
TESTS_SH+=	t_nointerpreter
TESTS_SH+=	t_origin
TESTS_SH+=	t_procpath
TESTS_SH+=	t_fexecve
TESTS_SH+=	t_fpufork
.if ${MKRUMP} != "no"
TESTS_SH+=	t_simplehook
.endif

BINDIR=		${TESTSDIR}
PROGS+=		h_cloexec
PROGS+=		h_fexecve
PROGS+=		h_ps_strings1
PROGS+=		h_ps_strings2
PROGS+=		h_segv
PROGS+=		h_getprocpath
PROGS+=		h_fpufork

SCRIPTSDIR=	${TESTSDIR}
SCRIPTS=	h_interpreter.sh

LDADD.t_mqueue+= -lrt


.if (${MKRUMP} != "no") && !defined(BSD_MK_COMPAT_FILE)
TESTS_SUBDIRS+=	tty

TESTS_C+=	t_extattrctl
TESTS_C+=	t_filedesc
TESTS_C+=	t_rnd
LDADD.t_extattrctl+= ${LIBRUMPBASE}
LDADD.t_filedesc+=  ${LDADD.t_rnd}
LDADD.t_rnd+=  -lrumpdev_rnd -lrumpdev ${LIBRUMPBASE}

.endif

LDADD.t_timeleft+=	-lpthread

CPPFLAGS+=	-D_KERNTYPES

.PATH:			${NETBSDSRCDIR}/sys/kern
TESTS_C+=		t_extent
SRCS.t_extent=		t_extent.c subr_extent.c
CPPFLAGS.t_extent.c=	-D_EXTENT_TESTING -D__POOL_EXPOSE -D_KERNTYPES
CPPFLAGS.subr_extent.c=	-D_EXTENT_TESTING -D__POOL_EXPOSE -D_KERNTYPES

t_subr_prf.c: gen_t_subr_prf ${NETBSDSRCDIR}/sys/kern/subr_prf.c
	${HOST_SH} ${.ALLSRC} ${.TARGET}
.if ${MKSANITIZER:Uno} == "yes"
	# These symbols will be redefined by MKSANITIZER
	${TOOL_SED} -i '/undef .*printf/d' ${.TARGET}
.endif

CPPFLAGS.t_subr_prf.c=	-Wno-pointer-sign	# XXX platform vs kernel SHA2

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
# Test explicitly tests failure modes.
CPPFLAGS.t_subr_prf.c+=	-Wno-error=format-truncation
.endif

SANITIZER_RENAME_CLASSES+=		t_subr_prf
SANITIZER_RENAME_FILES.t_subr_prf+=	t_subr_prf.c
SANITIZER_RENAME_SYMBOL.t_subr_prf+=	snprintf
SANITIZER_RENAME_SYMBOL.t_subr_prf+=	vsnprintf
SANITIZER_RENAME_SYMBOL.t_subr_prf+=	sprintf
SANITIZER_RENAME_SYMBOL.t_subr_prf+=	vsnprintf
SANITIZER_RENAME_SYMBOL.t_subr_prf+=	vasprintf

CLEANFILES+=	t_subr_prf.c

LDADD.h_segv+=	-lm

.include <bsd.test.mk>
