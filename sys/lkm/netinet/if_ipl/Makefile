#	$NetBSD: Makefile,v 1.17 2004/10/01 15:20:45 christos Exp $

.include "../Makefile.inc"

CPPFLAGS+=	-DIPFILTER_LOG -I$S/dist/ipf -DUSE_INET6 -DIPFILTER_LKM
KMOD=		if_ipl
SRCS=		ip_fil_netbsd.c fil.c mln_ipl.c ip_nat.c ip_frag.c ip_state.c \
		ip_proxy.c ip_auth.c ip_log.c

.PATH:		${NETBSDSRCDIR}/sys/dist/ipf/netinet

.include <bsd.kmod.mk>
