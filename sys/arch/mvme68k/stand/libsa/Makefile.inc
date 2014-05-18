#	$NetBSD: Makefile.inc,v 1.5.22.1 2014/05/18 17:45:18 rmind Exp $

S!= cd ${SA_EXTRADIR}/../../../..; pwd

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRC_sa=   dev_net.c

SRC_mvme= exec_mvme.c

SRC_here= bugdev.c \
	  chiptotime.c clock.c \
	  parse_args.c

SRCS+= ${SRC_sa} ${SRC_mvme} ${SRC_here}
