/*++
 * 
 * TPM structures extracted from the TPM specification 1.2, 
 * Part 2 (Structures), rev 85.
 *
 * Errata:
 *
 * *) The individual bits of TPM_STARTUP_EFFECTS were not given names in
 * the TPM spec so they are not defined in tpm.h.
 * 
 * *) A few typedefs not present in the TPM 1.2 specification have been
 * added. This was generally done when the TPM 1.2 spec defined a set of
 * related values (either bitmasks or enumeration values) but did not
 * define an associated type to hold these values. The typedefs have been
 * added and structure fields that were to hold those values have been
 * switched from generic UINT* types to the more specific types. This was
 * done to highlight exactly where those #defined values were to be used.
 * The types that have been added are:
 *   TPM_NV_PER_ATTRIBUTES
 *   TPM_DELEGATE_TYPE
 *
 * *) The layout of bitfields within a structure are compiler-dependent
 * and the use of structure bitfields has been avoided where possible. In
 * cases where a value is a collection of independent bits the type is
 * given a name (typedeffed to UINT16 or UINT32 as appropriate) and masks
 * are #defined to access the individual bits. This is not possible for
 * TPM_VERSION_BYTE because the fields are 4-bit values. A best attempt
 * has been made to make this compiler independent but it has only been
 * checked on GCC and Visual C++ on little-endian machines.
 * 
 * *) The TPM_DELEGATIONS per1 and per2 fields field are a bitmask but
 * are defined as a UINT32 because the bitfields have different meaning
 * based on the type of delegation blob.
 * 
 * *) The definitions of TPM_PERMANENT_DATA, TPM_STCLEAR_DATA,
 * TPM_STANY_DATA, and TPM_DELEGATE_TABLE_ROW are commented out. These
 * structures are internal to the TPM and are not directly accessible by
 * external software so this should not be a problem.
 * 
 * *) The definitions of TPM_FAMILY_TABLE and TPM_DELEGATE_TABLE are
 * commented out because they are variable length arrays internal to the
 * TPM. As above they are not directly accessible by external software
 * so this should not be a problem.
 */

#ifndef __TPM_H__
#define __TPM_H__

#ifdef __midl
#define SIZEIS(x)  [size_is(x)]
#else
#define SIZEIS(x)
#endif

#include <tss/platform.h>

//-------------------------------------------------------------------
// Part 2, section 2.1: Basic data types
typedef BYTE   TPM_BOOL;
#ifndef FALSE
#define FALSE  0x00
#define TRUE   0x01
#endif /* ifndef FALSE */

//-------------------------------------------------------------------
// Part 2, section 2.3: Helper Redefinitions
//   Many of the helper redefinitions appear later in this file
//   so that they are declared next to the list of valid values
//   they may hold.
typedef BYTE TPM_LOCALITY_MODIFIER;
typedef UINT32 TPM_COMMAND_CODE;                            /* 1.1b */
typedef UINT32 TPM_COUNT_ID;
typedef UINT32 TPM_REDIT_COMMAND;
typedef UINT32 TPM_HANDLE;
typedef UINT32 TPM_AUTHHANDLE;
typedef UINT32 TPM_TRANSHANDLE;
typedef UINT32 TPM_KEYHANDLE;
typedef UINT32 TPM_DIRINDEX;
typedef UINT32 TPM_PCRINDEX;
typedef UINT32 TPM_RESULT;
typedef UINT32 TPM_MODIFIER_INDICATOR;



//-------------------------------------------------------------------
// Part 2, section 2.2.4: Vendor Specific
#define TPM_Vendor_Specific32  0x00000400
#define TPM_Vendor_Specific8   0x80


//-------------------------------------------------------------------
// Part 2, section 3: Structure Tags
typedef UINT16  TPM_STRUCTURE_TAG;
#define TPM_TAG_CONTEXTBLOB            ((UINT16)0x0001)
#define TPM_TAG_CONTEXT_SENSITIVE      ((UINT16)0x0002)
#define TPM_TAG_CONTEXTPOINTER         ((UINT16)0x0003)
#define TPM_TAG_CONTEXTLIST            ((UINT16)0x0004)
#define TPM_TAG_SIGNINFO               ((UINT16)0x0005)
#define TPM_TAG_PCR_INFO_LONG          ((UINT16)0x0006)
#define TPM_TAG_PERSISTENT_FLAGS       ((UINT16)0x0007)
#define TPM_TAG_VOLATILE_FLAGS         ((UINT16)0x0008)
#define TPM_TAG_PERSISTENT_DATA        ((UINT16)0x0009)
#define TPM_TAG_VOLATILE_DATA          ((UINT16)0x000a)
#define TPM_TAG_SV_DATA                ((UINT16)0x000b)
#define TPM_TAG_EK_BLOB                ((UINT16)0x000c)
#define TPM_TAG_EK_BLOB_AUTH           ((UINT16)0x000d)
#define TPM_TAG_COUNTER_VALUE          ((UINT16)0x000e)
#define TPM_TAG_TRANSPORT_INTERNAL     ((UINT16)0x000f)
#define TPM_TAG_TRANSPORT_LOG_IN       ((UINT16)0x0010)
#define TPM_TAG_TRANSPORT_LOG_OUT      ((UINT16)0x0011)
#define TPM_TAG_AUDIT_EVENT_IN         ((UINT16)0x0012)
#define TPM_TAG_AUDIT_EVENT_OUT        ((UINT16)0x0013)
#define TPM_TAG_CURRENT_TICKS          ((UINT16)0x0014)
#define TPM_TAG_KEY                    ((UINT16)0x0015)
#define TPM_TAG_STORED_DATA12          ((UINT16)0x0016)
#define TPM_TAG_NV_ATTRIBUTES          ((UINT16)0x0017)
#define TPM_TAG_NV_DATA_PUBLIC         ((UINT16)0x0018)
#define TPM_TAG_NV_DATA_SENSITIVE      ((UINT16)0x0019)
#define TPM_TAG_DELEGATIONS            ((UINT16)0x001a)
#define TPM_TAG_DELEGATE_PUBLIC        ((UINT16)0x001b)
#define TPM_TAG_DELEGATE_TABLE_ROW     ((UINT16)0x001c)
#define TPM_TAG_TRANSPORT_AUTH         ((UINT16)0x001d)
#define TPM_TAG_TRANSPORT_PUBLIC       ((UINT16)0x001e)
#define TPM_TAG_PERMANENT_FLAGS        ((UINT16)0x001f)
#define TPM_TAG_STCLEAR_FLAGS          ((UINT16)0x0020)
#define TPM_TAG_STANY_FLAGS            ((UINT16)0x0021)
#define TPM_TAG_PERMANENT_DATA         ((UINT16)0x0022)
#define TPM_TAG_STCLEAR_DATA           ((UINT16)0x0023)
#define TPM_TAG_STANY_DATA             ((UINT16)0x0024)
#define TPM_TAG_FAMILY_TABLE_ENTRY     ((UINT16)0x0025)
#define TPM_TAG_DELEGATE_SENSITIVE     ((UINT16)0x0026)
#define TPM_TAG_DELG_KEY_BLOB          ((UINT16)0x0027)
#define TPM_TAG_KEY12                  ((UINT16)0x0028)
#define TPM_TAG_CERTIFY_INFO2          ((UINT16)0x0029)
#define TPM_TAG_DELEGATE_OWNER_BLOB    ((UINT16)0x002a)
#define TPM_TAG_EK_BLOB_ACTIVATE       ((UINT16)0x002b)
#define TPM_TAG_DAA_BLOB               ((UINT16)0x002c)
#define TPM_TAG_DAA_CONTEXT            ((UINT16)0x002d)
#define TPM_TAG_DAA_ENFORCE            ((UINT16)0x002e)
#define TPM_TAG_DAA_ISSUER             ((UINT16)0x002f)
#define TPM_TAG_CAP_VERSION_INFO       ((UINT16)0x0030)
#define TPM_TAG_DAA_SENSITIVE          ((UINT16)0x0031)
#define TPM_TAG_DAA_TPM                ((UINT16)0x0032)
#define TPM_TAG_CMK_MIGAUTH            ((UINT16)0x0033)
#define TPM_TAG_CMK_SIGTICKET          ((UINT16)0x0034)
#define TPM_TAG_CMK_MA_APPROVAL        ((UINT16)0x0035)
#define TPM_TAG_QUOTE_INFO2            ((UINT16)0x0036)
#define TPM_TAG_DA_INFO                ((UINT16)0x0037)
#define TPM_TAG_DA_INFO_LIMITED        ((UINT16)0x0038)
#define TPM_TAG_DA_ACTION_TYPE         ((UINT16)0x0039)


//-------------------------------------------------------------------
// Part 2, section 4: Types
typedef UINT32 TPM_RESOURCE_TYPE;
#define TPM_RT_KEY                     ((UINT32)0x00000001)
#define TPM_RT_AUTH                    ((UINT32)0x00000002)
#define TPM_RT_HASH                    ((UINT32)0x00000003)
#define TPM_RT_TRANS                   ((UINT32)0x00000004)
#define TPM_RT_CONTEXT                 ((UINT32)0x00000005)
#define TPM_RT_COUNTER                 ((UINT32)0x00000006)
#define TPM_RT_DELEGATE                ((UINT32)0x00000007)
#define TPM_RT_DAA_TPM                 ((UINT32)0x00000008)
#define TPM_RT_DAA_V0                  ((UINT32)0x00000009)
#define TPM_RT_DAA_V1                  ((UINT32)0x0000000a)


