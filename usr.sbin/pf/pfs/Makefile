
SRCS=	pfs.c token.l parse.y
PROG=	pfs
CPPFLAGS+=-I${NETBSDSRCDIR}/sys/dist/pf
CPPFLAGS+=-I${.CURDIR}
WARNS=   4

YHEADER=parse.h

LDADD+= -ll -ly
DPADD+= ${LIBL} ${LIBY}

BINDIR=/sbin

MAN= pfs.8


.include <bsd.prog.mk>
