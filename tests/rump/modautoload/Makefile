#	$NetBSD: Makefile,v 1.14 2023/04/24 20:55:53 uwe Exp $
#

.include <bsd.init.mk>

TESTSDIR=	${TESTSBASE}/rump/modautoload

TESTS_C=	t_modautoload
PAXCTL_FLAGS=	+ma

SRCS.t_modautoload+=	t_modautoload.c

# Note: we link the rump kernel into the application to make this work
# on amd64.  This is the reason we keep this test in its own
# subdirectory -- otherwise the LDADD lines would get a little hairy.
LDFLAGS+=	-Wl,-E
.if ${MACHINE} == "alpha"
LDFLAGS+=	-Wl,--no-relax
.endif
LDADD+= \
    -Wl,--whole-archive -Wl,-Bstatic \
	-lrumpvfs -lrumpvfs_nofifofs -lrump \
    -Wl,-Bdynamic -Wl,--no-whole-archive

LDADD+=		-lrumpuser -lpthread
DPADD+=		${LIBRUMPVFS} ${LIBRUMP} ${LIBRUMPUSER}

.if ${RUMP_SANITIZE:Uno} != "no"
LDADD+=	-fsanitize=${RUMP_SANITIZE}
.endif

WARNS=	4

# To include a rump version of sysctlbyname()

.PATH:		${NETBSDSRCDIR}/lib/libc/gen
SRCS.t_modautoload+=		sysctlbyname.c
SRCS.t_modautoload+=		sysctlgetmibinfo.c
CPPFLAGS+=	-DRUMP_ACTION

SANITIZER_RENAME_CLASSES+=		t_modautoload
SANITIZER_RENAME_FILES.t_modautoload+=	${SRCS.t_modautoload}
SANITIZER_RENAME_SYMBOL.t_modautoload+=	sysctlbyname
SANITIZER_RENAME_SYMBOL.t_modautoload+=	sysctlgetmibinfo

.include <bsd.test.mk>