typedef BYTE TPM_PAYLOAD_TYPE;                              /* 1.1b */
#define TPM_PT_ASYM                    ((BYTE)0x01)         /* 1.1b */
#define TPM_PT_BIND                    ((BYTE)0x02)         /* 1.1b */
#define TPM_PT_MIGRATE                 ((BYTE)0x03)         /* 1.1b */
#define TPM_PT_MAINT                   ((BYTE)0x04)         /* 1.1b */
#define TPM_PT_SEAL                    ((BYTE)0x05)         /* 1.1b */
#define TPM_PT_MIGRATE_RESTRICTED      ((BYTE)0x06)
#define TPM_PT_MIGRATE_EXTERNAL        ((BYTE)0x07)
#define TPM_PT_CMK_MIGRATE             ((BYTE)0x08)


typedef UINT16 TPM_ENTITY_TYPE;                             /* 1.1b */
#define TPM_ET_KEYHANDLE               ((UINT16)0x0001)     /* 1.1b */
#define TPM_ET_OWNER                   ((UINT16)0x0002)     /* 1.1b */
#define TPM_ET_DATA                    ((UINT16)0x0003)     /* 1.1b */
#define TPM_ET_SRK                     ((UINT16)0x0004)     /* 1.1b */
#define TPM_ET_KEY                     ((UINT16)0x0005)     /* 1.1b */
#define TPM_ET_REVOKE                  ((UINT16)0x0006)
#define TPM_ET_DEL_OWNER_BLOB          ((UINT16)0x0007)
#define TPM_ET_DEL_ROW                 ((UINT16)0x0008)
#define TPM_ET_DEL_KEY_BLOB            ((UINT16)0x0009)
#define TPM_ET_COUNTER                 ((UINT16)0x000a)
#define TPM_ET_NV                      ((UINT16)0x000b)
#define TPM_ET_OPERATOR                ((UINT16)0x000c)
#define TPM_ET_RESERVED_HANDLE         ((UINT16)0x0040)

/* The following values may be ORed into the MSB of the TPM_ENTITY_TYPE
 * to indicate particular encryption scheme
 */
#define TPM_ET_XOR                     ((BYTE)0x00)
#define TPM_ET_AES                     ((BYTE)0x06)

typedef UINT32 TPM_KEY_HANDLE;                              /* 1.1b */
#define TPM_KH_SRK                     ((UINT32)0x40000000)
#define TPM_KH_OWNER                   ((UINT32)0x40000001)
#define TPM_KH_REVOKE                  ((UINT32)0x40000002)
#define TPM_KH_TRANSPORT               ((UINT32)0x40000003)
#define TPM_KH_OPERATOR                ((UINT32)0x40000004)
#define TPM_KH_ADMIN                   ((UINT32)0x40000005)
#define TPM_KH_EK                      ((UINT32)0x40000006)
/* 1.1b used different names, but the same values */
#define TPM_KEYHND_SRK                 (TPM_KH_SRK)        /* 1.1b */
#define TPM_KEYHND_OWNER               (TPM_KH_OWNER)      /* 1.1b */


typedef UINT16 TPM_STARTUP_TYPE;                            /* 1.1b */
#define TPM_ST_CLEAR                   ((UINT16)0x0001)     /* 1.1b */
#define TPM_ST_STATE                   ((UINT16)0x0002)     /* 1.1b */
#define TPM_ST_DEACTIVATED             ((UINT16)0x0003)     /* 1.1b */


//typedef UINT32 TPM_STARTUP_EFFECTS;
// 32-bit mask, see spec for meaning. Names not currently defined.
// bits 0-8 have meaning

typedef UINT16 TPM_PROTOCOL_ID;                             /* 1.1b */
#define TPM_PID_OIAP                   ((UINT16)0x0001)     /* 1.1b */
#define TPM_PID_OSAP                   ((UINT16)0x0002)     /* 1.1b */
#define TPM_PID_ADIP                   ((UINT16)0x0003)     /* 1.1b */
#define TPM_PID_ADCP                   ((UINT16)0x0004)     /* 1.1b */
#define TPM_PID_OWNER                  ((UINT16)0x0005)     /* 1.1b */
#define TPM_PID_DSAP                   ((UINT16)0x0006)
#define TPM_PID_TRANSPORT              ((UINT16)0x0007)


// Note in 1.2 rev 104, DES and 3DES are eliminated
typedef UINT32 TPM_ALGORITHM_ID;                            /* 1.1b */
#define TPM_ALG_RSA                    ((UINT32)0x00000001) /* 1.1b */
#define TPM_ALG_DES                    ((UINT32)0x00000002) /* 1.1b */
#define TPM_ALG_3DES                   ((UINT32)0x00000003) /* 1.1b */
#define TPM_ALG_SHA                    ((UINT32)0x00000004) /* 1.1b */
#define TPM_ALG_HMAC                   ((UINT32)0x00000005) /* 1.1b */
#define TPM_ALG_AES                    ((UINT32)0x00000006) /* 1.1b */
#define TPM_ALG_AES128                 (TPM_ALG_AES)
#define TPM_ALG_MGF1                   ((UINT32)0x00000007)
#define TPM_ALG_AES192                 ((UINT32)0x00000008)
#define TPM_ALG_AES256                 ((UINT32)0x00000009)
#define TPM_ALG_XOR                    ((UINT32)0x0000000a)


typedef UINT16 TPM_PHYSICAL_PRESENCE;                        /* 1.1b */
#define TPM_PHYSICAL_PRESENCE_LOCK          ((UINT16)0x0004) /* 1.1b */
#define TPM_PHYSICAL_PRESENCE_PRESENT       ((UINT16)0x0008) /* 1.1b */
#define TPM_PHYSICAL_PRESENCE_NOTPRESENT    ((UINT16)0x0010) /* 1.1b */
#define TPM_PHYSICAL_PRESENCE_CMD_ENABLE    ((UINT16)0x0020) /* 1.1b */
#define TPM_PHYSICAL_PRESENCE_HW_ENABLE     ((UINT16)0x0040) /* 1.1b */
#define TPM_PHYSICAL_PRESENCE_LIFETIME_LOCK ((UINT16)0x0080) /* 1.1b */
#define TPM_PHYSICAL_PRESENCE_CMD_DISABLE   ((UINT16)0x0100)
#define TPM_PHYSICAL_PRESENCE_HW_DISABLE    ((UINT16)0x0200)


typedef UINT16 TPM_MIGRATE_SCHEME;                          /* 1.1b */
#define TPM_MS_MIGRATE                   ((UINT16)0x0001)   /* 1.1b */
#define TPM_MS_REWRAP                    ((UINT16)0x0002)   /* 1.1b */
#define TPM_MS_MAINT                     ((UINT16)0x0003)   /* 1.1b */
#define TPM_MS_RESTRICT_MIGRATE          ((UINT16)0x0004)
#define TPM_MS_RESTRICT_APPROVE_DOUBLE   ((UINT16)0x0005)


typedef UINT16 TPM_EK_TYPE;
#define TPM_EK_TYPE_ACTIVATE           ((UINT16)0x0001)
#define TPM_EK_TYPE_AUTH               ((UINT16)0x0002)


typedef UINT16 TPM_PLATFORM_SPECIFIC;
#define TPM_PS_PC_11                   ((UINT16)0x0001)
#define TPM_PS_PC_12                   ((UINT16)0x0002)
#define TPM_PS_PDA_12                  ((UINT16)0x0003)
#define TPM_PS_Server_12               ((UINT16)0x0004)
#define TPM_PS_Mobile_12               ((UINT16)0x0005)

//-------------------------------------------------------------------
// Part 2, section 5: Basic Structures

typedef struct tdTPM_STRUCT_VER
{
    BYTE   major;
    BYTE   minor;
    BYTE   revMajor;
    BYTE   revMinor;
} TPM_STRUCT_VER;

typedef struct tdTPM_VERSION_BYTE
{
    // This needs to be made compiler-independent.
    int leastSigVer : 4; // least significant 4 bits
    int mostSigVer  : 4; // most significant 4 bits
} TPM_VERSION_BYTE;

typedef struct tdTPM_VERSION
{
    BYTE   major;      // Should really be a TPM_VERSION_BYTE
    BYTE   minor;      // Should really be a TPM_VERSION_BYTE
    BYTE   revMajor;
    BYTE   revMinor;
} TPM_VERSION;


// Put this in the right place:
// byte size definition for 160 bit SHA1 hash value
#define TPM_SHA1_160_HASH_LEN    0x14
#define TPM_SHA1BASED_NONCE_LEN  TPM_SHA1_160_HASH_LEN

typedef struct tdTPM_DIGEST
{
    BYTE  digest[TPM_SHA1_160_HASH_LEN];
} TPM_DIGEST;

typedef TPM_DIGEST TPM_CHOSENID_HASH;
typedef TPM_DIGEST TPM_COMPOSITE_HASH;
typedef TPM_DIGEST TPM_DIRVALUE;
typedef TPM_DIGEST TPM_HMAC;
typedef TPM_DIGEST TPM_PCRVALUE;
typedef TPM_DIGEST TPM_AUDITDIGEST;

typedef struct tdTPM_NONCE                                  /* 1.1b */
{
    BYTE  nonce[TPM_SHA1BASED_NONCE_LEN];
} TPM_NONCE;

typedef TPM_NONCE TPM_DAA_TPM_SEED;
typedef TPM_NONCE TPM_DAA_CONTEXT_SEED;

typedef struct tdTPM_AUTHDATA                               /* 1.1b */
{
    BYTE  authdata[TPM_SHA1_160_HASH_LEN];
} TPM_AUTHDATA;

typedef TPM_AUTHDATA TPM_SECRET;
typedef TPM_AUTHDATA TPM_ENCAUTH;


typedef struct tdTPM_KEY_HANDLE_LIST                        /* 1.1b */
{
    UINT16              loaded;
    SIZEIS(loaded)
        TPM_KEY_HANDLE *handle;
} TPM_KEY_HANDLE_LIST;


//-------------------------------------------------------------------
// Part 2, section 5.8: Key usage values

