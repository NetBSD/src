#	$NetBSD: Makefile.inc,v 1.1 2011/01/02 09:40:52 tsutsui Exp $

S!= cd ${SA_EXTRADIR}/../../../..; pwd

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRC_sa= dev_net.c

SRC_here= srt0.S bugsyscalls.S exec_mvme.c parse_args.c getchar.c putchar.c
SRC_here+= if_bug.c clock.c

SRCS+= ${SRC_sa} ${SRC_here}
