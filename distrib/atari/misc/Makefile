#	$NetBSD: Makefile,v 1.1 2004/12/12 08:25:59 jmc Exp $
#

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

all: ${UUDECODE_FILES}

release: check_RELEASEDIR .WAIT ${UUDECODE_FILES}
	${RELEASE_INSTALL} ${UUDECODE_FILES} \
		${RELEASEDIR}/${MACHINE}/installation/misc
	${MAKESUMS} -t ${RELEASEDIR}/${MACHINE}/installation/misc *

UUDECODE_FILES=chg_pid.ttp gzip.ttp rawwrite.ttp aptck.ttp file2swp.ttp
UUDECODE_FILES+=loadbsd.ttp

.include <bsd.files.mk>
.include <bsd.prog.mk>
