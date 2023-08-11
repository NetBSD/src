#!/bin/sh

# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# SPDX-License-Identifier: MPL-2.0
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0.  If a copy of the MPL was not distributed with this
# file, you can obtain one at https://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

set -x

PWD=`pwd`

KRB5_CONFIG="${PWD}/krb5.conf"
export KRB5_CONFIG

KRB5_KDC_PROFILE=${PWD}/krb5kdc
export KRB5_KDC_PROFILE

now=`date +%s`
lifetime=`expr 2147483647 - $now`
lifetime=`expr $lifetime / 3600 / 24 - 30`

cat << EOF > "${KRB5_CONFIG}"
[libdefaults]
   default_realm = EXAMPLE.COM
   dns_lookup_kdc = false
   # Depending on what you are testing, you may want something like:
   # default_keytab_name = FILE:/usr/local/var/keytab
[realms]
   EXAMPLE.COM = {
     admin_server = 127.0.0.1:50001
     kdc = 127.0.0.1:50000
     database_module = DB2
     kdc_ports = 50000
     kadmind_port = 50001
   }
[dbmodules]
   DB2 = {
     db_library = db2
   }
[logging]
   # Use any pathnames you want here.
   kdc = FILE:${PWD}/kdc.log
   admin_server = FILE:${PWD}/kadmin.log
# Depending on what you are testing, you may want:
# [domain_realm]
#   your.domain = EXAMPLE.COM
EOF

rm -rf ${KRB5_KDC_PROFILE}
mkdir -p ${KRB5_KDC_PROFILE}
chmod 700 ${KRB5_KDC_PROFILE}

cat << EOF > "${KRB5_KDC_PROFILE}"/kdc.conf
[kdcdefaults]
  kdc_ports = 50000
  kdc_tcp_ports = 50000

[realms]
  EXAMPLE.COM = {
    key_stash_file = ${KRB5_KDC_PROFILE}/.k5.EXAMPLE.COM
    database_module = EXAMPLE.COM
    max_life = ${lifetime}d
}

[dbmodules]
  EXAMPLE.COM = {
    db_library = db2
    database_name = ${KRB5_KDC_PROFILE}/principal
  }
EOF

kdb5_util create -s <<EOF
master
master
EOF

krb5kdc -n &
krb5kdcpid=$!
#trap "kill $krb5kdcpid; wait; trap 0; exit" 0 15


kadmin.local addprinc -maxlife ${lifetime}d -randkey DNS/ns7.example.com@EXAMPLE.COM
kadmin.local addprinc -maxlife ${lifetime}d -randkey DNS/ns8.example.com@EXAMPLE.COM
kadmin.local addprinc -maxlife ${lifetime}d -randkey host/machine.example.com@EXAMPLE.COM

kadmin.local ktadd -k ns7-server.keytab DNS/ns7.example.com@EXAMPLE.COM
kadmin.local ktadd -k ns8-server.keytab DNS/ns8.example.com@EXAMPLE.COM
kadmin.local ktadd -k krb5-machine.keytab host/machine.example.com@EXAMPLE.COM

kadmin.local addprinc -maxlife ${lifetime}d -randkey 'DNS/ns9.example.com@EXAMPLE.COM'
kadmin.local addprinc -maxlife ${lifetime}d -randkey 'DNS/ns10.example.com@EXAMPLE.COM'
kadmin.local addprinc -maxlife ${lifetime}d -randkey 'machine$@EXAMPLE.COM'

kadmin.local ktadd -k ns9-server.keytab 'DNS/ns9.example.com@EXAMPLE.COM'
kadmin.local ktadd -k ns10-server.keytab 'DNS/ns10.example.com@EXAMPLE.COM'
kadmin.local ktadd -k ms-machine.keytab 'machine$@EXAMPLE.COM'

kinit -V -k -t krb5-machine.keytab -l ${lifetime}d -c krb5-machine.ccache host/machine.example.com@EXAMPLE.COM
kinit -V -k -t ms-machine.keytab -l ${lifetime}d -c ms-machine.ccache 'machine$@EXAMPLE.COM'

cp ns7-server.keytab ../ns7/dns.keytab
cp ns8-server.keytab ../ns8/dns-other-than-KRB5_KTNAME.keytab
cp ns9-server.keytab ../ns9/dns.keytab
cp ns10-server.keytab ../ns10/dns.keytab

cp krb5-machine.ccache ../ns7/machine.ccache
cp krb5-machine.ccache ../ns8/machine.ccache
cp ms-machine.ccache ../ns9/machine.ccache
cp ms-machine.ccache ../ns10/machine.ccache

echo krb5kdc pid:$krb5kdcpid
