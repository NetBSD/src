#	$NetBSD: Makefile.inc,v 1.3.2.2 2009/05/13 18:51:52 jym Exp $

DIST=${.CURDIR}/../dist
WARNS=4

BINDIR?= /usr/bin
USE_FORT?= yes	# data-driven bugs?

TOOL_MKMAGIC?=	${.OBJDIR}/file

MFILESDIR?=	/usr/share/misc
MFILES?=	magic.mgc
MAGIC?=		${MFILESDIR}/magic

CPPFLAGS+=	-DMAGIC='"${MAGIC}"' -DHAVE_CONFIG_H -DQUICK -DBUILTIN_ELF \
		-DELFCORE
CPPFLAGS+=	-I${.CURDIR}/../include -I${DIST}/src

.PATH:		${DIST}/src ${DIST}/doc
