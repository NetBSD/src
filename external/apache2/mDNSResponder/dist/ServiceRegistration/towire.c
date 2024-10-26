/* towire.c
 *
 * Copyright (c) 2018-2021 Apple, Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * DNS to-wire wire-format functions.
 *
 * These are really simple functions for constructing DNS messages in wire format.
 * The flow is that there is a transaction structure which contains pointers to both
 * a message output buffer and a response input buffer.   The structure is initialized,
 * and then the various wire format functions are called repeatedly to store data.
 * If an error occurs during this process, it's okay to just keep going, because the
 * error is recorded in the transaction; once all of the copy-in functions have been
 * called, the error status can be checked once at the end.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#ifndef THREAD_DEVKIT_ADK
#include <arpa/inet.h>
#endif
#include <stdlib.h>

#include "srp.h"
#include "dns-msg.h"
#include "srp-crypto.h"

#ifndef NO_CLOCK
#include <sys/time.h>
#endif

static int
dns_parse_label(const char *cur, const char *NONNULL *NONNULL nextp, uint8_t *NONNULL lenp, uint8_t *NONNULL buf,
                ssize_t max)
{
    const char *end;
    int tlen;
    const char *s;
    uint8_t *t;

    end = strchr(cur, '.');
    if (end == NULL) {
        end = cur + strlen(cur);
        if (end == cur) {
            *lenp = 0;
            *nextp = NULL;
            return 0;
        }
        *nextp = NULL;
    } else {
        if (end == cur) {
            return EINVAL;
        }
        *nextp = end + 1;
    }

    // Figure out the length of the label after escapes have been converted.
    tlen = 0;
    for (s = cur; s < end; s++) {
        if (*s == '\\') {
            if (s + 4 <= end) {
                tlen++;
                s += 3;
            } else {
                tlen++;
            }
        } else {
            tlen++;
        }
    }

    // Is there no space?

    if (tlen >= max) {
        return ENOBUFS;
    }

    // Is the label too long?
    if (end - cur > DNS_MAX_LABEL_SIZE) {
        return ENAMETOOLONG;
    }

    // Store the label length
    *lenp = (uint8_t)(tlen);

    // Store the label.
    t = buf;
    for (s = cur; s < end; s++) {
        if (*s == '\\') {
            if (s + 4 <= end) {
                int v0 = s[1] - '0';
                int v1 = s[2] - '0';
                int v2 = s[3] - '0';
                int val = v0 * 100 + v1 * 10 + v2;
                if (val < 255) {
                    *t++ = (uint8_t)val;
                    s += 3;
                } else {
                    return EINVAL;
                }
            } else {
                return EINVAL;
            }
        } else {
            *t++ = (uint8_t)*s;
        }
    }
    return 0;
}

// Convert a name to wire format.   Does not store the root label (0) at the end.   Does not support binary labels.
void
dns_name_to_wire_(dns_name_pointer_t *NULLABLE r_pointer, dns_towire_state_t *NONNULL txn,
                  const char *NONNULL name, int line)
{
    const char *next, *cur;
    int status;
    dns_name_pointer_t np;

    if (!txn->error) {
        memset(&np, 0, sizeof np);
        np.message_start = (uint8_t *)txn->message;
        np.name_start = txn->p;

        cur = name;
        do {
            // Note that nothing is stored through txn->p until dns_name_parse has verified that
            // there is space in the buffer for the label as well as the length.
            status = dns_parse_label(cur, &next, txn->p, txn->p + 1, txn->lim - txn->p - 1);
            if (status) {
                if (status == ENOBUFS) {
                    txn->truncated = true;
                }
                txn->error = (unsigned)status;
                txn->line = line;
                return;
            }

            // Don't use the root label if it was parsed.
            if (*txn->p != 0) {
                np.num_labels++;
                np.length += 1 + *txn->p;
                txn->p = txn->p + *txn->p + 1;
                cur = next;
            }
        } while (next != NULL);

        if (np.length > DNS_MAX_NAME_SIZE) {
            txn->error = ENAMETOOLONG;
            txn->line = line;
            return;
        }
        if (r_pointer != NULL) {
            *r_pointer = np;
        }
    }
}

// Like dns_name_to_wire, but includes the root label at the end.
void
dns_full_name_to_wire_(dns_name_pointer_t *NULLABLE r_pointer, dns_towire_state_t *NONNULL txn,
                       const char *NONNULL name, int line)
{
    dns_name_pointer_t np;
    if (!txn->error) {
        memset(&np, 0, sizeof np);
        dns_name_to_wire(&np, txn, name);
        if (!txn->error) {
            if (txn->p + 1 >= txn->lim) {
                txn->error = ENOBUFS;
                txn->truncated = true;
                txn->line = line;
                return;
            }
            *txn->p++ = 0;
            np.num_labels++;
            np.length += 1;
            if (np.length > DNS_MAX_NAME_SIZE) {
                txn->error = ENAMETOOLONG;
                txn->line = line;
                return;
            }
            if (r_pointer) {
                *r_pointer = np;
            }
        }
    }
}

// Store a pointer to a name that's already in the message.
void
dns_pointer_to_wire_(dns_name_pointer_t *NULLABLE r_pointer, dns_towire_state_t *NONNULL txn,
                     dns_name_pointer_t *NONNULL pointer, int line)
{
    if (!txn->error) {
        uint16_t offset = (uint16_t)(pointer->name_start - pointer->message_start);
        if (offset > DNS_MAX_POINTER) {
            txn->error = ETOOMANYREFS;
            txn->line = line;
            return;
        }
        if (txn->p + 2 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        *txn->p++ = 0xc0 | (offset >> 8);
        *txn->p++ = offset & 0xff;
        if (r_pointer) {
            r_pointer->num_labels += pointer->num_labels;
            r_pointer->length += pointer->length + 1;
            if (r_pointer->length > DNS_MAX_NAME_SIZE) {
                txn->error = ENAMETOOLONG;
                txn->line = line;
                return;
            }
        }
    }
}

void
dns_u8_to_wire_(dns_towire_state_t *NONNULL txn, uint8_t val, int line)
{
    if (!txn->error) {
        if (txn->p + 1 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        *txn->p++ = val;
    }
}

// Store a 16-bit integer in network byte order
void
dns_u16_to_wire_(dns_towire_state_t *NONNULL txn, uint16_t val, int line)
{
    if (!txn->error) {
        if (txn->p + 2 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        *txn->p++ = val >> 8;
        *txn->p++ = val & 0xff;
    }
}

void
dns_u32_to_wire_(dns_towire_state_t *NONNULL txn, uint32_t val, int line)
{
    if (!txn->error) {
        if (txn->p + 4 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        *txn->p++ = val >> 24;
        *txn->p++ = (val >> 16) & 0xff;
        *txn->p++ = (val >> 8) & 0xff;
        *txn->p++ = val & 0xff;
    }
}

void
dns_u64_to_wire_(dns_towire_state_t *NONNULL txn, uint64_t val, int line)
{
    if (!txn->error) {
        if (txn->p + 8 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        *txn->p++ = val >> 56;
        *txn->p++ = (val >> 48) & 0xff;
        *txn->p++ = (val >> 40) & 0xff;
        *txn->p++ = (val >> 32) & 0xff;
        *txn->p++ = (val >> 24) & 0xff;
        *txn->p++ = (val >> 16) & 0xff;
        *txn->p++ = (val >> 8) & 0xff;
        *txn->p++ = val & 0xff;
    }
}

void
dns_ttl_to_wire_(dns_towire_state_t *NONNULL txn, int32_t val, int line)
{
    if (!txn->error) {
        dns_u32_to_wire_(txn, (uint32_t)val, line);
    }
}

void
dns_rdlength_begin_(dns_towire_state_t *NONNULL txn, int line)
{
    if (!txn->error) {
        if (txn->p + 2 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        if (txn->p_rdlength != NULL) {
            txn->error = EINVAL;
            txn->line = line;
            return;
        }
        txn->p_rdlength = txn->p;
        txn->p += 2;
    }
}

void
dns_rdlength_end_(dns_towire_state_t *NONNULL txn, int line)
{
    ssize_t rdlength;
    if (!txn->error) {
        if (txn->p_rdlength == NULL) {
            txn->error = EINVAL;
            txn->line = line;
            return;
        }
        rdlength = txn->p - txn->p_rdlength - 2;
        txn->p_rdlength[0] = (uint8_t)(rdlength >> 8);
        txn->p_rdlength[1] = (uint8_t)(rdlength & 0xff);
        txn->p_rdlength = NULL;
    }
}

#ifndef THREAD_DEVKIT_ADK
void
dns_rdata_a_to_wire_(dns_towire_state_t *NONNULL txn, const char *NONNULL ip_address, int line)
{
    if (!txn->error) {
        if (txn->p + 4 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        if (!inet_pton(AF_INET, ip_address, txn->p)) {
            txn->error = EINVAL;
            txn->line = line;
            return;
        }
        txn->p += 4;
    }
}

void
dns_rdata_aaaa_to_wire_(dns_towire_state_t *NONNULL txn, const char *NONNULL ip_address, int line)
{
    if (!txn->error) {
        if (txn->p + 16 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        if (!inet_pton(AF_INET6, ip_address, txn->p)) {
            txn->error = EINVAL;
            txn->line = line;
            return;
        }
        txn->p += 16;
    }
}
#endif

uint16_t
dns_rdata_key_to_wire_(dns_towire_state_t *NONNULL txn, unsigned key_type, unsigned name_type,
                       uint8_t signatory, srp_key_t *key, int line)
{
    size_t key_len = srp_pubkey_length(key), copied_len;
    uint8_t *rdata = txn->p;
    uint32_t key_tag;
    int i;
    ssize_t rdlen;

    if (!txn->error) {
        if (key_type > 3 || name_type > 3 || signatory > 15) {
            txn->error = EINVAL;
            txn->line = line;
            return 0;
        }
        if (txn->p + key_len + 4 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return 0;
        }
        *txn->p++ = (uint8_t)((key_type << 6) | name_type);
        *txn->p++ = signatory;
        *txn->p++ = 3; // protocol type is always 3
        *txn->p++ = srp_key_algorithm(key);
        copied_len = srp_pubkey_copy(txn->p, key_len, key);
        if (copied_len == 0) {
            txn->error = EINVAL;
            txn->line = line;
            return 0;
        }
        txn->p += key_len;
    }
    rdlen = txn->p - rdata;

    // Compute the key tag
    key_tag = 0;
    for (i = 0; i < rdlen; i++) {
        key_tag += (i & 1) ? rdata[i] : (uint16_t)(rdata[i] << 8);
    }
    key_tag += (key_tag >> 16) & 0xFFFF;
    return (uint16_t)(key_tag & 0xFFFF);
}

void
dns_rdata_txt_to_wire_(dns_towire_state_t *NONNULL txn, const char *NONNULL txt_record, int line)
{
    if (!txn->error) {
        size_t len = strlen(txt_record);
        if (txn->p + len + 1 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        if (len > 255) {
            txn->error = ENAMETOOLONG;
            txn->line = line;
            return;
        }
        *txn->p++ = (uint8_t)len;
        memcpy(txn->p, txt_record, len);
        txn->p += len;
    }
}

void
dns_rdata_raw_data_to_wire_(dns_towire_state_t *NONNULL txn, const void *NONNULL raw_data, size_t length, int line)
{
    if (!txn->error) {
        if (txn->p + length > txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        memcpy(txn->p, raw_data, length);
        txn->p += length;
    }
}

void
dns_edns0_header_to_wire_(dns_towire_state_t *NONNULL txn, uint16_t mtu, uint8_t xrcode, uint8_t version, bool DO, int line)
{
    if (!txn->error) {
        if (txn->p + 9 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        *txn->p++ = 0; // root label
        dns_u16_to_wire(txn, dns_rrtype_opt);
        dns_u16_to_wire(txn, mtu);
        *txn->p++ = xrcode;
        *txn->p++ = version;
        *txn->p++ = DO ? 1 << 7 : 0; // flags (usb)
        *txn->p++ = 0;       // flags (lsb, mbz)
    }
}

void
dns_edns0_option_begin_(dns_towire_state_t *NONNULL txn, int line)
{
    if (!txn->error) {
        if (txn->p + 2 >= txn->lim) {
            txn->error = ENOBUFS;
            txn->truncated = true;
            txn->line = line;
            return;
        }
        if (txn->p_opt != NULL) {
            txn->error = EINVAL;
            txn->line = line;
            return;
        }
        txn->p_opt = txn->p;
        txn->p += 2;
    }
}

void
dns_edns0_option_end_(dns_towire_state_t *NONNULL txn, int line)
{
    ssize_t opt_length;
    if (!txn->error) {
        if (txn->p_opt == NULL) {
            txn->error = EINVAL;
            txn->line = line;
            return;
        }
        opt_length = txn->p - txn->p_opt - 2;
        txn->p_opt[0] = (uint8_t)(opt_length >> 8);
        txn->p_opt[1] = opt_length & 0xff;
        txn->p_opt = NULL;
    }
}

void
dns_sig0_signature_to_wire_(dns_towire_state_t *NONNULL txn, srp_key_t *key, uint16_t key_tag,
                            dns_name_pointer_t *NONNULL signer, const char *NONNULL signer_hostname,
                            const char *NONNULL signer_domain, uint32_t timenow, int line)
{
    size_t siglen = srp_signature_length(key);
    uint8_t *start, *p_signer, *p_signature, *rrstart = txn->p;

    // 1 name (root)
    // 2 type (SIG)
    // 2 class (255) ANY
    // 4 TTL (0)
    // 18 SIG RDATA up to signer name
    // 2 signer name (always a pointer)
    // 29 bytes so far
    // signature data (depends on algorithm, e.g. 64 for ECDSASHA256)
    // so e.g. 93 bytes total

    if (!txn->error) {
        dns_u8_to_wire(txn, 0);	// root label
        dns_u16_to_wire(txn, dns_rrtype_sig);
        dns_u16_to_wire(txn, dns_qclass_any); // class
        dns_ttl_to_wire(txn, 0); // SIG RR TTL
        dns_rdlength_begin(txn);
        start = txn->p;
        dns_u16_to_wire(txn, 0); // type = 0 for transaction signature
        dns_u8_to_wire(txn, srp_key_algorithm(key));
        dns_u8_to_wire(txn, 0); // labels field doesn't apply for transaction signature
        dns_ttl_to_wire(txn, 0); // original ttl doesn't apply
        // If timenow is <300, it's either just after the epoch, or the caller doesn't know what time it is.
        if (timenow < 300) {
            dns_u32_to_wire(txn, 0); // Indicate that we have no clock: set expiry and inception times to zero
            dns_u32_to_wire(txn, 0);
        } else {
            dns_u32_to_wire(txn, timenow + 300); // signature expiration time is five minutes from now
            dns_u32_to_wire(txn, timenow - 300); // signature inception time, five minutes in the past
        }
        dns_u16_to_wire(txn, key_tag);

        p_signer = txn->p;
        // We store the name in uncompressed form because that's what we have to sign
        if (signer_hostname != NULL) {
            dns_name_to_wire(NULL, txn, signer_hostname);
        }
        dns_full_name_to_wire(NULL, txn, signer_domain);
        // And that means we're going to have to copy the signature back earlier in the packet.
        p_signature = txn->p;

        // Sign the message, signature RRDATA (less signature) first.
        if (!srp_sign(txn->p, siglen, (uint8_t *)txn->message, (size_t)(rrstart - (uint8_t *)txn->message),
                      start, (size_t)(txn->p - start), key)) {
            txn->error = true;
            txn->line = __LINE__;
        } else {
            // Now that it's signed, back up and store the pointer to the name, because we're trying
            // to be as compact as possible.
            txn->p = p_signer;
            dns_pointer_to_wire(NULL, txn, signer); // Pointer to the owner name the key is attached to
            // And move the signature earlier in the packet.
            memmove(txn->p, p_signature, siglen);

            txn->p += siglen;
            dns_rdlength_end(txn);
        }

        if (txn->error) {
            txn->outer_line = line;
        }
    }
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
