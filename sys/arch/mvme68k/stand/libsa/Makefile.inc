#	$NetBSD: Makefile.inc,v 1.4.96.1 2011/03/05 20:51:19 rmind Exp $

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
