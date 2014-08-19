# $NetBSD: Makefile,v 1.3.12.1 2014/08/20 00:04:56 tls Exp $

PROG=	audiocfg
SRCS=	audiodev.c drvctl.c dtmf.c
SRCS+=	main.c

LDADD+=	-lprop -lm
DPADD+=	$(LIBPROP) $(LIBM)

.include <bsd.prog.mk>
