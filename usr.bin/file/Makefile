#	$NetBSD: Makefile,v 1.29 2001/03/27 08:45:02 pooka Exp $

.include <bsd.own.mk>

WFORMAT=	1
MFILESDIR=	/usr/share/misc
MFILES=		magic magic.mgc magic.mime
MAGIC=		${MFILESDIR}/magic
.if ${MKSHARE} != "no"
FILESDIR=	${MFILESDIR}
FILES=		${MFILES}
.endif

PROG=		file
SRCS=		file.c apprentice.c fsmagic.c softmagic.c ascmagic.c is_tar.c \
		print.c compress.c readelf.c 
CPPFLAGS+=	-DMAGIC='"$(MAGIC)"' -DHAVE_CONFIG_H -I${.CURDIR}
CPPFLAGS+=	-DQUICK -DBUILTIN_ELF -DELFCORE
MAN=		file.1 magic.5

CLEANFILES+=	magic magic.mgc
realall:		file magic

MAGDIRF:sh=	echo $(.CURDIR)/magdir/[0-9a-z]*
MAGFILES=	$(.CURDIR)/Header \
		$(.CURDIR)/Localstuff \
		$(.CURDIR)/magdir/netbsd \
		${MAGDIRF}

.if ${MKSHARE} != "no"
magic:		$(MAGFILES)
	cat $(MAGFILES) > $(.TARGET)
	$(.OBJDIR)/$(PROG) -C -m $(.TARGET)
.else
magic:
.endif

.include <bsd.prog.mk>
