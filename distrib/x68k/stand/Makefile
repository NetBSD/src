#	$NetBSD: Makefile,v 1.1 2002/11/03 02:13:20 isaki Exp $

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
	${UUDECODE} ${.CURDIR}/${archive}.uue
	${PAX} -zrvpp -f ${archive}.tar.gz
.endfor

release:
.for archive in ${ARCHIVES}
	${RELEASE_INSTALL} -p ${MEMBER_${archive}} \
		${RELEASEDIR}/installation/misc
.endfor

.include <bsd.prog.mk>
