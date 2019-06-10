echo off
rem
rem Copyright (C) 2016  Internet Systems Consortium, Inc. ("ISC")
rem 
rem This Source Code Form is subject to the terms of the Mozilla Public
rem License, v. 2.0. If a copy of the MPL was not distributed with this
rem file, You can obtain one at http://mozilla.org/MPL/2.0/.

rem ifconfig.bat
rem Set up interface aliases for bind9 system tests.
rem
rem IPv4: 10.53.0.{1..10}			RFC 1918
rem       10.53.1.{0..2}
rem       10.53.2.{0..2}
rem IPv6: fd92:7065:b8e:ffff::{1..10}		ULA
rem       fd92:7065:b8e:99ff::{1..2}
rem       fd92:7065:b8e:ff::{1..2}
rem
echo Please adapt this script to your system
rem remove the following line when the script is ready
exit /b 1

rem for IPv4 adding these static addresses to a physical interface
rem will switch it from DHCP to static flushing DHCP setup.
rem So it is highly recommended to install the loopback pseudo-interface
rem and add IPv4 addresses to it.

rem for IPv6 your interface can have a different name, e.g.,
rem "Local Area Connection". Please update this script and remove the
rem exit line

echo on

FOR %%I IN (1,2,3,4,5,6,7,8) DO (
	netsh interface ipv4 add address name=Loopback 10.53.0.%%I 255.255.255.0
	netsh interface ipv6 add address interface=Loopback fd92:7065:b8e:ffff::%%I/64
)
FOR %%I IN (1,2) DO (
	netsh interface ipv4 add address name=Loopback 10.53.1.%%I 255.255.255.0
	netsh interface ipv4 add address name=Loopback 10.53.2.%%I 255.255.255.0

	netsh interface ipv6 add address interface=Loopback fd92:7065:b8e:99ff::%%I/64
	netsh interface ipv6 add address interface=Loopback fd92:7065:b8e:ff::%%I/64
)
