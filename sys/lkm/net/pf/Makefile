#	$NetBSD: Makefile,v 1.2 2004/06/22 18:04:05 christos Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-I$S/dist/pf -I$S/dist/pf/net -I$S -DINET6 -DINET
KMOD=		pf_lkm
SRCS=		if_pflog.c pf.c pf_ioctl.c pf_osfp.c pf_if.c \
		pf_norm.c pf_table.c mln_pf.c

.PATH:		$S/dist/pf/net

.include <bsd.kmod.mk>
