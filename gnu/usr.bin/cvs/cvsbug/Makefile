#	$NetBSD: Makefile,v 1.4.2.1 2003/09/27 14:54:13 tron Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/man ${IDIST}

SCRIPTS= cvsbug
MAN=	cvsbug.8
CLEANFILES+=.fname cvsbug

cvsbug: cvsbug.in
	sed -e 's,@PACKAGE_BUGREPORT@,bug-cvs@gnu.org,g' \
	    -e "s,@VERSION@,`cat .fname`,g" ${.ALLSRC} > ${.TARGET}
	chmod a+x ${.TARGET}

cvsbug: .fname

.fname: configure
	echo > .fname \
	  cvs-`sed < ${.ALLSRC} \
		    -e '/PACKAGE_VERSION/!d' \
		    -e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
		    -e q`

.include <bsd.prog.mk>
