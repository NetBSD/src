include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBURCU_CFLAGS)	\
	$(LIBUV_CFLAGS)		\
	-I$(top_srcdir)/lib/isc

LDADD +=			\
	$(LIBISC_LIBS)		\
	$(LIBURCU_LIBS)		\
	$(LIBUV_LIBS)

check_PROGRAMS =	\
	ascii_test	\
	async_test	\
	buffer_test	\
	counter_test	\
	crc64_test	\
	dnsstream_utils_test \
	errno_test	\
	file_test	\
	hash_test	\
	hashmap_test	\
	heap_test	\
	histo_test	\
	hmac_test	\
	ht_test		\
	job_test	\
	lex_test	\
	loop_test	\
	md_test		\
	mem_test	\
	mutex_test	\
	netaddr_test	\
	parse_test	\
	proxyheader_test	\
	proxystream_test	\
	proxyudp_test	\
	quota_test	\
	radix_test	\
	ratelimiter_test\
	regex_test	\
	result_test	\
	rwlock_test	\
	safe_test	\
	siphash_test	\
	sockaddr_test	\
	spinlock_test	\
	stats_test	\
	symtab_test	\
	tcp_test	\
	tcpdns_test	\
	time_test	\
	timer_test	\
	tls_test	\
	tlsdns_test	\
	udp_test	\
	work_test

if HAVE_LIBNGHTTP2
check_PROGRAMS +=	\
	doh_test

doh_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(LIBNGHTTP2_CFLAGS)	\
	$(OPENSSL_CFLAGS)

doh_test_LDADD =	\
	$(LDADD)	\
	$(LIBNGHTTP2_LIBS)

doh_test_SOURCES =	\
	doh_test.c	\
	uv_wrap.h

endif HAVE_LIBNGHTTP2

dnsstream_utils_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)

dnsstream_utils_test_LDADD =	\
	$(LDADD)

dnsstream_utils_test_SOURCES =	\
	dnsstream_utils_test.c \
	dnsstream_utils_test_data.h

hashmap_test_SOURCES =	\
	hashmap_test.c	\
	hashmap_nodes.h

hmac_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(OPENSSL_CFLAGS)

hmac_test_LDADD =	\
	$(LDADD)	\
	$(OPENSSL_LIBS)

md_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(OPENSSL_CFLAGS)

md_test_LDADD =		\
	$(LDADD)	\
	$(OPENSSL_LIBS)

proxyheader_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)

proxyheader_test_LDADD =	\
	$(LDADD)

proxyheader_test_SOURCES =	\
	proxyheader_test.c \
	proxyheader_test_data.h

proxystream_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)		\
	$(OPENSSL_CFLAGS)

proxystream_test_LDADD =	\
	$(LDADD)		\
	$(OPENSSL_LIBS)

proxystream_test_SOURCES =	\
	proxystream_test.c \
	netmgr_common.h \
	netmgr_common.c \
	stream_shutdown.c

proxyudp_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(OPENSSL_CFLAGS)

proxyudp_test_LDADD =	\
	$(LDADD)	\
	$(OPENSSL_LIBS)

proxyudp_test_SOURCES =	\
	proxyudp_test.c	\
	netmgr_common.h \
	netmgr_common.c \
	uv_wrap.h

tcp_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(OPENSSL_CFLAGS)

tcp_test_LDADD =	\
	$(LDADD)	\
	$(OPENSSL_LIBS)

tcp_test_SOURCES =	\
	tcp_test.c	\
	netmgr_common.h \
	netmgr_common.c \
	stream_shutdown.c \
	uv_wrap.h

tcpdns_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(OPENSSL_CFLAGS)

tcpdns_test_LDADD =	\
	$(LDADD)	\
	$(OPENSSL_LIBS)

tcpdns_test_SOURCES =	\
	tcpdns_test.c	\
	netmgr_common.h \
	netmgr_common.c \
	uv_wrap.h

tls_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(OPENSSL_CFLAGS)

tls_test_LDADD =	\
	$(LDADD)	\
	$(OPENSSL_LIBS)

tls_test_SOURCES =	\
	tls_test.c	\
	netmgr_common.h \
	netmgr_common.c \
	stream_shutdown.c \
	uv_wrap.h

tlsdns_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(OPENSSL_CFLAGS)

tlsdns_test_LDADD =	\
	$(LDADD)	\
	$(OPENSSL_LIBS)

tlsdns_test_SOURCES =	\
	tlsdns_test.c	\
	netmgr_common.h \
	netmgr_common.c \
	uv_wrap.h

udp_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(OPENSSL_CFLAGS)

udp_test_LDADD =	\
	$(LDADD)	\
	$(OPENSSL_LIBS)

udp_test_SOURCES =	\
	udp_test.c	\
	netmgr_common.h \
	netmgr_common.c \
	uv_wrap.h

EXTRA_DIST = testdata

include $(top_srcdir)/Makefile.tests
