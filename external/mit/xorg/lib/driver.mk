#	$NetBSD: driver.mk,v 1.3 2019/03/10 10:51:58 mrg Exp $

# stuff both dri and gallium drivers need.

# util
.PATH:		${X11SRCDIR.Mesa}/src/util

SRCS.util=	\
	hash_table.c \
	build_id.c \
	crc32.c \
	disk_cache.c \
	fast_idiv_by_const.c \
	half_float.c \
	format_srgb.c \
	mesa-sha1.c \
	os_time.c \
	ralloc.c \
	UTILdebug.c \
	rand_xor.c \
	rb_tree.c \
	register_allocate.c \
	rgtc.c \
	set.c \
	slab.c \
	string_buffer.c \
	strtod.c \
	u_atomic.c \
	u_cpu_detect.c \
	u_math.c \
	u_queue.c \
	u_process.c \
	u_vector.c \
	vma.c

CPPFLAGS.format_srgb.c+=	-I${X11SRCDIR.Mesa}/src/util
CPPFLAGS.hash_table.c+=		-I${X11SRCDIR.Mesa}/src/util
CPPFLAGS.MESAralloc.c+=		-I${X11SRCDIR.Mesa}/src/util
CPPFLAGS.UTILdebug.c+=		-I${X11SRCDIR.Mesa}/src/util \
				-I${X11SRCDIR.Mesa}/src/mesa \
				-I${X11SRCDIR.Mesa}/src \
				-I${X11SRCDIR.Mesa}/src/gallium/include

BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/util/debug.c UTILdebug.c

SRCS+=	${SRCS.util}

# also need to pull in libdricommon.la libmegadriver_stub.la
.PATH: ${X11SRCDIR.Mesa}/src/mesa/drivers/dri/common
SRCS+=	utils.c dri_util.c xmlconfig.c
SRCS+=	megadriver_stub.c
