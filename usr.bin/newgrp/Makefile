#	$NetBSD: Makefile,v 1.3.14.1 2009/05/13 19:19:59 jym Exp $
#

.include <bsd.own.mk>

PROG=	newgrp
SRCS=	newgrp.c grutil.c
BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DGRUTIL_ACCEPT_GROUP_NUMBERS
CPPFLAGS+=-DGRUTIL_ALLOW_GROUP_ERRORS	# for POSIX.1 compliance
CPPFLAGS+=-DLOGIN_CAP

DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DKERBEROS
.endif

.include <bsd.prog.mk>
