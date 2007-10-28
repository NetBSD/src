#	$NetBSD: Makefile,v 1.1 2007/06/21 14:09:23 ginsbach Exp $
#

.include <bsd.own.mk>

PROG=	newgrp
BINOWN=	root
BINMODE=4555

CPPFLAGS+=-DLOGIN_CAP

DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+= -lcrypt -lutil

.if (${USE_KERBEROS} != "no")
CPPFLAGS+=-DKERBEROS
.endif

.include <bsd.prog.mk>
