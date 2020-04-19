# $NetBSD: Makefile,v 1.2 2020/04/19 18:07:00 kre Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libi386

.if ${MACHINE} == "amd64"
COPTS+=		-m32
LDFLAGS+=	-m32
LDADD+=		-li386
TESTS_C+=	t_user_ldt
.endif

.include <bsd.test.mk>
