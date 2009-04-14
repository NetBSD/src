# $NetBSD: Makefile,v 1.3 2009/04/14 22:15:20 lukem Exp $

PROG=	grep
SRCS=	binary.c file.c grep.c mmfile.c queue.c util.c

LINKS=  ${BINDIR}/grep ${BINDIR}/egrep	\
	${BINDIR}/grep ${BINDIR}/fgrep	\
	${BINDIR}/grep ${BINDIR}/zgrep	\
	${BINDIR}/grep ${BINDIR}/zegrep	\
	${BINDIR}/grep ${BINDIR}/zfgrep

MLINKS= grep.1 egrep.1	\
	grep.1 fgrep.1	\
	grep.1 zgrep.1	\
	grep.1 zegrep.1	\
	grep.1 zfgrep.1

LDADD=  -lz

.include <bsd.prog.mk>
