#	$NetBSD: Makefile,v 1.5 1997/09/22 22:10:49 christos Exp $

PROG = am-utils
SRCS = am-utils.texinfo

${PROG}: am-utils.dvi

am-utils.dvi: ${SRCS}
	-tex am-utils.texinfo; tex am-utils.texinfo

clean:
	-rm -f am-utils.aux am-utils.cp am-utils.cps am-utils.dvi am-utils.fn \
	 am-utils.fns am-utils.ky am-utils.kys am-utils.log am-utils.pg \
	 am-utils.pgs am-utils.toc am-utils.tp am-utils.tps am-utils.vr \
	 am-utils.vrs
