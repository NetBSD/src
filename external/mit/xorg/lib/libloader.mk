#	$NetBSD: libloader.mk,v 1.4 2019/09/24 19:29:41 maya Exp $

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
			-DGL_LIB_NAME=\"libGL.so.3\" \
			-DDEFAULT_DRIVER_DIR=\"${X11USRLIBDIR}/modules/dri\" \
			-DUSE_DRICONF
.endfor

SRCS+=	${SRCS.loader}
