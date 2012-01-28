/*++

TSS structures for TSS

*/

#ifndef __TSS_STRUCTS_H__
#define __TSS_STRUCTS_H__

#include <tss/platform.h>
#include <tss/tss_typedef.h>
#include <tss/tpm.h>

typedef struct tdTSS_VERSION 
{
    BYTE   bMajor;
    BYTE   bMinor;
    BYTE   bRevMajor;
    BYTE   bRevMinor;
} TSS_VERSION;

typedef struct tdTSS_PCR_EVENT 
{
    TSS_VERSION   versionInfo;
    UINT32        ulPcrIndex;
    TSS_EVENTTYPE eventType;
    UINT32        ulPcrValueLength;
#ifdef __midl
    [size_is(ulPcrValueLength)] 
#endif
    BYTE*         rgbPcrValue;
    UINT32        ulEventLength;
#ifdef __midl
    [size_is(ulEventLength)] 
#endif 
    BYTE*         rgbEvent;
} TSS_PCR_EVENT;


typedef struct tdTSS_EVENT_CERT
{
    TSS_VERSION       versionInfo;
    UINT32    ulCertificateHashLength;
#ifdef __midl
    [size_is(ulCertificateHashLength)] 
#endif
    BYTE*     rgbCertificateHash;
    UINT32    ulEntityDigestLength;
#ifdef __midl
    [size_is(ulEntityDigestLength)] 
#endif
    BYTE*     rgbentityDigest;
    TSS_BOOL  fDigestChecked;
    TSS_BOOL  fDigestVerified;
    UINT32    ulIssuerLength;
#ifdef __midl
    [size_is(ulIssuerLength)]
#endif
    BYTE*     rgbIssuer;
} TSS_EVENT_CERT;

typedef struct tdTSS_UUID 
{
    UINT32  ulTimeLow;
    UINT16  usTimeMid;
    UINT16  usTimeHigh;
    BYTE   bClockSeqHigh;
    BYTE   bClockSeqLow;
    BYTE   rgbNode[6];
} TSS_UUID;

typedef struct tdTSS_KM_KEYINFO 
{
    TSS_VERSION  versionInfo;
    TSS_UUID     keyUUID;
    TSS_UUID     parentKeyUUID;
    BYTE         bAuthDataUsage;   // whether auth is needed to load child keys
    TSS_BOOL     fIsLoaded;           // TRUE: actually loaded in TPM
    UINT32       ulVendorDataLength;  // may be 0
#ifdef __midl
    [size_is(ulVendorDataLength)]
#endif
    BYTE        *rgbVendorData;       // may be NULL
} TSS_KM_KEYINFO;


typedef struct tdTSS_KM_KEYINFO2
{
    TSS_VERSION  versionInfo;
    TSS_UUID     keyUUID;
    TSS_UUID     parentKeyUUID;
    BYTE         bAuthDataUsage;   // whether auth is needed to load child keys
    TSS_FLAG     persistentStorageType;
    TSS_FLAG     persistentStorageTypeParent;
    TSS_BOOL     fIsLoaded;           // TRUE: actually loaded in TPM
    UINT32       ulVendorDataLength;  // may be 0
#ifdef __midl
    [size_is(ulVendorDataLength)]
#endif
    BYTE        *rgbVendorData;       // may be NULL
} TSS_KM_KEYINFO2;


typedef struct tdTSS_NONCE
{
    BYTE  nonce[TPM_SHA1BASED_NONCE_LEN];
} TSS_NONCE;


typedef struct tdTSS_VALIDATION
{ 
    TSS_VERSION  versionInfo;
    UINT32       ulExternalDataLength;
#ifdef __midl
    [size_is(ulExternalDataLength)]
#endif
    BYTE*        rgbExternalData;
    UINT32       ulDataLength;
#ifdef __midl
    [size_is(ulDataLength)]
#endif
    BYTE*     rgbData;
    UINT32    ulValidationDataLength;
#ifdef __midl
    [size_is(ulValidationDataLength)]
#endif
    BYTE*     rgbValidationData;
} TSS_VALIDATION;


