#	$NetBSD: Makefile,v 1.3.2.3 2004/09/18 14:54:14 skrll Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-I$S/dist/pf -I$S -DINET6 -DINET
KMOD=		pf
SRCS=		if_pflog.c pf_real.c pf_ioctl.c pf_osfp.c pf_if.c \
		pf_norm.c pf_table.c pf_lkm.c

.PATH:		$S/dist/pf/net

BUILDSYMLINKS+=	$S/dist/pf/net/pf.c pf_real.c

.include <bsd.kmod.mk>
