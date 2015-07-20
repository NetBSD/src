#	$NetBSD: xorg-pkg-ver.mk,v 1.2 2015/07/20 02:07:36 mrg Exp $

# when including this make sure PROG is set so that $X11SRCDIR.$PROG
# is a valid setting.  set XORG_PKG_VER_PROG if PROG is wrong.

XORG_PKG_VER_PROG?=	${PROG}

.if exists(${X11SRCDIR.${XORG_PKG_VER_PROG}}/configure)
XORG_PKG_PACKAGE_VERSION!= \
	awk -F= '/^PACKAGE_VERSION=/ {				\
	     match($$2, "([0-9]+\\.)+[0-9]+");			\
	     version = substr($$2, RSTART, RLENGTH);		\
	     print version;					\
	     exit 0;						\
	}' ${X11SRCDIR.${XORG_PKG_VER_PROG}}/configure
.if !empty(XORG_PKG_PACKAGE_VERSION)
CPPFLAGS+=	-DPACKAGE_VERSION=\"${XORG_PKG_PACKAGE_VERSION:Q}\"
.endif

XORG_PKG_PACKAGE_STRING!= \
	awk -F= '/^PACKAGE_STRING=/ {				\
	     match($$2, "[a-zA-Z-_]+[ 	]+([0-9]+\\.)+[0-9]+");	\
	     string = substr($$2, RSTART, RLENGTH);		\
	     print string;					\
	     exit 0;						\
	}' ${X11SRCDIR.${XORG_PKG_VER_PROG}}/configure
.if !empty(XORG_PKG_PACKAGE_STRING)
CPPFLAGS+=	-DPACKAGE_STRING=\"${XORG_PKG_PACKAGE_STRING:Q}\"
.endif

.endif
