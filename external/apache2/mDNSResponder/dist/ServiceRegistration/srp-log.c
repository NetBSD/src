/* srp-log.c
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
 * This file contains the functions that help to create better logs.
 */

#include "srp-log.h"

#ifndef THREAD_DEVKIT_ADK
#include <netinet/in.h> // For struct in_addr and struct in6_addr used in "dns-msg.h".
#endif
#include "dns-msg.h" // For dns_qclass_xxx and dns_rrtype_xxx.

//======================================================================================================================
// MARK: - Functions

const char *
dns_qclass_to_string(const uint16_t qclass)
{
#define CASE_TO_STR(s) case dns_qclass_ ## s: return (#s)
    switch(qclass)
    {
        CASE_TO_STR(in);
        CASE_TO_STR(chaos);
        CASE_TO_STR(hesiod);
        CASE_TO_STR(none);
        CASE_TO_STR(any);
    }
#undef CASE_TO_STR
    FAULT("Invalid qclass - qclass: %u", qclass);
    return "<INVALID dns_qclass>";
}

//======================================================================================================================

const char *
dns_rrtype_to_string(const uint16_t rrtype)
{
#define CASE_TO_STR(s) case dns_rrtype_ ## s: return (#s)
    switch(rrtype)
    {
        CASE_TO_STR(a);
        CASE_TO_STR(ns);
        CASE_TO_STR(md);
        CASE_TO_STR(mf);
        CASE_TO_STR(cname);
        CASE_TO_STR(soa);
        CASE_TO_STR(mb);
        CASE_TO_STR(mg);
        CASE_TO_STR(mr);
        CASE_TO_STR(null);
        CASE_TO_STR(wks);
        CASE_TO_STR(ptr);
        CASE_TO_STR(hinfo);
        CASE_TO_STR(minfo);
        CASE_TO_STR(mx);
        CASE_TO_STR(txt);
        CASE_TO_STR(rp);
        CASE_TO_STR(afsdb);
        CASE_TO_STR(x25);
        CASE_TO_STR(isdn);
        CASE_TO_STR(rt);
        CASE_TO_STR(nsap);
        CASE_TO_STR(nsap_ptr);
        CASE_TO_STR(sig);
        CASE_TO_STR(key);
        CASE_TO_STR(px);
        CASE_TO_STR(gpos);
        CASE_TO_STR(aaaa);
        CASE_TO_STR(loc);
        CASE_TO_STR(nxt);
        CASE_TO_STR(eid);
        CASE_TO_STR(nimloc);
        CASE_TO_STR(srv);
        CASE_TO_STR(atma);
        CASE_TO_STR(naptr);
        CASE_TO_STR(kx);
        CASE_TO_STR(cert);
        CASE_TO_STR(a6);
        CASE_TO_STR(dname);
        CASE_TO_STR(sink);
        CASE_TO_STR(opt);
        CASE_TO_STR(apl);
        CASE_TO_STR(ds);
        CASE_TO_STR(sshfp);
        CASE_TO_STR(ipseckey);
        CASE_TO_STR(rrsig);
        CASE_TO_STR(nsec);
        CASE_TO_STR(dnskey);
        CASE_TO_STR(dhcid);
        CASE_TO_STR(nsec3);
        CASE_TO_STR(nsec3param);
        CASE_TO_STR(tlsa);
        CASE_TO_STR(smimea);
        CASE_TO_STR(hip);
        CASE_TO_STR(ninfo);
        CASE_TO_STR(rkey);
        CASE_TO_STR(talink);
        CASE_TO_STR(cds);
        CASE_TO_STR(cdnskey);
        CASE_TO_STR(openpgpkey);
        CASE_TO_STR(csync);
        CASE_TO_STR(zonemd);
        CASE_TO_STR(svcb);
        CASE_TO_STR(https);
        CASE_TO_STR(spf);
        CASE_TO_STR(uinfo);
        CASE_TO_STR(uid);
        CASE_TO_STR(gid);
        CASE_TO_STR(unspec);
        CASE_TO_STR(nid);
        CASE_TO_STR(l32);
        CASE_TO_STR(l64);
        CASE_TO_STR(lp);
        CASE_TO_STR(eui48);
        CASE_TO_STR(eui64);
        CASE_TO_STR(tkey);
        CASE_TO_STR(tsig);
        CASE_TO_STR(ixfr);
        CASE_TO_STR(axfr);
        CASE_TO_STR(mailb);
        CASE_TO_STR(maila);
        CASE_TO_STR(any);
        CASE_TO_STR(uri);
        CASE_TO_STR(caa);
        CASE_TO_STR(avc);
        CASE_TO_STR(doa);
        CASE_TO_STR(amtrelay);
        CASE_TO_STR(ta);
        CASE_TO_STR(dlv);
    }
#undef CASE_TO_STR
    FAULT("Invalid dns_rrtype - rrtype: %u", rrtype);
    return "<INVALID dns_rrtype>";
}

#ifdef LOG_FPRINTF_STDERR
bool srp_log_timestamp_relative = false;

void
srp_log_timestamp(char *buf, size_t bufsize)
{
    if (srp_log_timestamp_relative) {
        double srp_fractional_time(void);
        static bool have_initial_timestamp = false;
        static double initial_timestamp = 0.0;
        if (!have_initial_timestamp) {
            have_initial_timestamp = true;
            initial_timestamp = srp_fractional_time();
        }
        snprintf(buf, bufsize, "%6.6lf", srp_fractional_time() - initial_timestamp);
    } else {
        char timebuf[20]; // YYYY-MM-DD HH:MM:SS
        char zonebuf[6]; // [-+]HMM
        struct timeval tv;
        struct tm tm;
        gettimeofday(&tv, NULL);
        localtime_r(&tv.tv_sec, &tm);
        strftime(timebuf, sizeof(timebuf), "%F %T", &tm);
        strftime(zonebuf, sizeof(zonebuf), "%z", &tm);
        snprintf(buf, bufsize, "%s.%06d%s", timebuf, tv.tv_usec, zonebuf);
    }
}
#endif

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 120
// indent-tabs-mode: nil
// End:
