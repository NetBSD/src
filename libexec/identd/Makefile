#	$NetBSD: Makefile,v 1.9 2003/04/19 20:57:35 christos Exp $

PROG=	identd
MAN=	identd.8 

CPPFLAGS+= -DINCLUDE_EXTENSIONS -DSTRONG_LOG -DALLOW_FORMAT
.ifdef DEFAULT_LIE_USER
CPPFLAGS+= -DDEFAULT_LIE_USER=\"${DEFAULT_LIE_USER}\"
.endif

SRCS=	netbsd.c version.c proxy.c config.c parse.c identd.c 

.include <bsd.prog.mk>
