# $NetBSD: Makefile,v 1.1 2020/04/19 13:22:58 maxv Exp $

.include <bsd.own.mk>

.if ${MACHINE} == "amd64"
TESTSDIR=	${TESTSBASE}/lib/libi386
COPTS+=		-m32
LDFLAGS+=	-m32
LDADD+=		-li386
TESTS_C+=	t_user_ldt
.endif

.include <bsd.test.mk>