typedef UINT16 TPM_KEY_USAGE;                               /* 1.1b */
#define TPM_KEY_SIGNING                ((UINT16)0x0010)     /* 1.1b */
#define TPM_KEY_STORAGE                ((UINT16)0x0011)     /* 1.1b */
#define TPM_KEY_IDENTITY               ((UINT16)0x0012)     /* 1.1b */
#define TPM_KEY_AUTHCHANGE             ((UINT16)0x0013)     /* 1.1b */
#define TPM_KEY_BIND                   ((UINT16)0x0014)     /* 1.1b */
#define TPM_KEY_LEGACY                 ((UINT16)0x0015)     /* 1.1b */
#define TPM_KEY_MIGRATE                ((UINT16)0x0016)

typedef UINT16 TPM_SIG_SCHEME;                              /* 1.1b */
#define TPM_SS_NONE                    ((UINT16)0x0001)     /* 1.1b */
#define TPM_SS_RSASSAPKCS1v15_SHA1     ((UINT16)0x0002)     /* 1.1b */
#define TPM_SS_RSASSAPKCS1v15_DER      ((UINT16)0x0003)     /* 1.1b */
#define TPM_SS_RSASSAPKCS1v15_INFO     ((UINT16)0x0004)

typedef UINT16 TPM_ENC_SCHEME;                              /* 1.1b */
#define TPM_ES_NONE                    ((UINT16)0x0001)     /* 1.1b */
#define TPM_ES_RSAESPKCSv15            ((UINT16)0x0002)     /* 1.1b */
#define TPM_ES_RSAESOAEP_SHA1_MGF1     ((UINT16)0x0003)     /* 1.1b */
#define TPM_ES_SYM_CNT                 ((UINT16)0x0004)
#define TPM_ES_SYM_CTR                 TPM_ES_SYM_CNT
#define TPM_ES_SYM_OFB                 ((UINT16)0x0005)
#define TPM_ES_SYM_CBC_PKCS5PAD        ((UINT16)0x00ff)

//-------------------------------------------------------------------
// Part 2, section 5.9: TPM_AUTH_DATA_USAGE values

typedef BYTE TPM_AUTH_DATA_USAGE;                           /* 1.1b */
#define TPM_AUTH_NEVER                 ((BYTE)0x00)         /* 1.1b */
#define TPM_AUTH_ALWAYS                ((BYTE)0x01)         /* 1.1b */
#define TPM_AUTH_PRIV_USE_ONLY         ((BYTE)0x11)


//-------------------------------------------------------------------
// Part 2, section 5.10: TPM_KEY_FLAGS flags

typedef UINT32 TPM_KEY_FLAGS;                               /* 1.1b */
#define TPM_REDIRECTION                ((UINT32)0x00000001) /* 1.1b */
#define TPM_MIGRATABLE                 ((UINT32)0x00000002) /* 1.1b */
#define TPM_VOLATILE                   ((UINT32)0x00000004) /* 1.1b */
#define TPM_PCRIGNOREDONREAD           ((UINT32)0x00000008)
#define TPM_MIGRATEAUTHORITY           ((UINT32)0x00000010)


//-------------------------------------------------------------------
// Part 2, section 5.11: TPM_CHANGEAUTH_VALIDATE

typedef struct tdTPM_CHANGEAUTH_VALIDATE
{
    TPM_SECRET newAuthSecret;
    TPM_NONCE  n1;
} TPM_CHANGEAUTH_VALIDATE;

//-------------------------------------------------------------------
// Part 2, section 5.12: TPM_MIGRATIONKEYAUTH
// declared after section 10 to catch declaration of TPM_PUBKEY 

//-------------------------------------------------------------------
// Part 2, section 5.13: TPM_COUNTER_VALUE;

typedef UINT32 TPM_ACTUAL_COUNT;
typedef struct tdTPM_COUNTER_VALUE
{
    TPM_STRUCTURE_TAG tag;
    BYTE              label[4];
    TPM_ACTUAL_COUNT  counter;
} TPM_COUNTER_VALUE;

//-------------------------------------------------------------------
// Part 2, section 5.14: TPM_SIGN_INFO structure

typedef struct tdTPM_SIGN_INFO
{
    TPM_STRUCTURE_TAG tag;
    BYTE              fixed[4];
    TPM_NONCE         replay;
    UINT32            dataLen;
    SIZEIS(dataLen)
        BYTE         *data;
} TPM_SIGN_INFO;

//-------------------------------------------------------------------
// Part 2, section 5.15: TPM_MSA_COMPOSITE

typedef struct tdTPM_MSA_COMPOSITE
{
    UINT32          MSAlist;
    SIZEIS(MSAlist)
        TPM_DIGEST *migAuthDigest;
} TPM_MSA_COMPOSITE;

//-------------------------------------------------------------------
// Part 2, section 5.16: TPM_CMK_AUTH

typedef struct tdTPM_CMK_AUTH
{
    TPM_DIGEST migrationAuthorityDigest;
    TPM_DIGEST destinationKeyDigest;
    TPM_DIGEST sourceKeyDigest;
} TPM_CMK_AUTH;

//-------------------------------------------------------------------
// Part 2, section 5.17: TPM_CMK_DELEGATE

typedef UINT32 TPM_CMK_DELEGATE;
#define TPM_CMK_DELEGATE_SIGNING       (((UINT32)1)<<31)
#define TPM_CMK_DELEGATE_STORAGE       (((UINT32)1)<<30)
#define TPM_CMK_DELEGATE_BIND          (((UINT32)1)<<29)
#define TPM_CMK_DELEGATE_LEGACY        (((UINT32)1)<<28)
#define TPM_CMK_DELEGATE_MIGRATE       (((UINT32)1)<<27)

//-------------------------------------------------------------------
// Part 2, section 5.18: TPM_SELECT_SIZE

typedef struct tdTPM_SELECT_SIZE
{
    BYTE   major;
    BYTE   minor;
    UINT16 reqSize;
} TPM_SELECT_SIZE;

//-------------------------------------------------------------------
// Part 2, section 5.19: TPM_CMK_MIGAUTH

typedef struct tdTPM_CMK_MIGAUTH
{
    TPM_STRUCTURE_TAG tag;
    TPM_DIGEST        msaDigest;
    TPM_DIGEST        pubKeyDigest;
} TPM_CMK_MIGAUTH;

//-------------------------------------------------------------------
// Part 2, section 5.20: TPM_CMK_SIGTICKET

typedef struct tdTPM_CMK_SIGTICKET
{
    TPM_STRUCTURE_TAG tag;
    TPM_DIGEST        verKeyDigest;
    TPM_DIGEST        signedData;
} TPM_CMK_SIGTICKET;

//-------------------------------------------------------------------
// Part 2, section 5.21: TPM_CMK_MA_APPROVAL

typedef struct tdTPM_CMK_MA_APPROVAL
{
    TPM_STRUCTURE_TAG tag;
    TPM_DIGEST        migrationAuthorityDigest;
} TPM_CMK_MA_APPROVAL;


//-------------------------------------------------------------------
// Part 2, section 6: Command Tags

typedef UINT16 TPM_TAG;                                     /* 1.1b */
#define TPM_TAG_RQU_COMMAND            ((UINT16)0x00c1)
#define TPM_TAG_RQU_AUTH1_COMMAND      ((UINT16)0x00c2)
#define TPM_TAG_RQU_AUTH2_COMMAND      ((UINT16)0x00c3)
#define TPM_TAG_RSP_COMMAND            ((UINT16)0x00c4)
#define TPM_TAG_RSP_AUTH1_COMMAND      ((UINT16)0x00c5)
#define TPM_TAG_RSP_AUTH2_COMMAND      ((UINT16)0x00c6)


//-------------------------------------------------------------------
// Part 2, section 7.1: TPM_PERMANENT_FLAGS

typedef struct tdTPM_PERMANENT_FLAGS
{
    TPM_STRUCTURE_TAG tag;
    TSS_BOOL disable;
    TSS_BOOL ownership;
    TSS_BOOL deactivated;
    TSS_BOOL readPubek;
    TSS_BOOL disableOwnerClear;
    TSS_BOOL allowMaintenance;
    TSS_BOOL physicalPresenceLifetimeLock;
    TSS_BOOL physicalPresenceHWEnable;
    TSS_BOOL physicalPresenceCMDEnable;
    TSS_BOOL CEKPUsed;
    TSS_BOOL TPMpost;
    TSS_BOOL TPMpostLock;
    TSS_BOOL FIPS;
    TSS_BOOL Operator;
    TSS_BOOL enableRevokeEK;
    TSS_BOOL nvLocked;
    TSS_BOOL readSRKPub;
    TSS_BOOL tpmEstablished;
    TSS_BOOL maintenanceDone;
    TSS_BOOL disableFullDALogicInfo;
} TPM_PERMANENT_FLAGS;

#define TPM_PF_DISABLE                      ((UINT32)0x00000001)
#define TPM_PF_OWNERSHIP                    ((UINT32)0x00000002)
#define TPM_PF_DEACTIVATED                  ((UINT32)0x00000003)
#define TPM_PF_READPUBEK                    ((UINT32)0x00000004)
#define TPM_PF_DISABLEOWNERCLEAR            ((UINT32)0x00000005)
#define TPM_PF_ALLOWMAINTENANCE             ((UINT32)0x00000006)
#define TPM_PF_PHYSICALPRESENCELIFETIMELOCK ((UINT32)0x00000007)
#define TPM_PF_PHYSICALPRESENCEHWENABLE     ((UINT32)0x00000008)
#define TPM_PF_PHYSICALPRESENCECMDENABLE    ((UINT32)0x00000009)
#define TPM_PF_CEKPUSED                     ((UINT32)0x0000000A)
#define TPM_PF_TPMPOST                      ((UINT32)0x0000000B)
#define TPM_PF_TPMPOSTLOCK                  ((UINT32)0x0000000C)
#define TPM_PF_FIPS                         ((UINT32)0x0000000D)
#define TPM_PF_OPERATOR                     ((UINT32)0x0000000E)
#define TPM_PF_ENABLEREVOKEEK               ((UINT32)0x0000000F)
#define TPM_PF_NV_LOCKED                    ((UINT32)0x00000010)
#define TPM_PF_READSRKPUB                   ((UINT32)0x00000011)
#define TPM_PF_RESETESTABLISHMENTBIT        ((UINT32)0x00000012)
#define TPM_PF_MAINTENANCEDONE              ((UINT32)0x00000013)
#define TPM_PF_DISABLEFULLDALOGICINFO       ((UINT32)0x00000014)


