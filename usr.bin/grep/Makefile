# $NetBSD: Makefile,v 1.2.30.1 2009/05/13 19:19:51 jym Exp $

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
