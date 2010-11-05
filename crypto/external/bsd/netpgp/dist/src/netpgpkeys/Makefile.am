## $NetBSD: Makefile.am,v 1.3 2010/11/05 07:40:41 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

bin_PROGRAMS		= netpgpkeys

netpgpkeys_SOURCES	= netpgpkeys.c

netpgpkeys_CPPFLAGS	= -I$(top_srcdir)/include -I../libmj

netpgpkeys_LDADD	= ../lib/libnetpgp.la ../libmj/libmj.la

man1_MANS		= netpgpkeys.1

dist_man_MANS		= netpgpkeys.1
