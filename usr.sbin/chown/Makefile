#	from: @(#)Makefile	8.1 (Berkeley) 6/6/93
#	$NetBSD: Makefile,v 1.8 2005/03/16 02:08:52 xtraeme Exp $

PROG=	chown
CPPFLAGS+=-DSUPPORT_DOT
MAN=	chgrp.1 chown.8
LINKS=	${BINDIR}/chown /usr/bin/chgrp

WARNS=  3

.include <bsd.prog.mk>
