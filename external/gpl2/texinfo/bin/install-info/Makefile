#	$NetBSD: Makefile,v 1.1 2016/01/14 00:34:52 christos Exp $

.include <bsd.init.mk>

PROG=		install-info

.include <bsd.prog.mk>

.PATH: ${IDIST}/util ${IDIST}/doc

infodir-meta:
.if (${MKUNPRIVED} != "no" && ${MKINFO} != "no")
	echo "./usr/share/info/dir type=file mode=0644 uname=${BINOWN} gname=${BINGRP}" \
	    | ${METALOG.add}
.endif
