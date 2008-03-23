#	Makefile,v 1.2 2004/12/12 08:18:44 christos Exp
# $FreeBSD: src/lib/libpam/modules/pam_group/Makefile,v 1.1 2003/02/06 14:27:48 des Exp $

NOLINT=		# don't build a lint library
NOPROFILE=	# don't build a profile library
NOPICINSTALL=	# don't install _pic.a library

.include <bsd.own.mk>

LIB=		pam_group
SRCS=		pam_group.c
MAN=		pam_group.8

LIBDPLIBS=	util	${LIB_ROOT_DIR}/libutil \
		crypt	${LIB_ROOT_DIR}/libcrypt

.include "${.CURDIR}/../mod.mk"
