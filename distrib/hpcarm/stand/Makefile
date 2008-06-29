#	$NetBSD: Makefile,v 1.1.6.3 2008/06/29 08:25:12 mjf Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand/binary/ARM
HPCBOOT=	hpcboot.exe

.PATH: ${HPCBOOTDIR}

UUDECODE_FILES=	${HPCBOOT}
UUDECODE_FILES_RENAME_${HPCBOOT}= ${HPCBOOT}

release:: check_RELEASEDIR .WAIT ${HPCBOOT}
	${INSTALL} -c -m ${NONBINMODE} \
		${HPCBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

.include <bsd.files.mk>
.include <bsd.obj.mk>
