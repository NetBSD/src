#	$NetBSD: Makefile,v 1.5.156.1 2020/04/13 08:03:28 martin Exp $

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

AFLAGS.start.S= ${${ACTIVE_CC} == "clang":?-no-integrated-as:}

.include "../Makefile.buildboot"
