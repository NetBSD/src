#	$NetBSD: Makefile,v 1.12 1997/04/03 07:17:24 mikel Exp $

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

MAGFILES=	$(.CURDIR)/magdir/Header \
		$(.CURDIR)/magdir/Localstuff \
		$(.CURDIR)/magdir/netbsd \
		$(.CURDIR)/magdir/[0-9a-z]*

magic:		$(MAGFILES)
	cat $(MAGFILES) > $(.TARGET)

.include <bsd.prog.mk>