//-------------------------------------------------------------------
// Part 2, section 7.2: TPM_STCLEAR_FLAGS

typedef struct tdTPM_STCLEAR_FLAGS
{
    TPM_STRUCTURE_TAG tag;
    TSS_BOOL          deactivated;
    TSS_BOOL          disableForceClear;
    TSS_BOOL          physicalPresence;
    TSS_BOOL          physicalPresenceLock;
    TSS_BOOL          bGlobalLock;
} TPM_STCLEAR_FLAGS;

#define TPM_SF_DEACTIVATED             ((UINT32)0x00000001)
#define TPM_SF_DISABLEFORCECLEAR       ((UINT32)0x00000002)
#define TPM_SF_PHYSICALPRESENCE        ((UINT32)0x00000003)
#define TPM_SF_PHYSICALPRESENCELOCK    ((UINT32)0x00000004)
#define TPM_SF_GLOBALLOCK              ((UINT32)0x00000005)


//-------------------------------------------------------------------
// Part 2, section 7.3: TPM_STANY_FLAGS

typedef struct tdTPM_STANY_FLAGS
{
    TPM_STRUCTURE_TAG      tag;
    TSS_BOOL               postInitialise;
    TPM_MODIFIER_INDICATOR localityModifier;
    TSS_BOOL               transportExclusive;
    TSS_BOOL               TOSPresent;
} TPM_STANY_FLAGS;

#define TPM_AF_POSTINITIALIZE          ((UINT32)0x00000001)
#define TPM_AF_LOCALITYMODIFIER        ((UINT32)0x00000002)
#define TPM_AF_TRANSPORTEXCLUSIVE      ((UINT32)0x00000003)
#define TPM_AF_TOSPRESENT              ((UINT32)0x00000004)


//-------------------------------------------------------------------
// Part 2, section 7.4: TPM_PERMANENT_DATA
// available inside TPM only
//
//#define TPM_MIN_COUNTERS          4
//#define TPM_NUM_PCR              16
//#define TPM_MAX_NV_WRITE_NOOWNER 64
//
//typedef struct tdTPM_PERMANENT_DATA
//{
//    TPM_STRUCTURE_TAG  tag;
//    BYTE               revMajor;
//    BYTE               revMinor;
//    TPM_NONCE          tpmProof;
//    TPM_NONCE          ekReset;
//    TPM_SECRET         ownerAuth;
//    TPM_SECRET         operatorAuth;
//    TPM_DIRVALUE       authDIR[1];
//    TPM_PUBKEY         manuMaintPub;
//    TPM_KEY            endorsementKey;
//    TPM_KEY            srk;
//    TPM_KEY            contextKey;
//    TPM_KEY            delegateKey;
//    TPM_COUNTER_VALUE  auditMonotonicCounter;
//    TPM_COUNTER_VALUE  monitonicCounter[TPM_MIN_COUNTERS];
//    TPM_PCR_ATTRIBUTES pcrAttrib[TPM_NUM_PCR];
//    BYTE               ordinalAuditStatus[];
//    BYTE              *rngState;
//    TPM_FAMILY_TABLE   familyTable;
//    TPM_DELEGATE_TABLE delegateTable;
//    UINT32             maxNVBufSize;
//    UINT32             lastFamilyID;
//    UINT32             noOwnerNVWrite;
//    TPM_CMK_DELEGATE   restrictDelegate;
//    TPM_DAA_TPM_SEED   tpmDAASeed;
//    TPM_NONCE          daaProof;
//    TPM_NONCE          daaBlobKey;
//} TPM_PERMANENT_DATA;


//-------------------------------------------------------------------
// Part 2, section 7.5: TPM_STCLEAR_DATA
// available inside TPM only
//
//typedef struct tdTPM_STCLEAR_DATA
//{
//    TPM_STRUCTURE_TAG tag;
//    TPM_NONCE         contextNonceKey;
//    TPM_COUNT_ID      countID;
//    UINT32            ownerReference;
//    TPM_BOOL          disableResetLock;
//    TPM_PCRVALUE      PCR[TPM_NUM_PCR];
//    UINT32            deferredPhysicalPresence;
//} TPM_STCLEAR_DATA;
    


//-------------------------------------------------------------------
// Part 2, section 7.5: TPM_STANY_DATA
// available inside TPM only
//
//typedef struct tdTPM_STANY_DATA
//{
//    TPM_STRUCTURE_TAG tag;
//    TPM_NONCE         contextNonceSession;
//    TPM_DIGEST        auditDigest;
//    TPM_CURRENT_TICKS currentTicks;
//    UINT32            contextCount;
//    UINT32            contextList[TPM_MIN_SESSION_LIST];
//    TPM_SESSION_DATA  sessions[TPM_MIN_SESSIONS];
//    // The following appear in section 22.6 but not in 7.5
//    TPM_DAA_ISSUER    DAA_issuerSettings;
//    TPM_DAA_TPM       DAA_tpmSpecific;
//    TPM_DAA_CONTEXT   DAA_session;
//    TPM_DAA_JOINDATA  DAA_joinSession;
//} TPM_STANY_DATA;
    


//-------------------------------------------------------------------
// Part 2, section 8: PCR Structures

typedef BYTE  TPM_LOCALITY_SELECTION;
#define TPM_LOC_FOUR                   (((UINT32)1)<<4)
#define TPM_LOC_THREE                  (((UINT32)1)<<3)
#define TPM_LOC_TWO                    (((UINT32)1)<<2)
#define TPM_LOC_ONE                    (((UINT32)1)<<1)
#define TPM_LOC_ZERO                   (((UINT32)1)<<0)

typedef struct tdTPM_PCR_SELECTION                          /* 1.1b */
{ 
    UINT16    sizeOfSelect;
    SIZEIS(sizeOfSelect)
        BYTE *pcrSelect;  
} TPM_PCR_SELECTION;

typedef struct tdTPM_PCR_COMPOSITE                          /* 1.1b */
{ 
    TPM_PCR_SELECTION select;
    UINT32            valueSize;
    SIZEIS(valueSize)
        TPM_PCRVALUE *pcrValue; 
} TPM_PCR_COMPOSITE;

typedef struct tdTPM_PCR_INFO                               /* 1.1b */
{
    TPM_PCR_SELECTION  pcrSelection;
    TPM_COMPOSITE_HASH digestAtRelease;
    TPM_COMPOSITE_HASH digestAtCreation;
}  TPM_PCR_INFO;

typedef struct tdTPM_PCR_INFO_LONG
{
    TPM_STRUCTURE_TAG      tag;
    TPM_LOCALITY_SELECTION localityAtCreation;
    TPM_LOCALITY_SELECTION localityAtRelease;
    TPM_PCR_SELECTION      creationPCRSelection;
    TPM_PCR_SELECTION      releasePCRSelection;
    TPM_COMPOSITE_HASH     digestAtCreation;
    TPM_COMPOSITE_HASH     digestAtRelease;
}  TPM_PCR_INFO_LONG;

typedef struct tdTPM_PCR_INFO_SHORT
{
    TPM_PCR_SELECTION      pcrSelection;
    TPM_LOCALITY_SELECTION localityAtRelease;
    TPM_COMPOSITE_HASH     digestAtRelease;
}  TPM_PCR_INFO_SHORT;

typedef struct tdTPM_PCR_ATTRIBUTES
{
    BYTE                   pcrReset;
    TPM_LOCALITY_SELECTION pcrExtendLocal;
    TPM_LOCALITY_SELECTION pcrResetLocal;
} TPM_PCR_ATTRIBUTES;



//-------------------------------------------------------------------
// Part 2, section 9:

typedef struct tdTPM_STORED_DATA                            /* 1.1b */
{
    TPM_STRUCT_VER ver;
    UINT32         sealInfoSize;
    SIZEIS(sealInfoSize)
        BYTE      *sealInfo;
    UINT32         encDataSize;
    SIZEIS(encDataSize)
        BYTE      *encData;
} TPM_STORED_DATA;

typedef struct tdTPM_STORED_DATA12
{
    TPM_STRUCTURE_TAG tag;
    TPM_ENTITY_TYPE   et;
    UINT32            sealInfoSize;
    SIZEIS(sealInfoSize)
        BYTE         *sealInfo;
    UINT32            encDataSize;
    SIZEIS(encDataSize)
        BYTE         *encData;
} TPM_STORED_DATA12;

typedef struct tdTPM_SEALED_DATA                            /* 1.1b */
{ 
    TPM_PAYLOAD_TYPE  payload;
    TPM_SECRET        authData;
    TPM_NONCE         tpmProof;
    TPM_DIGEST        storedDigest;
    UINT32            dataSize;
    SIZEIS(dataSize)
        BYTE         *data;
} TPM_SEALED_DATA;

typedef struct tdTPM_SYMMETRIC_KEY                          /* 1.1b */
{
    TPM_ALGORITHM_ID  algId;
    TPM_ENC_SCHEME    encScheme;
    UINT16            size;
    SIZEIS(size)
        BYTE         *data;
} TPM_SYMMETRIC_KEY;

typedef struct tdTPM_BOUND_DATA
{
    TPM_STRUCT_VER   ver;
    TPM_PAYLOAD_TYPE payload;
    BYTE            *payloadData; // length is implied
} TPM_BOUND_DATA;


//-------------------------------------------------------------------
// Part 2, section 10: TPM_KEY complex

