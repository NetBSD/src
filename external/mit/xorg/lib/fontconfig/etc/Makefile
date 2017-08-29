#	$NetBSD: Makefile,v 1.4 2017/08/29 20:53:56 mrg Exp $

# When updating fontconfig, be sure to update the fonts.conf.  This
# requires running ./configure like this:
#
#   env PKG_CONFIG_PATH=/usr/X11R7/share/pkgconfig \
#   ./configure \
#      --localstatedir=/var \
#      --with-default-fonts=/usr/X11R7/lib/X11/fonts \
#      --with-add-fonts=/usr/pkg/lib/X11/fonts,/usr/pkg/share/fonts \
#      --with-configdir=conf.d
#
# and then running gmake fonts.conf and copying the new file here.
# (gmake is required here as the FC_CONFIGDIR pattern uses $(subst).)
#

SUBDIR=	conf.d conf.avail

.include <bsd.own.mk>

.PATH:	${X11SRCDIR.fontconfig}
CONFIGFILES= \
	fonts.conf \
	fonts.dtd

FILESDIR= \
	/etc/fonts

.include <bsd.x11.mk>
.include <bsd.prog.mk>
.include <bsd.subdir.mk>
