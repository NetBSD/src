/*
 * Copyright (c) 2003-2024 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1.  Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of its
 *     contributors may be used to endorse or promote products derived from this
 *     software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dnssd_ipc.h"

#if defined(_WIN32)

#include <stdint.h>

char *win32_strerror(int inErrorCode)
{
    static char buffer[1024];
    DWORD n;
    memset(buffer, 0, sizeof(buffer));
    n = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        (DWORD) inErrorCode,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        buffer,
        sizeof(buffer),
        NULL);
    if (n > 0)
    {
        // Remove any trailing CR's or LF's since some messages have them.
        while ((n > 0) && isspace(((unsigned char *) buffer)[n - 1]))
            buffer[--n] = '\0';
    }
    return buffer;
}

#endif

#include "mdns_strict.h"

static uint8_t *_write_big32(uint8_t *ptr, const uint32_t u32)
{
    *ptr++ = (uint8_t)((u32 >> 24) & 0xFF);
    *ptr++ = (uint8_t)((u32 >> 16) & 0xFF);
    *ptr++ = (uint8_t)((u32 >>  8) & 0xFF);
    *ptr++ = (uint8_t)( u32        & 0xFF);
    return ptr;
}

void put_uint32(const uint32_t u32, uint8_t **const ptr)
{
    *ptr = _write_big32(*ptr, u32);
}

#define _assign_null_safe(PTR, VALUE) \
    do \
    { \
        if ((PTR)) \
        { \
            *(PTR) = (VALUE); \
        } \
    } while (0)

static uint32_t _read_big32(const uint8_t *ptr, const uint8_t **const out_end)
{
    uint32_t u32 = 0;
    u32 |= ((uint32_t)*ptr++) << 24;
    u32 |= ((uint32_t)*ptr++) << 16;
    u32 |= ((uint32_t)*ptr++) <<  8;
    u32 |= ((uint32_t)*ptr++);
    _assign_null_safe(out_end, ptr);
    return u32;
}

uint32_t get_uint32(const uint8_t **const ptr, const uint8_t *const end)
{
    if (!*ptr || *ptr + sizeof(uint32_t) > end)
    {
        *ptr = NULL;
        return(0);
    }
    else
    {
        return _read_big32(*ptr, ptr);
    }
}

static uint8_t *_write_big16(uint8_t *ptr, const uint16_t u16)
{
    *ptr++ = (uint8_t)((u16 >> 8) & 0xFF);
    *ptr++ = (uint8_t)( u16       & 0xFF);
    return ptr;
}

void put_uint16(const uint16_t u16, uint8_t **const ptr)
{
    *ptr = _write_big16(*ptr, u16);
}

static uint16_t _read_big16(const uint8_t *ptr, const uint8_t **const out_end)
{
    uint16_t u16 = 0;
    u16 |= ((uint16_t)*ptr++) << 8;
    u16 |= ((uint16_t)*ptr++);
    _assign_null_safe(out_end, ptr);
    return u16;
}

uint16_t get_uint16(const uint8_t **ptr, const uint8_t *const end)
{
    if (!*ptr || *ptr + sizeof(uint16_t) > end)
    {
        *ptr = NULL;
        return(0);
    }
    else
    {
        return _read_big16(*ptr, ptr);
    }
}

int put_string(const char *str, uint8_t **const ptr)
{
    size_t len;
    if (!str) str = "";
    len = strlen(str) + 1;
    memcpy(*ptr, str, len);
    *ptr += len;
    return 0;
}

int get_string(const uint8_t **const ptr, const uint8_t *const end, char *buffer, size_t buflen)
{
    if (!*ptr)
    {
        *buffer = 0;
        return(-1);
    }
    else
    {
        const char *const lim = buffer + buflen;    // Calculate limit
        while (*ptr < end && buffer < lim)
        {
            const uint8_t c = *(*ptr)++;
            *buffer++ = (char)c;
            if (c == 0) return(0);      // Success
        }
        if (buffer == lim) buffer--;
        *buffer = 0;                    // Failed, so terminate string,
        *ptr = NULL;                    // clear pointer,
        return(-1);                     // and return failure indication
    }
}

void put_rdata(const size_t rdlen, const uint8_t *const rdata, uint8_t **const ptr)
{
    memcpy(*ptr, rdata, rdlen);
    *ptr += rdlen;
}

const uint8_t *get_rdata(const uint8_t **const ptr, const uint8_t *const end, int rdlen)
{
    if (!*ptr || *ptr + rdlen > end)
    {
        *ptr = NULL;
        return(0);
    }
    else
    {
        const uint8_t *const rd = *ptr;
        *ptr += rdlen;
        return rd;
    }
}

#define IPC_TLV16_OVERHEAD_LENGTH (2 + 2) // 2 bytes for 16-bit type + 2 bytes for 16-bit length

size_t get_required_tlv_length(const uint16_t value_length)
{
    return (IPC_TLV16_OVERHEAD_LENGTH + value_length);
}

size_t get_required_tlv_string_length(const char *str_value)
{
    return (IPC_TLV16_OVERHEAD_LENGTH + strlen(str_value) + 1);
}

size_t get_required_tlv_uint8_length(void)
{
    return (IPC_TLV16_OVERHEAD_LENGTH + 1);
}

size_t get_required_tlv_uint32_length(void)
{
    return (IPC_TLV16_OVERHEAD_LENGTH + 4);
}

static size_t _tlv16_set(uint8_t *const dst, const uint8_t *const limit, const uint16_t type, const uint16_t length,
    const uint8_t *const value, uint8_t **const out_end)
{
    uint8_t *ptr = dst;
    const size_t required_len = IPC_TLV16_OVERHEAD_LENGTH + length;
    mdns_require_quiet(ptr, exit);
    mdns_require_quiet(ptr <= limit, exit);
    mdns_require_quiet((size_t)(limit - ptr) >= required_len, exit);

    ptr = _write_big16(ptr, type);
    ptr = _write_big16(ptr, length);
    if (length > 0)
    {
        memcpy(ptr, value, length);
        ptr += length;
    }

exit:
    mdns_assign(out_end, ptr);
    return required_len;
}

size_t put_tlv(const uint16_t type, const uint16_t length, const uint8_t *const value, uint8_t **const ptr,
    const uint8_t *const limit)
{
    uint8_t *const dst = ptr ? *ptr : NULL;
    return _tlv16_set(dst, limit, type, length, value, ptr);
}

void put_tlv_string(const uint16_t type, const char *const str_value, uint8_t **const ptr, const uint8_t *const limit,
    int *const out_error)
{
    int err = -1;
    size_t len = strlen(str_value) + 1;
    if (len <= UINT16_MAX)
    {
        put_tlv(type, (uint16_t)len, (const uint8_t *)str_value, ptr, limit);
        err = 0;
    }
    _assign_null_safe(out_error, err);
}

void put_tlv_uint8(const uint16_t type, const uint8_t u8, uint8_t **const ptr, const uint8_t *const limit)
{
    put_tlv(type, sizeof(u8), &u8, ptr, limit);
}

void put_tlv_uint16(const uint16_t type, const uint16_t u16, uint8_t **const ptr, const uint8_t *const limit)
{
    uint8_t value[2];
    _write_big16(value, u16);
    put_tlv(type, sizeof(value), value, ptr, limit);
}

size_t put_tlv_uint32(const uint16_t type, const uint32_t u32, uint8_t **const ptr, const uint8_t *const limit)
{
    uint8_t value[4];
    _write_big32(value, u32);
    return put_tlv(type, sizeof(value), value, ptr, limit);
}

size_t put_tlv_uuid(const uint16_t type, const uint8_t uuid[MDNS_STATIC_ARRAY_PARAM MDNS_UUID_SIZE], uint8_t **const ptr,
    const uint8_t *const limit)
{
    return put_tlv(type, MDNS_UUID_SIZE, uuid, ptr, limit);
}

static const uint8_t *_tlv16_get_next(const uint8_t *ptr, const uint8_t *const end, uint16_t *const out_type,
    size_t *const out_length, const uint8_t **const out_ptr)
{
    if ((end - ptr) >= IPC_TLV16_OVERHEAD_LENGTH)
    {
        const uint16_t type   = _read_big16(ptr, &ptr);
        const uint16_t length = _read_big16(ptr, &ptr);
        const uint8_t *const value = ptr;
        if ((end - value) >= length)
        {
            ptr += length;
            _assign_null_safe(out_type, type);
            _assign_null_safe(out_length, length);
            _assign_null_safe(out_ptr, ptr);
            return value;
        }
    }
    return NULL;
}

static const uint8_t *_tlv16_get_value(const uint8_t *const start, const uint8_t *const end, const uint16_t desired_type,
    size_t *const out_length, const uint8_t **const out_ptr)
{
    const uint8_t *ptr = start;
    uint16_t type;
    size_t length;
    const uint8_t *value;
    while ((value = _tlv16_get_next(ptr, end, &type, &length, &ptr)) != NULL)
    {
        if (type == desired_type)
        {
            _assign_null_safe(out_length, length);
            _assign_null_safe(out_ptr, ptr);
            break;
        }
    }
    return value;
}

const uint8_t *get_tlv(const uint8_t *const start, const uint8_t *const end, const uint16_t type, size_t *const out_length)
{
    return _tlv16_get_value(start, end, type, out_length, NULL);
}

const char *get_tlv_string(const uint8_t *const start, const uint8_t *const end, const uint16_t type)
{
    const char *str_value = NULL;
    size_t length;
    const char *value = (const char *)_tlv16_get_value(start, end, type, &length, NULL);
    if(strnlen(value, length) == (length - 1))
    {
        str_value = value;
    }
    return str_value;
}

uint32_t get_tlv_uint32(const uint8_t *const start, const uint8_t *const end, const uint16_t type, int *const out_error)
{
    size_t length;
    const uint8_t *value;
    int err = -1;
    uint32_t u32 = 0;
    if ((value = _tlv16_get_value(start, end, type, &length, NULL)) != NULL)
    {
        switch (length)
        {
            case 1:
                u32 = *value;
                err = 0;
                break;
            case 2:
                u32 = _read_big16(value, NULL);
                err = 0;
                break;
            case 4:
                u32 = _read_big32(value, NULL);
                err = 0;
                break;
            default:
                break;
        }
    }
    _assign_null_safe(out_error, err);
    return u32;
}

const uint8_t *get_tlv_uuid(const uint8_t *const start, const uint8_t *const end, const uint16_t type)
{
    const uint8_t *uuid = NULL;
    size_t length = 0;
    const uint8_t *const value = get_tlv(start, end, type, &length);
    mdns_require_quiet(value, exit);
    mdns_require_quiet(length == MDNS_UUID_SIZE, exit);

    uuid = value;

exit:
    return uuid;
}

void ConvertHeaderBytes(ipc_msg_hdr *hdr)
{
    hdr->version   = htonl(hdr->version);
    hdr->datalen   = htonl(hdr->datalen);
    hdr->ipc_flags = htonl(hdr->ipc_flags);
    hdr->op        = htonl(hdr->op );
    hdr->reg_index = htonl(hdr->reg_index);
}
