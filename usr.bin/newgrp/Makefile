#	$NetBSD: Makefile,v 1.1.4.1 2007/11/06 23:36:07 matt Exp $
#

.include <bsd.own.mk>

PROG=	newgrp
SRCS=	newgrp.c grutil.c
BINOWN=	root
BINMODE=4555
WARNS=	4

CPPFLAGS+=-DGRUTIL_ACCEPT_GROUP_NUMBERS
CPPFLAGS+=-DGRUTIL_ALLOW_GROUP_ERRORS	# for POSIX.1 compliance
CPPFLAGS+=-DLOGIN_CAP

DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DKERBEROS
.endif

.include <bsd.prog.mk>
