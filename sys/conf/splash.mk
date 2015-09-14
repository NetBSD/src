# $NetBSD: splash.mk,v 1.3 2015/09/14 01:40:03 uebayasi Exp $

# Option for embedding a splashscreen image.
.if defined(SPLASHSCREEN_IMAGE) 
.include "${S}/dev/splash/splash.mk"
.endif
