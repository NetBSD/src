/* srp-ioloop.c
 *
 * Copyright (c) 2019 Apple Computer, Inc. All rights reserved.
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
 * srp host API implementation for Posix using ioloop primitives.
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <dns_sd.h>
#include <errno.h>
#include <fcntl.h>

#include "srp.h"
#include "srp-api.h"
#include "dns-msg.h"
#include "srp-crypto.h"
#include "ioloop.h"

bool
srp_load_file_data(void *host_context, const char *filename, uint8_t *buffer, uint16_t *length, uint16_t buffer_size)
{
    off_t flen;
    ssize_t len;
    int file;
    (void)host_context;

    file = open(filename, O_RDONLY);
    if (file < 0) {
        ERROR("srp_load_file_data: %s: open: %s", filename, strerror(errno));
        return false;
    }

    // Get the length of the file.
    flen = lseek(file, 0, SEEK_END);
    lseek(file, 0, SEEK_SET);
    if (flen > buffer_size) {
        ERROR("srp_load_file_data: %s: lseek: %s", filename, strerror(errno));
        close(file);
        return false;
    }
    len = read(file, buffer, (size_t)flen); // Note: flen always positive, no loss of precision.
    if (len < 0 || len != flen) {
        if (len < 0) {
            ERROR("srp_load_file_data: %s: read: %s", filename, strerror(errno));
        } else {
            ERROR("srp_load_file_data: %s: short read %d out of %d", filename, (int)len, (int)flen);
        }
        close(file);
        return false;
    }
    close(file);
    *length = (uint16_t)len;
    return true;
}

bool
srp_store_file_data(void *host_context, const char *filename, uint8_t *buffer, uint16_t length)
{
    ssize_t len;
    int file;
    (void)host_context;
    file = open(filename, O_WRONLY | O_CREAT, 0600);
    if (file < 0) {
        ERROR("srp_store_file_data: %s: %s", filename, strerror(errno));
        return false;
   }
    len = write(file, buffer, length);
    if (len < 0 || len != length) {
        if (len < 0) {
            ERROR("srp_store_file_data: " PUB_S_SRP ": " PUB_S_SRP, filename, strerror(errno));
        } else {
            ERROR("srp_store_file_data: short write %d out of %d on file " PUB_S_SRP, (int)len, (int)length, filename);
        }
        unlink(filename);
        close(file);
        return false;
    }
    close(file);
    return true;
}


bool
srp_get_last_server(uint16_t *NONNULL rrtype, uint8_t *NONNULL rdata, uint16_t rdlim,
                    uint8_t *NONNULL port, void *NULLABLE host_context)
{
    uint8_t buffer[22];
    unsigned offset = 0;
    uint16_t length;
    uint16_t rdlength;

    if (!srp_load_file_data(host_context, "/var/run/srp-last-server", buffer, &length, sizeof(buffer))) {
        return false;
    }
    if (length < 10) { // rrtype + rdlength + ipv4 address + port
        ERROR("srp_get_last_server: stored server data is too short: %d", length);
        return false;
    }
    *rrtype = (((uint16_t)buffer[offset]) << 8) | buffer[offset + 1];
    offset += 2;
    rdlength = (((uint16_t)buffer[offset]) << 8) | buffer[offset + 1];
    offset += 2;
    if ((*rrtype == dns_rrtype_a && rdlength != 4) || (*rrtype == dns_rrtype_aaaa && rdlength != 16)) {
        ERROR("srp_get_last_server: invalid rdlength %d for %s record",
              rdlength, *rrtype == dns_rrtype_a ? "A" : "AAAA");
        return false;
    }
    if (length < rdlength + 6) { // rrtype + rdlength + address + port
        ERROR("srp_get_last_server: stored server data length %d is too short", length);
        return false;
    }
    if (rdlength > rdlim) {
        ERROR("srp_get_last_server: no space for %s data in provided buffer size %d",
              *rrtype == dns_rrtype_a ? "A" : "AAAA", rdlim);
        return false;
    }
    memcpy(rdata, &buffer[offset], rdlength);
    offset += rdlength;
    memcpy(port, &buffer[offset], 2);
    return true;
}

bool
srp_save_last_server(uint16_t rrtype, uint8_t *NONNULL rdata, uint16_t rdlength,
                     uint8_t *NONNULL port, void *NULLABLE host_context)
{
    dns_towire_state_t towire;
    uint8_t buffer[24];
    size_t length;
    memset(&towire, 0, sizeof(towire));
    towire.p = buffer;
    towire.lim = towire.p + sizeof(buffer);

    if (rdlength != 4 && rdlength != 16) {
        ERROR("srp_save_last_server: invalid IP address length %d", rdlength);
        return false;
    }
    dns_u16_to_wire(&towire, rrtype);
    dns_u16_to_wire(&towire, rdlength);
    dns_rdata_raw_data_to_wire(&towire, rdata, rdlength);
    dns_rdata_raw_data_to_wire(&towire, port, 2);

    if (towire.error) {
        ERROR("srp_save_last_server: " PUB_S_SRP " at %d (%p:%p:%p) while constructing output buffer",
              strerror(towire.error), towire.line, towire.p, towire.lim, buffer);
        return false;
    }

    length = towire.p - buffer;
    if (!srp_store_file_data(host_context, "/var/run/srp-last-server", buffer, length)) {
        return false;
    }
    return true;
}

#ifdef SRP_CRYPTO_MBEDTLS
int
srp_load_key_data(void *host_context, const char *key_name, uint8_t *buffer, uint16_t *length, uint16_t buffer_size)
{
    if (srp_load_file_data(host_context, key_name, buffer, length, buffer_size)) {
        return kDNSServiceErr_NoError;
    }
    return kDNSServiceErr_NoSuchKey;
}

int
srp_store_key_data(void *host_context, const char *key_name, uint8_t *buffer, uint16_t length)
{
    if (!srp_store_file_data(host_context, key_name, buffer, length)) {
        return kDNSServiceErr_Unknown;
    }
    return kDNSServiceErr_NoError;
}

int
srp_remove_key_file(void *host_context, const char *key_name)
{
	if (unlink(key_name) < 0) {
		return kDNSServiceErr_Unknown;
	}
	return kDNSServiceErr_NoError;
}
#endif // SRP_CRYPTO_MBEDTLS_INTERNAL
