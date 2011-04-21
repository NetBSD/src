#	$NetBSD: Makefile.inc,v 1.1.2.2 2011/04/21 01:41:31 rmind Exp $

# MD libsa files

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=		consio.c devopen.c parseutils.c sdcd.c fd.c fdsub.S chdsk.c
SRCS+=		putimage.S

#SRCS+=		dev_net.c
