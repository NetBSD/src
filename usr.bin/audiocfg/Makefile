# $NetBSD: Makefile,v 1.4 2013/08/11 06:31:00 dholland Exp $

PROG=	audiocfg
SRCS=	audiodev.c drvctl.c dtmf.c
SRCS+=	main.c

LDADD+=	-lprop -lm
DPADD+=	$(LIBPROP) $(LIBM)

.include <bsd.prog.mk>
