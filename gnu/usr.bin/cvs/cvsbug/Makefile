#	$NetBSD: Makefile,v 1.3 2000/09/04 22:35:01 christos Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/src
.PATH: ${DIST}
PROG=	cvsbug

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/man

STRIPFLAG=
MAN=	cvsbug.8
SRCS=	cvsbug.sh
CLEANFILES+=.fname

cvsbug: cvsbug.sh
	sed -e 's,xLIBDIRx,Not installed,g' \
	    -e "s,xVERSIONx,`cat .fname`,g" ${.ALLSRC} > ${.TARGET}
	chmod a+x ${.TARGET}

cvsbug: .fname

.fname: version.c
	echo > .fname \
	  cvs-`sed < ${.ALLSRC} \
		    -e '/version_string/!d' \
		    -e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
		    -e q`

.include <bsd.prog.mk>
