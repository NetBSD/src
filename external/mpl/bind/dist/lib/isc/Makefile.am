include $(top_srcdir)/Makefile.top

lib_LTLIBRARIES = libisc.la

libisc_ladir = $(includedir)/isc
libisc_la_HEADERS =			\
	include/isc/aes.h		\
	include/isc/align.h		\
	include/isc/app.h		\
	include/isc/assertions.h	\
	include/isc/astack.h		\
	include/isc/atomic.h		\
	include/isc/attributes.h	\
	include/isc/backtrace.h		\
	include/isc/barrier.h		\
	include/isc/base32.h		\
	include/isc/base64.h		\
	include/isc/buffer.h		\
	include/isc/cmocka.h		\
	include/isc/commandline.h	\
	include/isc/condition.h		\
	include/isc/counter.h		\
	include/isc/crc64.h		\
	include/isc/deprecated.h	\
	include/isc/dir.h		\
	include/isc/endian.h		\
	include/isc/errno.h		\
	include/isc/error.h		\
	include/isc/event.h		\
	include/isc/eventclass.h	\
	include/isc/file.h		\
	include/isc/formatcheck.h	\
	include/isc/fuzz.h		\
	include/isc/glob.h		\
	include/isc/hash.h		\
	include/isc/heap.h		\
	include/isc/hex.h		\
	include/isc/hmac.h		\
	include/isc/ht.h		\
	include/isc/httpd.h		\
	include/isc/interfaceiter.h	\
	include/isc/iterated_hash.h	\
	include/isc/lang.h		\
	include/isc/lex.h		\
	include/isc/list.h		\
	include/isc/log.h		\
	include/isc/magic.h		\
	include/isc/managers.h		\
	include/isc/md.h		\
	include/isc/mem.h		\
	include/isc/meminfo.h		\
	include/isc/mutex.h		\
	include/isc/mutexblock.h	\
	include/isc/net.h		\
	include/isc/netaddr.h		\
	include/isc/netdb.h		\
	include/isc/netmgr.h		\
	include/isc/netscope.h		\
	include/isc/nonce.h		\
	include/isc/offset.h		\
	include/isc/once.h		\
	include/isc/os.h		\
	include/isc/parseint.h		\
	include/isc/pool.h		\
	include/isc/portset.h		\
	include/isc/print.h		\
	include/isc/quota.h		\
	include/isc/radix.h		\
	include/isc/random.h		\
	include/isc/ratelimiter.h	\
	include/isc/refcount.h		\
	include/isc/regex.h		\
	include/isc/region.h		\
	include/isc/resource.h		\
	include/isc/result.h		\
	include/isc/rwlock.h		\
	include/isc/safe.h		\
	include/isc/serial.h		\
	include/isc/siphash.h		\
	include/isc/sockaddr.h		\
	include/isc/stat.h		\
	include/isc/stats.h		\
	include/isc/stdatomic.h		\
	include/isc/stdio.h		\
	include/isc/stdtime.h		\
	include/isc/strerr.h		\
	include/isc/string.h		\
	include/isc/symtab.h		\
	include/isc/syslog.h		\
	include/isc/task.h		\
	include/isc/taskpool.h		\
	include/isc/thread.h		\
	include/isc/time.h		\
	include/isc/timer.h		\
	include/isc/tls.h		\
	include/isc/tm.h		\
	include/isc/types.h		\
	include/isc/url.h		\
	include/isc/utf8.h		\
	include/isc/util.h

libisc_la_SOURCES =		\
	$(libisc_la_HEADERS)	\
	netmgr/netmgr-int.h	\
	netmgr/netmgr.c		\
	netmgr/tcp.c		\
	netmgr/tcpdns.c		\
	netmgr/timer.c		\
	netmgr/tlsdns.c		\
	netmgr/udp.c		\
	netmgr/uv-compat.c	\
	netmgr/uv-compat.h	\
	netmgr/uverr2result.c	\
	aes.c			\
	app.c			\
	assertions.c		\
	astack.c		\
	backtrace.c		\
	base32.c		\
	base64.c		\
	buffer.c		\
	commandline.c		\
	condition.c		\
	counter.c		\
	crc64.c			\
	dir.c			\
	entropy.c		\
	entropy_private.h	\
	errno.c			\
	errno2result.c		\
	errno2result.h		\
	error.c			\
	event.c			\
	file.c			\
	glob.c			\
	hash.c			\
	heap.c			\
	hex.c			\
	hmac.c			\
	ht.c			\
	httpd.c			\
	interfaceiter.c		\
	iterated_hash.c		\
	jemalloc_shim.h		\
	lex.c			\
	lib.c			\
	log.c			\
	managers.c		\
	md.c			\
	mem.c			\
	mem_p.h			\
	meminfo.c		\
	mutex.c			\
	mutexblock.c		\
	net.c			\
	netaddr.c		\
	netmgr_p.h		\
	netscope.c		\
	nonce.c			\
	openssl_shim.c		\
	openssl_shim.h		\
	os.c			\
	os_p.h			\
	parseint.c		\
	pool.c			\
	picohttpparser.c	\
	picohttpparser.h	\
	portset.c		\
	quota.c			\
	radix.c			\
	random.c		\
	ratelimiter.c		\
	regex.c			\
	region.c		\
	resource.c		\
	result.c		\
	rwlock.c		\
	safe.c			\
	serial.c		\
	siphash.c		\
	sockaddr.c		\
	stats.c			\
	stdio.c			\
	stdtime.c		\
	string.c		\
	symtab.c		\
	syslog.c		\
	task.c			\
	task_p.h		\
	taskpool.c		\
	thread.c		\
	time.c			\
	timer.c			\
	timer_p.h		\
	tls.c			\
	tls_p.h			\
	tm.c			\
	trampoline.c		\
	trampoline_p.h		\
	url.c			\
	utf8.c

libisc_la_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(LIBISC_CFLAGS)	\
	$(LIBUV_CFLAGS)		\
	$(OPENSSL_CFLAGS)	\
	$(ZLIB_CFLAGS)

libisc_la_LDFLAGS =		\
	$(AM_LDFLAGS)		\
	-release "$(PACKAGE_VERSION)"

libisc_la_LIBADD =		\
	$(LIBUV_LIBS)		\
	$(OPENSSL_LIBS)		\
	$(ZLIB_LIBS)

if HAVE_JEMALLOC
libisc_la_CPPFLAGS +=		\
	$(JEMALLOC_CFLAGS)

libisc_la_LIBADD +=		\
	$(JEMALLOC_LIBS)
endif HAVE_JEMALLOC

if HAVE_JSON_C
libisc_la_CPPFLAGS +=		\
	$(JSON_C_CFLAGS)

libisc_la_LIBADD +=		\
	$(JSON_C_LIBS)
endif HAVE_JSON_C

if HAVE_LIBNGHTTP2
libisc_la_SOURCES +=		\
	netmgr/http.c		\
	netmgr/tlsstream.c

libisc_la_CPPFLAGS +=		\
	$(LIBNGHTTP2_CFLAGS)

libisc_la_LIBADD +=		\
	$(LIBNGHTTP2_LIBS)
endif

if HAVE_LIBXML2
libisc_la_CPPFLAGS +=		\
	$(LIBXML2_CFLAGS)

libisc_la_LIBADD +=		\
	$(LIBXML2_LIBS)
endif HAVE_LIBXML2
