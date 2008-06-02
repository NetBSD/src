#	$NetBSD: Makefile,v 1.1.6.2 2008/06/02 13:21:13 mjf Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

.include <bsd.kernobj.mk>

HPCBOOTDIR=	${KERNSRCDIR}/arch/hpc/stand
HPCBOOT_SH3DIR=	${HPCBOOTDIR}/binary/SH3
HPCBOOT_SH3=	hpcboot-sh3.exe
HPCBOOT_SH4DIR=	${HPCBOOTDIR}/binary/SH4
HPCBOOT_SH4=	hpcboot-sh4.exe

UUDECODE_FILES= hpcboot-sh3.exe hpcboot-sh4.exe
UUDECODE_FILES_RENAME_hpcboot-sh3.exe= hpcboot-sh3.exe
UUDECODE_FILES_RENAME_hpcboot-sh4.exe= hpcboot-sh4.exe

CLEANFILES+= hpcboot-sh3.exe.uue hpcboot-sh4.exe.uue

hpcboot-sh3.exe.uue:
	cp ${HPCBOOT_SH3DIR}/hpcboot.exe.uue hpcboot-sh3.exe.uue
hpcboot-sh4.exe.uue:
	cp ${HPCBOOT_SH4DIR}/hpcboot.exe.uue hpcboot-sh4.exe.uue

release:: ${UUDECODE_FILES}
	${INSTALL} -c -m ${NONBINMODE} \
		${HPCBOOT_SH3} ${RELEASEDIR}/${MACHINE}/installation
	${INSTALL} -c -m ${NONBINMODE} \
		${HPCBOOT_SH4} ${RELEASEDIR}/${MACHINE}/installation

.include <bsd.files.mk>
.include <bsd.obj.mk>
