#	$NetBSD: Makefile,v 1.2 1999/04/28 09:18:48 christos Exp $

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
