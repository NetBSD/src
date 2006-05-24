#	$NetBSD: Makefile,v 1.4.12.1 2006/05/24 15:47:49 tron Exp $

PROG=		boot32
PROGSOURCE=	rmheader.S rmvers.c srt0.S boot32.c start.S
NEWVERSWHAT=	"RISC OS Boot"
RISCOSTYPE=	ffa

SAMISCMAKEFLAGS+= SA_USE_LOADFILE=yes SA_USE_CREAD=yes

CPPFLAGS+=	-DBOOT_MAXIMISE_MEMORY \
		-DLIBSA_NO_FS_WRITE \
		-DLIBSA_SINGLE_FILESYSTEM=riscos

rmvers.c: ${.CURDIR}/version
	${HOST_SH} ${.CURDIR}/../lib/newvers_rm.sh ${.CURDIR}/version "NetBSD Boot"
.PHONY: rmvers.c

CLEANFILES+=	rmvers.c

.include "../Makefile.buildboot"
