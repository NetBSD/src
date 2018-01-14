#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.4 2018/01/14 22:51:12 christos Exp $
#
# Extract the new tarball and run this script with the first argument
# the mDNSResponder-X.Y directory that was created. cd to that directory
# and import:
# cvs -d cvs.netbsd.org:/cvsroot import \
#	 src/external/apache2/mDNSResponder/dist APPLE mdnsresponder-x-y-z
# This scripts deletes extra content.

set -e
cd "$1"
rm -rf mDNSMacOS9 mDNSMacOSX mDNSVxWorks mDNSWindows
rm -f Makefile mDNSResponder.sln README.txt PrivateDNS.txt Documents

(cd Clients
rm -rf BonjourExample DNS-SD.VisualStudio DNS-SD.xcodeproj
rm -rf DNSServiceBrowser-Info.plist DNSServiceBrowser.NET
rm -rf DNSServiceBrowser.VB DNSServiceBrowser.m DNSServiceBrowser.nib
rm -rf DNSServiceReg-Info.plist DNSServiceReg.m DNSServiceReg.nib
rm -rf ExplorerPlugin FirefoxExtension Java Makefile PrinterSetupWizard
rm -rf ReadMe.txt SimpleChat.NET SimpleChat.VB mDNSNetMonitor.VisualStudio)

(cd mDNSCore
rm -f Implementer\ Notes.txt)

(cd mDNSPosix
rm -f Client.c ExampleClientApp.c ExampleClientApp.h Identify.c
rm -f Makefile NetMonitor.c ProxyResponder.c ReadMe.txt Responder.c
rm -f Services.txt libnss_mdns.8 mdnsd.sh nss_ReadMe.txt nss_mdns.c
rm -f nss_mdns.conf nss_mdns.conf.5 parselog.py)

(cd mDNSShared
rm -rf DebugServices.c DebugServices.h Java dnssd_clientshim.c mDNS.1
rm -f dnsextd.8 dnsextd.c dnsextd.conf dnsextd.h dnsextd_lexer.l
rm -f dnsextd_parser.)

# Kill RCS Tag lines
cleantags .
