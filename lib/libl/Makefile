#	from: @(#)Makefile	5.1 (Berkeley) 6/18/90
#	$Id: Makefile,v 1.4 1994/12/20 15:51:13 cgd Exp $

LIB=	l
SRCS=	libmain.c libyywrap.c
NOPIC=

LINKS=	/usr/lib/libl.a /usr/lib/libfl.a

.PATH:	${.CURDIR}/../../usr.bin/lex
.include <bsd.lib.mk>

.if !defined(NOPROFILE)
LINKS+=	/usr/lib/libl_p.a /usr/lib/libfl_p.a
.endif
