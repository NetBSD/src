#	$NetBSD: Makefile,v 1.21 1999/02/16 12:32:52 he Exp $

.include <bsd.own.mk>

MFILESDIR=	/usr/share/misc
MFILES=		magic
MAGIC=		${MFILESDIR}/${MFILES}
.if ${MKSHARE} != "no"
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

.if ${MKSHARE} != "no"
magic:		$(MAGFILES)
	cat $(MAGFILES) > $(.TARGET)
.else
magic:
.endif

.include <bsd.prog.mk>
