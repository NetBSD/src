/* ioloop-common.c
 *
 * Copyright (c) 2021 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * This file contains common code shared by service registration code.
 */

//======================================================================================================================
// MARK: - Headers

//======================================================================================================================
// MARK: - Headers

#include <dns_sd.h> // for DNSServiceRef and DNSRecordRef.
#include <stdlib.h>
#include <netinet/in.h>
#include "srp.h"
#include "dns-msg.h"
#include "ioloop.h"
#include "ioloop-common.h"
#include "mdns_strict.h"

//======================================================================================================================
// MARK: - Functions

dnssd_txn_t *NULLABLE
dnssd_txn_create_shared_(const char *const NONNULL file, const int line)
{
    DNSServiceRef service_ref = NULL;
    dnssd_txn_t *txn = NULL;

    const DNSServiceErrorType dns_err = DNSServiceCreateConnection(&service_ref);
    if (dns_err != kDNSServiceErr_NoError) {
        goto exit;
    }

    txn = ioloop_dnssd_txn_add_(service_ref, NULL, NULL, NULL, file, line);
    if (txn == NULL) {
        MDNS_DISPOSE_DNS_SERVICE_REF(service_ref);
        goto exit;
    }

exit:
    return txn;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
