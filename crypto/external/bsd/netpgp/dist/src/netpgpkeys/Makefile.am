## $NetBSD: Makefile.am,v 1.1 2009/06/10 00:38:10 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

bin_PROGRAMS		= netpgpkeys

netpgpkeys_SOURCES	= netpgpkeys.c

netpgpkeys_CPPFLAGS	= -I$(top_srcdir)/include

netpgpkeys_LDADD	= ../lib/libnetpgp.la

man1_MANS		= netpgpkeys.1

dist_man_MANS		= netpgpkeys.1
