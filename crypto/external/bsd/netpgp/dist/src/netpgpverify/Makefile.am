## $NetBSD: Makefile.am,v 1.3 2010/11/05 07:41:20 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

bin_PROGRAMS		= netpgpverify

netpgpverify_SOURCES	= verify.c

netpgpverify_CPPFLAGS	= -I$(top_srcdir)/include

netpgpverify_LDADD	= ../lib/libnetpgp.la ../libmj/libmj.la

man1_MANS		= netpgpverify.1

dist_man_MANS		= netpgpverify.1