typedef struct tdTSS_CALLBACK
{
    PVOID            callback;
    PVOID            appData;
    TSS_ALGORITHM_ID alg;
} TSS_CALLBACK;


typedef struct tdTSS_DAA_PK
{
    TSS_VERSION versionInfo;
    UINT32      modulusLength;
#ifdef __midl
    [size_is(modulusLength)] 
#endif
    BYTE*       modulus;
    UINT32      capitalSLength;
#ifdef __midl
    [size_is(capitalSLength)] 
#endif
    BYTE*       capitalS;
    UINT32      capitalZLength;
#ifdef __midl
    [size_is(capitalZLength)] 
#endif
    BYTE*       capitalZ;
    UINT32      capitalR0Length;
#ifdef __midl
    [size_is(capitalR0Length)] 
#endif
    BYTE*       capitalR0;
    UINT32      capitalR1Length;
#ifdef __midl
    [size_is(capitalR1Length)] 
#endif
    BYTE*       capitalR1;
    UINT32      gammaLength;
#ifdef __midl
    [size_is(gammaLength)] 
#endif
    BYTE*       gamma;
    UINT32      capitalGammaLength;
#ifdef __midl
    [size_is(capitalGammaLength)] 
#endif
    BYTE*       capitalGamma;
    UINT32      rhoLength;
#ifdef __midl
    [size_is(rhoLength)] 
#endif
    BYTE*       rho;
    UINT32      capitalYLength;         // Length of first dimenstion
    UINT32      capitalYLength2;        // Length of second dimension
#ifdef __midl
    [size_is(capitalYLength,capitalYLength2)] 
#endif
    BYTE**      capitalY;
    UINT32      capitalYPlatformLength;
    UINT32      issuerBaseNameLength;
#ifdef __midl
    [size_is(issuerBaseName)] 
#endif
    BYTE*       issuerBaseName;
    UINT32      numPlatformAttributes;
    UINT32      numIssuerAttributes;
} TSS_DAA_PK;

typedef struct tdTSS_DAA_PK_PROOF
{
    TSS_VERSION versionInfo;
    UINT32      challengeLength;
#ifdef __midl
    [size_is(challengeLength)] 
#endif
    BYTE*       challenge;
    UINT32      responseLength;         // Length of first dimension
    UINT32      responseLength2;        // Length of second dimension
#ifdef __midl
    [size_is(responseLength,responseLength2)] 
#endif
    BYTE**      response;
} TSS_DAA_PK_PROOF;

typedef struct tdTSS_DAA_SK
{
    TSS_VERSION versionInfo;
    UINT32      productPQprimeLength;
#ifdef __midl
    [size_is(productPQprimeLength)] 
#endif
    BYTE*       productPQprime;
} TSS_DAA_SK;


typedef struct tdTSS_DAA_KEY_PAIR
{
    TSS_VERSION versionInfo;
    TSS_DAA_SK  secretKey;
    TSS_DAA_PK  publicKey;
} TSS_DAA_KEY_PAIR;

typedef struct tdTSS_DAA_AR_PK
{
    TSS_VERSION versionInfo;
    UINT32      etaLength;
#ifdef __midl
    [size_is(etaLength)] 
#endif
    BYTE*       eta;
    UINT32      lambda1Length;
#ifdef __midl
    [size_is(lambda1Length)] 
#endif
    BYTE*       lambda1;
    UINT32      lambda2Length;
#ifdef __midl
    [size_is(lambda2Length)] 
#endif
    BYTE*       lambda2;
    UINT32      lambda3Length;
#ifdef __midl
    [size_is(lambda3Length)] 
#endif
    BYTE*       lambda3;
} TSS_DAA_AR_PK;

