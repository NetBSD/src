#	$NetBSD: Makefile,v 1.9 2003/07/18 08:26:03 lukem Exp $

PROG=		install-info
SRCS=		install-info.c

.include <bsd.prog.mk>

.PATH: ${IDIST}/util ${IDIST}/doc

infodir-meta:
.if (${MKUNPRIVED} != "no" && ${MKINFO} != "no")
	echo "./usr/share/info/dir type=file mode=0644 uname=root gname=wheel" \
	    | ${METALOG.add}
.endif
