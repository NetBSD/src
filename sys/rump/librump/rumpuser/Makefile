#	$NetBSD: Makefile,v 1.3.6.5 2008/03/17 09:15:47 yamt Exp $
#

.include <bsd.own.mk>

LIB=		rumpuser

SRCS=		rumpuser.c rumpuser_pth.c

CPPFLAGS+=	-DRUMP_WITHOUT_THREADS

RUMPKERNEL=	no

.include "${NETBSDSRCDIR}/sys/rump/Makefile.rump"
.include <bsd.lib.mk>
