#	$NetBSD: Makefile.inc,v 1.2 2012/10/12 20:15:52 tsutsui Exp $

# MD libsa files

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=		consio.c devopen.c parseutils.c sdcd.c fd.c fdsub.S chdsk.c
SRCS+=		putimage.S

# network support
SRCS+=		clock.c
SRCS+=		dev_net.c
