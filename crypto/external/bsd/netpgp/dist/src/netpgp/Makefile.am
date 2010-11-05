## $NetBSD: Makefile.am,v 1.2 2010/11/05 07:39:59 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

bin_PROGRAMS		= netpgp

netpgp_SOURCES		= netpgp.c

netpgp_CPPFLAGS		= -I$(top_srcdir)/include

netpgp_LDADD		= ../lib/libnetpgp.la ../libmj/libmj.la

man1_MANS		= netpgp.1

dist_man_MANS		= netpgp.1
