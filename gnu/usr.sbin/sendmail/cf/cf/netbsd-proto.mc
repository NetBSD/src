# $NetBSD: netbsd-proto.mc,v 1.8 2001/01/15 17:49:26 itojun Exp $

#
#  This is the prototype file for a configuration that supports SMTP
#  connections via TCP and some commonly required features.
#

# The path is relative to ${CFDIR}/cf, see Makefile:
include(`../../../../usr.sbin/sendmail/cf/cf/netbsd-proto-IPv4only.mc')

# Enable IPv6.  IPv6 is marked as optional so the configuration file
# can be used on IPV4-only kernel as well.
DAEMON_OPTIONS(`Family=inet, address=0.0.0.0, Name=MTA')dnl
DAEMON_OPTIONS(`Family=inet6, address=::, Name=MTA6, Modifiers=O')dnl
