#	$NetBSD: xorg-pkg-ver.mk,v 1.1 2015/07/19 19:49:17 mrg Exp $

# when including this make sure PROG is set so that $X11SRCDIR.$PROG
# is a valid setting.

.if exists(${X11SRCDIR.${PROG}}/configure)
XORG_PKG_PACKAGE_VERSION!= \
	awk -F= '/^PACKAGE_VERSION=/ {				\
	     match($$2, "([0-9]+\\.)+[0-9]+");			\
	     version = substr($$2, RSTART, RLENGTH);		\
	     print version;					\
	     exit 0;						\
	}' ${X11SRCDIR.${PROG}}/configure
.if !empty(XORG_PKG_PACKAGE_VERSION)
CPPFLAGS+=	-DPACKAGE_VERSION=\"${XORG_PKG_PACKAGE_VERSION:Q}\"
.endif

XORG_PKG_PACKAGE_STRING!= \
	awk -F= '/^PACKAGE_STRING=/ {				\
	     match($$2, "[a-zA-Z-_]+[ 	]+([0-9]+\\.)+[0-9]+");	\
	     string = substr($$2, RSTART, RLENGTH);		\
	     print string;					\
	     exit 0;						\
	}' ${X11SRCDIR.${PROG}}/configure
.if !empty(XORG_PKG_PACKAGE_STRING)
CPPFLAGS+=	-DPACKAGE_STRING=\"${XORG_PKG_PACKAGE_STRING:Q}\"
.endif

.endif
