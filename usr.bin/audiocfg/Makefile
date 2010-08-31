PROG=	audiocfg
SRCS=	audiodev.c drvctl.c
SRCS+=	main.c
WARNS=	3

LDADD+=	-lprop
DPADD+=	$(LIBPROP)

.include <bsd.prog.mk>
