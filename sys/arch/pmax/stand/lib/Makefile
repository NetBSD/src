#	$NetBSD: Makefile,v 1.1.4.1 1999/06/21 00:59:10 thorpej Exp $

S?=	${.CURDIR}/../../../../

LIB=	pmax
MKPIC=	no
MKPROFILE=no

CPPFLAGS= -I$S/lib/libsa ${PMAXCPPFLAGS} ${PMAXMISCCPPFLAGS}

SRCS=	bootinfo.c callvec.c conf.c devopen.c putchar.c rz.c
SRCS+=	bootinit.S bootread.S startprog.S

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`
