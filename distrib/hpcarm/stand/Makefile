#	$NetBSD: Makefile,v 1.2.22.1 2014/05/22 12:01:29 yamt Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand/binary/ARM
HPCBOOT=	hpcboot.exe

.PATH: ${HPCBOOTDIR}

UUDECODE_FILES=	${HPCBOOT}

release:: check_RELEASEDIR .WAIT ${HPCBOOT}
	${INSTALL} -c -m ${NONBINMODE} \
		${HPCBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

.include <bsd.obj.mk>
.include <bsd.files.mk>
