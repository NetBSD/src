#	$NetBSD: Makefile,v 1.1.2.2 2009/05/13 18:52:48 jym Exp $

.include "${.CURDIR}/../../Makefile.inc"

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/man ${IDIST}

SCRIPTS= cvsbug
MAN=	cvsbug.8
CLEANFILES+=.fname cvsbug

cvsbug: cvsbug.in
	${_MKTARGET_CREATE}
	${TOOL_SED} -e 's,@PACKAGE_BUGREPORT@,bug-cvs@gnu.org,g' \
	    -e "s,@VERSION@,`cat .fname`,g" ${.ALLSRC} > ${.TARGET}
	chmod a+x ${.TARGET}

cvsbug: .fname

.fname: configure
	${_MKTARGET_CREATE}
	echo > .fname \
	  cvs-`${TOOL_SED} < ${.ALLSRC} \
		    -e '/PACKAGE_VERSION/!d' \
		    -e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
		    -e q`

.include <bsd.prog.mk>
