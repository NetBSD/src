#	$NetBSD: Makefile,v 1.6 1998/07/15 07:31:55 msaitoh Exp $

PROG=	identd
MAN=	identd.8 

CPPFLAGS+= -DINCLUDE_EXTENSIONS -DSTRONG_LOG -DALLOW_FORMAT
LDADD+=	-lutil -lkvm
DPADD+=	${LIBUTIL} ${LIBKVM}

SRCS=	netbsd.c version.c proxy.c config.c parse.c identd.c 

.include <bsd.prog.mk>
