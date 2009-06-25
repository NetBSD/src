## $NetBSD: Makefile.am,v 1.1 2009/06/25 13:47:11 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS) -pthread

bin_PROGRAMS		= iscsi-target

iscsi_target_SOURCES	= iscsi-target.c

iscsi_target_CPPFLAGS	= -I$(top_srcdir)/include

iscsi_target_LDADD	= ../lib/libiscsi.la -lpthread

man1_MANS		= iscsi-target.8

dist_man_MANS		= iscsi-target.8
