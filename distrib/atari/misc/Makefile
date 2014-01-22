#	$NetBSD: Makefile,v 1.3 2014/01/22 00:28:35 christos Exp $
#

.include <bsd.own.mk>

all: ${UUDECODE_FILES}

release: check_RELEASEDIR .WAIT ${UUDECODE_FILES}
	${RELEASE_INSTALL} ${UUDECODE_FILES} \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc *

UUDECODE_FILES=chg_pid.ttp gzip.ttp rawwrite.ttp aptck.ttp file2swp.ttp
UUDECODE_FILES+=loadbsd.ttp

.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"
