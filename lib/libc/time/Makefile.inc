#	$NetBSD: Makefile.inc,v 1.1 1995/03/09 23:26:43 jtc Exp $

# gen sources
.PATH: ${.CURDIR}/gen

SRCS+=	asctime.c difftime.c localtime.c
MAN+=	time2posix.3 tzfile.5
