#	$NetBSD: Makefile,v 1.18 1998/09/27 17:22:11 lukem Exp $

MFILESDIR=	/usr/share/misc
MFILES=		magic
MAGIC=		${FILESDIR}/${FILES}
.if !defined(NOSHARE)
FILESDIR=	${MFILESDIR}
FILES=		${MFILES}
.endif

PROG=		file
SRCS=		file.c apprentice.c fsmagic.c softmagic.c ascmagic.c is_tar.c \
		print.c compress.c readelf.c internat.c
CPPFLAGS+=	-DMAGIC='"$(MAGIC)"' -DHAVE_SYS_WAIT_H -DHAVE_ST_RDEV
CPPFLAGS+=	-DBUILTIN_ELF -DELFCORE
MAN=		file.1 magic.5

CLEANFILES+=	magic
all:		file magic

MAGDIRF:sh=	echo $(.CURDIR)/magdir/[0-9a-z]*
MAGFILES=	$(.CURDIR)/Header \
		$(.CURDIR)/Localstuff \
		$(.CURDIR)/magdir/netbsd \
		${MAGDIRF}

.if !defined(NOSHARE)
magic:		$(MAGFILES)
	cat $(MAGFILES) > $(.TARGET)
.else
magic:
.endif

.include <bsd.prog.mk>
