#	$NetBSD: libloader.mk,v 1.6 2020/03/29 21:06:03 maya Exp $

# makefile fragment for mesa src/loader

# loader stuff.
.PATH:		${X11SRCDIR.Mesa}/src/loader
.PATH:		${X11SRCDIR.Mesa}/src/util
SRCS.loader += \
	loader.c \
	pci_id_driver_map.c \
	xmlconfig.c

.for _f in ${SRCS.loader}
CPPFLAGS.${_f}= 	-I${X11SRCDIR.Mesa}/src/util \
			-I${X11SRCDIR.Mesa}/../src/util \
			-I${X11SRCDIR.Mesa}/src/mesa \
			-I${X11SRCDIR.Mesa}/src \
			-DDEFAULT_DRIVER_DIR=\"${X11USRLIBDIR}/modules/dri\" \
			-DUSE_DRICONF \
			-DHAVE_LIBDRM
.endfor

SRCS+=	${SRCS.loader}
