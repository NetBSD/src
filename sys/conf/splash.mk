# $NetBSD: splash.mk,v 1.1 2015/08/30 07:33:53 uebayasi Exp $

# Option for embedding a splashscreen image.
.if defined(SPLASHSCREEN_IMAGE) 
.include "${S}/dev/splash/splash.mk"
init_main.o: splash_image.o
.endif
