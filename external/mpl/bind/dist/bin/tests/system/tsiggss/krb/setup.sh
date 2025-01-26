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

PWD=$(pwd)

KRB5_CONFIG="${PWD}/krb5.conf"
export KRB5_CONFIG

KRB5_KDC_PROFILE=${PWD}/krb5kdc
export KRB5_KDC_PROFILE

now=$(date +%s)
lifetime=$(2147483647 - now)
lifetime=$(lifetime / 3600 / 24 - 30)

cat <<EOF >"${KRB5_CONFIG}"
[libdefaults]
   default_realm = EXAMPLE.NIL
   dns_lookup_kdc = false
   # Depending on what you are testing, you may want something like:
   # default_keytab_name = FILE:/usr/local/var/keytab
[realms]
   EXAMPLE.NIL = {
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

cat <<EOF >"${KRB5_KDC_PROFILE}"/kdc.conf
[kdcdefaults]
  kdc_ports = 50000
  kdc_tcp_ports = 50000

[realms]
  EXAMPLE.NIL = {
    key_stash_file = ${KRB5_KDC_PROFILE}/.k5.EXAMPLE.NIL
    database_module = EXAMPLE.NIL
    max_life = ${lifetime}d
}

[dbmodules]
  EXAMPLE.NIL = {
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

kadmin.local addprinc -maxlife ${lifetime}d -randkey DNS/example.nil@EXAMPLE.NIL
kadmin.local addprinc -maxlife ${lifetime}d -randkey DNS/blu.example.nil@EXAMPLE.NIL
kadmin.local addprinc -maxlife ${lifetime}d -randkey dns-blu@EXAMPLE.NIL
kadmin.local addprinc -maxlife ${lifetime}d -randkey administrator@EXAMPLE.NIL
kadmin.local addprinc -maxlife ${lifetime}d -randkey testdenied@EXAMPLE.NIL

kadmin.local ktadd -k dns.keytab DNS/example.nil@EXAMPLE.NIL
kadmin.local ktadd -k dns.keytab DNS/blu.example.nil@EXAMPLE.NIL
kadmin.local ktadd -k dns.keytab dns-blu@EXAMPLE.NIL
kadmin.local ktadd -k administrator.keytab administrator@EXAMPLE.NIL
kadmin.local ktadd -k testdenied.keytab testdenied@EXAMPLE.NIL

kinit -V -k -t administrator.keytab -l ${lifetime}d -c administrator.ccache administrator@EXAMPLE.NIL
kinit -V -k -t testdenied.keytab -l ${lifetime}d -c testdenied.ccache testdenied@EXAMPLE.NIL

cp dns.keytab administrator.ccache testdenied.ccache ../ns1/

echo "krb5kdc pid:$krb5kdcpid"
echo "KRB5_CONFIG=$KRB5_CONFIG"
