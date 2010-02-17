#	$NetBSD: Makefile,v 1.2 2010/02/17 17:36:32 christos Exp $

.PATH:  ${.CURDIR}/../../dist/lib/libnvpair
.PATH:  ${.CURDIR}/../../dist/common/nvpair

LIB=	nvpair

SRCS=	lib_nvpair.c nvpair_alloc_system.c nvpair_alloc_fixed.c nvpair.c

lib_nvpair.c: libnvpair.c
	@rm -f ${.TARGET}
	@ln -s ${.ALLSRC} ${.TARGET}

CLEANFILES+=lib_nvpair.c

.include "../../Makefile.zfs"
.include <bsd.lib.mk>
