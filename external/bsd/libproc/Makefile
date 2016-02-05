# $NetBSD: Makefile,v 1.2 2016/02/05 10:16:12 roy Exp $

.include <bsd.own.mk>

TEST_SUBDIRS=
.if ${MKATF} != "no"
TEST_SUBDIRS+=	tests
.endif

SUBDIR= lib .WAIT ${TEST_SUBDIRS}

.include <bsd.subdir.mk>
