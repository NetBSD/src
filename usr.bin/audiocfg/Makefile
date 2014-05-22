# $NetBSD: Makefile,v 1.3.6.1 2014/05/22 11:42:42 yamt Exp $

PROG=	audiocfg
SRCS=	audiodev.c drvctl.c dtmf.c
SRCS+=	main.c

LDADD+=	-lprop -lm
DPADD+=	$(LIBPROP) $(LIBM)

.include <bsd.prog.mk>
