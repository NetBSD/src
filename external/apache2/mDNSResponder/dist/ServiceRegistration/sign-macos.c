/* sign-macos.c
 *
 * Copyright (c) 2018-2022 Apple Inc. All rights reserved.
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
 * DNS SIG(0) signature generation for DNSSD SRP using mbedtls.
 *
 * Functions required for loading, saving, and generating public/private keypairs, extracting the public key
 * into KEY RR data, and computing signatures.
 *
 * This is the implementation for Mac OS X.
 */


#include <stdio.h>
#include <arpa/inet.h>
#include <sys/random.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <dns_sd.h>

#include "srp.h"
#include "srp-api.h"
#include "dns-msg.h"
#define SRP_CRYPTO_MACOS_INTERNAL
#include "srp-crypto.h"

// Key is stored in an opaque data structure, for mbedtls this is an mbedtls_pk_context.
// Function to read a public key from a KEY record
// Function to validate a signature given some data and a public key (not required on client)

// Function to free a key
void
srp_keypair_free(srp_key_t *key)
{
    free(key);
}

uint16_t
srp_random16(void)
{
    return (uint16_t)(arc4random_uniform(65536));
}

uint32_t
srp_random32(void)
{
    return arc4random();
}

uint64_t
srp_random64(void)
{
    uint64_t ret;
    arc4random_buf(&ret, sizeof(ret));
    return ret;
}

bool
srp_randombytes(uint8_t *dest, size_t num)
{
    arc4random_buf(dest, num);
    return true;
}

static void
srp_sec_error_print(const char *reason, OSStatus status)
{
    const char *utf8 = NULL;
    CFStringRef err = SecCopyErrorMessageString(status, NULL);
    if (err != NULL) {
        utf8 = CFStringGetCStringPtr(err, kCFStringEncodingUTF8);
    }
    if (utf8 != NULL) {
        ERROR(PUB_S_SRP ": " PUB_S_SRP, reason, utf8);
    } else {
        ERROR(PUB_S_SRP ": %d", reason, (int)status);
    }
    if (err != NULL) {
        CFRelease(err);
    }
}

// Function to generate a key
static srp_key_t *
srp_get_key_internal(const char *key_name, bool delete)
{
    srp_key_t *key = NULL;
    long two56 = 256;
    OSStatus status;

    CFMutableDictionaryRef key_parameters = CFDictionaryCreateMutable(NULL, 8,
                                                                      &kCFTypeDictionaryKeyCallBacks,
                                                                      &kCFTypeDictionaryValueCallBacks);
    CFMutableDictionaryRef pubkey_parameters;

    if (key_parameters != NULL) {
        CFDictionaryAddValue(key_parameters, kSecAttrIsPermanent, kCFBooleanTrue);
        CFDictionaryAddValue(key_parameters, kSecAttrKeyType, kSecAttrKeyTypeECSECPrimeRandom);
        CFNumberRef num = CFNumberCreate(NULL, kCFNumberLongType, &two56);
        CFDictionaryAddValue(key_parameters, kSecAttrKeySizeInBits, num);
        CFRelease(num);
        CFStringRef str = CFStringCreateWithCString(NULL, key_name, kCFStringEncodingUTF8);
        CFDictionaryAddValue(key_parameters, kSecAttrLabel, str);
        CFRelease(str);
        CFDictionaryAddValue(key_parameters, kSecReturnRef, kCFBooleanTrue);
        CFDictionaryAddValue(key_parameters, kSecMatchLimit, kSecMatchLimitOne);
        CFDictionaryAddValue(key_parameters, kSecClass, kSecClassKey);
        CFDictionaryAddValue(key_parameters, kSecUseDataProtectionKeychain, kCFBooleanTrue);
        pubkey_parameters = CFDictionaryCreateMutableCopy(NULL, 8, key_parameters);
        if (pubkey_parameters != NULL) {
            CFDictionaryAddValue(key_parameters, kSecAttrKeyClass, kSecAttrKeyClassPrivate);
            CFDictionaryAddValue(pubkey_parameters, kSecAttrKeyClass, kSecAttrKeyClassPublic);
            CFDictionaryAddValue(pubkey_parameters, kSecAttrIsExtractable, kCFBooleanTrue);
            CFDictionaryAddValue(key_parameters, kSecAttrIsExtractable, kCFBooleanTrue);
            CFDictionaryAddValue(pubkey_parameters, kSecAttrAccessible,
                                 kSecAttrAccessibleAfterFirstUnlockThisDeviceOnly);
            if (delete) {
                status = SecItemDelete(key_parameters);
                if (status == errSecSuccess) {
                    status = SecItemDelete(pubkey_parameters);
                    if (status != errSecSuccess) {
                        ERROR("srp_get_key_internal: failed to delete the public key");
                    }
                }
                key = NULL;
            } else {
                key = calloc(1, sizeof(*key));

                if (key != NULL) {
                    CFErrorRef error = NULL;

                    // See if the key is already on the keychain.
                    status = SecItemCopyMatching(key_parameters, (CFTypeRef *)&key->private);
                    if (status == errSecSuccess) {
                        status = SecItemCopyMatching(pubkey_parameters, (CFTypeRef *)&key->public);
                    } else {
                        key->private = SecKeyCreateRandomKey(key_parameters, &error);
                        if (key->private != NULL) {
                            key->public = SecKeyCopyPublicKey(key->private);
                        }
                    }
                    if (key->public == NULL || key->private == NULL) {
                        if (error != NULL) {
                            CFShow(error);
                        } else {
                            srp_sec_error_print("Failed to get key pair", status);
                        }
                        free(key);
                        key = NULL;
                    }
                }
            }
            CFRelease(key_parameters);
            CFRelease(pubkey_parameters);
        }
    }
    (void)key_name;
    (void)delete;
    return key;
}

