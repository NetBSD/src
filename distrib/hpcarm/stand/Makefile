#	$NetBSD: Makefile,v 1.4 2014/01/22 00:27:36 christos Exp $

.include <bsd.own.mk>

.include <bsd.kernobj.mk>

HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand/binary/ARM
HPCBOOT=	hpcboot.exe

.PATH: ${HPCBOOTDIR}

UUDECODE_FILES=	${HPCBOOT}

release:: check_RELEASEDIR .WAIT ${HPCBOOT}
	${INSTALL} -c -m ${NONBINMODE} \
		${HPCBOOT} ${RELEASEDIR}/${RELEASEMACHINEDIR}/installation

.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"
.include <bsd.obj.mk>
