Copyright (C) Internet Systems Consortium, Inc. ("ISC")

SPDX-License-Identifier: MPL-2.0

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0.  If a copy of the MPL was not distributed with this
file, you can obtain one at https://mozilla.org/MPL/2.0/.

See the COPYRIGHT file distributed with this work for additional
information regarding copyright ownership.

The `rsabigexponent` test is used to `check max-rsa-exponent-size`.

We only run this test on builds without PKCS#11, as we have control over
the RSA exponent size with plain OpenSSL. We have not explored how to do
this with PKCS#11, which would require generating such a key and then
signing a zone with it. Additionally, even with control of the exponent
size with PKCS#11, generating a DNSKEY with this property and signing
such a zone would be slow and undesirable for each test run; instead, we
use a pregenerated DNSKEY and a saved signed zone.  These are located in
`rsabigexponent/ns2` and currently use RSASHA1 for the `DNSKEY`
algorithm; however, that may need to be changed in the future.

To generate the `DNSKEY` used in this test, we used `bigkey.c`, as
dnssec-keygen is not capable of generating such keys.

Do **not** remove `bigkey.c` as it may be needed to generate a new
`DNSKEY` for testing purposes.

`bigkey` is used to both test that we are not running under PKCS#11 and
generate a `DNSKEY` key with a large RSA exponent.

To regenerate `ns2/example.db.bad` comment out the range test in
opensslrsa_parse before signing the zone with a ZSK key generated
by `bigkey`.

        if (BN_num_bits(e) > RSA_MAX_PUBEXP_BITS) {
                DST_RET(ISC_R_RANGE);
        }
