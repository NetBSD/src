#	$NetBSD: Makefile,v 1.4 2003/07/10 10:34:17 lukem Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

ARCHIVES=	gzip rawrite
MEMBER_gzip=	gz124x5f.lzh gzip.x
MEMBER_rawrite=	rawrite.doc rawrite.lzh rawrite.x

.for archive in ${ARCHIVES}
CLEANFILES+=	${archive}.tar.gz ${MEMBER_${archive}}
.endfor

realall:
.for archive in ${ARCHIVES}
	${TOOL_UUDECODE} ${.CURDIR}/${archive}.uue
	${TOOL_PAX} -O -zrvpp -f ${archive}.tar.gz
.endfor

release:
.for archive in ${ARCHIVES}
	${RELEASE_INSTALL} -p ${MEMBER_${archive}} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
.endfor

.include <bsd.prog.mk>
