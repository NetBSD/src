#	$NetBSD: Makefile,v 1.22 2002/01/14 01:31:58 thorpej Exp $

ELFSIZE=	64

#Uncomment the next line to enable the new .init fallthru
CPPFLAGS+=	-I- -I${.CURDIR}

OBJS+=	crtfm.o

.include "${.CURDIR}/../common_elf/Makefile.inc"

crtfm.o: crtfm.c
	@echo "${COMPILE.c} ${.ALLSRC} -o ${.TARGET}"
	@${COMPILE.c} ${.ALLSRC} -o ${.TARGET}.o
	@${LD} -x -r -o ${.TARGET} ${.TARGET}.o
	@rm -f ${.TARGET}.o
