#	$NetBSD: Makefile,v 1.5 2004/03/19 08:07:28 jmc Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

ARCHIVES=	gzip rawrite
MEMBER_gzip=	gz124x5f.lzh gzip.x
MEMBER_rawrite=	rawrite.doc rawrite.lzh rawrite.x

UUDECODE_FILES=	${ARCHIVES:C/$/.tar.gz/}

.for archive in ${ARCHIVES}
CLEANFILES+=	${MEMBER_${archive}}
.endfor

realall: ${UUDECODE_FILES}
.for archive in ${UUDECODE_FILES}
	${TOOL_PAX} -O -zrvpp -f ${archive}
.endfor

release:
.for archive in ${ARCHIVES}
	${RELEASE_INSTALL} -p ${MEMBER_${archive}} \
	    ${RELEASEDIR}/${MACHINE}/installation/misc
.endfor

.include <bsd.files.mk>
.include <bsd.prog.mk>
