#	$NetBSD: Makefile.inc,v 1.2 2002/04/18 14:54:38 tsutsui Exp $

BINDIR= /usr/mdec

# if there is a 'version' file, add rule for vers.c and add it to SRCS
# and CLEANFILES
.if exists(version)
.PHONY: vers.c
vers.c: version
	sh ${S}/conf/newvers_stand.sh ${.CURDIR}/version ${MACHINE}
SRCS+=  vers.c
CLEANFILES+= vers.c
.endif
