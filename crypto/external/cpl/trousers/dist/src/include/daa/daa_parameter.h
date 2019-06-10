
/*
 * Licensed Materials - Property of IBM
 *
 * trousers - An open source TCG Software Stack
 *
 * (C) Copyright International Business Machines Corp. 2006
 *
 */

#ifndef DAA_PARAMETER_H_
#define DAA_PARAMETER_H_

// for message digest
//#include <openssl/evp.h>
#include "trousers/tss.h"
//#include "spi_internal_types.h"
#include "spi_utils.h"

#define DAA_PARAM_TSS_VERSION_LENGTH (4)
static const BYTE DAA_PARAM_TSS_VERSION[] =  { 1, 2, 0, 0 };

#define DAA_PARAM_DEFAULT_CRYPTO_PROVIDER_NAME "BC"

// Name of default hash function
#define DAA_PARAM_MESSAGE_DIGEST_ALGORITHM "SHA1"

//  Name of hash function used independently in TSS
#define DAA_PARAM_MESSAGE_DIGEST_ALGORITHM_TSS "SHA1"

// l_n (bits)
#define DAA_PARAM_SIZE_RSA_MODULUS (2048)

// l_f (bits)
#define DAA_PARAM_SIZE_F_I (104)

// l_q  (2 * SIZE_F_I)
#define DAA_PARAM_SIZE_RHO (208)

// l_e
#define DAA_PARAM_SIZE_EXPONENT_CERTIFICATE (368)

// lPrime_e
#define DAA_PARAM_SIZE_INTERVAL_EXPONENT_CERTIFICATE (120)

// l_zero
#define DAA_PARAM_SAFETY_MARGIN (80)

//  Byte length of TPM message digest (sha-1)
#define DAA_PARAM_LENGTH_MESSAGE_DIGEST (20)

//  Byte length of TSS message digest (sha-256)
#define DAA_PARAM_LENGTH_MESSAGE_DIGEST_TSS (32)

//  l_H depends on the message digest algo
#define DAA_PARAM_SIZE_MESSAGE_DIGEST (160)
// 8 * LENGTH_MESSAGE_DIGEST;

//   l_GAMMA
#define DAA_PARAM_SIZE_MODULUS_GAMMA (1632)

#define DAA_PARAM_SIZE_SPLIT_EXPONENT (1024)

// TPM asym key size (bits)
#define DAA_PARAM_KEY_SIZE (2048)

//  Default RSA public key exponent (Fermat 4)
#define DAA_PARAM_LENGTH_MFG1_ANONYMITY_REVOCATION (25)
// (SIZE_RHO-1)/8;

#define DAA_PARAM_LENGTH_MFG1_GAMMA (214)
// (SIZE_MODULUS_GAMMA + SIZE_SAFETY_MARGIN)/8;

#define DAA_PARAM_SIZE_RND_VALUE_CERTIFICATE (2536)

// (bits)
#define DAA_PARAM_SIZE_RANDOMIZED_ATTRIBUTES (DAA_PARAM_SIZE_F_I+DAA_PARAM_SAFETY_MARGIN+DAA_PARAM_SIZE_MESSAGE_DIGEST)

#define TSS_FLAG_DAA_SIGN_IDENTITY_KEY 0
#define TSS_FLAG_DAA_SIGN_MESSAGE_HASH 1


extern EVP_MD *DAA_PARAM_get_message_digest(void);

extern char *err_string(TSS_RESULT r);

#endif /*DAA_PARAMETER_H_*/
