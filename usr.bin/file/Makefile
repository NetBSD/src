#	$Id: Makefile,v 1.7 1996/10/05 20:20:24 christos Exp $

MAGIC=		/etc/magic
MAGICOWN=	bin
MAGICGRP=	bin
MAGICMODE=	444

PROG=		file
SRCS=		file.c apprentice.c fsmagic.c softmagic.c ascmagic.c is_tar.c \
		print.c compress.c readelf.c
CFLAGS+=	-DMAGIC='"$(MAGIC)"'
MAN=		file.1 magic.5

CLEANFILES+=	magic
all:		file magic

MAGFILES=	$(.CURDIR)/magdir/Header\
		$(.CURDIR)/magdir/Localstuff\
		$(.CURDIR)/magdir/NetBSD\
		$(.CURDIR)/magdir/[a-z]*
magic:		$(MAGFILES)
	cat $(MAGFILES) > $(.TARGET)

afterinstall:
	install ${COPY} -o $(MAGICOWN) -g $(MAGICGRP) -m $(MAGICMODE) magic \
		$(DESTDIR)$(MAGIC)

.include <bsd.prog.mk>