srp_key_t *
srp_get_key(const char *key_name, void *__unused os_context)
{
    return srp_get_key_internal(key_name, false);
}

// Remove an existing key
int
srp_reset_key(const char *key_name, void *__unused os_context)
{
    srp_get_key_internal(key_name, true);
    return kDNSServiceErr_NoError;
}

// void to get the length of the public key
size_t
srp_pubkey_length(srp_key_t *key)
{
    (void)key;
    return ECDSA_KEY_SIZE;
}

uint8_t
srp_key_algorithm(srp_key_t *key)
{
    (void)key;
    return dnssec_keytype_ecdsa;
}

size_t
srp_signature_length(srp_key_t *key)
{
    (void)key;
    return ECDSA_KEY_SIZE;
}

// Function to copy out the public key as binary data
size_t
srp_pubkey_copy(uint8_t *buf, size_t max, srp_key_t *key)
{
    size_t ret = 0;
    CFErrorRef error = NULL;
    CFDataRef pubkey = SecKeyCopyExternalRepresentation(key->public, &error);
    if (pubkey == NULL) {
        if (error != NULL) {
            CFShow(error);
        } else {
            ERROR("Unknown failure in SecKeyCopyExternalRepresentation");
        }
    } else {
        const uint8_t *bytes = CFDataGetBytePtr(pubkey);
        unsigned long len = (unsigned long)CFDataGetLength(pubkey);

        // Should be 04 | X | Y
        if (bytes[0] != 4) {
            ERROR("Unexpected preamble to public key: %d", bytes[0]);
        } else if (len - 1 > max) {
            ERROR("Not enough room for public key in buffer: %ld > %zd", len - 1, max);
        } else if (len - 1 != ECDSA_KEY_SIZE) {
            ERROR("Unexpected key size %ld", len - 1);
        } else {
            memcpy(buf, bytes + 1, len - 1);
            ret = ECDSA_KEY_SIZE;
        }
        CFRelease(pubkey);
    }
    return ret;
}

