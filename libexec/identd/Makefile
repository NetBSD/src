#	$NetBSD: Makefile,v 1.8 2002/11/30 03:18:45 lukem Exp $

PROG=	identd
MAN=	identd.8 

CPPFLAGS+= -DINCLUDE_EXTENSIONS -DSTRONG_LOG -DALLOW_FORMAT
.ifdef DEFAULT_LIE_USER
CPPFLAGS+= -DDEFAULT_LIE_USER=\"${DEFAULT_LIE_USER}\"
.endif
LDADD+=	-lkvm
DPADD+=	${LIBKVM}

SRCS=	netbsd.c version.c proxy.c config.c parse.c identd.c 

.include <bsd.prog.mk>
