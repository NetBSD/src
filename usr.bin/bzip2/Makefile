#	$NetBSD: Makefile,v 1.8 2002/09/18 14:00:34 lukem Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

PROG=	   bzip2
LDDIR!=	   cd ${NETBSDSRCDIR}/lib/libbz2 && ${PRINTOBJDIR}
LDADD+=	   -L${LDDIR} -lbz2
DPADD+=	   ${LIBBZ2}

.if (${MKDYNAMICROOT} == "no")
LDSTATIC?= -static
.endif

DIST=	   ${NETBSDSRCDIR}/dist/bzip2
.PATH:	   ${DIST}

MLINKS+=   bzip2.1 bunzip2.1\
	   bzip2.1 bzcat.1\
	   bzip2.1 bzip2recover.1

LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bunzip2
LINKS+=    ${BINDIR}/bzip2 ${BINDIR}/bzcat

.include <bsd.prog.mk>
