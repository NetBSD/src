include $(top_srcdir)/Makefile.top

lib_LTLIBRARIES = libisc.la

libisc_ladir = $(includedir)/isc
libisc_la_HEADERS =			\
	include/isc/ascii.h		\
	include/isc/assertions.h	\
	include/isc/async.h		\
	include/isc/atomic.h		\
	include/isc/attributes.h	\
	include/isc/backtrace.h		\
	include/isc/barrier.h		\
	include/isc/base32.h		\
	include/isc/base64.h		\
	include/isc/buffer.h		\
	include/isc/commandline.h	\
	include/isc/condition.h		\
	include/isc/counter.h		\
	include/isc/crc64.h		\
	include/isc/dir.h		\
	include/isc/dnsstream.h		\
	include/isc/endian.h		\
	include/isc/errno.h		\
	include/isc/error.h		\
	include/isc/file.h		\
	include/isc/fips.h		\
	include/isc/formatcheck.h	\
	include/isc/fuzz.h		\
	include/isc/getaddresses.h	\
	include/isc/hash.h		\
	include/isc/hashmap.h		\
	include/isc/heap.h		\
	include/isc/helper.h		\
	include/isc/hex.h		\
	include/isc/histo.h		\
	include/isc/hmac.h		\
	include/isc/ht.h		\
	include/isc/httpd.h		\
	include/isc/interfaceiter.h	\
	include/isc/iterated_hash.h	\
	include/isc/job.h		\
	include/isc/lang.h		\
	include/isc/lex.h		\
	include/isc/list.h		\
	include/isc/log.h		\
	include/isc/loop.h		\
	include/isc/magic.h		\
	include/isc/managers.h		\
	include/isc/md.h		\
	include/isc/mem.h		\
	include/isc/meminfo.h		\
	include/isc/mutex.h		\
	include/isc/mutexblock.h	\
	include/isc/net.h		\
	include/isc/netaddr.h		\
	include/isc/netmgr.h		\
	include/isc/netscope.h		\
	include/isc/nonce.h		\
	include/isc/once.h		\
	include/isc/os.h		\
	include/isc/overflow.h		\
	include/isc/parseint.h		\
	include/isc/pause.h		\
	include/isc/portset.h		\
	include/isc/proxy2.h		\
	include/isc/queue.h		\
	include/isc/quota.h		\
	include/isc/radix.h		\
	include/isc/random.h		\
	include/isc/ratelimiter.h	\
	include/isc/refcount.h		\
	include/isc/regex.h		\
	include/isc/region.h		\
	include/isc/result.h		\
	include/isc/rwlock.h		\
	include/isc/safe.h		\
	include/isc/serial.h		\
	include/isc/signal.h		\
	include/isc/siphash.h		\
	include/isc/sockaddr.h		\
	include/isc/spinlock.h		\
	include/isc/stats.h		\
	include/isc/stdio.h		\
	include/isc/stdtime.h		\
	include/isc/strerr.h		\
	include/isc/string.h		\
	include/isc/symtab.h		\
	include/isc/syslog.h		\
	include/isc/thread.h		\
	include/isc/tid.h		\
	include/isc/time.h		\
	include/isc/timer.h		\
	include/isc/tls.h		\
	include/isc/tm.h		\
	include/isc/types.h		\
	include/isc/urcu.h		\
	include/isc/url.h		\
	include/isc/utf8.h		\
	include/isc/util.h		\
	include/isc/uv.h		\
	include/isc/xml.h		\
	include/isc/work.h

libisc_la_SOURCES =		\
	$(libisc_la_HEADERS)	\
	netmgr/netmgr-int.h	\
	netmgr/netmgr.c		\
	netmgr/proxystream.c	\
	netmgr/proxyudp.c	\
	netmgr/socket.c		\
	netmgr/streamdns.c	\
	netmgr/tcp.c		\
	netmgr/timer.c		\
	netmgr/tlsstream.c	\
	netmgr/udp.c		\
	ascii.c			\
	assertions.c		\
	async.c			\
	async_p.h		\
	backtrace.c		\
	base32.c		\
	base64.c		\
	commandline.c		\
	condition.c		\
	counter.c		\
	crc64.c			\
	dir.c			\
	errno.c			\
	errno2result.c		\
	errno2result.h		\
	error.c			\
	file.c			\
	fips.c			\
	getaddresses.c		\
	hash.c			\
	hashmap.c		\
	heap.c			\
	helper.c		\
	hex.c			\
	histo.c			\
	hmac.c			\
	ht.c			\
	httpd.c			\
	interfaceiter.c		\
	iterated_hash.c		\
	jemalloc_shim.h		\
	job.c			\
	job_p.h			\
	lex.c			\
	lib.c			\
	log.c			\
	loop.c			\
	loop_p.h		\
	managers.c		\
	md.c			\
	mem.c			\
	mem_p.h			\
	meminfo.c		\
	mutex.c			\
	mutex_p.h		\
	mutexblock.c		\
	net.c			\
	netaddr.c		\
	netscope.c		\
	openssl_shim.c		\
	openssl_shim.h		\
	os.c			\
	os_p.h			\
	parseint.c		\
	picohttpparser.c	\
	picohttpparser.h	\
	portset.c		\
	probes.d		\
	proxy2.c		\
	quota.c			\
	radix.c			\
	random.c		\
	ratelimiter.c		\
	regex.c			\
	region.c		\
	result.c		\
	safe.c			\
	serial.c		\
	signal.c		\
	sockaddr.c		\
	stats.c			\
	stdio.c			\
	stdtime.c		\
	string.c		\
	symtab.c		\
	syslog.c		\
	thread.c		\
	tid.c			\
	time.c			\
	timer.c			\
	tls.c			\
	tm.c			\
	url.c			\
	utf8.c			\
	uv.c			\
	xml.c			\
	work.c

if USE_ISC_RWLOCK
libisc_la_SOURCES +=		\
	rwlock.c
endif USE_ISC_RWLOCK

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
	netmgr/http.c

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

if !HAVE_SYSTEMTAP
DTRACE_DEPS = libisc_la-rwlock.lo libisc_la-job.lo
DTRACE_OBJS = .libs/libisc_la-rwlock.$(OBJEXT) .libs/libisc_la-job.$(OBJEXT)
endif

include $(top_srcdir)/Makefile.dtrace

libisc_la_LIBADD += $(DTRACE_LIBADD)
