#	$NetBSD: Makefile,v 1.15.16.3 2004/09/21 13:36:24 skrll Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DIPFILTER_LOG -I$S/netinet -DUSE_INET6 -DIPFILTER_LKM
KMOD=		if_ipl
SRCS=		ip_fil_netbsd.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c \
		ip_proxy.c ip_auth.c ip_log.c

.PATH:		${NETBSDSRCDIR}/sys/netinet

.include <bsd.kmod.mk>
