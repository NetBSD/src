#	from: @(#)Makefile	5.1 (Berkeley) 6/18/90
#	$Id: Makefile,v 1.3 1994/02/11 04:24:34 hpeyerl Exp $

LIB=	l
SRCS=	libmain.c libyywrap.c
NOPIC=

LINKS=	/usr/lib/libl.a /usr/lib/libfl.a
.if !defined(NOPROFILE)
LINKS+=	/usr/lib/libl_p.a /usr/lib/libfl_p.a
.endif

.PATH:	${.CURDIR}/../../usr.bin/lex
.include <bsd.lib.mk>
