/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. 
 * 
 * You may obtain a copy of the License at 
 *     http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" BASIS, 
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
 * 
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __TESTS__
#define __TESTS_

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <direct.h>
#define snprintf _snprintf
#define random   rand
#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <openpgpsdk/memory.h>
#include <openpgpsdk/create.h>

// test suites

#include "CUnit/Basic.h"

CU_pSuite suite_crypto();
extern CU_pSuite suite_cmdline();
extern CU_pSuite suite_packet_types();
extern CU_pSuite suite_rsa_decrypt();
extern CU_pSuite suite_rsa_encrypt();
extern CU_pSuite suite_rsa_signature();
extern CU_pSuite suite_rsa_verify();
extern CU_pSuite suite_rsa_keys();

extern CU_pSuite suite_dsa_signature();
extern CU_pSuite suite_dsa_verify();

extern CU_pSuite suite_rsa_decrypt_GPGtest();
extern CU_pSuite suite_rsa_encrypt_GPGtest();
extern CU_pSuite suite_rsa_signature_GPGtest();
extern CU_pSuite suite_rsa_verify_GPGtest();

// utility functions

extern char gpgcmd[];
void setup();
void cleanup();

int run(const char* cmd);

int mktmpdir();
extern char dir[];
char* create_testtext(const char *text, const unsigned int repeats);
void create_testdata(const char *text, unsigned char *buf, const int maxlen);
void create_small_testfile(const char *name);
void create_large_testfile(const char *name);
void create_testfile(const char *name, const unsigned int repeats);
#define MAXBUF 1024

ops_parse_cb_return_t
callback_general(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
callback_cmd_get_secret_key(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
test_cb_get_passphrase(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
callback_pk_session_key(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
callback_data_signature(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
callback_verify(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
callback_verify_example(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);


void reset_vars();
int file_compare(char* file1, char* file2);

ops_keyring_t pub_keyring;
ops_keyring_t sec_keyring;
//ops_memory_t* mem_literal_data;

// "Alpha" is the user who has NO passphrase on his key - RSA
char alpha_user_id[MAXBUF+1];
char* alpha_comment;
char* alpha_name;
const ops_keydata_t *alpha_pub_keydata;
const ops_keydata_t *alpha_sec_keydata;
const ops_public_key_t *alpha_pkey;
const ops_secret_key_t *alpha_skey;
char* alpha_passphrase;

// "Bravo" is the user who has a passphrase on his key - RSA
char* bravo_name;
char* bravo_comment;
char* bravo_passphrase;
char bravo_user_id[MAXBUF+1];
const ops_keydata_t *bravo_pub_keydata;
const ops_keydata_t *bravo_sec_keydata;
const ops_public_key_t *bravo_pkey;
const ops_secret_key_t *bravo_skey;
//const ops_keydata_t *decrypter;

// "AlphaDSA" is the user who has NO passphrase on his key - DSA
char alphadsa_user_id[MAXBUF+1];
char* alphadsa_name;
const ops_keydata_t *alphadsa_pub_keydata;
const ops_keydata_t *alphadsa_sec_keydata;
const ops_public_key_t *alphadsa_pkey;
const ops_secret_key_t *alphadsa_skey;
char* alphadsa_passphrase;

// "BravoDSA" is the user who has a passphrase on his key - DSA
char* bravodsa_name;
char* bravodsa_passphrase;
char bravodsa_user_id[MAXBUF+1];
const ops_keydata_t *bravodsa_pub_keydata;
const ops_keydata_t *bravodsa_sec_keydata;
const ops_public_key_t *bravodsa_pkey;
const ops_secret_key_t *bravodsa_skey;
//const ops_keydata_t *decrypter;

// "Charlie" is a 3rd user, used to test cmd line generation
//char* charlie_name;
//char* charlie_passphrase;
char* charlie_user_id;

// defs
#define MAXBUF 1024
#define MAX_COMPRESS_LEVEL 7

#ifndef ATTRIBUTE_UNUSED

#ifndef WIN32
#define ATTRIBUTE_UNUSED __attribute__ ((__unused__))
#else
#define ATTRIBUTE_UNUSED 
#endif // #ifndef WIN32

#endif /* ATTRIBUTE_UNUSED */

typedef struct 
    {
    unsigned keysize;
    unsigned qsize;
    ops_hash_algorithm_t hashalg;
    char userid[MAXBUF+1];
    char filename[MAXBUF+1];
    } dsatest_t;
extern dsatest_t dsstests[];
extern const unsigned sz_dsstests;

#endif

