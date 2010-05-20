## $NetBSD: Makefile.am,v 1.2 2010/05/20 00:33:01 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

bin_PROGRAMS		= netpgpverify

netpgpverify_SOURCES	= verify.c

netpgpverify_CPPFLAGS	= -I$(top_srcdir)/include

netpgpverify_LDADD	= ../lib/libnetpgp.la

man1_MANS		= netpgpverify.1

dist_man_MANS		= netpgpverify.1
