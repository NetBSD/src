#	$NetBSD: Makefile,v 1.2 2007/10/17 21:07:49 christos Exp $
#

.include <bsd.own.mk>

PROG=	newgrp
SRCS=	newgrp.c grutil.c
BINOWN=	root
BINMODE=4555
WARNS=	4

.PATH.c: ${.CURDIR}/../su
CPPFLAGS+=-I${.CURDIR}/../su
CPPFLAGS+=-DGRUTIL_ACCEPT_GROUP_NUMBERS
CPPFLAGS+=-DGRUTIL_ALLOW_GROUP_ERRORS	# for POSIX.1 compliance
CPPFLAGS+=-DLOGIN_CAP

DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DKERBEROS
.endif

.include <bsd.prog.mk>
