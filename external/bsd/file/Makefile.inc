#	$NetBSD: Makefile.inc,v 1.1 2009/05/08 17:28:01 christos Exp $

DIST=${.CURDIR}/../dist

USE_FORT?= yes	# data-driven bugs?

TOOL_MKMAGIC?=	${.OBJDIR}/file

MFILESDIR?=	/usr/share/misc
MFILES?=	magic.mgc
MAGIC?=		${MFILESDIR}/magic

CPPFLAGS+=	-DMAGIC='"${MAGIC}"' -DHAVE_CONFIG_H -DQUICK -DBUILTIN_ELF \
		-DELFCORE
CPPFLAGS+=	-I${.CURDIR}/../include -I${DIST}/src

.PATH:		${DIST}/src ${DIST}/doc
