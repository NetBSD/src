#	from: @(#)Makefile	6.2 (Berkeley) 4/30/91
#	$Id: Makefile,v 1.4 1993/10/07 07:25:38 cgd Exp $

PROG=	ld
SRCS=	ld.c cplus-dem.c
NOMAN=	noman
LDADD+=	-lgnumalloc
DPADD+=	/usr/lib/libgnumalloc.a

.include <bsd.prog.mk>
