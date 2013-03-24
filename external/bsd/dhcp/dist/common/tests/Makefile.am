AM_CPPFLAGS = -I../..

check_PROGRAMS = test_alloc

TESTS = test_alloc

test_alloc_SOURCES = test_alloc.c
test_alloc_LDADD = ../libdhcp.a ../../tests/libt_api.a \
	../../omapip/libomapi.a ../../bind/lib/libdns.a \
        ../../bind/lib/libisc.a

