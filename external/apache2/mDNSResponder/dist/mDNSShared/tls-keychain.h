/*
 * Copyright (c) 2021-2024 Apple Inc. All rights reserved.
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
 * This file contains function declarations for tls-keychain.c, which deals
 * with TLS certificate fetching and evaluation.
 */

#ifndef __TLS_KEYCHAIN_H__
#define __TLS_KEYCHAIN_H__

#if __APPLE__
#include <Security/Security.h>
#endif

//======================================================================================================================
// MARK: - Macros

#define SRP_APPLICATION_IDENTIFIER "com.apple.srp-mdns-proxy" // Application identifier for srp-mdns-proxy
#define DNSSD_PROXY_APPLICATION_IDENTIFIER "com.apple.dnssd-proxy" // Application identifier for dnssd-proxy
#define KEYCHAIN_ACCESS_GROUP SRP_APPLICATION_IDENTIFIER // Keychain access group of dnssd-proxy and mDNSResponder
#define DNSSD_PROXY_IDENTITY_NAME SRP_APPLICATION_IDENTIFIER " identity" // The identity name used by dnssd-proxy
#define KEY_ATTRIBUTE_LABEL_PREFIX "Key " // User-visible string put into the attribute label of the private key
#define CERTIFICATE_ATTRIBUTE_LABEL_PREFIX "Certificate " // User-visible string put into the attribute label of the certificate

// The TLS certificate in the keychain will be updated every two weeks(1209600s).
#define TLS_CERTIFICATE_VALID_PERIOD_SECS 1209600
// The TLS certificate that has been created for more than four weeks(2419200s) will be deleted from iCloud keychain no
// matter who creates it.
#define TLS_CERTIFICATE_EXISTENCE_PERIOD_SECS (TLS_CERTIFICATE_VALID_PERIOD_SECS * 2)

//======================================================================================================================
// MARK: - Structures

// Context set in sec_protocol_options_set_verify_block() when trying to setup TLS connection with the server.
typedef struct tls_keychain_context_t tls_keychain_context_t;
struct tls_keychain_context_t {
#if __APPLE__
	sec_protocol_metadata_t _Nonnull metadata;
	sec_trust_t _Nonnull trust_ref;
	bool trusts_alternative_server_certificates;
#else // __APPLE__
	uint8_t not_a_real_member;
#endif // __APPLE__
};

//======================================================================================================================
// MARK: - Functions

/*!
 *	@brief
 *		Get the TLS certificate from the iCloud keychain.
 *
 *	@result
 *		True if the operation succeeds, otherwise, false.
 */
bool
tls_cert_init(void);

/*!
 *	@brief
 *		Specify alternative trusted TLS certificates that can be used to perform trust evaluation.
 *
 *	@param certs
 *		An array of TLS certificates as CFDataRef objects.
 *
 *	@result
 *		kNoErr if the trusted server certificates were successfully updated. Otherwise, a non-zero error code to indicate why
 *		the operation failed.
 *
 *	@discussion
 *		Calling this function for the second time after the first call overrides the previously configured certificates.
 *		Calling this function with NULL clears any alternative certificate that we have trusted before.
 */
OSStatus
tls_cert_set_alternative_trusted_certificates(CFArrayRef _Nullable certs);

/*!
 *	@brief
 *		Given the context, verify if the current TLS certificate should be trusted or not.
 *
 *	@param context
 *		Variables that are required to finish the TLS certificate evaluation.
 *
 *	@result
 *		True if it is trusted, false if not.
 */
bool
tls_cert_evaluate(const tls_keychain_context_t * _Nonnull context);

/*!
 *	@brief
 *		Release the TLS certificate get from iCloud keychain.
 *
 *	@discussion
 *		If the certificate has been fetched from iCloud keychain, it will be released. If not, nothing will happen.
 */
void
tls_cert_dispose(void);

#ifdef __APPLE__

/*!
 *	@brief
 *		Add the identity into keychain.
 *
 *	@param identity
 *		A SecIdentityRef that contains a pair of SecKeyRef private key and SecCertificateRef certificate.
 *
 *	@param uuid
 *		An UUID that will be used to set three properties of the SecIdentityRef containing a pair of SecKeyRef and SecCertificateRef:
 *		1. UUID is used to set SecKeyRef's attribute label: "Key <UUID>".
 *		2. UUID is used to set SecCertificate's attribute label: "Certificate <UUID>".
 *		3. UUID is used to set the common name property in the subjects of SecCertificateRef: "<App Identifier> <UUID>".
 *		All the properties set above are used to match the specific SecItem when manipulating them.
 *
 *	@result
 *		errSecSuccess if the identity is added into keychain successfully, otherwise, an error code to indicate the error.
 *
 *	@discussion
 *		When an identity is added into keychain, the private key part of the identity will remain locally, and it will not be synced to iCloud keychain.
 *		The certificate part of the identity will be synced to iCloud keychain.
 */
OSStatus
keychain_identity_add(const SecIdentityRef _Nonnull identity, const CFStringRef _Nonnull uuid);

/*!
 *	@brief
 *		Retrieve the identity added by <code>keychain_identity_add()</code> on the same device.
 *
 *	@param out_identity
 * 		A pointer to a SecIdentityRef variable that can be used to return the retrieved identity.
 *
 *	@param out_identity_creation_time
 * 		A pointer to a CFAbsoluteTime variable that can be used to return the creation time of the identity.
 *
 *	@result
 * 		errSecSuccess if the identity is found, errSecItemNotFound if the identity is not found, otherwise, an error code to indicate the error.
 *
 *	@discussion
 * 		Note that the identity being returned here is the one that gets added by <code>keychain_identity_add()</code> on the same device.
 * 		Which means the identity can only be returned to its creator by this function.
 */
OSStatus
keychain_identity_copy(CF_RETURNS_RETAINED SecIdentityRef * _Nonnull out_identity,
					   CFAbsoluteTime * _Nonnull out_identity_creation_time);

/*!
 *	@brief
 *		Remove the identity added by <code>keychain_identity_add()</code> on the same device.
 *
 *	@result
 *		errSecSuccess if the identity is removed from keychain, errSecItemNotFound if the identity is not found. Otherwise, an error code to indicate the error.
 *
 *	@discussion
 *		Here the identity is said to be removed from keychain if:
 *		1. The private key of the identity is removed from the local keychain.
 *		2. The certificate of the identity is removed from the iCloud keychain.
 */
OSStatus
keychain_identity_remove(void);

/*!
 *	@brief
 *		Retrieve all the certificates from iCloud keychain that are added by <code>keychain_identity_add()</code>.
 *
 *	@param out_certificates
 *		A pointer to a CFArrayRef variable that can be used to return the retrieved SecCertificateRef array.
 *
 *	@result
 *		errSecSuccess if the certificates on the iCloud keychain are found, errSecItemNotFound if the certificates are not found, otherwise an error code to indicate
 *		the error.
 */
OSStatus
keychain_certificates_copy(CF_RETURNS_RETAINED CFArrayRef * const _Nonnull out_certificates);

/*!
 *	@brief
 *		Removes all the certificates that are more than TLS_CERTIFICATE_EXISTENCE_PERIOD_SECS seconds old.
 *
 *	@result
 *		errSecSuccess if there is any expired certificate that has been removed from keychain, errSecItemNotFound if the identity is not found. Otherwise, an error
 *		code to indicate the error.
 */
OSStatus
keychain_certificates_remove_expired(void);

#endif // __APPLE__

#endif // __TLS_KEYCHAIN_H__
