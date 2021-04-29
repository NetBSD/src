# Id: Makefile 3948 2021-04-24 19:21:00Z jkoshy

TOP=	..

SUBDIR=		sys

INCS=		elfdefinitions.h
INCSDIR=	/usr/include

.PHONY:	all clean clobber depend obj

all depend:

cleandepend:
	rm -f ${.OBJDIR}/.depend

.include "${TOP}/mk/elftoolchain.inc.mk"
.include "${TOP}/mk/elftoolchain.subdir.mk"
