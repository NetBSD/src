#	$NetBSD: Makefile,v 1.13 1997/05/17 21:15:35 mjacob Exp $

FILESDIR=	/etc
FILES=		magic
MAGIC=		${FILESDIR}/${FILES}

PROG=		file
SRCS=		file.c apprentice.c fsmagic.c softmagic.c ascmagic.c is_tar.c \
		print.c compress.c readelf.c internat.c
CFLAGS+=	-DMAGIC='"$(MAGIC)"'
MAN=		file.1 magic.5

CLEANFILES+=	magic
all:		file magic

MAGDIRF:sh=	echo $(.CURDIR)/magdir/[0-9a-z]*
MAGFILES=	$(.CURDIR)/magdir/Header \
		$(.CURDIR)/magdir/Localstuff \
		$(.CURDIR)/magdir/netbsd \
		${MAGDIRF}

magic:		$(MAGFILES)
	cat $(MAGFILES) > $(.TARGET)

.include <bsd.prog.mk>
