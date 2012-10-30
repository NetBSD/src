#	$NetBSD: Makefile.inc,v 1.1.8.1 2012/10/30 17:20:31 yamt Exp $

# MD libsa files

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=		consio.c devopen.c parseutils.c sdcd.c fd.c fdsub.S chdsk.c
SRCS+=		putimage.S

# network support
SRCS+=		clock.c
SRCS+=		dev_net.c
