#	$Id: Makefile,v 1.1 1993/12/05 11:35:48 cgd Exp $

PROG=	atrun
BINDIR=	/usr/libexec

CFLAGS+= -I${.CURDIR}/../../usr.bin/at
LDADD+=	-lutil
DPADD+=	${LIBUTIL}

.include <bsd.prog.mk>
