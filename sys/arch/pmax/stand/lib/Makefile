#	$NetBSD: Makefile,v 1.1 1999/03/25 04:16:15 simonb Exp $

S?=	${.CURDIR}/../../../../

LIB=	pmax
MKPIC=	no
MKPROFILE=no

CPPFLAGS= -I$S/lib/libsa ${PMAXCPPFLAGS} ${PMAXMISCCPPFLAGS}

SRCS=	bootinfo.c callvec.c conf.c devopen.c loadfile.c putchar.c rz.c
SRCS+=	bootinit.S bootread.S startprog.S

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`