typedef struct tdTSS_DAA_AR_SK
{
    TSS_VERSION versionInfo;
    UINT32      x0Length;
#ifdef __midl
    [size_is(x0Length)] 
#endif
    BYTE*       x0;
    UINT32      x1Length;
#ifdef __midl
    [size_is(x1Length)] 
#endif
    BYTE*       x1;
    UINT32      x2Length;
#ifdef __midl
    [size_is(x2Length)] 
#endif
    BYTE*       x2;
    UINT32      x3Length;
#ifdef __midl
    [size_is(x3Length)] 
#endif
    BYTE*       x3;
    UINT32      x4Length;
#ifdef __midl
    [size_is(x4Length)] 
#endif
    BYTE*       x4;
    UINT32      x5Length;
#ifdef __midl
    [size_is(x5Length)] 
#endif
    BYTE*       x5;
} TSS_DAA_AR_SK;

typedef struct tdTSS_DAA_AR_KEY_PAIR
{
    TSS_VERSION   versionInfo;
    TSS_DAA_AR_SK secretKey;
    TSS_DAA_AR_PK publicKey;
} TSS_DAA_AR_KEY_PAIR;

typedef struct tdTSS_DAA_CRED_ISSUER
{
    TSS_VERSION versionInfo;
    UINT32      capitalALength;
#ifdef __midl
    [size_is(capitalALength)] 
#endif
    BYTE*       capitalA;
    UINT32      eLength;
#ifdef __midl
    [size_is(eLength)] 
#endif
    BYTE*       e;
    UINT32      vPrimePrimeLength;
#ifdef __midl
    [size_is(vPrimePrimeLength)] 
#endif
    BYTE*       vPrimePrime;
    UINT32      attributesIssuerLength;         // Length of first dimension
    UINT32      attributesIssuerLength2;        // Length of second dimension
#ifdef __midl
    [size_is(attributesIssuerLength,attributesIssuerLength2)] 
#endif
    BYTE**      attributesIssuer;
    UINT32      cPrimeLength;
#ifdef __midl
    [size_is(cPrimeLength)] 
#endif
    BYTE*       cPrime;
    UINT32      sELength;
#ifdef __midl
    [size_is(sELength)] 
#endif
    BYTE*       sE;
} TSS_DAA_CRED_ISSUER;

typedef struct tdTSS_DAA_CREDENTIAL
{
    TSS_VERSION versionInfo;
    UINT32      capitalALength;
#ifdef __midl
    [size_is(capitalALength)] 
#endif
    BYTE*       capitalA;
    UINT32      exponentLength;
#ifdef __midl
    [size_is(exponentLength)] 
#endif
    BYTE*       exponent;
    UINT32      vBar0Length;
#ifdef __midl
    [size_is(vBar0Length)] 
#endif
    BYTE*       vBar0;
    UINT32      vBar1Length;
#ifdef __midl
    [size_is(vBar1Length)] 
#endif
    BYTE*       vBar1;
    UINT32      attributesLength;       // Length of first dimension
    UINT32      attributesLength2;      // Length of second dimension
#ifdef __midl
    [size_is(attributesLength,attributesLength2)] 
#endif
    BYTE**      attributes;
    TSS_DAA_PK  issuerPK;
    UINT32      tpmSpecificEncLength;
#ifdef __midl
    [size_is(tpmSpecificEncLength)]
#endif
    BYTE*       tpmSpecificEnc;
    UINT32      daaCounter;
} TSS_DAA_CREDENTIAL;

typedef struct tdTSS_DAA_ATTRIB_COMMIT
{
    TSS_VERSION versionInfo;
    UINT32      betaLength;
#ifdef __midl
    [size_is(betaLength)]
#endif
    BYTE*       beta;
    UINT32      sMuLength;
#ifdef __midl
    [size_is(sMuLength)]
#endif
    BYTE*       sMu;
} TSS_DAA_ATTRIB_COMMIT;

