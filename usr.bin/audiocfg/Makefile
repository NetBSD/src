PROG=	audiocfg
SRCS=	audiodev.c drvctl.c
SRCS+=	main.c
NOMAN=	# defined
WARNS=	3

LDADD+=	-lprop
DPADD+=	$(LIBPROP)

.include <bsd.prog.mk>
