#	$NetBSD: Makefile.inc,v 1.1.6.2 2011/06/06 09:07:03 jruoho Exp $

# MD libsa files

.PATH.c: ${SA_EXTRADIR}
.PATH.S: ${SA_EXTRADIR}

SRCS+=		consio.c devopen.c parseutils.c sdcd.c fd.c fdsub.S chdsk.c
SRCS+=		putimage.S

#SRCS+=		dev_net.c
