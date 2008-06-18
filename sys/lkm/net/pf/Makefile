#	$NetBSD: Makefile,v 1.6.78.1 2008/06/18 16:33:49 simonb Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-I$S/dist/pf -I$S -DINET6 -DINET
KMOD=		pf
SRCS=		if_compat.c if_pflog.c \
		pf_real.c pf_if.c pf_ioctl.c pf_mtag.c pf_norm.c pf_osfp.c \
		pf_ruleset.c pf_table.c pf_lkm.c \
		tcp_rndiss.c

.PATH:		$S/dist/pf/net $S/dist/pf/netinet

BUILDSYMLINKS+=	$S/dist/pf/net/pf.c pf_real.c

.include <bsd.kmod.mk>
