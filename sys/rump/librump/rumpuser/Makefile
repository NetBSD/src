#	$NetBSD: Makefile,v 1.5.2.1 2008/03/24 07:16:28 keiichi Exp $
#

.include <bsd.own.mk>

LIB=		rumpuser

SRCS=		rumpuser.c rumpuser_pth.c

CPPFLAGS+=	-DRUMP_WITHOUT_THREADS

RUMPKERNEL=	no

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
