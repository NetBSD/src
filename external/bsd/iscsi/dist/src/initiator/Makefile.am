## $NetBSD: Makefile.am,v 1.3 2010/04/14 19:52:20 sborrill Exp $

AM_CFLAGS		= $(WARNCFLAGS)

bin_PROGRAMS		= iscsi-initiator

iscsi_initiator_SOURCES	= iscsi-initiator.c libkmod.c virtdir.c

iscsi_initiator_CPPFLAGS= -I$(top_srcdir)/include

iscsi_initiator_LDADD	= ../lib/libiscsi.la -lrefuse

man1_MANS		= iscsi-initiator.8

dist_man_MANS		= iscsi-initiator.8
