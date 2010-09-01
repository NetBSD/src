# $NetBSD: Makefile,v 1.3 2010/09/01 09:04:16 jmcneill Exp $

PROG=	audiocfg
SRCS=	audiodev.c drvctl.c dtmf.c
SRCS+=	main.c
WARNS=	3

LDADD+=	-lprop -lm
DPADD+=	$(LIBPROP) $(LIBM)

.include <bsd.prog.mk>
