#	$NetBSD: libloader.old.mk,v 1.1.2.2 2019/06/10 22:02:37 christos Exp $

# makefile fragment for mesa src/loader

# loader stuff.
.PATH:		${X11SRCDIR.Mesa}/src/loader
.PATH:		${X11SRCDIR.Mesa}/src/mesa/drivers/dri/common
SRCS.loader += \
	loader.c \
	pci_id_driver_map.c \
	xmlconfig.c

.for _f in ${SRCS.loader}
CPPFLAGS.${_f}= 	-I${X11SRCDIR.Mesa}/src/mesa \
			-I${X11SRCDIR.Mesa}/src
.endfor

SRCS+=	${SRCS.loader}
