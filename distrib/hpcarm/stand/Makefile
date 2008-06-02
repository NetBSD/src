#	$NetBSD: Makefile,v 1.1.6.2 2008/06/02 13:21:13 mjf Exp $

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
		${HPCBOOT} ${RELEASEDIR}/${MACHINE}/installation

.include <bsd.files.mk>
.include <bsd.obj.mk>
