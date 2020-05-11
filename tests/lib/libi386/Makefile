# $NetBSD: Makefile,v 1.4 2020/05/11 21:51:25 yhardy Exp $

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libi386

.if ${MACHINE} == "amd64" && ${MKCOMPAT} == "yes"
SHLIBINSTALLDIR=	/usr/lib/i386
COPTS+=		-m32
LDFLAGS+=	-m32
LDADD+=		-li386
TESTS_C+=	t_user_ldt
.endif

.include <bsd.test.mk>
