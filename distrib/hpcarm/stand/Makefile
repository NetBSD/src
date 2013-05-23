#	$NetBSD: Makefile,v 1.3 2013/05/23 20:57:35 christos Exp $

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

.include <bsd.files.mk>
.include <bsd.obj.mk>