typedef struct tdTSS_DAA_CREDENTIAL_REQUEST
{
    TSS_VERSION versionInfo;
    UINT32      capitalULength;
#ifdef __midl
    [size_is(capitalULength)]
#endif
    BYTE*       capitalU;
    UINT32      capitalNiLength;
#ifdef __midl
    [size_is(capitalNiLength)]
#endif
    BYTE*       capitalNi;
    UINT32      authenticationProofLength;
#ifdef __midl
    [size_is(authenticationProofLength)]
#endif
    BYTE*       authenticationProof;
    UINT32      challengeLength;
#ifdef __midl
    [size_is(challengeLength)]
#endif
    BYTE*       challenge;
    UINT32      nonceTpmLength;
#ifdef __midl
    [size_is(nonceTpmLength)]
#endif
    BYTE*       nonceTpm;
    UINT32      noncePlatformLength;
#ifdef __midl
    [size_is(noncePlatformLength)]
#endif
    BYTE*       noncePlatform;
    UINT32      sF0Length;
#ifdef __midl
    [size_is(sF0Length)]
#endif
    BYTE*       sF0;
    UINT32      sF1Length;
#ifdef __midl
    [size_is(sF1Length)]
#endif
    BYTE*       sF1;
    UINT32      sVprimeLength;
#ifdef __midl
    [size_is(sVprimeLength)]
#endif
    BYTE*       sVprime;
    UINT32      sVtildePrimeLength;
#ifdef __midl
    [size_is(sVtildePrimeLength)]
#endif
    BYTE*       sVtildePrime;
    UINT32      sALength;       // Length of first dimension
    UINT32      sALength2;      // Length of second dimension
#ifdef __midl
    [size_is(sALength,sALength2)]
#endif
    BYTE**      sA;
    UINT32      attributeCommitmentsLength;
    TSS_DAA_ATTRIB_COMMIT* attributeCommitments;
} TSS_DAA_CREDENTIAL_REQUEST;

typedef struct tdTSS_DAA_SELECTED_ATTRIB
{
    TSS_VERSION versionInfo;
    UINT32      indicesListLength;
#ifdef __midl
    [size_is(indicesListLength)]
#endif
    TSS_BOOL*   indicesList;
} TSS_DAA_SELECTED_ATTRIB;

typedef struct tdTSS_DAA_PSEUDONYM
{
    TSS_VERSION versionInfo;
    TSS_FLAG    payloadFlag;
    UINT32      payloadLength;
#ifdef __midl
    [size_is(payloadLength)]
#endif
    BYTE*       payload;
} TSS_DAA_PSEUDONYM;

typedef struct tdTSS_DAA_PSEUDONYM_PLAIN
{
    TSS_VERSION versionInfo;
    UINT32      capitalNvLength;
#ifdef __midl
    [size_is(capitalNvLength)]
#endif
    BYTE*       capitalNv;
} TSS_DAA_PSEUDONYM_PLAIN;

typedef struct tdTSS_DAA_PSEUDONYM_ENCRYPTED
{
    TSS_VERSION versionInfo;
    UINT32      delta1Length;
#ifdef __midl
    [size_is(delta1Length)]
#endif
    BYTE*       delta1;
    UINT32      delta2Length;
#ifdef __midl
    [size_is(delta2Length)]
#endif
    BYTE*       delta2;
    UINT32      delta3Length;
#ifdef __midl
    [size_is(delta3Length)]
#endif
    BYTE*       delta3;
    UINT32      delta4Length;
#ifdef __midl
    [size_is(delta4Length)]
#endif
    BYTE*       delta4;
    UINT32      sTauLength;
#ifdef __midl
    [size_is(sTauLength)]
#endif
    BYTE*       sTau;
} TSS_DAA_PSEUDONYM_ENCRYPTED;

typedef struct tdTSS_DAA_SIGN_CALLBACK
{
    TSS_VERSION versionInfo;
    TSS_HHASH   challenge;
    TSS_FLAG    payloadFlag;
    UINT32      payloadLength;
#ifdef __midl
    [size_is(payloadLength)]
#endif
    BYTE*       payload;
} TSS_DAA_SIGN_CALLBACK;

