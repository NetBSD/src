/*! \mainpage Documentation
 *
 * The OpenPGP::SDK library has 2 APIs (High Level and Core), which can be used interchangeably by a user of the library. 

There are also some functions documented here as Internal API, which will be of use to OpenPGP::SDK developers.

\section section_highlevel_api The High-Level API

The High-Level API provides easy access to common crypto tasks. 

Examples are:

- "find the key in the keyring corresponding to this id"
- "sign this text with that key".

It is built on functions offered by the Core API.

Developers should initially consider using the High-Level API, unless they need the additional control available in the Core API.

- \ref HighLevelAPI : follow this link for more details

\section section_core_api The Core API

The Core API offers detailed control over all aspects of the SDK.

- \ref CoreAPI : follow this link for more details

\section section_internal_api The Internal API

The Internal API contains functions for use by SDK developers.

- \ref InternalAPI: follow this link for more details

*/

/** @defgroup HighLevelAPI High Level API
\brief Easy access to OpenPGP::SDK functions

This API provides basic high-level functionality, which should be
suitable for most users.

If you want more fine-grained control, consider using the Core API.

A good place to look for example code which uses the High Level API is the command line application, found in openpgpsdk/src/app/openpgp.c.

Example code:
\code
void main()
{
  ops_uid_id_t uid;
  char* pubring_name="pubring.gpg";
  char* secring_name="secring.gpg";
  char* passphrase="mysecret";
  int pplen=strlen(passphrase);
  ops_keyring_t* pubring=NULL;
  ops_keyring_t* secring=NULL;
  ops_keydata_t * keydata=NULL;
  ops_create_info_t* cinfo=NULL;
  ops_secret_key_t* skey=NULL;
  ops_validate_result* validate_result=NULL;
  int fd=0;

  ops_init();

  // Create a new self-signed RSA key pair
  uid.user_id=(unsigned char *) "Test User (RSA 2048-bit key) <testuser@test.com>";
  keydata=ops_rsa_create_selfsigned_keypair(2048,65537,&uid);
  if (!keydata)
    exit (-1);

  // Append to keyrings
  fd=ops_setup_file_append(&cinfo, pubring_name);
  ops_write_transferable_public_key(keydata, ARMOUR_NO, cinfo);
  ops_teardown_file_write(cinfo,fd)

  fd=ops_setup_file_append(&cinfo, secring_name);
  ops_write_transferable_secret_key(keydata, passphrase, pplen, ARMOUR_NO, cinfo);
  ops_teardown_file_write(cinfo,fd)

  // Load public and secret keyrings
  if (!ops_keyring_read_from_file(pubring, ARMOUR_NO, pubring_name))
    exit(-1);
  if (!ops_keyring_read_from_file(secring, ARMOUR_NO, secring_name))
    exit(-1);

  // Sign a file with the new secret key
  skey=ops_decrypt_secret_key_from_data(keydata,passphrase);
  if (!ops_sign_file("mytestfile", NULL, skey, ARMOUR_YES, OVERWRITE_YES))
    exit(-1);

  // Verify signed file with new public key
  validate_result=ops_mallocz(sizeof *validate_result);
  if (ops_validate_file(validate_result, "mytestfile.asc", ARMOUR_YES, pubring)==ops_true)
    printf("OK\n")
  else
    printf("Not verified OK: %d invalid signatures, %d unknown signatures\n", validate_result->invalid_count, validate_result->unknown_signer_count);
  ops_validate_result_free(validate_result);

  // Encrypt a file with the new public key
  if (ops_encrypt_file("mytestfile2", NULL, keydata, ARMOUR_YES, OVERWRITE_YES)!=ops_true)
    exit(-1);

  // Decrypt encrypted file with the new secret key
  if (ops_decrypt_file("mytestfile2.asc",NULL, secring, ARMOUR_YES, OVERWRITE_YES, callback_cmd_get_passphrase_from_cmdline)!=ops_true)
    exit(-1)

  ops_finish();
}
\endcode
*/

/** @defgroup CoreAPI Core API
This API provides detailed control over all aspects of the SDK.

You may find that the easier-to-use High Level API meets your needs.

Please note that the Core API documentation is still a Work-In-Progress.

If you are using the Core API for the first time, you may find that the best place to start if by finding an existing function which does most of what you need and modifying it. A good place to look for such a function is in the test suite (openpgpsdk/tests).
 *
 * \par To Read OpenPGP packets
 * - \ref Core_ReadPackets
 * - \ref Core_Readers
 *
 * \par Usage 1 : To Parse an input stream (discarding parsed data)
 * - Configure an ops_parse_options_t structure
 *   - Set "Reader" function and args (to get the data)
 *   - Set "Callback" function and args (to do something useful with the parsed data)
 * - Call ops_parse_options() to specify whether individual subpacket types are to parsed, left raw or ignored
 * - Finally, call ops_parse() 
 *
 * \par Usage 2 : To Parse an input stream (storing parsed data in keyring)
 * - Get keyring
 * - Configure an ops_parse_options_t structure
 *   - Set "Reader" function and args (to get the data)
 *   - No need to set "Callback" function and args 
 * - No need to call ops_parse_options() to specify whether individual subpacket types are to parsed, left raw or ignored
 * - Call ops_parse_and_accumulate() to populate keyring
 * - Don't forget to call ops_keyring_free() when you've finished with the keyring to release the memory.
 */

