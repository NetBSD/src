#	$NetBSD: Makefile.inc,v 1.2 1995/03/09 23:35:13 jtc Exp $

.PATH: ${.CURDIR}/time

SRCS+=	asctime.c difftime.c localtime.c
MAN+=	time2posix.3 tzfile.5