// Function to generate a signature given some data and a private key
int
srp_sign(uint8_t *output, size_t max, uint8_t *message, size_t msglen,
         uint8_t *rr, size_t rdlen, srp_key_t *key)
{
    int ret = 0;
    CFMutableDataRef payload = NULL;
    CFDataRef signature = NULL;
    CFErrorRef error = NULL;
    const uint8_t *bytes;
    unsigned long len;

    if (max < ECDSA_SHA256_SIG_SIZE) {
        ERROR("srp_sign: not enough space in output buffer (%lu) for signature (%d).",
              (unsigned long)max, ECDSA_SHA256_SIG_SIZE);
        return 0;
    }

    payload = CFDataCreateMutable(NULL, (CFIndex)(msglen + rdlen));
    if (payload == NULL) {
        ERROR("srp_sign: CFDataCreateMutable failed on length %zd", msglen + rdlen);
        return 0;
    }
    CFDataAppendBytes(payload, rr, (CFIndex)rdlen);
    CFDataAppendBytes(payload, message, (CFIndex)msglen);

    signature = SecKeyCreateSignature(key->private,
                                      kSecKeyAlgorithmECDSASignatureMessageX962SHA256, payload, &error);
    CFRelease(payload);
    if (error != NULL) {
        CFRelease(signature);
        CFShow(error);
        return 0;
    }
    if (signature == NULL) {
        ERROR("No error, but no signature.");
        return 0;
    }

    len = (unsigned long)CFDataGetLength(signature);
    bytes = CFDataGetBytePtr(signature);
    // The buffer is ASN.1 DER encoded as two numbers, so should be 30 <len> 02 <len> <bytes> 02 <len> <bytes>.
    if (len < 8) {
        ERROR("signature is too short to parse: %lu bytes", len);
        goto out;
    }
#define ASN1_SEQUENCE 0x30
    if (bytes[0] != ASN1_SEQUENCE) {
        ERROR("Unexpected ASN.1 type for signature: %x", bytes[0]);
        goto out;
    }
    unsigned len_sequence = bytes[1];
    if (len_sequence != len - 2) { // This is the length of the sequence, which is the remainder of the buffer
        ERROR("Unexpected ASN.1 sequence length %u when %lu bytes remain", len_sequence, len - 2);
        goto out;
    }
    const uint8_t *sequence_start = &bytes[2];
    unsigned sequence_available = len_sequence;
    int index = 0;
    while (sequence_available > 0) {
        if (index > 1) {
            ERROR("Unexpected extra datum in top-level ASN.1 sequence for signature.");
            goto out;
        }
#define ASN1_INTEGER 0x02
        if (sequence_start[0] != ASN1_INTEGER) {
            ERROR("Unexpected ASN.1 type for key half %d: %x", index, bytes[2]);
            goto out;
        }
        unsigned len_half = sequence_start[1];
        if (len_half > sequence_available - 2 || len_half > ECDSA_SHA256_SIG_PART_SIZE + 1) {
            ERROR("key half %d is too long: length %u, sequence length %u, sig part size %d",
                  index, len_half, len_sequence, ECDSA_SHA256_SIG_PART_SIZE);
            goto out;
        }
        // Now that we've bounds-checked this key half, reduce the available space by its length for
        // the next round.
        sequence_available = sequence_available - len_half - 2;

        // The key data are bignums. If the first byte of either bignum is >0x7f, it will include a leading zero
        // to make it unsigned, which we don't need.
        const uint8_t *key_half = sequence_start[2] == 0 ? &sequence_start[3] : &sequence_start[2];
        if (sequence_start[2] == 0) {
            len_half--;
        }

        unsigned long diff = ECDSA_SHA256_SIG_PART_SIZE - len_half;
        uint8_t *output_start = output + index * ECDSA_SHA256_SIG_PART_SIZE;
        if (diff > 0) {
            memset(output_start, 0, diff);
        }
        memcpy(output_start + diff, key_half, len_half);
        sequence_start = key_half + len_half;
        index++;
    }
    ret = ECDSA_SHA256_SIG_PART_SIZE * 2;

    for (int j = 0; j < 2; j++) {
        const uint8_t *buf = j == 0 ? bytes : output;
        char sigbuf[300];
        char *sigbufp = sigbuf;
        for (unsigned i = 0; i < len && (size_t)(sigbufp - sigbuf) < sizeof(sigbuf) - 3; i++) {
            snprintf(sigbufp, 4, "%02x ", buf[i]);
            sigbufp += 3;
        }
        *sigbufp = 0;
        INFO("%s: %s", j == 0 ? "input" : "output", sigbuf);
    }

out:
    if (signature != NULL) {
        CFRelease(signature);
    }
    return ret;
}

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
