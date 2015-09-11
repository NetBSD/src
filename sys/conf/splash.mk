# $NetBSD: splash.mk,v 1.2 2015/09/11 15:56:56 nat Exp $

# Option for embedding a splashscreen image.
.if defined(SPLASHSCREEN_IMAGE) 
.include "${S}/dev/splash/splash.mk"
OBJS+= splash_image.o
.endif
