#	$NetBSD: xorg-pkg-ver.mk,v 1.6 2015/08/08 22:26:59 aymeric Exp $

# when including this make sure PROG is set so that $X11SRCDIR.$PROG
# is a valid setting.  set XORG_PKG_VER_PROG if PROG is wrong.
# set XORG_PKG_VER_CONFIG_PATH if "configure" at the top-level is wrong.

XORG_PKG_VER_PROG?=	${PROG}

XORG_PKG_VER_CONFIG_PATH?=	configure
_XORG_PKG_CONFIGURE_PATH=	${X11SRCDIR.${XORG_PKG_VER_PROG}}/${XORG_PKG_VER_CONFIG_PATH}

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
CPPFLAGS+=	-DVERSION=\"${XORG_PKG_PACKAGE_VERSION:Q}\"
.endif

XORG_PKG_PACKAGE_STRING!= \
	awk -F= '/^PACKAGE_STRING=/ {				\
	     match($$2, "[a-zA-Z_-]+[ 	]+([0-9]+\\.)+[0-9]+");	\
	     string = substr($$2, RSTART, RLENGTH);		\
	     print string;					\
	     exit 0;						\
	}' ${X11SRCDIR.${XORG_PKG_VER_PROG}}/configure
.if !empty(XORG_PKG_PACKAGE_STRING)
CPPFLAGS+=	-DPACKAGE_STRING=\"${XORG_PKG_PACKAGE_STRING:Q}\"
.endif

XORG_PKG_PACKAGE_NAME!= \
	awk -F= '/^PACKAGE_NAME=/ {				\
	     match($$2, "'"'"'[a-zA-Z0-9_-]+'"'"'");		\
	     name = substr($$2, RSTART, RLENGTH);		\
	     print name;					\
	     exit 0;						\
	}' ${X11SRCDIR.${XORG_PKG_VER_PROG}}/configure
.if !empty(XORG_PKG_PACKAGE_NAME)
CPPFLAGS+=	-DPACKAGE_NAME=\"${XORG_PKG_PACKAGE_NAME:Q}\"
.endif

.endif
