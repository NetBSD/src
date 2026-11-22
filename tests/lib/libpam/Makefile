# $NetBSD$

.include <bsd.own.mk>

TESTSDIR=	${TESTSBASE}/lib/libpam

DPADD+=		${LIBPAM}
LDADD+=		-lpam

TESTS_C=	t_pam

.if ${MKKERBEROS} != "no"
CPPFLAGS.t_pam.c+=	-DHAVE_PAM_KRB5
.endif

.include <bsd.test.mk>
