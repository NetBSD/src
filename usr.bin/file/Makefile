#	$NetBSD: Makefile,v 1.15 1997/10/24 09:00:29 lukem Exp $

FILESDIR=	/usr/share/misc
FILES=		magic
MAGIC=		${FILESDIR}/${FILES}

PROG=		file
SRCS=		file.c apprentice.c fsmagic.c softmagic.c ascmagic.c is_tar.c \
		print.c compress.c readelf.c internat.c
CPPFLAGS+=	-DMAGIC='"$(MAGIC)"'
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
