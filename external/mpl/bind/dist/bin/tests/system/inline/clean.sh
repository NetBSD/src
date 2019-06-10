# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

rm -rf ./*/*.jbk \
   ./*/*.nzd ./*/*.nzd-lock ./*/*.nzf \
   ./*/named.conf ./*/named.memstats ./*/named.run* ./*/named.lock \
   ./*/trusted.conf \
   ./K* ./*/K* \
   ./checkecdsa \
   ./freeze.test* thaw.test* \
   ./import.key \
   ././ns*/managed-keys.bind* ./ns*/*.mkeys* \
   ./*/dsset-* ./*/nzf-* \
   ./*/*.db ./*/*.db.signed ./*/*.db.jnl ./*/*.db.signed.jnl \
   ./*.out ./*.out* ./*/*.out ./*/*.out* \
   ./*/*.bk ./*/*.bk.jnl ./*/*.bk.signed ./*/*.bk.signed.jnl \
   ns3/a-file ns3/removedkeys