typedef struct tdTPM_KEY_PARMS                              /* 1.1b */
{
    TPM_ALGORITHM_ID  algorithmID;
    TPM_ENC_SCHEME    encScheme;
    TPM_SIG_SCHEME    sigScheme;
    UINT32            parmSize;
    SIZEIS(parmSize)
        BYTE         *parms;
} TPM_KEY_PARMS;

typedef struct tdTPM_RSA_KEY_PARMS                          /* 1.1b */
{  
    UINT32    keyLength; 
    UINT32    numPrimes; 
    UINT32    exponentSize;
    SIZEIS(exponentSize)
        BYTE *exponent;
} TPM_RSA_KEY_PARMS;

typedef struct tdTPM_SYMMETRIC_KEY_PARMS
{
    UINT32 keyLength;
    UINT32 blockSize;
    UINT32 ivSize;
    SIZEIS(ivSize)
        BYTE *IV;
} TPM_SYMMETRIC_KEY_PARMS;

typedef struct tdTPM_STORE_PUBKEY                           /* 1.1b */
{
    UINT32    keyLength;
    SIZEIS(keyLength)
        BYTE *key;
} TPM_STORE_PUBKEY;

typedef struct tdTPM_PUBKEY                                 /* 1.1b */
{
    TPM_KEY_PARMS     algorithmParms;
    TPM_STORE_PUBKEY  pubKey;
} TPM_PUBKEY;

typedef struct tdTPM_STORE_PRIVKEY                          /* 1.1b */
{
    UINT32    keyLength;
    SIZEIS(keyLength)
        BYTE *key;   
} TPM_STORE_PRIVKEY;

typedef struct tdTPM_STORE_ASYMKEY                          /* 1.1b */
{         
    TPM_PAYLOAD_TYPE  payload;   
    TPM_SECRET        usageAuth;    
    TPM_SECRET        migrationAuth;  
    TPM_DIGEST        pubDataDigest;   
    TPM_STORE_PRIVKEY privKey;   
} TPM_STORE_ASYMKEY;

typedef struct tdTPM_KEY                                    /* 1.1b */
{
    TPM_STRUCT_VER      ver;
    TPM_KEY_USAGE       keyUsage;
    TPM_KEY_FLAGS       keyFlags;
    TPM_AUTH_DATA_USAGE authDataUsage;
    TPM_KEY_PARMS       algorithmParms; 
    UINT32              PCRInfoSize;
    SIZEIS(PCRInfoSize)
        BYTE           *PCRInfo;
    TPM_STORE_PUBKEY    pubKey;
    UINT32              encSize;
    SIZEIS(encSize)
        BYTE           *encData; 
} TPM_KEY;

typedef struct tdTPM_KEY12
{
    TPM_STRUCTURE_TAG   tag;
    UINT16              fill;
    TPM_KEY_USAGE       keyUsage;
    TPM_KEY_FLAGS       keyFlags;
    TPM_AUTH_DATA_USAGE authDataUsage;
    TPM_KEY_PARMS       algorithmParms;
    UINT32              PCRInfoSize;
    SIZEIS(PCRInfoSize)
       BYTE            *PCRInfo;
    TPM_STORE_PUBKEY    pubKey;
    UINT32              encSize;
    SIZEIS(encSize)
       BYTE            *encData;
} TPM_KEY12;

typedef struct tdTPM_MIGRATE_ASYMKEY
{
    TPM_PAYLOAD_TYPE payload;
    TPM_SECRET       usageAuth;
    TPM_DIGEST       pubDataDigest;
    UINT32           partPrivKeyLen;
    SIZEIS(partPrivKeyLen)
        BYTE        *partPrivKey;
} TPM_MIGRATE_ASYMKEY;


typedef UINT32 TPM_KEY_CONTROL;
#define TPM_KEY_CONTROL_OWNER_EVICT    ((UINT32)0x00000001)


//-------------------------------------------------------------------
// Part 2, section 5.12: TPM_MIGRATIONKEYAUTH

typedef struct tdTPM_MIGRATIONKEYAUTH                       /* 1.1b */
{
    TPM_PUBKEY         migrationKey;
    TPM_MIGRATE_SCHEME migrationScheme;
    TPM_DIGEST         digest;
} TPM_MIGRATIONKEYAUTH;


//-------------------------------------------------------------------
// Part 2, section 11: Signed Structures

typedef struct tdTPM_CERTIFY_INFO                           /* 1.1b */
{
    TPM_STRUCT_VER      version;
    TPM_KEY_USAGE       keyUsage;
    TPM_KEY_FLAGS       keyFlags;
    TPM_AUTH_DATA_USAGE authDataUsage;
    TPM_KEY_PARMS       algorithmParms;
    TPM_DIGEST          pubkeyDigest;
    TPM_NONCE           data;
    TPM_BOOL            parentPCRStatus;
    UINT32              PCRInfoSize;
    SIZEIS(PCRInfoSize)
        BYTE           *PCRInfo;
} TPM_CERTIFY_INFO;

typedef struct tdTPM_CERTIFY_INFO2
{
    TPM_STRUCTURE_TAG   tag;
    BYTE                fill;
    TPM_PAYLOAD_TYPE    payloadType;
    TPM_KEY_USAGE       keyUsage;
    TPM_KEY_FLAGS       keyFlags;
    TPM_AUTH_DATA_USAGE authDataUsage;
    TPM_KEY_PARMS       algorithmParms;
    TPM_DIGEST          pubkeyDigest;
    TPM_NONCE           data;
    TPM_BOOL            parentPCRStatus;
    UINT32              PCRInfoSize;
    SIZEIS(PCRInfoSize) 
        BYTE           *PCRInfo;
    UINT32              migrationAuthoritySize;
    SIZEIS(migrationAuthoritySize)
        BYTE           *migrationAuthority;
} TPM_CERTIFY_INFO2;

typedef struct tdTPM_QUOTE_INFO                             /* 1.1b */
{
    TPM_STRUCT_VER     version;
    BYTE               fixed[4];
    TPM_COMPOSITE_HASH compositeHash; /* in 1.2 TPM spec, named digestValue */
    TPM_NONCE          externalData;
} TPM_QUOTE_INFO;

typedef struct tdTPM_QUOTE_INFO2
{
    TPM_STRUCTURE_TAG  tag;
    BYTE               fixed[4];
    TPM_NONCE          externalData;
    TPM_PCR_INFO_SHORT infoShort;
} TPM_QUOTE_INFO2;



//-------------------------------------------------------------------
// Part 2, section 12: Identity Structures


typedef struct tdTPM_EK_BLOB
{
    TPM_STRUCTURE_TAG tag;
    TPM_EK_TYPE       ekType;
    UINT32            blobSize;
    SIZEIS(blobSize)
        BYTE         *blob;
} TPM_EK_BLOB;

typedef struct tdTPM_EK_BLOB_ACTIVATE
{
    TPM_STRUCTURE_TAG  tag;
    TPM_SYMMETRIC_KEY  sessionKey;
    TPM_DIGEST         idDigest;
    TPM_PCR_INFO_SHORT pcrInfo;
} TPM_EK_BLOB_ACTIVATE;

typedef struct tdTPM_EK_BLOB_AUTH
{
    TPM_STRUCTURE_TAG tag;
    TPM_SECRET        authValue;
} TPM_EK_BLOB_AUTH;


typedef struct tdTPM_IDENTITY_CONTENTS
{
    TPM_STRUCT_VER    ver;
    UINT32            ordinal;
    TPM_CHOSENID_HASH labelPrivCADigest;
    TPM_PUBKEY        identityPubKey;
} TPM_IDENTITY_CONTENTS;

typedef struct tdTPM_IDENTITY_REQ                           /* 1.1b */
{
    UINT32         asymSize;
    UINT32         symSize;
    TPM_KEY_PARMS  asymAlgorithm;
    TPM_KEY_PARMS  symAlgorithm;
    SIZEIS(asymSize)
        BYTE      *asymBlob;
    SIZEIS(symSize)
        BYTE      *symBlob;
} TPM_IDENTITY_REQ;

typedef struct tdTPM_IDENTITY_PROOF                         /* 1.1b */
{
    TPM_STRUCT_VER  ver;
    UINT32          labelSize;
    UINT32          identityBindingSize;
    UINT32          endorsementSize;
    UINT32          platformSize;
    UINT32          conformanceSize;
    TPM_PUBKEY      identityKey;
    SIZEIS(labelSize)
      BYTE         *labelArea;
    SIZEIS(identityBindingSize)
      BYTE         *identityBinding;
    SIZEIS(endorsementSize)
      BYTE         *endorsementCredential;
    SIZEIS(platformSize)
      BYTE         *platformCredential;
    SIZEIS(conformanceSize)
      BYTE         *conformanceCredential;
} TPM_IDENTITY_PROOF;

typedef struct tdTPM_ASYM_CA_CONTENTS                       /* 1.1b */
{
    TPM_SYMMETRIC_KEY sessionKey;
    TPM_DIGEST        idDigest;
} TPM_ASYM_CA_CONTENTS;

typedef struct tdTPM_SYM_CA_ATTESTATION
{
    UINT32         credSize;
    TPM_KEY_PARMS  algorithm;
    SIZEIS(credSize)
        BYTE      *credential;
} TPM_SYM_CA_ATTESTATION;



//-------------------------------------------------------------------
// Part 2, section 15: Tick Structures
// Placed here out of order because definitions are used in section 13.

typedef struct tdTPM_CURRENT_TICKS
{
    TPM_STRUCTURE_TAG tag;
    UINT64            currentTicks;
    UINT16            tickRate;
    TPM_NONCE         tickNonce;
} TPM_CURRENT_TICKS;



//-------------------------------------------------------------------
// Part 2, section 13: Transport structures

typedef UINT32 TPM_TRANSPORT_ATTRIBUTES;
#define TPM_TRANSPORT_ENCRYPT          ((UINT32)0x00000001)
#define TPM_TRANSPORT_LOG              ((UINT32)0x00000002)
#define TPM_TRANSPORT_EXCLUSIVE        ((UINT32)0x00000004)

