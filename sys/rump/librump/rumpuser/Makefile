#	$NetBSD: Makefile,v 1.5.6.1 2008/04/03 12:43:11 mjf Exp $
#

.include <bsd.own.mk>

LIB=		rumpuser

SRCS=		rumpuser.c rumpuser_pth.c

CPPFLAGS+=	-DRUMP_WITHOUT_THREADS

RUMPKERNEL=	no

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
