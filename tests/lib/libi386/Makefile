# $NetBSD: Makefile,v 1.3 2020/04/20 12:08:08 joerg Exp $

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
