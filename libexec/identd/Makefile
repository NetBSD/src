#	$NetBSD: Makefile,v 1.7 1999/05/18 04:49:41 jwise Exp $

PROG=	identd
MAN=	identd.8 

CPPFLAGS+= -DINCLUDE_EXTENSIONS -DSTRONG_LOG -DALLOW_FORMAT
.ifdef DEFAULT_LIE_USER
CPPFLAGS+= -DDEFAULT_LIE_USER=\"${DEFAULT_LIE_USER}\"
.endif
LDADD+=	-lutil -lkvm
DPADD+=	${LIBUTIL} ${LIBKVM}

SRCS=	netbsd.c version.c proxy.c config.c parse.c identd.c 

.include <bsd.prog.mk>
