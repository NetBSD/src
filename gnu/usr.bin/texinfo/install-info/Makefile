#	$NetBSD: Makefile,v 1.7 2002/12/23 06:52:50 lukem Exp $

PROG=		install-info
SRCS=		install-info.c

.include <bsd.prog.mk>

.PATH: ${DIST}/texinfo/util ${DIST}/texinfo/doc

infodir-meta:
.if defined(UNPRIVED) && (${MKINFO} != "no")
	echo "./usr/share/info/dir type=file mode=0644 uname=root gname=wheel" \
	    | ${METALOG.add}
.endif
