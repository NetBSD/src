#	$NetBSD: Makefile.inc,v 1.1.14.1 2012/11/18 19:05:20 riz Exp $

# MD libsa files

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=		consio.c devopen.c parseutils.c sdcd.c fd.c fdsub.S chdsk.c
SRCS+=		putimage.S

# network support
SRCS+=		clock.c
SRCS+=		dev_net.c
