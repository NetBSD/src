#	$Id: Makefile,v 1.3 1994/01/28 00:28:48 cgd Exp $

PROG=	atrun
BINDIR=	/usr/libexec

BINOWN=	root

CFLAGS+= -I${.CURDIR}/../../usr.bin/at
LDADD+=	-lc -lkvm # XXX (for getloadavg())
DPADD+=	${LIBKVM}

.include <bsd.prog.mk>
