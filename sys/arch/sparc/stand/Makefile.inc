#	$NetBSD: Makefile.inc,v 1.2 1995/09/18 19:52:15 chuck Exp $

.if !defined(__stand_makefile_inc)
__stand_makefile_inc=1

S= ../../../${R}

RELOC_SUN4=	0x240000
RELOC_SUN4C=	0x340000
RELOC_SUN4M=	0x440000

# Note: a `RELOC' value of 0x340000 seems to work on most machines
RELOC?=		${RELOC_SUN4C}

INCLUDES+=	-I.. -I${S}/arch -I${S} -I${S}/lib/libsa
DEFS+=		-DSTANDALONE -DRELOC=${RELOC} -DSUN4 -DSUN4C -DSUN_BOOTPARAMS
CFLAGS+=	-O2 ${INCLUDES} ${DEFS}

srt0.o: srt0.S
	${CC} ${CFLAGS} -DLOCORE -c ${.IMPSRC}

.endif
