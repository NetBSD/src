# $NetBSD: netbsd-proto.mc,v 1.6 2000/08/25 02:36:23 hubertf Exp $

#
#  This is the prototype file for a configuration that supports nothing
#  but basic SMTP connections via TCP, via IPv4 and IPv6.
#

# The path is relative to ${CFDIR}/cf, see Makefile:
include(`../../../../usr.sbin/sendmail/cf/cf/netbsd-proto-IPv4only.mc')

# Enable IPv6:
DAEMON_OPTIONS(`Family=inet, address=0.0.0.0, Name=MTA')dnl
DAEMON_OPTIONS(`Family=inet6, address=::, Name=MTA6')dnl
