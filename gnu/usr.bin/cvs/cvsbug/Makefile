#	$NetBSD: Makefile,v 1.4 2000/09/05 05:04:00 enami Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=${IDIST}/src
.PATH: ${DIST}
SCRIPTS= cvsbug

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/man

MAN=	cvsbug.8
CLEANFILES+=.fname cvsbug

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
