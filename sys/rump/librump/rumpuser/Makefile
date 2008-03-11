#	$NetBSD: Makefile,v 1.6 2008/03/11 10:50:16 pooka Exp $
#

.include <bsd.own.mk>

LIB=		rumpuser

SRCS=		rumpuser.c rumpuser_pth.c

CPPFLAGS+=	-DRUMP_WITHOUT_THREADS

RUMPKERNEL=	no

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
