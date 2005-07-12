#	$NetBSD: Makefile,v 1.4.2.2 2005/07/12 03:34:14 snj Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/man ${IDIST}

SCRIPTS= cvsbug
MAN=	cvsbug.8
CLEANFILES+=.fname cvsbug

cvsbug: cvsbug.in
	${_MKTARGET_CREATE}
	sed -e 's,@PACKAGE_BUGREPORT@,bug-cvs@gnu.org,g' \
	    -e "s,@VERSION@,`cat .fname`,g" ${.ALLSRC} > ${.TARGET}
	chmod a+x ${.TARGET}

cvsbug: .fname

.fname: configure
	${_MKTARGET_CREATE}
	echo > .fname \
	  cvs-`sed < ${.ALLSRC} \
		    -e '/PACKAGE_VERSION/!d' \
		    -e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
		    -e q`

.include <bsd.prog.mk>
