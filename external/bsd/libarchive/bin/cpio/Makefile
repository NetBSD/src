# $NetBSD: Makefile,v 1.3 2010/04/23 19:41:03 joerg Exp $

PROG=	cpio
SRCS=	cmdline.c cpio.c

.include <bsd.init.mk>

.PATH:		${LIBARCHIVEDIR}/cpio

CLEANFILES+=	cpio.1

cpio.1: ${LIBARCHIVEDIR}/cpio/bsdcpio.1
	${TOOL_CAT} ${LIBARCHIVEDIR}/cpio/bsdcpio.1 > $@

SYMLINKS+=${BINDIR}/cpio /usr/bin/cpio

.include <bsd.prog.mk>
