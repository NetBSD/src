#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.5 2011/01/13 22:28:36 haad Exp $

PROG=	chown
CPPFLAGS+=-DSUPPORT_DOT
MAN=	chgrp.1 chown.8
LINKS=	${BINDIR}/chown /bin/chgrp

LINKS+=	${BINDIR}/chown /usr/bin/chgrp
LINKS+=	${BINDIR}/chown /usr/sbin/chown

.include <bsd.prog.mk>
