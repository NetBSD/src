# $NetBSD: splash.mk,v 1.3.2.2 2015/06/06 14:40:13 skrll Exp $

# Makefile for embedding splash image into kernel.
.include <bsd.endian.mk>

MI_OBJS+=	splash_image.o
CFLAGS+=	-DSPLASHSCREEN_IMAGE

.if (${OBJECT_FMTS:Melf64})
BFD_ELFTARGET=elf64
.else
BFD_ELFTARGET=elf32
.endif

BFD_ENDIANNESS=${TARGET_ENDIANNESS:S/1234/little/C/4321/big/}
BFD_CPU=${MACHINE_CPU:S/_/-/}

.if (${BFD_CPU:Maarch64} || ${BFD_CPU:Marm} || ${BFD_CPU:Mmips} || ${BFD_CPU:Mscore})
BFD_TARGET=${BFD_ELFTARGET}-${BFD_ENDIANNESS}${BFD_CPU}
.else
BFD_TARGET=${BFD_ELFTARGET}-${BFD_CPU}
.endif

splash_image.o:	${SPLASHSCREEN_IMAGE}
	cp ${SPLASHSCREEN_IMAGE} splash.image
	${OBJCOPY} -I binary -B ${MACHINE_CPU:C/x86_64/i386/} \
		-O ${BFD_TARGET} splash.image splash_image.o
	rm splash.image
