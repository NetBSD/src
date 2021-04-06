# Id: Makefile 3938 2021-04-04 18:16:18Z jkoshy

TOP=	..

SUBDIR=		sys

INCS=		elfdefinitions.h
INCSDIR=	/usr/include

.PHONY:	all clean clobber depend obj

all depend:

clean clobber:
	rm -f ${CLEANFILES}

cleandepend:
	rm -f ${.OBJDIR}/.depend

.include "${TOP}/mk/elftoolchain.inc.mk"
.include "${TOP}/mk/elftoolchain.subdir.mk"
