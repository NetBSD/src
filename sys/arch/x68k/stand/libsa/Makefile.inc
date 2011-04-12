#	$NetBSD: Makefile.inc,v 1.1 2011/04/12 14:07:35 tsutsui Exp $

# MD libsa files

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=		consio.c devopen.c parseutils.c sdcd.c fd.c fdsub.S chdsk.c
SRCS+=		putimage.S

#SRCS+=		dev_net.c
