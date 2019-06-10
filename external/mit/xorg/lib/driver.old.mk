#	$NetBSD: driver.old.mk,v 1.1.2.2 2019/06/10 22:02:37 christos Exp $

# stuff both dri and gallium drivers need.

# util
.PATH:		${X11SRCDIR.Mesa}/src/util
SRCS.util=	\
	hash_table.c    \
        MESAralloc.c
.PATH:		${X11SRCDIR.Mesa}/../src/util
SRCS.util+=	\
	format_srgb.c
CPPFLAGS.format_srgb.c+=	-I${X11SRCDIR.Mesa}/src/util
CPPFLAGS.hash_table.c+=		-I${X11SRCDIR.Mesa}/src/util
CPPFLAGS.MESAralloc.c+=		-I${X11SRCDIR.Mesa}/src/util

BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/util/ralloc.c MESAralloc.c

SRCS+=	${SRCS.util}

# also need to pull in libdricommon.la libmegadriver_stub.la
.PATH: ${X11SRCDIR.Mesa}/src/mesa/drivers/dri/common
SRCS+=	utils.c dri_util.c xmlconfig.c
SRCS+=	megadriver_stub.c
