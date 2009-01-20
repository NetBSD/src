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

#include "tests.h"

#include "CUnit/Basic.h"

#include <openpgpsdk/types.h>
#include "openpgpsdk/keyring.h"
#include <openpgpsdk/armour.h>
#include "openpgpsdk/packet.h"
#include "openpgpsdk/packet-parse.h"
#include "openpgpsdk/readerwriter.h"
#include "openpgpsdk/util.h"
#include "openpgpsdk/std_print.h"
#include "../src/lib/parse_local.h"

static char *compress_algos[]={ "zip", "zlib", "bzip2" };
static int n_compress_algos=3;

static ops_parse_cb_return_t
callback(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    ops_parser_content_union_t* content=(ops_parser_content_union_t *)&content_->content;
    static ops_boolean_t skipping;

    OPS_USED(cbinfo);

//    ops_print_packet(content_);

    if(content_->tag != OPS_PTAG_CT_UNARMOURED_TEXT && skipping)
	{
	puts("...end of skip");
	skipping=ops_false;
	}

    switch(content_->tag)
	{
    case OPS_PTAG_CT_UNARMOURED_TEXT:
	printf("OPS_PTAG_CT_UNARMOURED_TEXT\n");
	if(!skipping)
	    {
	    puts("Skipping...");
	    skipping=ops_true;
	    }
	fwrite(content->unarmoured_text.data,1,
	       content->unarmoured_text.length,stdout);
	break;

    case OPS_PTAG_CT_PK_SESSION_KEY:
        return callback_pk_session_key(content_,cbinfo);
        break;

    case OPS_PARSER_CMD_GET_SECRET_KEY:
        return callback_cmd_get_secret_key(content_,cbinfo);
        break;

    case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
        return test_cb_get_passphrase(content_,cbinfo);
        break;

    case OPS_PTAG_CT_LITERAL_DATA_BODY:
        return callback_literal_data(content_,cbinfo);
		break;

    case OPS_PTAG_CT_ARMOUR_HEADER:
    case OPS_PTAG_CT_ARMOUR_TRAILER:
    case OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY:
    case OPS_PTAG_CT_COMPRESSED:
    case OPS_PTAG_CT_LITERAL_DATA_HEADER:
    case OPS_PTAG_CT_SE_IP_DATA_BODY:
    case OPS_PTAG_CT_SE_IP_DATA_HEADER:
    case OPS_PTAG_CT_SE_DATA_BODY:
    case OPS_PTAG_CT_SE_DATA_HEADER:

	// Ignore these packets 
	// They're handled in ops_parse_one_packet()
	// and nothing else needs to be done
	break;

    default:
        return callback_general(content_,cbinfo);
        //	fprintf(stderr,"Unexpected packet tag=%d (0x%x)\n",content_->tag,
        //		content_->tag);
        //	assert(0);
	}

    return OPS_RELEASE_MEMORY;
    }

/* Decryption suite initialization.
 * Create temporary directory.
 * Create temporary test files.
 */

int init_suite_rsa_decrypt(void)
    {
    // Return success
    return 0;
    }

int clean_suite_rsa_decrypt(void)
    {
	
    reset_vars();

    return 0;
    }

static void test_rsa_decrypt(const int has_armour, const char *filename)
    {
    char encfile[MAXBUF+1];
    char* testtext=NULL;
    char *suffix= has_armour ? "asc" : "gpg";
    int fd=0;
    ops_parse_info_t *pinfo=NULL;
    ops_memory_t* mem_out=NULL;
    int rtn=0;
    int repeats=10;

    // open encrypted file
    snprintf(encfile,sizeof encfile,"%s/%s.%s",dir,
             filename,suffix);

    // setup for reading from given input file
    ops_setup_file_read(&pinfo, encfile,
                        NULL, /* arg */
                        callback,
                        ops_false /* accumulate */
                        );

    // setup keyring and passphrase callback
    pinfo->cbinfo.cryptinfo.keyring=&sec_keyring;
    pinfo->cbinfo.cryptinfo.cb_get_passphrase=test_cb_get_passphrase;

    // Set up armour/passphrase options

    if (has_armour)
        ops_reader_push_dearmour(pinfo);
    
    // setup for writing parsed data to mem_out
    ops_setup_memory_write(&pinfo->cbinfo.cinfo, &mem_out, 128);

    // do it
    rtn=ops_parse_and_print_errors(pinfo);
    CU_ASSERT(rtn==1);

    // Tidy up
    if (has_armour)
        ops_reader_pop_dearmour(pinfo);

    close(fd);
    
    // File contents should match
    testtext=create_testtext(filename,repeats);
    CU_ASSERT(strlen(testtext)==ops_memory_get_length(mem_out));
    CU_ASSERT(memcmp(ops_memory_get_data(mem_out),
                     testtext,
                     ops_memory_get_length(mem_out))==0);
    }

