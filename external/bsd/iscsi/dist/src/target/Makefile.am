## $NetBSD: Makefile.am,v 1.2 2009/06/30 02:44:53 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS) -pthread

bin_PROGRAMS		= iscsi-target

iscsi_target_SOURCES	= iscsi-target.c

iscsi_target_CPPFLAGS	= -I$(top_srcdir)/include

iscsi_target_LDADD	= ../lib/libiscsi.la -lpthread

man1_MANS		= iscsi-target.8

dist_man_MANS		= iscsi-target.8
