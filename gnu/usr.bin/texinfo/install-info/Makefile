#	$NetBSD: Makefile,v 1.8 2003/01/17 15:32:24 wiz Exp $

PROG=		install-info
SRCS=		install-info.c

.include <bsd.prog.mk>

.PATH: ${IDIST}/util ${IDIST}/doc

infodir-meta:
.if defined(UNPRIVED) && (${MKINFO} != "no")
	echo "./usr/share/info/dir type=file mode=0644 uname=root gname=wheel" \
	    | ${METALOG.add}
.endif
