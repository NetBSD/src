
sbin_PROGRAMS = setkey

AM_CFLAGS = -I${top_srcdir}/src/libipsec @GLIBC_BUGS@
AM_YFLAGS = -d

BUILT_SOURCES = parse.h

setkey_SOURCES = \
  setkey.c \
  parse.y \
  token.l

setkey_LDFLAGS = ../libipsec/libipsec.la
setkey_DEPENDENCIES = ../libipsec/libipsec.la
setkey_LDADD = $(LEXLIB)

noinst_HEADERS = vchar.h extern.h
man8_MANS = setkey.8

EXTRA_DIST = ${man8_MANS} sample-policy01.cf sample-policy02.cf sample.cf \
	scriptdump.pl test-pfkey.c
