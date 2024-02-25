include $(top_srcdir)/Makefile.top

AM_CPPFLAGS +=			\
	$(LIBISC_CFLAGS)	\
	$(LIBUV_CFLAGS)		\
	-I$(top_srcdir)/lib/isc

LDADD +=			\
	$(LIBISC_LIBS)		\
	$(LIBUV_LIBS)

check_PROGRAMS =	\
	aes_test	\
	buffer_test	\
	counter_test	\
	crc64_test	\
	errno_test	\
	file_test	\
	hash_test	\
	heap_test	\
	hmac_test	\
	ht_test		\
	lex_test	\
	md_test		\
	mem_test	\
	netaddr_test	\
	netmgr_test	\
	parse_test	\
	pool_test	\
	quota_test	\
	radix_test	\
	random_test	\
	regex_test	\
	result_test	\
	safe_test	\
	siphash_test	\
	sockaddr_test	\
	stats_test	\
	symtab_test	\
	task_test	\
	taskpool_test	\
	time_test	\
	timer_test

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

netmgr_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)	\
	$(OPENSSL_CFLAGS)

netmgr_test_LDADD =	\
	$(LDADD)	\
	$(OPENSSL_LIBS)

netmgr_test_SOURCES =	\
	netmgr_test.c	\
	uv_wrap.h

random_test_LDADD =	\
	$(LDADD)	\
	-lm

task_test_CPPFLAGS =	\
	$(AM_CPPFLAGS)

task_test_LDADD =	\
	$(LDADD)

if HAVE_LIBXML2
task_test_CPPFLAGS += $(LIBXML2_CFLAGS)
task_test_LDADD += $(LIBXML2_LIBS)
endif HAVE_LIBXML2

if HAVE_JSON_C
task_test_CPPFLAGS += $(JSON_C_CFLAGS)
task_test_LDADD += $(JSON_C_LIBS)
endif HAVE_JSON_C


EXTRA_DIST = testdata

include $(top_srcdir)/Makefile.tests
