#	from: @(#)Makefile	5.7 (Berkeley) 9/30/90
#	$Id: Makefile,v 1.4 1994/01/28 00:26:35 cgd Exp $

# -DLOG		log games
PROG=	dm
MAN8=	dm.0
MAN5=	dm.conf.0
BINOWN= games
BINGRP=	kmem
BINMODE=6555
LDADD=	-lc -lkvm	# XXX (for getloadavg)
DPADD=	${LIBKVM}

.include <bsd.prog.mk>
