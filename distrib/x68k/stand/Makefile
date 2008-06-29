#	$NetBSD: Makefile,v 1.6.12.1 2008/06/29 08:25:14 mjf Exp $

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
	${RELEASE_INSTALL} ${MEMBER_${archive}} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
.endfor

.include <bsd.files.mk>
.include <bsd.prog.mk>