typedef struct tdTSS_DAA_SIGNATURE
{
    TSS_VERSION            versionInfo;
    UINT32                 zetaLength;
#ifdef __midl
    [size_is(zetaLength)]
#endif
    BYTE*                  zeta;
    UINT32                 capitalTLength;
#ifdef __midl
    [size_is(capitalTLength)]
#endif
    BYTE*                  capitalT;
    UINT32                 challengeLength;
#ifdef __midl
    [size_is(challengeLength)]
#endif
    BYTE*                  challenge;
    UINT32                 nonceTpmLength;
#ifdef __midl
    [size_is(nonceTpmLength)]
#endif
    BYTE*                  nonceTpm;
    UINT32                 sVLength;
#ifdef __midl
    [size_is(sVLength)]
#endif
    BYTE*                  sV;
    UINT32                 sF0Length;
#ifdef __midl
    [size_is(sF0Length)]
#endif
    BYTE*                  sF0;
    UINT32                 sF1Length;
#ifdef __midl
    [size_is(sF1Length)]
#endif
    BYTE*                  sF1;
    UINT32                 sELength;
#ifdef __midl
    [size_is(sELength)]
#endif
    BYTE*                  sE;
    UINT32                 sALength;    // Length of first dimension
    UINT32                 sALength2;   // Length of second dimension
#ifdef __midl
    [size_is(sALength,sALength2)]
#endif
    BYTE**                 sA;
    UINT32                 attributeCommitmentsLength;
#ifdef __midl
    [size_is(attributeCommitmentsLength)]
#endif
    TSS_DAA_ATTRIB_COMMIT* attributeCommitments;
    TSS_DAA_PSEUDONYM      signedPseudonym;
    TSS_DAA_SIGN_CALLBACK  callbackResult;
} TSS_DAA_SIGNATURE;

typedef struct tdTSS_DAA_IDENTITY_PROOF
{
    TSS_VERSION versionInfo;
    UINT32      endorsementLength;
#ifdef __midl
    [size_is(endorsementLength)]
#endif
    BYTE*       endorsementCredential;
    UINT32      platformLength;
#ifdef __midl
    [size_is(platformLength)]
#endif
    BYTE*       platform;
    UINT32      conformanceLength;
#ifdef __midl
    [size_is(conformanceLength)]
#endif
    BYTE*       conformance;
} TSS_DAA_IDENTITY_PROOF;


////////////////////////////////////////////////////////////////////

typedef UINT32 TSS_FAMILY_ID;
typedef BYTE   TSS_DELEGATION_LABEL;
// Values are TSS_DELEGATIONTYPE_KEY or TSS_DELEGATIONTYPE_OWNER
typedef UINT32 TSS_DELEGATION_TYPE;

typedef struct tdTSS_PCR_INFO_SHORT
{
    UINT32               sizeOfSelect;
#ifdef __midl
    [size_is(sizeOfSelect)]
#endif
    BYTE                *selection;
    BYTE                 localityAtRelease;
    UINT32               sizeOfDigestAtRelease;
#ifdef __midl
    [size_is(sizeOfDigestAtRelease)]
#endif
    BYTE                *digestAtRelease;
} TSS_PCR_INFO_SHORT;

typedef struct tdTSS_FAMILY_TABLE_ENTRY
{
    TSS_FAMILY_ID        familyID;
    TSS_DELEGATION_LABEL label;
    UINT32               verificationCount;
    TSS_BOOL             enabled;
    TSS_BOOL             locked;
} TSS_FAMILY_TABLE_ENTRY;

typedef struct tdTSS_DELEGATION_TABLE_ENTRY
{
    UINT32               tableIndex;
    TSS_DELEGATION_LABEL label;
    TSS_PCR_INFO_SHORT   pcrInfo;
    UINT32               per1;
    UINT32               per2;
    TSS_FAMILY_ID        familyID;
    UINT32               verificationCount;
} TSS_DELEGATION_TABLE_ENTRY;

typedef struct tdTSS_PLATFORM_CLASS
{
    UINT32 platformClassSimpleIdentifier;
    UINT32 platformClassURISize;
    BYTE*  pPlatformClassURI;
} TSS_PLATFORM_CLASS;

#endif // __TSS_STRUCTS_H__

