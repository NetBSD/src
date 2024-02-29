include $(top_srcdir)/Makefile.top

EXTRA_DIST = convert-trs-to-junit.py

SUBDIRS = system

noinst_PROGRAMS =	\
	test_client	\
	test_server	\
	wire_test

AM_CFLAGS +=			\
	$(TEST_CFLAGS)

test_client_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(LIBISC_CFLAGS)

test_client_LDADD =		\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)

test_server_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(LIBISC_CFLAGS)

test_server_LDADD =		\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)

wire_test_CPPFLAGS =		\
	$(AM_CPPFLAGS)		\
	$(LIBISC_CFLAGS)	\
	$(LIBDNS_CFLAGS)

wire_test_LDADD =		\
	$(LIBISC_LIBS)		\
	$(LIBDNS_LIBS)
