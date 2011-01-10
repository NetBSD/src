#	$NetBSD: Makefile.inc,v 1.4.90.1 2011/01/10 00:37:35 jym Exp $

S!= cd ${SA_EXTRADIR}/../../../..; pwd

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRC_sa=   dev_net.c

SRC_mvme= exec_mvme.c

SRC_here= SRT0.S \
	  bugdev.c \
	  chiptotime.c clock.c \
	  parse_args.c

SRCS+= ${SRC_sa} ${SRC_mvme} ${SRC_here}
