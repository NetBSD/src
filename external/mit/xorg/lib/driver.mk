#	$NetBSD: driver.mk,v 1.5 2023/07/16 22:20:54 rjs Exp $

# stuff both dri and gallium drivers need.

# util
.PATH:		${X11SRCDIR.Mesa}/src/util
.PATH:		${X11SRCDIR.Mesa}/../src/util
.PATH:		${X11SRCDIR.Mesa}/src/util/format
.PATH:		${X11SRCDIR.Mesa}/../src/util/format
.PATH:		${X11SRCDIR.Mesa}/src/util/perf
.PATH:		${X11SRCDIR.Mesa}/../src/util/perf

SRCS.util=	\
	anon_file.c \
	build_id.c \
	compress.c \
	crc32.c \
	dag.c \
	disk_cache.c \
	disk_cache_os.c \
	double.c \
	format_srgb.c \
	fossilize_db.c \
	hash_table.c \
	fast_idiv_by_const.c \
	half_float.c \
	memstream.c \
	mesa-sha1.c \
	os_file.c \
	os_memory_fd.c \
	os_time.c \
	ralloc.c \
	UTILdebug.c \
	UTILlog.c \
	rand_xor.c \
	rb_tree.c \
	register_allocate.c \
	rgtc.c \
	set.c \
	slab.c \
	softfloat.c \
	string_buffer.c \
	strtod.c \
	u_atomic.c \
	u_cpu_detect.c \
	u_debug.c \
	u_hash_table.c \
	u_idalloc.c \
	u_math.c \
	u_mm.c \
	u_qsort.cpp \
	u_queue.c \
	u_printf.cpp \
	u_process.c \
	u_vector.c \
	vma.c

CPPFLAGS.hash_table.c+=		-I${X11SRCDIR.Mesa}/src/util
CPPFLAGS.UTILdebug.c+=		-I${X11SRCDIR.Mesa}/src/util \
				-I${X11SRCDIR.Mesa}/src/mesa \
				-I${X11SRCDIR.Mesa}/src \
				-I${X11SRCDIR.Mesa}/src/gallium/include
CPPFLAGS.format_srgb.c+=	-I${X11SRCDIR.Mesa}/src/util
CPPFLAGS.u_hash_table.c+=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary

BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/util/debug.c UTILdebug.c
BUILDSYMLINKS+=	${X11SRCDIR.Mesa}/src/util/log.c UTILlog.c

SRCS.format= \
	u_format.c \
	u_format_bptc.c \
	u_format_etc.c \
	u_format_fxt1.c \
	u_format_latc.c \
	u_format_other.c \
	u_format_rgtc.c \
	u_format_s3tc.c \
	u_format_unpack_neon.c \
	u_format_yuv.c \
	u_format_zs.c \
	u_format_table.c

.for _f in ${SRCS.format}
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/src/util/format
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/../src/util/format
CPPFLAGS.${_f} +=	-I${X11SRCDIR.Mesa}/../src
.endfor

SRCS.perf= \
	u_trace.c

CPPFLAGS.u_trace.c+=	-I${X11SRCDIR.Mesa}/src/util/perf
CPPFLAGS.u_trace.c+=	-I${X11SRCDIR.Mesa}/src/gallium/auxiliary

SRCS+=	${SRCS.util} ${SRCS.format} ${SRCS.perf}

# also need to pull in libdricommon.la libmegadriver_stub.la
.PATH: ${X11SRCDIR.Mesa}/src/mesa/drivers/dri/common
SRCS+=	utils.c dri_util.c xmlconfig.c
SRCS+=	megadriver_stub.c

CPPFLAGS.dri_util.c+=		-I${X11SRCDIR.Mesa}/../src/util