typedef struct tdTPM_TRANSPORT_PUBLIC
{
    TPM_STRUCTURE_TAG        tag;
    TPM_TRANSPORT_ATTRIBUTES transAttributes;
    TPM_ALGORITHM_ID         algId;
    TPM_ENC_SCHEME           encScheme;
} TPM_TRANSPORT_PUBLIC;

typedef struct tdTPM_TRANSPORT_INTERNAL
{
    TPM_STRUCTURE_TAG    tag;
    TPM_AUTHDATA         authData;
    TPM_TRANSPORT_PUBLIC transPublic;
    TPM_TRANSHANDLE      transHandle;
    TPM_NONCE            transNonceEven;
    TPM_DIGEST           transDigest;
} TPM_TRANSPORT_INTERNAL;

typedef struct tdTPM_TRANSPORT_LOG_IN
{
    TPM_STRUCTURE_TAG tag;
    TPM_DIGEST        parameters;
    TPM_DIGEST        pubKeyHash;
} TPM_TRANSPORT_LOG_IN;

typedef struct tdTPM_TRANSPORT_LOG_OUT
{
    TPM_STRUCTURE_TAG      tag;
    TPM_CURRENT_TICKS      currentTicks;
    TPM_DIGEST             parameters;
    TPM_MODIFIER_INDICATOR locality;
} TPM_TRANSPORT_LOG_OUT;

typedef struct tdTPM_TRANSPORT_AUTH
{
    TPM_STRUCTURE_TAG tag;
    TPM_AUTHDATA      authData;
} TPM_TRANSPORT_AUTH;



//-------------------------------------------------------------------
// Part 2, section 14: Audit Structures

typedef struct tdTPM_AUDIT_EVENT_IN
{
    TPM_STRUCTURE_TAG tag;
    TPM_DIGEST        inputParms;
    TPM_COUNTER_VALUE auditCount;
} TPM_AUDIT_EVENT_IN;

typedef struct tdTPM_AUDIT_EVENT_OUT
{
    TPM_STRUCTURE_TAG tag;
    TPM_COMMAND_CODE  ordinal;
    TPM_DIGEST        outputParms;
    TPM_COUNTER_VALUE auditCount;
    TPM_RESULT        returnCode;
} TPM_AUDIT_EVENT_OUT;



//-------------------------------------------------------------------
// Part 2, section 16: Return codes

#include <tss/tpm_error.h>


//-------------------------------------------------------------------
// Part 2, section 17: Ordinals

#include <tss/tpm_ordinal.h>

//-------------------------------------------------------------------
// Part 2, section 18: Context structures

typedef struct tdTPM_CONTEXT_BLOB
{
    TPM_STRUCTURE_TAG  tag;
    TPM_RESOURCE_TYPE  resourceType;
    TPM_HANDLE         handle;
    BYTE               label[16];
    UINT32             contextCount;
    TPM_DIGEST         integrityDigest;
    UINT32             additionalSize;
    SIZEIS(additionalSize)
        BYTE          *additionalData;
    UINT32             sensitiveSize;
    SIZEIS(sensitiveSize)
        BYTE          *sensitiveData;
} TPM_CONTEXT_BLOB;

typedef struct tdTPM_CONTEXT_SENSITIVE
{
    TPM_STRUCTURE_TAG tag;
    TPM_NONCE         contextNonce;
    UINT32            internalSize;
    SIZEIS(internalSize)
        BYTE         *internalData;
} TPM_CONTEXT_SENSITIVE;

//-------------------------------------------------------------------
// Part 2, section 19: NV Structures

typedef UINT32 TPM_NV_INDEX;
#define TPM_NV_INDEX_LOCK              ((UINT32)0xffffffff)
#define TPM_NV_INDEX0                  ((UINT32)0x00000000)
#define TPM_NV_INDEX_DIR               ((UINT32)0x10000001)
#define TPM_NV_INDEX_EKCert            ((UINT32)0x0000f000)
#define TPM_NV_INDEX_TPM_CC            ((UINT32)0x0000f001)
#define TPM_NV_INDEX_PlatformCert      ((UINT32)0x0000f002)
#define TPM_NV_INDEX_Platform_CC       ((UINT32)0x0000f003)
// The following define ranges of reserved indices.
#define TPM_NV_INDEX_TSS_BASE          ((UINT32)0x00011100)
#define TPM_NV_INDEX_PC_BASE           ((UINT32)0x00011200)
#define TPM_NV_INDEX_SERVER_BASE       ((UINT32)0x00011300)
#define TPM_NV_INDEX_MOBILE_BASE       ((UINT32)0x00011400)
#define TPM_NV_INDEX_PERIPHERAL_BASE   ((UINT32)0x00011500)
#define TPM_NV_INDEX_GROUP_RESV_BASE   ((UINT32)0x00010000)


typedef UINT32 TPM_NV_PER_ATTRIBUTES;
#define TPM_NV_PER_READ_STCLEAR        (((UINT32)1)<<31)
#define TPM_NV_PER_AUTHREAD            (((UINT32)1)<<18)
#define TPM_NV_PER_OWNERREAD           (((UINT32)1)<<17)
#define TPM_NV_PER_PPREAD              (((UINT32)1)<<16)
#define TPM_NV_PER_GLOBALLOCK          (((UINT32)1)<<15)
#define TPM_NV_PER_WRITE_STCLEAR       (((UINT32)1)<<14)
#define TPM_NV_PER_WRITEDEFINE         (((UINT32)1)<<13)
#define TPM_NV_PER_WRITEALL            (((UINT32)1)<<12)
#define TPM_NV_PER_AUTHWRITE           (((UINT32)1)<<2)
#define TPM_NV_PER_OWNERWRITE          (((UINT32)1)<<1)
#define TPM_NV_PER_PPWRITE             (((UINT32)1)<<0)

typedef struct tdTPM_NV_ATTRIBUTES
{
    TPM_STRUCTURE_TAG     tag;
    TPM_NV_PER_ATTRIBUTES attributes;
} TPM_NV_ATTRIBUTES;


typedef struct tdTPM_NV_DATA_PUBLIC
{
    TPM_STRUCTURE_TAG  tag;
    TPM_NV_INDEX       nvIndex;
    TPM_PCR_INFO_SHORT pcrInfoRead;
    TPM_PCR_INFO_SHORT pcrInfoWrite;
    TPM_NV_ATTRIBUTES  permission;
    TPM_BOOL           bReadSTClear;
    TPM_BOOL           bWriteSTClear;
    TPM_BOOL           bWriteDefine;
    UINT32             dataSize;
} TPM_NV_DATA_PUBLIC;


#if 0
// Internal to TPM:
typedef struct tdTPM_NV_DATA_SENSITIVE
{
    TPM_STRUCTURE_TAG  tag;
    TPM_NV_DATA_PUBLIC pubInfo;
    TPM_AUTHDATA       authValue;
    SIZEIS(pubInfo.dataSize)
        BYTE          *data;
} TPM_NV_DATA_SENSITIVE;
#endif


//-------------------------------------------------------------------
// Part 2, section 20: Delegation

//-------------------------------------------------------------------
// Part 2, section 20.3: Owner Permissions Settings for per1 bits
#define TPM_DELEGATE_SetOrdinalAuditStatus          (((UINT32)1)<<30)
#define TPM_DELEGATE_DirWriteAuth                   (((UINT32)1)<<29)
#define TPM_DELEGATE_CMK_ApproveMA                  (((UINT32)1)<<28)
#define TPM_DELEGATE_NV_WriteValue                  (((UINT32)1)<<27)
#define TPM_DELEGATE_CMK_CreateTicket               (((UINT32)1)<<26)
#define TPM_DELEGATE_NV_ReadValue                   (((UINT32)1)<<25)
#define TPM_DELEGATE_Delegate_LoadOwnerDelegation   (((UINT32)1)<<24)
#define TPM_DELEGATE_DAA_Join                       (((UINT32)1)<<23)
#define TPM_DELEGATE_AuthorizeMigrationKey          (((UINT32)1)<<22)
#define TPM_DELEGATE_CreateMaintenanceArchive       (((UINT32)1)<<21)
#define TPM_DELEGATE_LoadMaintenanceArchive         (((UINT32)1)<<20)
#define TPM_DELEGATE_KillMaintenanceFeature         (((UINT32)1)<<19)
#define TPM_DELEGATE_OwnerReadInternalPub           (((UINT32)1)<<18)
#define TPM_DELEGATE_ResetLockValue                 (((UINT32)1)<<17)
#define TPM_DELEGATE_OwnerClear                     (((UINT32)1)<<16)
#define TPM_DELEGATE_DisableOwnerClear              (((UINT32)1)<<15)
#define TPM_DELEGATE_NV_DefineSpace                 (((UINT32)1)<<14)
#define TPM_DELEGATE_OwnerSetDisable                (((UINT32)1)<<13)
#define TPM_DELEGATE_SetCapability                  (((UINT32)1)<<12)
#define TPM_DELEGATE_MakeIdentity                   (((UINT32)1)<<11)
#define TPM_DELEGATE_ActivateIdentity               (((UINT32)1)<<10)
#define TPM_DELEGATE_OwnerReadPubek                 (((UINT32)1)<<9)
#define TPM_DELEGATE_DisablePubekRead               (((UINT32)1)<<8)
#define TPM_DELEGATE_SetRedirection                 (((UINT32)1)<<7)
#define TPM_DELEGATE_FieldUpgrade                   (((UINT32)1)<<6)
#define TPM_DELEGATE_Delegate_UpdateVerification    (((UINT32)1)<<5)
#define TPM_DELEGATE_CreateCounter                  (((UINT32)1)<<4)
#define TPM_DELEGATE_ReleaseCounterOwner            (((UINT32)1)<<3)
#define TPM_DELEGATE_DelegateManage                 (((UINT32)1)<<2)
#define TPM_DELEGATE_Delegate_CreateOwnerDelegation (((UINT32)1)<<1)
#define TPM_DELEGATE_DAA_Sign                       (((UINT32)1)<<0)

