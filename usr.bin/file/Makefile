#	$NetBSD: Makefile,v 1.40.2.1 2004/06/16 01:34:10 jmc Exp $

.include <bsd.own.mk>

WFORMAT=	1
MFILESDIR=	/usr/share/misc
MFILES=		magic magic.mgc magic.mime magic.mime.mgc
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
LDADD+=		-lz
MAN=		file.1 magic.5

CLEANFILES+=	mkmagic magic magic.mgc magic.mime magic.mime.mgc
.if ${MKSHARE} != "no"
realall:	file magic magic.mgc magic.mime.mgc
.endif

# Avoid possible collation problems w. LANG set to something else
MAGDIRF:sh=	LANG=C sh -c 'echo ${.CURDIR}/magdir/[0-9a-z]*'
MAGFILES=	${.CURDIR}/Header \
		${.CURDIR}/Localstuff \
		${.CURDIR}/magdir/netbsd \
		${MAGDIRF}

.if ${MKSHARE} != "no"
magic:		${MAGFILES}
	cat ${MAGFILES} > ${.TARGET}

magic.mgc: mkmagic magic
	./mkmagic magic

magic.mime: magic.mime.in
	cp ${.CURDIR}/magic.mime.in ./magic.mime

magic.mime.mgc:	mkmagic magic.mime
	./mkmagic ./magic.mime
.else
magic:
.endif

mkmagic: apprentice.c print.c
	${HOST_LINK.c} -o mkmagic -DHAVE_CONFIG_H -DQUICK -DCOMPILE_ONLY \
		-I${.CURDIR} ${.CURDIR}/apprentice.c ${.CURDIR}/print.c

.include <bsd.prog.mk>
