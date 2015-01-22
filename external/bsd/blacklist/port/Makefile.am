#
ACLOCAL_AMFLAGS = -Im4
lib_LTLIBRARIES = libblacklist.la
include_HEADERS = blacklist.h

bin_PROGRAMS = blacklistd blacklistctl

VPATH = ../bin:../lib:../test

AM_CPPFLAGS = -I../include 
AM_CFLAGS = @WARNINGS@

libblacklist_la_SOURCES = bl.c blacklist.c
libblacklist_la_LDFLAGS = -no-undefined -version-info 0:0:0
libblacklist_la_LIBADD = $(LTLIBOBJS)

SRCS = internal.c util.c run.c conf.c state.c
blacklistd_SOURCES = blacklistd.c ${SRCS}
blacklistd_LDADD = libblacklist.la
blacklistctl_SOURCES = blacklistctl.c ${SRCS}
blacklistctl_LDADD = libblacklist.la
