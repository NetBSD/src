# Id: Makefile 3943 2021-04-05 17:03:34Z jkoshy

TOP=	../..

SRCS=		elfdefinitions.m4 elfconstants.m4
INCS=		elfdefinitions.h
INCSDIR=	/usr/include/sys

CLEANFILES=	${INCS}

.PHONY:	all clean clobber depend obj

all:	${INCS}

elfdefinitions.h:	elfdefinitions.m4 elfconstants.m4
	m4 -I${.CURDIR} -D SRCDIR=${.CURDIR} ${M4FLAGS} \
		elfdefinitions.m4 > ${.TARGET}

depend cleandepend:

clean clobber:
	rm -f ${CLEANFILES}

.include "${TOP}/mk/elftoolchain.inc.mk"
.include "${TOP}/mk/elftoolchain.m4.mk"
