#	$NetBSD: Makefile.inc,v 1.1.2.2 2002/06/20 16:02:24 gehenna Exp $

#	$OpenBSD: Makefile.inc,v 1.14 2000/12/29 19:04:41 mickey Exp $

SRCS+=	__main.c imax.c imin.c lmax.c lmin.c max.c min.c ulmax.c ulmin.c \
	random.c ashrdi3.c divdi3.c strncpy.c strncmp.c strcmp.c memchr.c \
	memcmp.c memset.c strlen.c strcpy.c \
	strcat.c skpc.c strncasecmp.c \
	bcmp.c scanc.c ffs.c
SRCS+=	__assert.c

SRCS+=	milli.S strlcpy.S
OBJS+=	bzero.o

SRCS+=	bcopy.S spcopy.S memcpy.S memmove.S
CLEANFILES+=	bzero.o

bzero.o: ${KERNDIR}/memset.c
	@echo "${COMPILE.c} -DBZERO ${KERNDIR}/memset.c -o ${.TARGET}"
	@${COMPILE.c} -DBZERO ${KERNDIR}/memset.c  -o ${.TARGET}.o
	@${LD} -x -r ${.TARGET}.o -o ${.TARGET}
	@rm -f ${.TARGET}.o
