#	$NetBSD: Makefile,v 1.10 2017/02/11 03:07:06 christos Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.if ${MKREPRO_TIMESTAMP:Uno} != "no"
MAKEFS_TIMESTAMP=-T "${MKREPRO_TIMESTAMP}"
PAX_TIMESTAMP=--timestamp "${MKREPRO_TIMESTAMP}"
.endif

ARCHIVES=	gzip rawrite
MEMBER_gzip=	gz124x5f.lzh gzip.x
MEMBER_rawrite=	rawrite.doc rawrite.lzh rawrite.x

UUDECODE_FILES=	${ARCHIVES:C/$/.tar.gz/}

.for archive in ${ARCHIVES}
CLEANFILES+=	${MEMBER_${archive}}
.endfor

realall: ${UUDECODE_FILES}
.for archive in ${UUDECODE_FILES}
	${TOOL_PAX} ${PAX_TIMESTAMP} -O -zrvpp -f ${archive}
.endfor

release:
.for archive in ${ARCHIVES}
	${RELEASE_INSTALL} ${MEMBER_${archive}} \
	    ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
.endfor

.include <bsd.files.mk>
