#	$NetBSD: Makefile,v 1.15.16.1 2004/08/03 10:54:03 skrll Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DIPFILTER_LOG -I$S/netinet -DUSE_INET6 -DIPFILTER_LKM
KMOD=		if_ipl
SRCS=		ip_fil_netbsd.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c \
		ip_proxy.c ip_auth.c ip_log.c

.PATH:		${NETBSDSRCDIR}/sys/netinet

.include <bsd.kmod.mk>
