# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

#
# Sample lookup procedure for tcldb
#
# This lookup procedure defines zones with identical SOA, NS, and MX
# records at the apex and a single A record that varies from zone to
# zone at the name "www".
#
# Something like this could be used by a web hosting company to serve
# a number of domains without needing to create a separate master file
# for each domain.  Instead, all per-zone data (in this case, a single
# IP address) specified in the named.conf file like this:
#
#   zone "a.com." { type master; database "tcl 10.0.0.42"; };
#   zone "b.com." { type master; database "tcl 10.0.0.99"; };
#
# Since the tcldb driver doesn't support zone transfers, there should
# be at least two identically configured master servers.  In the
# example below, they are assumed to be called ns1.isp.nil and
# ns2.isp.nil.
#

proc lookup {zone name} {
    global dbargs
    switch -- $name {
	@ { return [list \
		{SOA 86400 "ns1.isp.nil. hostmaster.isp.nil. \
		    1 3600 1800 1814400 3600"} \
		{NS 86400 "ns1.isp.nil."} \
		{NS 86400 "ns2.isp.nil."} \
		{MX 86400 "10 mail.isp.nil."} ] }
	www { return [list [list A 3600 $dbargs($zone)] ] }
    }
    return NXDOMAIN
}
