.include <bsd.own.mk>

PROG=	bc
BINDIR= /usr/bin

SRCDIR= ${.CURDIR}/../dist
.PATH:  ${SRCDIR}

SRCS=	 	global.c load.c util.c execute.c main.c number.c storage.c \
		bc.y scan.l warranty.c

CPPFLAGS+=	-I. -I${SRCDIR} -D_POSIX_SOURCE
CFLAGS+=	-funsigned-char
YFLAGS+=	
LFLAGS+=	-I8
YHEADER=1
WARNS=5

LDADD+= -ll -ledit -ltermcap
DPADD+=	${LIBL} ${LIBEDIT} ${LIBTERMCAP}

.include <bsd.prog.mk>