//-------------------------------------------------------------------
// Part 2, section 20.3: Key Permissions Settings for per1 bits
#define TPM_KEY_DELEGATE_CMK_ConvertMigration       (((UINT32)1)<<28)
#define TPM_KEY_DELEGATE_TickStampBlob              (((UINT32)1)<<27)
#define TPM_KEY_DELEGATE_ChangeAuthAsymStart        (((UINT32)1)<<26)
#define TPM_KEY_DELEGATE_ChangeAuthAsymFinish       (((UINT32)1)<<25)
#define TPM_KEY_DELEGATE_CMK_CreateKey              (((UINT32)1)<<24)
#define TPM_KEY_DELEGATE_MigrateKey                 (((UINT32)1)<<23)
#define TPM_KEY_DELEGATE_LoadKey2                   (((UINT32)1)<<22)
#define TPM_KEY_DELEGATE_EstablishTransport         (((UINT32)1)<<21)
#define TPM_KEY_DELEGATE_ReleaseTransportSigned     (((UINT32)1)<<20)
#define TPM_KEY_DELEGATE_Quote2                     (((UINT32)1)<<19)
#define TPM_KEY_DELEGATE_Sealx                      (((UINT32)1)<<18)
#define TPM_KEY_DELEGATE_MakeIdentity               (((UINT32)1)<<17)
#define TPM_KEY_DELEGATE_ActivateIdentity           (((UINT32)1)<<16)
#define TPM_KEY_DELEGATE_GetAuditDigestSigned       (((UINT32)1)<<15)
#define TPM_KEY_DELEGATE_Sign                       (((UINT32)1)<<14)
#define TPM_KEY_DELEGATE_CertifyKey2                (((UINT32)1)<<13)
#define TPM_KEY_DELEGATE_CertifyKey                 (((UINT32)1)<<12)
#define TPM_KEY_DELEGATE_CreateWrapKey              (((UINT32)1)<<11)
#define TPM_KEY_DELEGATE_CMK_CreateBlob             (((UINT32)1)<<10)
#define TPM_KEY_DELEGATE_CreateMigrationBlob        (((UINT32)1)<<9)
#define TPM_KEY_DELEGATE_ConvertMigrationBlob       (((UINT32)1)<<8)
#define TPM_KEY_DELEGATE_CreateKeyDelegation        (((UINT32)1)<<7)
#define TPM_KEY_DELEGATE_ChangeAuth                 (((UINT32)1)<<6)
#define TPM_KEY_DELEGATE_GetPubKey                  (((UINT32)1)<<5)
#define TPM_KEY_DELEGATE_UnBind                     (((UINT32)1)<<4)
#define TPM_KEY_DELEGATE_Quote                      (((UINT32)1)<<3)
#define TPM_KEY_DELEGATE_Unseal                     (((UINT32)1)<<2)
#define TPM_KEY_DELEGATE_Seal                       (((UINT32)1)<<1)
#define TPM_KEY_DELEGATE_LoadKey                    (((UINT32)1)<<0)

typedef UINT32 TPM_FAMILY_VERIFICATION;

typedef UINT32 TPM_FAMILY_ID;

typedef UINT32 TPM_DELEGATE_INDEX;

typedef UINT32 TPM_FAMILY_OPERATION;
#define TPM_FAMILY_CREATE              ((UINT32)0x00000001)
#define TPM_FAMILY_ENABLE              ((UINT32)0x00000002)
#define TPM_FAMILY_ADMIN               ((UINT32)0x00000003)
#define TPM_FAMILY_INVALIDATE          ((UINT32)0x00000004)

typedef UINT32 TPM_FAMILY_FLAGS;
#define TPM_FAMFLAG_DELEGATE_ADMIN_LOCK   (((UINT32)1)<<1)
#define TPM_FAMFLAG_ENABLE                (((UINT32)1)<<0)

typedef struct tdTPM_FAMILY_LABEL
{
    BYTE label;
} TPM_FAMILY_LABEL;

typedef struct tdTPM_FAMILY_TABLE_ENTRY
{
    TPM_STRUCTURE_TAG       tag;
    TPM_FAMILY_LABEL        label;
    TPM_FAMILY_ID           familyID;
    TPM_FAMILY_VERIFICATION verificationCount;
    TPM_FAMILY_FLAGS        flags;
} TPM_FAMILY_TABLE_ENTRY;


#define TPM_FAMILY_TABLE_ENTRY_MIN 8
//typedef struct tdTPM_FAMILY_TABLE
//{
//    TPM_FAMILY_TABLE_ENTRY FamTableRow[TPM_NUM_FAMILY_TABLE_ENTRY_MIN];
//} TPM_FAMILY_TABLE;


typedef struct tdTPM_DELEGATE_LABEL
{
    BYTE label;
} TPM_DELEGATE_LABEL;


typedef UINT32 TPM_DELEGATE_TYPE;
#define TPM_DEL_OWNER_BITS             ((UINT32)0x00000001)
#define TPM_DEL_KEY_BITS               ((UINT32)0x00000002)

typedef struct tdTPM_DELEGATIONS
{
    TPM_STRUCTURE_TAG tag;
    TPM_DELEGATE_TYPE delegateType;
    UINT32            per1;
    UINT32            per2;
} TPM_DELEGATIONS;

typedef struct tdTPM_DELEGATE_PUBLIC
{
    TPM_STRUCTURE_TAG       tag;
    TPM_DELEGATE_LABEL      label;
    TPM_PCR_INFO_SHORT      pcrInfo;
    TPM_DELEGATIONS         permissions;
    TPM_FAMILY_ID           familyID;
    TPM_FAMILY_VERIFICATION verificationCount;
} TPM_DELEGATE_PUBLIC;

typedef struct tdTPM_DELEGATE_TABLE_ROW
{
    TPM_STRUCTURE_TAG   tag;
    TPM_DELEGATE_PUBLIC pub;
    TPM_SECRET          authValue;
} TPM_DELEGATE_TABLE_ROW;


#define TPM_NUM_DELEGATE_TABLE_ENTRY_MIN 2
//typedef struct tdTPM_DELEGATE_TABLE
//{
//    TPM_DELEGATE_TABLE_ROW delRow[TPM_NUM_DELEGATE_TABLE_ENTRY_MIN];
//} TPM_DELEGATE_TABLE;

typedef struct tdTPM_DELEGATE_SENSITIVE
{
    TPM_STRUCTURE_TAG tag;
    TPM_SECRET        authValue;
} TPM_DELEGATE_SENSITIVE;

typedef struct tdTPM_DELEGATE_OWNER_BLOB
{
    TPM_STRUCTURE_TAG   tag;
    TPM_DELEGATE_PUBLIC pub;
    TPM_DIGEST          integrityDigest;
    UINT32              additionalSize;
    SIZEIS(additionalSize)
        BYTE           *additionalArea;
    UINT32              sensitiveSize;
    SIZEIS(sensitiveSize)
        BYTE           *sensitiveArea;
} TPM_DELEGATE_OWNER_BLOB;

typedef struct tdTPM_DELEGATE_KEY_BLOB
{
    TPM_STRUCTURE_TAG   tag;
    TPM_DELEGATE_PUBLIC pub;
    TPM_DIGEST          integrityDigest;
    TPM_DIGEST          pubKeyDigest;
    UINT32              additionalSize;
    SIZEIS(additionalSize)
        BYTE           *additionalArea;
    UINT32              sensitiveSize;
    SIZEIS(sensitiveSize)
        BYTE           *sensitiveArea;
} TPM_DELEGATE_KEY_BLOB;


//-------------------------------------------------------------------
// Part 2, section 21.1: TPM_CAPABILITY_AREA

typedef UINT32 TPM_CAPABILITY_AREA;                         /* 1.1b */
#define TPM_CAP_ORD                    ((UINT32)0x00000001) /* 1.1b */
#define TPM_CAP_ALG                    ((UINT32)0x00000002) /* 1.1b */
#define TPM_CAP_PID                    ((UINT32)0x00000003) /* 1.1b */
#define TPM_CAP_FLAG                   ((UINT32)0x00000004) /* 1.1b */
#define TPM_CAP_PROPERTY               ((UINT32)0x00000005) /* 1.1b */
#define TPM_CAP_VERSION                ((UINT32)0x00000006) /* 1.1b */
#define TPM_CAP_KEY_HANDLE             ((UINT32)0x00000007) /* 1.1b */
#define TPM_CAP_CHECK_LOADED           ((UINT32)0x00000008) /* 1.1b */
#define TPM_CAP_SYM_MODE               ((UINT32)0x00000009)
#define TPM_CAP_KEY_STATUS             ((UINT32)0x0000000C)
#define TPM_CAP_NV_LIST                ((UINT32)0x0000000D)
#define TPM_CAP_MFR                    ((UINT32)0x00000010)
#define TPM_CAP_NV_INDEX               ((UINT32)0x00000011)
#define TPM_CAP_TRANS_ALG              ((UINT32)0x00000012)
#define TPM_CAP_HANDLE                 ((UINT32)0x00000014)
#define TPM_CAP_TRANS_ES               ((UINT32)0x00000015)
#define TPM_CAP_AUTH_ENCRYPT           ((UINT32)0x00000017)
#define TPM_CAP_SELECT_SIZE            ((UINT32)0x00000018)
#define TPM_CAP_DA_LOGIC               ((UINT32)0x00000019)
#define TPM_CAP_VERSION_VAL            ((UINT32)0x0000001A)

// Part 2, section 21.1: Subcap values for CAP_FLAG
#define TPM_CAP_FLAG_PERMANENT         ((UINT32)0x00000108)
#define TPM_CAP_FLAG_VOLATILE          ((UINT32)0x00000109)

//-------------------------------------------------------------------
// Part 2, section 21.2: Subcap values for CAP_PROPERTY

