# $NetBSD: Makefile,v 1.1.58.1 2023/08/11 13:43:42 martin Exp $

.include <bsd.own.mk>

HELPER_NAME=	conftest
HELPER_DIR=	conf
HELPER_SRCS=	test.c

FILESDIR=	${TESTSDIR}
FILES=		d_conf_ssleay.cnf
.if ${HAVE_OPENSSL} <= 11
FILES+=		d_conf-1x.out
FILESNAME_d_conf-1x.out= d_conf.out
.else
FILES+=		d_conf-3x.out
FILESNAME_d_conf_ssleay.out= ssleay.out
FILESNAME_d_conf-3x.out= d_conf.out
.endif

.include <bsd.init.mk>
