# $NetBSD: netbsd-proto.mc,v 1.7 2000/08/25 02:54:29 hubertf Exp $

#
#  This is the prototype file for a configuration that supports SMTP
#  connections via TCP and some commonly required features.
#

# The path is relative to ${CFDIR}/cf, see Makefile:
include(`../../../../usr.sbin/sendmail/cf/cf/netbsd-proto-IPv4only.mc')

# Enable IPv6:
DAEMON_OPTIONS(`Family=inet, address=0.0.0.0, Name=MTA')dnl
DAEMON_OPTIONS(`Family=inet6, address=::, Name=MTA6')dnl
