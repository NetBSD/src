#	$NetBSD: Makefile,v 1.23.20.1 2008/09/18 04:39:18 wrstuden Exp $

.include <bsd.own.mk>

ELFSIZE=	64

#Uncomment the next line to enable the new .init fallthru
CPPFLAGS+=	-I${.CURDIR}

OBJS+=	crtfm.o

.include "${.CURDIR}/../common_elf/Makefile.inc"

crtfm.o: crtfm.c
	${_MKTARGET_COMPILE}
	${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	rm -f ${.TARGET}.o
