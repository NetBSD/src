/*
	Copyright (c) 2020 Apple Inc. All rights reserved.
*/

#ifndef	__DNSServerDNSSEC_h
#define	__DNSServerDNSSEC_h

#include <CoreUtils/CoreUtils.h>

CU_ASSUME_NONNULL_BEGIN

__BEGIN_DECLS

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief	Zone Label Argument Limits
*/

#define kZoneLabelIndexArgMin		1
#define kZoneLabelIndexArgMax		3

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief	Reference to a DNSKeyInfo object.
*/
typedef const union DNSKeyInfo *		DNSKeyInfoRef;

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Gets a constant DNSKeyInfo object, which represents a DNSSEC DNS key.
	
	@param		inAlgorithm		The desired DNSKeyInfo object's DNSSEC algorithm number.
	@param		inIndex			The desired DNSKeyInfo object's index number.
	@param		inGetZSK		If true, gets a zone-signing key. Otherwise a key-signing key.
	
	@result		A reference to the DNSKeyInfo object if it exists, otherwise, NULL.
*/
DNSKeyInfoRef _Nullable	GetDNSKeyInfoEx( uint32_t inAlgorithm, uint32_t inIndex, Boolean inGetZSK );
#define GetDNSKeyInfoKSK( ALGORITHM, INDEX )		GetDNSKeyInfoEx( ALGORITHM, INDEX, false )
#define GetDNSKeyInfoZSK( ALGORITHM, INDEX )		GetDNSKeyInfoEx( ALGORITHM, INDEX, true )

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Gets a DNSKeyInfo object's DNSSEC algorithm number.
	
	@param		inKeyInfo		The DNSKeyInfo object.
	
	@result		The DNSSEC algorithm number.
	
	@discussion	See <https://www.iana.org/assignments/dns-sec-alg-numbers/dns-sec-alg-numbers.xhtml#dns-sec-alg-numbers-1>.
*/
uint8_t	DNSKeyInfoGetAlgorithm( DNSKeyInfoRef inKeyInfo );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Gets a pointer to a DNSKeyInfo object's DNSKEY record data.
	
	@param		inKeyInfo		The DNSKeyInfo object.
	
	@result		The DNSKEY record data in wire format. See <https://tools.ietf.org/html/rfc4034#section-2.1>.
	
	@discussion	Use DNSKeyInfoGetRDataLen() to get the record data's length.
*/
const uint8_t *	DNSKeyInfoGetRDataPtr( DNSKeyInfoRef inKeyInfo );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Gets the length of a DNSKeyInfo object's DNSKEY record data.
	
	@param		inKeyInfo		The DNSKeyInfo object.
	
	@result		The length of the record data.
*/
uint16_t	DNSKeyInfoGetRDataLen( DNSKeyInfoRef inKeyInfo );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Gets a pointer to a DNSKeyInfo object's public key.
	
	@param		inKeyInfo		The DNSKeyInfo object.
	
	@result		A pointer to the public key.
	
	@discussion	Use DNSKeyInfoGetPubKeyLen() to get the public key's length.
*/
const uint8_t *	_Nullable DNSKeyInfoGetPubKeyPtr( DNSKeyInfoRef inKeyInfo );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Gets the length of a DNSKeyInfo object's public key.
	
	@param		inKeyInfo		The DNSKeyInfo object.
	
	@result		The length of the public key.
*/
size_t	DNSKeyInfoGetPubKeyLen( DNSKeyInfoRef inKeyInfo );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Gets the DNSSEC key tag of DNSKeyInfo objects' DNSKEY record data.
	
	@param		inKeyInfo		The DNSKeyInfo object.
	
	@result		The DNSSEC key tag.
*/
uint16_t	DNSKeyInfoGetKeyTag( DNSKeyInfoRef inKeyInfo );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@defined	kDNSServerSignatureLengthMax
	
	@discussion	The maximum length of a DNSSEC signature for DNSSEC algorithms currently implemented by the test DNS server.
*/
#define kDNSServerSignatureLengthMax		256

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Signs a message using a DNSKeyInfo object's secret key.
	
	@param		inKeyInfo			The DNSKeyInfo object.
	@param		inMsgPtr			Pointer to the message to sign.
	@param		inMsgLen			Length, in bytes, of the message to sign.
	@param		outSignature		Buffer to which to write the signature.
	@param		outSignatureLen		Pointer of variable to get set to the signature's length.
	
	@result		Returns true if the message was able to be signed, otherwise, returns false.
*/
Boolean
	DNSKeyInfoSign(
		DNSKeyInfoRef	inKeyInfo,
		const uint8_t *	inMsgPtr,
		size_t			inMsgLen,
		uint8_t			outSignature[ STATIC_PARAM kDNSServerSignatureLengthMax ],
		size_t *		outSignatureLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Verifies a signature using a DNSKeyInfo object's public key.
	
	@param		inKeyInfo			The DNSKeyInfo object.
	@param		inMsgPtr			Pointer to the message that was signed.
	@param		inMsgLen			Length, in bytes, of the message that was signed.
	@param		inSignaturePtr		Pointer to the supposed signature.
	@param		inSignatureLen		Length, in bytes, of the supposed signature.
	
	@result		Returns true if the signature was verified, otherwise, returns false.
*/
Boolean
	DNSKeyInfoVerify(
		DNSKeyInfoRef	inKeyInfo,
		const uint8_t *	inMsgPtr,
		size_t			inMsgLen,
		const uint8_t *	inSignaturePtr,
		size_t			inSignatureLen );

//---------------------------------------------------------------------------------------------------------------------------
/*!	@brief		Gets a short description of a DNSKeyInfo object's DNSSEC algorithm.
	
	@param		inKeyInfo		The DNSKeyInfo object.
	
	@result		The description as a UTF-8 C string.
*/
const char *	DNSKeyInfoGetAlgorithmDescription( DNSKeyInfoRef inKeyInfo );

__END_DECLS

CU_ASSUME_NONNULL_END

#endif	// __DNSServerDNSSEC_h