/** @defgroup InternalAPI Internal API
This API provides code used by SDK developers.
*/

/** \defgroup HighLevel_Sign Sign File or Buffer
    \ingroup HighLevelAPI
 */
    
/** \defgroup HighLevel_Verify Verify File, Buffer, Key or Keyring
    \ingroup HighLevelAPI
 */
    
/** \defgroup HighLevel_Crypto Encrypt or Decrypt File
    \ingroup HighLevelAPI
 */
    
/**
    \defgroup HighLevel_Keyring Keys and Keyrings
    \ingroup HighLevelAPI
*/

/** \defgroup HighLevel_Supported Supported Algorithms
    \ingroup HighLevelAPI
 */
    
/** \defgroup HighLevel_Memory Memory
    \ingroup HighLevelAPI
 */
    
/**
    \defgroup HighLevel_General General
    \ingroup HighLevelAPI
*/

/**
    \defgroup HighLevel_KeyringRead Read Keyring
    \ingroup HighLevel_Keyring
*/

/**
    \defgroup HighLevel_KeyringList List Keyring
    \ingroup HighLevel_Keyring
*/

/**
    \defgroup HighLevel_KeyringFind Find Key
    \ingroup HighLevel_Keyring
*/

/**
    \defgroup HighLevel_KeyGenerate Generate Key
    \ingroup HighLevel_Keyring
*/

/**
    \defgroup HighLevel_KeyWrite Write Key
    \ingroup HighLevel_Keyring
*/

/**
    \defgroup HighLevel_KeyGeneral Other Key Functions
    \ingroup HighLevel_Keyring
*/

/** \defgroup Core_ReadPackets Read OpenPGP packets
    \ingroup CoreAPI
*/

/**
   \defgroup Core_Readers Readers
   \ingroup CoreAPI
*/

/** \defgroup Core_WritePackets Write OpenPGP packets
    \ingroup CoreAPI
*/


/**
   \defgroup HighLevel_Callbacks Callbacks
   \ingroup HighLevelAPI
*/

/**
   \defgroup Core_Writers Writers
   \ingroup CoreAPI
*/

/**
   \defgroup Core_WritersFirst First (stacks start with one of these)
   \ingroup Core_Writers
*/

/**
   \defgroup Core_WritersNext Next (stacks may use these)
   \ingroup Core_Writers
*/

/**
   \defgroup Core_Errors Error Handling
   \ingroup CoreAPI
*/

/**
   \defgroup Core_Readers_First First (stacks start with one of these)
   \ingroup Core_Readers
*/

/**
   \defgroup Core_Readers_Additional Additional (stacks may use these)
   \ingroup Core_Readers
*/

/**
   \defgroup Core_Readers_Armour Armoured Data
   \ingroup Core_Readers_Additional
*/

/**
   \defgroup Core_Readers_SE Symmetrically-Encrypted Data
   \ingroup Core_Readers_Additional
*/

/**
  \defgroup Core_Readers_SEIP Symmetrically-Encrypted-Integrity-Protected Data
  \ingroup Core_Readers_Additional
*/

/** \defgroup Core_Keys Keys and Keyrings
    \ingroup CoreAPI
 */
    
/** \defgroup Core_Hashes Hashes
    \ingroup CoreAPI
 */
    
/** \defgroup Core_Crypto Encryption/Decryption
    \ingroup CoreAPI
 */
    
/** \defgroup Core_Signature Signatures/Verification
    \ingroup CoreAPI
 */
    
/** \defgroup Core_Compress Compression/Decompression
    \ingroup CoreAPI
 */
    
/** \defgroup Core_MPI Functions to do with MPIs
    \ingroup CoreAPI
*/

/**
    \defgroup Core_Print Print
    \ingroup CoreAPI
*/

/** \defgroup Core_Lists Linked Lists
    \ingroup CoreAPI
 */
    
/** \defgroup Core_Memory Memory
    \ingroup CoreAPI
 */
    
/**
   \defgroup Core_Callbacks Callbacks
   \ingroup CoreAPI
   These callback functions are used when parsing or creating.
*/

/** 
   \defgroup Internal_Readers Readers
   \ingroup InternalAPI
*/

/**
   \defgroup Internal_Readers_Generic Generic
   \ingroup Internal_Readers
*/

/**
  \defgroup Internal_Readers_Hash Hashed Data
  \ingroup Internal_Readers
*/

/**
  \defgroup Internal_Readers_Sum16 Sum16
  \ingroup Internal_Readers
*/

/**
 * \defgroup Core_Create Create 
 * \ingroup CoreAPI
 * These functions allow an OpenPGP object to be created. 
 */