static void create_filename(char* buf, int maxbuf, int armour, int passphrase, char * sym_alg, char * compress_alg, int compress_level)
    {
    snprintf(buf,maxbuf,"gpg_enc_rsa_%s_%s_%s_%s_%d.txt",
        armour ? "arm" : "noarm",
        passphrase ? "pp" : "nopp",
        sym_alg,
        compress_alg, compress_level);
    }

static void test_rsa_decrypt_generic(char* sym_alg)
    {
    char filename[MAXBUF+1];
    char cmd[MAXBUF+1];
    int armour=0;
    int passphrase=0;
    int compress_alg=0;
    int compress_lvl=0;

    for (compress_alg=0; compress_alg<n_compress_algos; compress_alg++)
        {
        for (compress_lvl=0; compress_lvl<=MAX_COMPRESS_LEVEL; compress_lvl++)
            {
            /* only need to check every compression level if we're debugging */
            if (compress_lvl>0 && compress_lvl < MAX_COMPRESS_LEVEL)
                continue;
            for (armour=0; armour<=1; armour++)
                {
                char *armour_cmd= armour ? "--armor " : "";
                char *suffix= armour ? "asc" : "gpg";
                
                for (passphrase=0; passphrase<=1; passphrase++)
                    {
                    char *rcpt= passphrase ? "Bravo" : "Alpha";

                    // Create filename matching these params
                    create_filename(&filename[0],sizeof filename,
                                    armour, passphrase,
                                    sym_alg, 
                                    compress_algos[compress_alg], compress_lvl);
                    
                    // Create file with unique text matching this test
                    create_small_testfile(filename);
                    
                    // Encrypt file using GPG
                    snprintf(cmd,sizeof cmd,"gpg --quiet --no-tty --homedir=%s --cipher-algo \"%s\" --compress-algo \"%s\" --compress-level %d --output=%s/%s.%s  --force-mdc --encrypt --recipient %s %s %s/%s", 
                             dir, //homedir
                             sym_alg,
                             compress_algos[compress_alg],
                             compress_lvl,
                             dir, filename, suffix, // for output file
                             rcpt,
                             armour_cmd,
                             dir, filename);
                    if (run(cmd))
                        {
                        fprintf(stderr,"Err: cmd is %s\n", cmd);
                        //                        return 1;
                        }
                    
                    // Decrypt using OPS
                    test_rsa_decrypt(armour,filename);
                    }
                }
            }
        }
    }

static void test_rsa_decrypt_cast5(void)
    {
    return test_rsa_decrypt_generic("cast5");
    }

static void test_rsa_decrypt_aes128(void)
    {
    return test_rsa_decrypt_generic("aes");
    }

static void test_rsa_decrypt_aes256(void)
    {
    return test_rsa_decrypt_generic("aes256");
    }

static void test_rsa_decrypt_3des(void)
    {
    return test_rsa_decrypt_generic("3des");
    }

//

/*
static void test_todo(void)
    {
    CU_FAIL("Test FUTURE: Decryption with multiple keys in same keyring");
    CU_FAIL("Test FUTURE: Decryption with multiple keys where some are not in my keyring");
    CU_FAIL("Test FUTURE: Decryption with multiple keys where my key is (a) first key in list; (b) last key in list; (c) in the middle of the list");
    }
*/

static int add_tests(CU_pSuite suite)
    {
    // add tests to suite
    
    if (NULL == CU_add_test(suite, "CAST5", test_rsa_decrypt_cast5))
	    return 0;

    if (NULL == CU_add_test(suite, "AES128", test_rsa_decrypt_aes128))
	    return 0;

    if (NULL == CU_add_test(suite, "AES256", test_rsa_decrypt_aes256))
	    return 0;

    if (NULL == CU_add_test(suite, "3DES", test_rsa_decrypt_3des))
	    return 0;

    /*
    if (NULL == CU_add_test(suite, "Tests to be implemented", test_todo))
	    return 0;
    */
    return 1;
    }

CU_pSuite suite_rsa_decrypt()
{
    CU_pSuite suite = NULL;

    suite = CU_add_suite("RSA Decryption Suite", init_suite_rsa_decrypt, clean_suite_rsa_decrypt);
    if (!suite)
	    return NULL;

    if (!add_tests(suite))
        return NULL;

    return suite;
}

