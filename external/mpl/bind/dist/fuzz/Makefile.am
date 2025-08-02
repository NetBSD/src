include $(top_srcdir)/Makefile.top

AM_CFLAGS +=				\
	$(TEST_CFLAGS)

AM_CPPFLAGS +=				\
	$(LIBISC_CFLAGS)		\
	$(LIBDNS_CFLAGS)		\
	$(LIBURCU_CFLAGS)		\
	$(LIBUV_CFLAGS)			\
	-DFUZZDIR=\"$(abs_srcdir)\"	\
	-I$(top_srcdir)/lib/dns		\
	-I$(top_srcdir)/lib/isc		\
	-I$(top_srcdir)/tests/include

AM_LDFLAGS +=				\
	$(FUZZ_LDFLAGS)

LDADD +=						\
	libfuzzmain.la					\
	$(top_builddir)/tests/libtest/libtest.la	\
	$(LIBDNS_LIBS)					\
	$(LIBISC_LIBS)

check_LTLIBRARIES = libfuzzmain.la
libfuzzmain_la_SOURCES =		\
	fuzz.h				\
	main.c

check_PROGRAMS =			\
	dns_master_load			\
	dns_message_checksig		\
	dns_message_parse		\
	dns_name_fromtext_target	\
	dns_name_fromwire		\
	dns_qp				\
	dns_qpkey_name			\
	dns_rdata_fromtext		\
	dns_rdata_fromwire_text		\
	isc_lex_getmastertoken		\
	isc_lex_gettoken

EXTRA_DIST =				\
	dns_master_load.in		\
	dns_message_checksig.in		\
	dns_message_parse.in		\
	dns_name_fromtext_target.in	\
	dns_name_fromwire.in		\
	dns_qp.in			\
	dns_qpkey_name.in		\
	dns_rdata_fromtext.in		\
	dns_rdata_fromwire_text.in	\
	isc_lex_getmastertoken.in	\
	isc_lex_gettoken.in

dns_name_fromwire_SOURCES =		\
	dns_name_fromwire.c		\
	old.c				\
	old.h

TESTS = $(check_PROGRAMS)

if HAVE_FUZZ_LOG_COMPILER
LOG_COMPILER = $(srcdir)/$(FUZZ_LOG_COMPILER)
AM_LOG_FLAGS = $(srcdir)
endif HAVE_FUZZ_LOG_COMPILER

unit-local: check
