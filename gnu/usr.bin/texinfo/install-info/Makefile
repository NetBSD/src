#	$NetBSD: Makefile,v 1.5.2.1 2002/06/11 16:14:05 lukem Exp $

PROG=		install-info
SRCS=		install-info.c

.include <bsd.prog.mk>

.PATH: ${DIST}/texinfo/util ${DIST}/texinfo/doc

infodir-meta:
.if defined(UNPRIVED) && (${MKINFO} != "no")
	echo "${DESTDIR}/usr/share/info/dir type=file mode=0644 uname=root gname=wheel" | \
		sed -e 's|^/|./|g' -e 's|//|/|g' | ${CAT} -l >>${METALOG}
.endif
