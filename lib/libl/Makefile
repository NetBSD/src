#	from: @(#)Makefile	5.1 (Berkeley) 6/18/90
#	$Id: Makefile,v 1.2 1994/02/06 06:28:12 cgd Exp $

LIB=	l
SRCS=	libmain.c libyywrap.c
NOPIC=

LINKS=	/usr/lib/libl.a /usr/lib/libfl.a \
	/usr/lib/libl_p.a /usr/lib/libfl_p.a

.PATH:	${.CURDIR}/../../usr.bin/lex
.include <bsd.lib.mk>
