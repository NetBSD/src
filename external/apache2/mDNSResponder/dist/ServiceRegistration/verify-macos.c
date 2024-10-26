/* verify_macos.c
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
 * DNS SIG(0) signature verification for DNSSD SRP using MacOS Security Framework.
 *
 * Provides functions for generating a public key validating context based on SIG(0) KEY RR data, and
 * validating a signature using a context generated with that public key.  Currently only ECDSASHA256 is
 * supported.
 */

#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <CommonCrypto/CommonDigest.h>
#include <AssertMacros.h>

#include "srp.h"
#define SRP_CRYPTO_MACOS_INTERNAL
#include "dns-msg.h"
#include "srp-crypto.h"

//======================================================================================================================

#if !TARGET_OS_OSX
static SecKeyRef
create_public_sec_key(const dns_rr_t *const key_record);

static CFDataRef
create_data_to_verify(dns_wire_t *const message, const dns_rr_t *const signature);
#endif // !TARGET_OS_OSX

bool
srp_sig0_verify(dns_wire_t *message, dns_rr_t *key, dns_rr_t *signature)
{
#if !TARGET_OS_OSX
    bool valid = false;
    CFErrorRef cf_error = NULL;
    SecKeyRef public_key = NULL;
    CFDataRef data_to_verify_cfdata = NULL;
    CFDataRef sig_to_match_cfdata = NULL;

    // The algorithm in KEY and SIG(0) has to match.
    require_action_quiet(key->data.key.algorithm == signature->data.sig.algorithm, exit,
                         ERROR("KEY algorithm does not match the SIG(0) algorithm - "
                               "KEY algorithm: %u, SIG(0) algorithm: %u",
                               key->data.key.algorithm, signature->data.sig.algorithm));

    // The only supported algorithm now is ECDSA Curve P-256 with SHA-256.
    require_action_quiet(signature->data.sig.algorithm == dnssec_keytype_ecdsa, exit,
                         ERROR("Unsupported KEY algorithm - KEY algorithm: %u", signature->data.sig.algorithm));

    // The key size should always be ECDSA_KEY_SIZE since only ECDSA Curve P-256 with SHA-256 is used now.
    require_action_quiet(key->data.key.len == ECDSA_KEY_SIZE, exit,
                         ERROR("Invalid KEY length - KEY len: %d", key->data.key.len));

    // The signature size should always be ECDSA_SHA256_SIG_SIZE, since only ECDSA Curve P-256 with SHA-256 is used now.
    require_action_quiet(signature->data.sig.len == ECDSA_SHA256_SIG_SIZE, exit,
                         ERROR("Invalid SIG(0) length - SIG(0) length: %d", signature->data.sig.len));

    // Get SecKeyRef given the KEY data.
    public_key = create_public_sec_key(key);
    require_action_quiet(public_key != NULL, exit, ERROR("Failed to create public_key"));

    // Create signature to check.
    sig_to_match_cfdata = CFDataCreate(kCFAllocatorDefault, signature->data.sig.signature, signature->data.sig.len);
    require_action_quiet(sig_to_match_cfdata != NULL, exit,
                         ERROR("CFDataCreate failed when creating sig_to_match_cfdata"));

    // Reconstruct the data (the digest of the raw data) that is signed by SIG(0).
    data_to_verify_cfdata = create_data_to_verify(message, signature);
    require_action_quiet(data_to_verify_cfdata != NULL, exit, ERROR("Failed to data_to_verify_cfdata"));

    // Set the corresponding SecKeyAlgorithm for SecKeyVerifySignature.
    SecKeyAlgorithm verify_algorithm;
    switch (key->data.key.algorithm) {
        case dnssec_keytype_ecdsa:
            verify_algorithm = kSecKeyAlgorithmECDSASignatureDigestRFC4754;
            break;

        default:
            FAULT("Unsupported KEY algorithm - KEY algorithm: %u", key->data.key.algorithm);
            goto exit;
    }

    // Validate the signature.
    valid = SecKeyVerifySignature(public_key, verify_algorithm, data_to_verify_cfdata, sig_to_match_cfdata, &cf_error);
    if (!valid) {
        char errbuf[200];
        CFStringRef error_cfstring = CFErrorCopyDescription(cf_error);
        CFStringGetCString(error_cfstring, errbuf, sizeof(errbuf), kCFStringEncodingUTF8);
        ERROR("SecKeyVerifySignature failed to validate - Error Description: %s", errbuf);
        CFRelease(error_cfstring);
        CFRelease(cf_error);
        cf_error = NULL;
    }

exit:
    if (data_to_verify_cfdata != NULL) {
        CFRelease(data_to_verify_cfdata);
    }
    if (sig_to_match_cfdata != NULL) {
        CFRelease(sig_to_match_cfdata);
    }
    if (public_key != NULL) {
        CFRelease(public_key);
    }

    return valid;
#else
    (void)message;
    (void)key;
    (void)signature;

    return true;
#endif // !TARGET_OS_OSX
}