#define TPM_CAP_PROP_PCR               ((UINT32)0x00000101) /* 1.1b */
#define TPM_CAP_PROP_DIR               ((UINT32)0x00000102) /* 1.1b */
#define TPM_CAP_PROP_MANUFACTURER      ((UINT32)0x00000103) /* 1.1b */
#define TPM_CAP_PROP_KEYS              ((UINT32)0x00000104)
#define TPM_CAP_PROP_SLOTS             (TPM_CAP_PROP_KEYS)
#define TPM_CAP_PROP_MIN_COUNTER       ((UINT32)0x00000107)
#define TPM_CAP_PROP_AUTHSESS          ((UINT32)0x0000010A)
#define TPM_CAP_PROP_TRANSSESS         ((UINT32)0x0000010B)
#define TPM_CAP_PROP_COUNTERS          ((UINT32)0x0000010C)
#define TPM_CAP_PROP_MAX_AUTHSESS      ((UINT32)0x0000010D)
#define TPM_CAP_PROP_MAX_TRANSSESS     ((UINT32)0x0000010E)
#define TPM_CAP_PROP_MAX_COUNTERS      ((UINT32)0x0000010F)
#define TPM_CAP_PROP_MAX_KEYS          ((UINT32)0x00000110)
#define TPM_CAP_PROP_OWNER             ((UINT32)0x00000111)
#define TPM_CAP_PROP_CONTEXT           ((UINT32)0x00000112)
#define TPM_CAP_PROP_MAX_CONTEXT       ((UINT32)0x00000113)
#define TPM_CAP_PROP_FAMILYROWS        ((UINT32)0x00000114)
#define TPM_CAP_PROP_TIS_TIMEOUT       ((UINT32)0x00000115)
#define TPM_CAP_PROP_STARTUP_EFFECT    ((UINT32)0x00000116)
#define TPM_CAP_PROP_DELEGATE_ROW      ((UINT32)0x00000117)
#define TPM_CAP_PROP_MAX_DAASESS       ((UINT32)0x00000119)
#define TPM_CAP_PROP_DAA_MAX           TPM_CAP_PROP_MAX_DAASESS
#define TPM_CAP_PROP_DAASESS           ((UINT32)0x0000011A)
#define TPM_CAP_PROP_SESSION_DAA       TPM_CAP_PROP_DAASESS
#define TPM_CAP_PROP_CONTEXT_DIST      ((UINT32)0x0000011B)
#define TPM_CAP_PROP_DAA_INTERRUPT     ((UINT32)0x0000011C)
#define TPM_CAP_PROP_SESSIONS          ((UINT32)0x0000011D)
#define TPM_CAP_PROP_MAX_SESSIONS      ((UINT32)0x0000011E)
#define TPM_CAP_PROP_CMK_RESTRICTION   ((UINT32)0x0000011F)
#define TPM_CAP_PROP_DURATION          ((UINT32)0x00000120)
#define TPM_CAP_PROP_ACTIVE_COUNTER    ((UINT32)0x00000122)
#define TPM_CAP_PROP_NV_AVAILABLE      ((UINT32)0x00000123)
#define TPM_CAP_PROP_INPUT_BUFFER      ((UINT32)0x00000124)


// Part 2, section 21.4: SetCapability Values
#define TPM_SET_PERM_FLAGS             ((UINT32)0x00000001)
#define TPM_SET_PERM_DATA              ((UINT32)0x00000002)
#define TPM_SET_STCLEAR_FLAGS          ((UINT32)0x00000003)
#define TPM_SET_STCLEAR_DATA           ((UINT32)0x00000004)
#define TPM_SET_STANY_FLAGS            ((UINT32)0x00000005)
#define TPM_SET_STANY_DATA             ((UINT32)0x00000006)
#define TPM_SET_VENDOR                 ((UINT32)0x00000007)


// Part 2, section 21.6: TPM_CAP_VERSION_INFO
typedef struct tdTPM_CAP_VERSION_INFO
{
    TPM_STRUCTURE_TAG tag;
    TPM_VERSION       version;
    UINT16            specLevel;
    BYTE              errataRev;
    BYTE              tpmVendorID[4];
    UINT16            vendorSpecificSize;
    SIZEIS(vendorSpecificSize)
        BYTE         *vendorSpecific;
} TPM_CAP_VERSION_INFO;


// Part 2, section 21.9: TPM_DA_STATE
// out of order to make it available for structure definitions
typedef BYTE TPM_DA_STATE;
#define TPM_DA_STATE_INACTIVE          (0x00)
#define TPM_DA_STATE_ACTIVE            (0x01)

// Part 2, section 21.10: TPM_DA_ACTION_TYPE
typedef struct tdTPM_DA_ACTION_TYPE
{
    TPM_STRUCTURE_TAG tag;
    UINT32            actions;
} TPM_DA_ACTION_TYPE;
#define TPM_DA_ACTION_TIMEOUT          ((UINT32)0x00000001)
#define TPM_DA_ACTION_DISABLE          ((UINT32)0x00000002)
#define TPM_DA_ACTION_DEACTIVATE       ((UINT32)0x00000004)
#define TPM_DA_ACTION_FAILURE_MODE     ((UINT32)0x00000008)

// Part 2, section 21.7: TPM_DA_INFO
typedef struct tdTPM_DA_INFO
{
    TPM_STRUCTURE_TAG  tag;
    TPM_DA_STATE       state;
    UINT16             currentCount;
    UINT16             threshholdCount;
    TPM_DA_ACTION_TYPE actionAtThreshold;
    UINT32             actionDependValue;
    UINT32             vendorDataSize;
    SIZEIS(vendorDataSize)
        BYTE          *vendorData;
} TPM_DA_INFO;

// Part 2, section 21.8: TPM_DA_INFO_LIMITED
typedef struct tdTPM_DA_INFO_LIMITED
{
    TPM_STRUCTURE_TAG  tag;
    TPM_DA_STATE       state;
    TPM_DA_ACTION_TYPE actionAtThreshold;
    UINT32             vendorDataSize;
    SIZEIS(vendorDataSize)
        BYTE          *vendorData;
} TPM_DA_INFO_LIMITED;



//-------------------------------------------------------------------
// Part 2, section 22: DAA Structures

#define TPM_DAA_SIZE_r0                (43)
#define TPM_DAA_SIZE_r1                (43)
#define TPM_DAA_SIZE_r2                (128)
#define TPM_DAA_SIZE_r3                (168)
#define TPM_DAA_SIZE_r4                (219)
#define TPM_DAA_SIZE_NT                (20)
#define TPM_DAA_SIZE_v0                (128)
#define TPM_DAA_SIZE_v1                (192)
#define TPM_DAA_SIZE_NE                (256)
#define TPM_DAA_SIZE_w                 (256)
#define TPM_DAA_SIZE_issuerModulus     (256)
#define TPM_DAA_power0                 (104)
#define TPM_DAA_power1                 (1024)

typedef struct tdTPM_DAA_ISSUER
{
    TPM_STRUCTURE_TAG tag;
    TPM_DIGEST        DAA_digest_R0;
    TPM_DIGEST        DAA_digest_R1;
    TPM_DIGEST        DAA_digest_S0;
    TPM_DIGEST        DAA_digest_S1;
    TPM_DIGEST        DAA_digest_n;
    TPM_DIGEST        DAA_digest_gamma;
    BYTE              DAA_generic_q[26];
} TPM_DAA_ISSUER;


typedef struct tdTPM_DAA_TPM
{
    TPM_STRUCTURE_TAG tag;
    TPM_DIGEST        DAA_digestIssuer;
    TPM_DIGEST        DAA_digest_v0;
    TPM_DIGEST        DAA_digest_v1;
    TPM_DIGEST        DAA_rekey;
    UINT32            DAA_count;
} TPM_DAA_TPM;

typedef struct tdTPM_DAA_CONTEXT
{
    TPM_STRUCTURE_TAG    tag;
    TPM_DIGEST           DAA_digestContext;
    TPM_DIGEST           DAA_digest;
    TPM_DAA_CONTEXT_SEED DAA_contextSeed;
    BYTE                 DAA_scratch[256];
    BYTE                 DAA_stage;
} TPM_DAA_CONTEXT;

typedef struct tdTPM_DAA_JOINDATA
{
    BYTE       DAA_join_u0[128];
    BYTE       DAA_join_u1[138];
    TPM_DIGEST DAA_digest_n0;
} TPM_DAA_JOINDATA;

typedef struct tdTPM_DAA_BLOB
{
    TPM_STRUCTURE_TAG tag;
    TPM_RESOURCE_TYPE resourceType;
    BYTE              label[16];
    TPM_DIGEST        blobIntegrity;
    UINT32            additionalSize;
    SIZEIS(additionalSize)
        BYTE         *additionalData;
    UINT32            sensitiveSize;
    SIZEIS(sensitiveSize)
        BYTE         *sensitiveData;
} TPM_DAA_BLOB;

typedef struct tdTPM_DAA_SENSITIVE
{
    TPM_STRUCTURE_TAG tag;
    UINT32            internalSize;
    SIZEIS(internalSize)
        BYTE         *internalData;
} TPM_DAA_SENSITIVE;



//-------------------------------------------------------------------
// Part 2, section 23: Redirection

// This section of the TPM spec defines exactly one value but does not
// give it a name. The definition of TPM_SetRedirection in Part3
// refers to exactly one name but does not give its value. We join
// them here.
#define TPM_REDIR_GPIO              (0x00000001)


//-------------------------------------------------------------------
// Part 2, section 24.6: TPM_SYM_MODE
//    Deprecated by TPM 1.2 spec

typedef UINT32 TPM_SYM_MODE;
#define TPM_SYM_MODE_ECB            (0x00000001)
#define TPM_SYM_MODE_CBC            (0x00000002)
#define TPM_SYM_MODE_CFB            (0x00000003)

#endif // __TPM_H__

