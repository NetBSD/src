#	$NetBSD: Makefile,v 1.6 2003/10/20 01:15:03 lukem Exp $

.include "${.CURDIR}/../Makefile.inc"

DIST=	${IDIST}/src
.PATH:	${DIST} ${IDIST}/man ${IDIST}

SCRIPTS= cvsbug
MAN=	cvsbug.8
CLEANFILES+=.fname cvsbug

cvsbug: cvsbug.in
	${_MKMSGCREATE}
	${_MKCMD}\
	sed -e 's,@PACKAGE_BUGREPORT@,bug-cvs@gnu.org,g' \
	    -e "s,@VERSION@,`cat .fname`,g" ${.ALLSRC} > ${.TARGET}
	${_MKCMD}\
	chmod a+x ${.TARGET}

cvsbug: .fname

.fname: configure
	${_MKMSGCREATE}
	${_MKCMD}\
	echo > .fname \
	  cvs-`sed < ${.ALLSRC} \
		    -e '/PACKAGE_VERSION/!d' \
		    -e 's/[^0-9.]*\([0-9.]*\).*/\1/' \
		    -e q`

.include <bsd.prog.mk>
