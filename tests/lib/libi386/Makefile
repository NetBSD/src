# $NetBSD: Makefile,v 1.3.2.2 2020/04/21 18:42:47 martin Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libi386

.if ${MACHINE} == "amd64"
SHLIBINSTALLDIR=	/usr/lib/i386
COPTS+=		-m32
LDFLAGS+=	-m32
LDADD+=		-li386
TESTS_C+=	t_user_ldt
.endif

.include <bsd.test.mk>
