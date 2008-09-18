#	$NetBSD: Makefile,v 1.1.32.1 2008/09/18 04:41:09 wrstuden Exp $
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

all: ${UUDECODE_FILES}

release: check_RELEASEDIR .WAIT ${UUDECODE_FILES}
	${RELEASE_INSTALL} ${UUDECODE_FILES} \
		${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc
	${MAKESUMS} -t ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation/misc *

UUDECODE_FILES=chg_pid.ttp gzip.ttp rawwrite.ttp aptck.ttp file2swp.ttp
UUDECODE_FILES+=loadbsd.ttp

.include <bsd.files.mk>
.include <bsd.prog.mk>