#if !TARGET_OS_OSX
static SecKeyRef
create_public_sec_key(const dns_rr_t *const key_record)
{
    SecKeyRef key_ref = NULL;
    CFErrorRef cf_error = NULL;
    if (key_record->data.key.algorithm == dnssec_keytype_ecdsa){
        uint8_t four = 4;
        const void *public_key_options_keys[] = {kSecAttrKeyType, kSecAttrKeyClass};
        const void *public_key_options_values[] = {kSecAttrKeyTypeECSECPrimeRandom, kSecAttrKeyClassPublic};
        CFMutableDataRef public_key_cfdata = NULL;
        CFDictionaryRef public_key_options = NULL;

        // The format of the public key data is 04 | X | Y
        public_key_cfdata = CFDataCreateMutable(kCFAllocatorDefault, 1 + key_record->data.key.len);
        require_action_quiet(public_key_cfdata != NULL, ecdsa_exit,
                             ERROR("CFDataCreateMutable failed when creating public key CFMutableDataRef"));
        CFDataAppendBytes(public_key_cfdata, &four, sizeof(four));
        CFDataAppendBytes(public_key_cfdata, key_record->data.key.key, key_record->data.key.len);

        public_key_options = CFDictionaryCreate(kCFAllocatorDefault, public_key_options_keys, public_key_options_values,
                                                sizeof(public_key_options_keys) / sizeof(void *),
                                                &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        require_action_quiet(public_key_options != NULL, ecdsa_exit,
                             ERROR("CFDictionaryCreate failed when creating public key options CFDictionaryRef"));

        key_ref = SecKeyCreateWithData(public_key_cfdata, public_key_options, &cf_error);
        require_action_quiet(key_ref != NULL, ecdsa_exit,
                             ERROR("SecKeyCreateWithData failed when creating public key SecKeyRef"));

    ecdsa_exit:
        if (public_key_cfdata != NULL) {
            CFRelease(public_key_cfdata);
        }
        if (public_key_options != NULL) {
            CFRelease(public_key_options);
        }
    } else {
        key_ref = NULL;
    }

    if (cf_error != NULL) {
        CFRelease(cf_error);
        cf_error = NULL;
    }
    return key_ref;
}

static CFDataRef
create_data_to_verify(dns_wire_t *const message, const dns_rr_t *const signature)
{
    bool encounter_error;
    CFDataRef data_to_verify_cfdata = NULL;
    uint8_t *canonical_signer_name = NULL;
    uint8_t digest[ECDSA_SHA256_HASH_SIZE];

    // Right now, the only supported KEY algorithm is ECDSAP256.
    require_action_quiet(signature->data.sig.algorithm == dnssec_keytype_ecdsa, exit, encounter_error = true;
        FAULT("Unsupported SIG(0) algorithm - SIG(0) algorithm: %u", signature->data.sig.algorithm));

    CC_SHA256_CTX cc_digest_context;
    CC_SHA256_Init(&cc_digest_context);

    // data to be hashed = (SIG(0) RDATA without signature field) + (request - SIG(0)).

    // (SIG(0) RDATA without signature field) = SIG(0) fields without signer name + canonical signer name.
    // Copy SIG(0) fields without signer name.
    CC_SHA256_Update(&cc_digest_context, &message->data[signature->data.sig.start + SIG_HEADERLEN], SIG_STATIC_RDLEN);

    // Construct and copy canonical signer name.
    size_t canonical_signer_name_length = dns_name_wire_length(signature->data.sig.signer);
    // CC_SHA256_Update only accepts CC_LONG type (which is uint32_t) length parameter, so we need to check if the
    // canonical_signer_name_length has invalid value.
    require_action_quiet(canonical_signer_name_length <= MAXDOMNAMELEN, exit, encounter_error = true;
        FAULT("Invalid signer name length - signer name length: %zu", canonical_signer_name_length));

    canonical_signer_name = malloc(canonical_signer_name_length);
    require_action_quiet(canonical_signer_name != NULL, exit, encounter_error = true;
                         ERROR("malloc failed when allocating memory - for canonical_signer_name, len: %lu",
                               canonical_signer_name_length));

    bool convert_fail = !dns_name_to_wire_canonical(canonical_signer_name, canonical_signer_name_length,
                                                    signature->data.sig.signer);
    require_action_quiet(!convert_fail, exit, encounter_error = true;
                         ERROR("Failed to write canonical name - canonical_signer_name_length: %lu",
                               canonical_signer_name_length));

    CC_SHA256_Update(&cc_digest_context, canonical_signer_name, (CC_LONG)canonical_signer_name_length);

    // Copy (request - SIG(0)).
    // The authority response count is before the counts have been adjusted for the inclusion of the SIG(0).
    message->arcount = htons(ntohs(message->arcount) - 1);
    CC_SHA256_Update(&cc_digest_context, (uint8_t *)message, offsetof(dns_wire_t, data) + signature->data.sig.start);
    // Recover the count after copying.
    message->arcount = htons(ntohs(message->arcount) + 1);

    // Generate the final digest.
    CC_SHA256_Final(digest, &cc_digest_context);

    // Create CFDataRef.
    data_to_verify_cfdata = CFDataCreate(kCFAllocatorDefault, digest, sizeof(digest));
    require_action_quiet(data_to_verify_cfdata != NULL, exit, encounter_error = true;
                         ERROR("CFDataCreate failed when creating data_to_verify_cfdata"));

    encounter_error = false;
exit:
    if (canonical_signer_name != NULL) {
        free(canonical_signer_name);
    }
    if (encounter_error) {
        if (data_to_verify_cfdata != NULL) {
            CFRelease(data_to_verify_cfdata);
        }
    }
    return data_to_verify_cfdata;
}
#endif // !TARGET_OS_OSX

//======================================================================================================================

#if SECTRANSFORM_AVAILABLE
// Function to copy out the public key as binary data
void
srp_print_key(srp_key_t *key)
{
    SecTransformRef b64encoder;
    CFDataRef key_data, public_key = NULL;
    CFDataRef encoded;
    const uint8_t *data;
    CFErrorRef error = NULL;

    key_data = SecKeyCopyExternalRepresentation(key->public, &error);
    if (error == NULL) {
        data = CFDataGetBytePtr(key_data);
        public_key = CFDataCreateWithBytesNoCopy(NULL, data + 1,
                                                 CFDataGetLength(key_data) - 1, kCFAllocatorNull);
        if (public_key != NULL) {
            b64encoder = SecEncodeTransformCreate(public_key, &error);
            if (error == NULL) {
                SecTransformSetAttribute(b64encoder, kSecTransformInputAttributeName, key, &error);
                if (error == NULL) {
                    encoded = SecTransformExecute(b64encoder, &error);
                    if (error == NULL) {
                        data = CFDataGetBytePtr(encoded);
                        fputs("thread-demo.default.service.arpa. IN KEY 513 3 13 ", stdout);
                        fwrite(data, CFDataGetLength(encoded), 1, stdout);
                        putc('\n', stdout);
                    }
                    if (encoded != NULL) {
                        CFRelease(encoded);
                    }
                }
            }
            if (b64encoder != NULL) {
                CFRelease(b64encoder);
            }
            CFRelease(public_key);
        }
    }
    if (key_data != NULL) {
        CFRelease(key_data);
    }
}
#endif // SECTRANSFORM_AVAILABLE

// Local Variables:
// mode: C
// tab-width: 4
// c-file-style: "bsd"
// c-basic-offset: 4
// fill-column: 108
// indent-tabs-mode: nil
// End:
