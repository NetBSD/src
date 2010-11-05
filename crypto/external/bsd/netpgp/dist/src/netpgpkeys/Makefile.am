## $NetBSD: Makefile.am,v 1.2 2010/11/05 03:37:19 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

bin_PROGRAMS		= netpgpkeys

netpgpkeys_SOURCES	= netpgpkeys.c

netpgpkeys_CPPFLAGS	= -I$(top_srcdir)/include -I../libmj

netpgpkeys_LDADD	= ../lib/libnetpgp.la

man1_MANS		= netpgpkeys.1

dist_man_MANS		= netpgpkeys.1
