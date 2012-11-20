#	$NetBSD: Makefile.inc,v 1.1.18.1 2012/11/20 03:01:50 tls Exp $

# MD libsa files

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=		consio.c devopen.c parseutils.c sdcd.c fd.c fdsub.S chdsk.c
SRCS+=		putimage.S

# network support
SRCS+=		clock.c
SRCS+=		dev_net.c
