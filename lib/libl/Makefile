#	from: @(#)Makefile	5.1 (Berkeley) 6/18/90
#	$Id: Makefile,v 1.1 1994/02/04 19:10:17 jtc Exp $

LIB=	l
SRCS=	libmain.c libyywrap.c
NOPIC=

LINKS=	${DESTDIR}/usr/lib/libl.a ${DESTDIR}/usr/lib/libfl.a \
	${DESTDIR}/usr/lib/libl_p.a ${DESTDIR}/usr/lib/libfl_p.a

.PATH:	${.CURDIR}/../../usr.bin/lex
.include <bsd.lib.mk>
