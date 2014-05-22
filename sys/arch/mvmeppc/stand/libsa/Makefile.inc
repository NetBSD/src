#	$NetBSD: Makefile.inc,v 1.1.10.1 2014/05/22 11:40:00 yamt Exp $

S!= cd ${SA_EXTRADIR}/../../../..; pwd

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRC_sa= dev_net.c

SRC_here= bugsyscalls.S exec_mvme.c parse_args.c getchar.c putchar.c
SRC_here+= if_bug.c clock.c

SRCS+= ${SRC_sa} ${SRC_here}
