# Id: Makefile 4199 2025-08-09 21:03:54Z jkoshy

TOP=	../..

SRCS=		elfdefinitions.m4 elfconstants.m4
INCS=		elfdefinitions.h
INCSDIR=	/usr/include/sys

CLEANFILES=	${INCS}

.PHONY:	all clean clobber depend obj

all:	${INCS}

elfdefinitions.h:	elfdefinitions.m4 elfconstants.m4
	m4 -I${.CURDIR} ${M4FLAGS} elfdefinitions.m4 > ${.TARGET}

depend cleandepend:

clean clobber:
	rm -f ${CLEANFILES}

.include "${TOP}/mk/elftoolchain.inc.mk"
.include "${TOP}/mk/elftoolchain.m4.mk"
