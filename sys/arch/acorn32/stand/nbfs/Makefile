#	$NetBSD: Makefile,v 1.1.4.2 2006/04/19 02:31:57 elad Exp $

PROG=		nbfs
PROGSOURCE=	rmheader.S rmvers.c nbfs.c rmalloc.c
NEWVERSWHAT=	"RISC OS Boot"
MKMODULE=	yes

CPPFLAGS+=	-DLIBSA_SINGLE_DEVICE=rodisk_

rmvers.c: ${.CURDIR}/version
	${HOST_SH} ${.CURDIR}/../lib/newvers_rm.sh ${.CURDIR}/version "NetBSD Boot FS"
.PHONY: rmvers.c

CLEANFILES+=	rmvers.c

.include "../Makefile.buildboot"
