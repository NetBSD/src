#!/bin/sh
# $NetBSD: prepare-import.sh,v 1.1 2009/09/29 23:56:27 tsarna Exp $
#
# Extract the new tarball and rename the mDNSResponder-X.Y directory
# to dist.  Run this script and check for additional files and
# directories to prune, only relevant content is included.

set -e

cd dist
rm -rf mDNSMacOS9 mDNSMacOSX mDNSVxWorks mDNSWindows
rm -f Makefile mDNSResponder.sln README.txt PrivateDNS.txt

cd Clients
rm -rf BonjourExample DNS-SD.VisualStudio DNS-SD.xcodeproj
rm -rf DNSServiceBrowser-Info.plist DNSServiceBrowser.NET
rm -rf DNSServiceBrowser.VB DNSServiceBrowser.m DNSServiceBrowser.nib
rm -rf DNSServiceReg-Info.plist DNSServiceReg.m DNSServiceReg.nib
rm -rf ExplorerPlugin Java Makefile PrinterSetupWizard ReadMe.txt
rm -rf SimpleChat.NET SimpleChat.VB

cd ../mDNSCore
rm -f Implementer\ Notes.txt

cd ../mDNSPosix
rm -f Client.c ExampleClientApp.c ExampleClientApp.h Identify.c
rm -f Makefile NetMonitor.c ProxyResponder.c ReadMe.txt Responder.c
rm -f Services.txt libnss_mdns.8 mdnsd.sh nss_ReadMe.txt nss_mdns.c
rm -f nss_mdns.conf nss_mdns.conf.5 parselog.py

cd ../mDNSShared
rm -rf DebugServices.c DebugServices.h Java dnssd_clientshim.c mDNS.1
rm -f dnsextd.8 dnsextd.c dnsextd.conf dnsextd.h dnsextd_lexer.l dnsextd_parser.y

# Kill RCS Log lines
cd ..
find . | xargs qsubst '$Log' 'Log' -noask
