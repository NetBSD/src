#	$NetBSD: Makefile,v 1.33 2001/05/09 14:05:52 simonb Exp $

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
CPPFLAGS+=	-DMAGIC='"${MAGIC}"' -DHAVE_CONFIG_H -I${.CURDIR}
CPPFLAGS+=	-DQUICK -DBUILTIN_ELF -DELFCORE
MAN=		file.1 magic.5

CLEANFILES+=	mkmagic magic magic.mgc
realall:	file magic magic.mgc

MAGDIRF:sh=	echo ${.CURDIR}/magdir/[0-9a-z]*
MAGFILES=	${.CURDIR}/Header \
		${.CURDIR}/Localstuff \
		${.CURDIR}/magdir/netbsd \
		${MAGDIRF}

.if ${MKSHARE} != "no"
magic:		${MAGFILES}
	cat ${MAGFILES} > ${.TARGET}
magic.mgc:	mkmagic magic
	${.OBJDIR}/mkmagic magic
.else
magic:
.endif

mkmagic:	apprentice.c print.c
	${HOST_LINK.c} -o mkmagic -DHAVE_CONFIG_H -DQUICK -DCOMPILE_ONLY \
		-I${.CURDIR} ${.CURDIR}/apprentice.c ${.CURDIR}/print.c

.include <bsd.prog.mk>
