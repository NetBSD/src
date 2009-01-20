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

#include "CUnit/Basic.h"

#include <openpgpsdk/defs.h>
#include <openpgpsdk/types.h>
#include "openpgpsdk/keyring.h"
#include <openpgpsdk/armour.h>
#include "openpgpsdk/memory.h"
#include "openpgpsdk/packet.h"
#include "openpgpsdk/packet-parse.h"
#include "openpgpsdk/util.h"
#include "openpgpsdk/std_print.h"
#include "openpgpsdk/readerwriter.h"
#include "openpgpsdk/validate.h"

#include "../src/lib/parse_local.h"

#include "tests.h"

static int debug=0;

static char *filename_rsa_armour_nopassphrase="gpg_rsa_sign_armour_nopassphrase.txt";
static char *filename_rsa_armour_passphrase="gpg_rsa_sign_armour_passphrase.txt";

static char *filename_rsa_noarmour_nopassphrase="gpg_rsa_sign_noarmour_nopassphrase.txt";
static char *filename_rsa_noarmour_passphrase="gpg_rsa_sign_noarmour_passphrase.txt";
static char *filename_rsa_noarmour_fail_bad_sig="gpg_rsa_sign_noarmour_fail_bad_sig.txt";

static char *filename_rsa_clearsign_nopassphrase="gpg_rsa_clearsign_nopassphrase.txt";
static char *filename_rsa_clearsign_passphrase="gpg_rsa_clearsign_passphrase.txt";
static char *filename_rsa_clearsign_fail_bad_sig="gpg_rsa_clearsign_fail_bad_sig.txt";

static char *filename_rsa_noarmour_compress_base="gpg_rsa_sign_noarmour_compress";
static char *filename_rsa_armour_compress_base="gpg_rsa_sign_armour_compress";

static char *filename_rsa_v3sig="gpg_rsa_sign_v3sig.txt";
static char *filename_rsa_v3sig_fail_bad_sig="gpg_rsa_sign_v3sig_fail_bad_sig.txt";

static char *filename_rsa_hash_md5="gpg_rsa_hash_md5.txt";

static char *filename_rsa_hash_sha256="gpg_rsa_hash_sha256.txt";

static int num_malformed=0;
static int num_wellformed=0;

typedef ops_parse_cb_return_t (*ops_callback)(const ops_parser_content_t *, ops_parse_cb_info_t *);

/* Signature verification suite initialization.
 * Create temporary test files.
 */

static void make_filename_malformed(char* filename, int maxlen, const int i)
    {
    snprintf(filename,maxlen,"malformed_%d.txt",i);
    }

static void make_filename_wellformed(char* filename, int maxlen, const int i)
    {
    snprintf(filename,maxlen,"wellformed_%d.txt",i);
    }

static void create_wellformed_testfiles()
    {
    int i=0;
    int fd=0;
    char* wellformed[]=
        {
        // no headers in header
        "-----BEGIN PGP SIGNED MESSAGE-----\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PGP SIGNATURE-----\n",
        // no headers in trailer
        "-----BEGIN PGP SIGNED MESSAGE-----\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PGP SIGNATURE-----\n",
        };
    num_wellformed=sizeof (wellformed)/sizeof(char *);
    for (i=0; i<num_wellformed; i++)
        {
        char fullname[MAXBUF];
        char filename[MAXBUF];
        make_filename_wellformed(filename,MAXBUF,i);
        snprintf(fullname,MAXBUF,"%s/%s.asc",dir,filename);
        if ((fd=open(fullname,O_WRONLY | O_CREAT, 0600)) < 0)
            {
            fprintf(stderr,"create_wellformed_testfiles: cannot open file %s for writing\n", fullname);
            return;
            }
        write(fd,wellformed[i],strlen(wellformed[i]));
        close(fd);
        }
}

static void create_malformed_testfiles()
    {
    int i=0;
    int fd=0;
    char* malformed[]=
        {
        // no signature
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA1\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\n-----END PGP SIGNATURE-----\n", 
        // no signature and early EOF
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA1\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\n-----END PGP SIGNATURE-----", 
        // early EOF
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA1\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n",
        // no signature
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA1\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: -----END PGP SIGNATURE-----GnuPG v1.4.6 (GNU/Linux)\n", 
        // no gap after armour headers in message
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA1\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PGP SIGNATURE-----\n",
        // no gap after armour headers in signature
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA1\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PGP SIGNATURE-----\n",
        // unsupported hash
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: RIPEMD160\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PGP SIGNATURE-----\n",
        // missing BEGIN SIG
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA1\n\nmessage to encrypt\n-----END PGP SIGNATURE-----",
        // missing END SIG
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA1\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n",
        // bad header line
        "-----BEGIN MESSAGE-----\nHash: SHA1\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PGP SIGNATURE-----\n",
        // bad header : no colon
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash SHA1\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PGP SIGNATURE-----\n",
        // bad header : invalid header
        "-----BEGIN PGP SIGNED MESSAGE-----\nUnknown: Header\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PGP SIGNATURE-----\n",
        // bad armour trailer
        "-----BEGIN PGP SIGNED MESSAGE-----\nHash: SHA1\n\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PPP SIGNATURE-----\n-----END PGP SIGNATURE-----",
        // no headers and no gap
        "-----BEGIN PGP SIGNED MESSAGE-----\nmessage to encrypt\n-----BEGIN PGP SIGNATURE-----\nVersion: GnuPG v1.4.6 (GNU/Linux)\n\niJwEAQECAAYFAkiup4kACgkQr5tWFB2nA4mpVwP8DeeMDFrp7ICHYleyW/UmBIQH\ndXuviEA9WK/BUyHVKxLOyciAw18vm1rKJE9Q30GUrFkPvaOV6XZXZMDBXY/CQixT\nHjKRoFapgbzA5hqDeLjjkJ59hjS5jmsOrdyIebOVrF7YaSRji15uAeeIzBQ0lClZ\nupkvjuuc6o0RoS/+otk=\n=itEi\n-----END PGP SIGNATURE-----\n",
    };
    num_malformed=sizeof (malformed)/sizeof(char *);
    for (i=0; i<num_malformed; i++)
        {
        char fullname[MAXBUF];
        char filename[MAXBUF];
        make_filename_malformed(filename,MAXBUF,i);
        snprintf(fullname,MAXBUF,"%s/%s.asc",dir,filename);
        if ((fd=open(fullname,O_WRONLY | O_CREAT, 0600)) < 0)
            {
            fprintf(stderr,"create_malformed_testfiles: cannot open file %s for writing\n", fullname);
            return;
            }
        write(fd,malformed[i],strlen(malformed[i]));
        close(fd);
        }
    }

int init_suite_rsa_verify(void)
    {
    char cmd[MAXBUF+1];

    // Create SIGNED test files

    create_small_testfile(filename_rsa_armour_nopassphrase);
    create_small_testfile(filename_rsa_armour_passphrase);

    create_small_testfile(filename_rsa_v3sig);
    create_small_testfile(filename_rsa_v3sig_fail_bad_sig);
    create_small_testfile(filename_rsa_hash_md5);
    create_small_testfile(filename_rsa_hash_sha256);

    create_small_testfile(filename_rsa_noarmour_nopassphrase);
    create_small_testfile(filename_rsa_noarmour_passphrase);
    create_small_testfile(filename_rsa_noarmour_fail_bad_sig);

    create_malformed_testfiles();
    create_wellformed_testfiles();

    // Now sign the test files with GPG

    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-level 0 --sign --local-user %s > %s/%s.gpg",
             dir, filename_rsa_noarmour_nopassphrase,
             gpgcmd, alpha_name, dir, filename_rsa_noarmour_nopassphrase);
    if (run(cmd))
        { return 1; }

    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-level 0 --sign --local-user %s --armor > %s/%s.asc",
             dir, filename_rsa_armour_nopassphrase,
             gpgcmd, alpha_name, dir, filename_rsa_armour_nopassphrase);
    if (run(cmd))
        { return 1; }

    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-level 0 --sign --local-user %s --passphrase %s > %s/%s.gpg",
             dir, filename_rsa_noarmour_passphrase,
             gpgcmd, bravo_name, bravo_passphrase, dir, filename_rsa_noarmour_passphrase);
    if (run(cmd))
        { return 1; }

    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-level 0 --sign --local-user %s --passphrase %s --armor > %s/%s.asc",
             dir, filename_rsa_armour_passphrase,
             gpgcmd, bravo_name, bravo_passphrase, dir, filename_rsa_armour_passphrase);
    if (run(cmd))
        { return 1; }

    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-level 0 --sign --local-user %s > %s/%s.gpg",
             dir, filename_rsa_noarmour_fail_bad_sig,
             gpgcmd, alpha_name, dir, filename_rsa_noarmour_fail_bad_sig);
    if (run(cmd))
        { return 1; }

    // V3 signature
    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --compress-level 0 --sign --force-v3-sigs --local-user %s > %s/%s.gpg",
             dir, filename_rsa_v3sig,
             gpgcmd, alpha_name, dir, filename_rsa_v3sig);
    if (run(cmd))
        { return 1; }

    // V3 signature to fail
    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --compress-level 0 --sign --force-v3-sigs --local-user %s > %s/%s.gpg",
             dir, filename_rsa_v3sig_fail_bad_sig,
             gpgcmd, alpha_name, dir, filename_rsa_v3sig_fail_bad_sig);
    if (run(cmd))
        { return 1; }

    // MD5 hash
    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --compress-level 0 --sign --digest-algo \"MD5\" --local-user %s > %s/%s.gpg",
             dir, filename_rsa_hash_md5,
             gpgcmd, alpha_name, dir, filename_rsa_hash_md5);
    if (run(cmd))
        { return 1; }

    // SHA256 hash
    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --compress-level 0 --sign --digest-algo \"SHA256\" --local-user %s > %s/%s.gpg",
             dir, filename_rsa_hash_sha256,
             gpgcmd, alpha_name, dir, filename_rsa_hash_sha256);
    if (run(cmd))
        { return 1; }

    /*
     * Create CLEARSIGNED test files
     */

    create_small_testfile(filename_rsa_clearsign_nopassphrase);
    create_small_testfile(filename_rsa_clearsign_passphrase);
    create_small_testfile(filename_rsa_clearsign_fail_bad_sig);

    // and sign them

    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-level 0 --clearsign --textmode --local-user %s --armor > %s/%s.asc",
             dir, filename_rsa_clearsign_nopassphrase,
             gpgcmd, alpha_name, dir, filename_rsa_clearsign_nopassphrase);
    if (run(cmd))
        { return 1; }

    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-level 0 --clearsign --textmode --local-user %s --passphrase %s --armor > %s/%s.asc",
             dir, filename_rsa_clearsign_passphrase,
             gpgcmd, bravo_name, bravo_passphrase, dir, filename_rsa_clearsign_passphrase);
    if (run(cmd))
        { return 1; }

    snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-level 0 --clearsign --textmode --local-user %s --armor > %s/%s.asc",
             dir, filename_rsa_clearsign_fail_bad_sig,
             gpgcmd, alpha_name, dir, filename_rsa_clearsign_fail_bad_sig);
    if (run(cmd))
        { return 1; }
    // sig will be turned bad on verification
    // \todo make sig bad here instead

    // compression

    int level=0;
    for (level=0; level<=MAX_COMPRESS_LEVEL; level++)
        {
        char filename[MAXBUF+1];

        // unarmoured
        snprintf(filename, sizeof filename, "%s_%d.txt", 
                 filename_rsa_noarmour_compress_base, level);
        create_small_testfile(filename);

        // just ZIP for now
        snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-algo \"ZIP\" --compress-level %d --sign --local-user %s > %s/%s.gpg", 
                 dir, filename, 
                 gpgcmd, level, alpha_name, dir, filename);
        if (run(cmd))
            {
            return 1;
            }

        // armoured
        snprintf(filename, sizeof filename, "%s_%d.txt", 
                 filename_rsa_armour_compress_base, level);
        create_small_testfile(filename);

        snprintf(cmd,sizeof cmd,"cat %s/%s | %s --openpgp --compress-algo \"ZIP\" --compress-level %d --sign --armour --local-user %s > %s/%s.asc", 
                 dir, filename, 
                 gpgcmd, level, alpha_name, dir, filename);
        if (run(cmd))
            {
            return 1;
            }
        }
    // Return success
    return 0;
    }

int clean_suite_rsa_verify(void)
    {
    ops_finish();

    reset_vars();

    return 0;
    }

static int test_rsa_verify(const int has_armour, const char *filename, ops_callback callback, ops_parse_info_t *pinfo)
    {
    char signedfile[MAXBUF+1];
    char *suffix= has_armour ? "asc" : "gpg";
    int fd=0;
    validate_data_cb_arg_t validate_arg;
    ops_validate_result_t* result;
    int rtn=0;
    ops_memory_t* mem=NULL;
    int errnum=0;

    result=ops_mallocz(sizeof (ops_validate_result_t));

    // open signed file
    snprintf(signedfile,sizeof signedfile,"%s/%s.%s",
             dir, filename,
             suffix);
    
    /* copy file into mem for later validation */
    mem=ops_write_mem_from_file(signedfile,&errnum);

    /* now validate file */
#ifdef WIN32
    fd=open(signedfile,O_RDONLY | O_BINARY);
#else
    fd=open(signedfile,O_RDONLY);
#endif
    if(fd < 0)
        {
        perror(signedfile);
        exit(2);
        }
    
    // Set verification reader and handling options

    memset(&validate_arg,'\0',sizeof validate_arg);
    validate_arg.result=result;
    validate_arg.keyring=&pub_keyring;
    validate_arg.rarg=ops_reader_get_arg_from_pinfo(pinfo);

    ops_parse_cb_set(pinfo,callback,&validate_arg);
    ops_reader_set_fd(pinfo,fd);
    pinfo->rinfo.accumulate=ops_true;

    // Set up armour options

    if (has_armour)
        ops_reader_push_dearmour(pinfo);
    
    // Do the verification

    rtn=ops_parse(pinfo);
    

    if (debug)
        {
        printf("valid=%d, invalid=%d, unknown=%d\n",
               result->valid_count,
               result->invalid_count,
               result->unknown_signer_count);
        }

    // Tidy up
    if (has_armour)
        ops_reader_pop_dearmour(pinfo);

    close(fd);
    ops_validate_result_free(result);
    if (!rtn)
        return(rtn);

    /* Now check it also works from memory */
    result=ops_mallocz(sizeof (ops_validate_result_t));

    rtn=ops_validate_mem(result, mem, has_armour, &pub_keyring);

    if (debug)
        {
        printf("from memory: valid=%d, invalid=%d, unknown=%d\n",
               result->valid_count,
               result->invalid_count,
               result->unknown_signer_count);
        }

    ops_validate_result_free(result);

    if (!rtn)
        return(rtn);

    // Now check it works from HighLevel API call
    result=ops_mallocz(sizeof (ops_validate_result_t));
    CU_ASSERT(ops_validate_file(result,signedfile,has_armour,&pub_keyring)==ops_true);
    ops_validate_result_free(result);

    return (rtn);
    }

static void test_rsa_verify_ok(const int has_armour, const char *filename)
    {
    int rtn=0;
    ops_parse_info_t *pinfo=NULL;

    // setup
    pinfo=ops_parse_info_new();

    // parse
    rtn=test_rsa_verify(has_armour, filename, callback_verify, pinfo);

    // handle result
    ops_print_errors(ops_parse_info_get_errors(pinfo));
    CU_ASSERT(rtn==1);

    // clean up
    ops_parse_info_delete(pinfo);
    }

static void test_rsa_verify_fail(const int has_armour, const char *filename, ops_callback callback, ops_errcode_t expected_errcode)
    {
    int rtn=0;
    ops_parse_info_t *pinfo=NULL;
    ops_callback cb=NULL;
    ops_error_t* errstack=NULL;

    cb = callback==NULL ? callback_verify : callback;

    // setup
    pinfo=ops_parse_info_new();

    // parse
    rtn=test_rsa_verify(has_armour, filename, cb, pinfo);

    // handle result - should fail
    errstack=ops_parse_info_get_errors(pinfo);

    CU_ASSERT(errstack!=NULL);

    // print out errors if we have actually got a different error
    // to the one expected
    if (errstack)
        {
        CU_ASSERT(ops_has_error(errstack,expected_errcode));
        if  (!ops_has_error(errstack,expected_errcode))
            {
            printf("\nfilename=%s: errstack->errcode=0x%2x\n", filename, errstack->errcode);
            ops_print_errors(errstack);
            }
        }
    CU_ASSERT(rtn==0);

    // clean up
    ops_parse_info_delete(pinfo);
    }

static void test_rsa_verify_wellformed(const int has_armour, const char *filename, ops_callback callback)
    {
    int rtn=0;
    ops_parse_info_t *pinfo=NULL;
    ops_callback cb=NULL;
    ops_error_t* errstack=NULL;

    cb = callback==NULL ? callback_verify : callback;

    // setup
    pinfo=ops_parse_info_new();

    // parse
    rtn=test_rsa_verify(has_armour, filename, cb, pinfo);

    // handle result - should fail with UNKNOWN SIGNER but not BAD FORMAT
    errstack=ops_parse_info_get_errors(pinfo);

    CU_ASSERT(errstack!=NULL);

    // print out errors if we have actually got a different error
    // to the one expected
    if (errstack)
        {
        CU_ASSERT(ops_has_error(errstack,OPS_E_V_UNKNOWN_SIGNER));
        CU_ASSERT(!ops_has_error(errstack,OPS_E_R_BAD_FORMAT));
        if  (ops_has_error(errstack,OPS_E_R_BAD_FORMAT)
             || !ops_has_error(errstack,OPS_E_V_UNKNOWN_SIGNER))
            {
            printf("\nfilename=%s: errstack->errcode=0x%2x\n", filename, errstack->errcode);
            ops_print_errors(errstack);
            }
        }
    CU_ASSERT(rtn==0);

    // clean up
    ops_parse_info_delete(pinfo);
    }

static void test_rsa_verify_v3sig(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_ok(OPS_UNARMOURED,filename_rsa_v3sig);
    }

static void test_rsa_verify_hash_md5(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_ok(OPS_UNARMOURED,filename_rsa_hash_md5);
    }

static void test_rsa_verify_hash_sha256(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_ok(OPS_UNARMOURED,filename_rsa_hash_sha256);
    }

static void test_rsa_verify_noarmour_nopassphrase(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_ok(OPS_UNARMOURED,filename_rsa_noarmour_nopassphrase);
    }

static void test_rsa_verify_noarmour_passphrase(void)
    {
    assert(pub_keyring.nkeys);
    test_rsa_verify_ok(OPS_UNARMOURED,filename_rsa_noarmour_passphrase);
    }

static void test_rsa_verify_armour_nopassphrase(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_ok(OPS_ARMOURED,filename_rsa_armour_nopassphrase);
    }

static void test_rsa_verify_armour_passphrase(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_ok(OPS_ARMOURED,filename_rsa_armour_passphrase);
    }

static void test_rsa_verify_clearsign_nopassphrase(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_ok(OPS_ARMOURED,filename_rsa_clearsign_nopassphrase);
    }

static void test_rsa_verify_clearsign_passphrase(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_ok(OPS_ARMOURED,filename_rsa_clearsign_passphrase);
    }

static ops_parse_cb_return_t callback_bad_sig(const ops_parser_content_t* content_, ops_parse_cb_info_t *cbinfo)
    {
    int target;
    unsigned char* data;
    unsigned char orig;
    switch (content_->tag)
        {
    case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
    case OPS_PTAG_CT_LITERAL_DATA_BODY:
        // change something in the signed text to break the sig
        switch (content_->tag)
            {
        case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
            target=content_->content.signed_cleartext_body.length;
            orig=content_->content.signed_cleartext_body.data[target];
            data=(unsigned char*) &content_->content.signed_cleartext_body.data[0];
            break;

        case OPS_PTAG_CT_LITERAL_DATA_BODY:
            target=content_->content.literal_data_body.length;
            orig=content_->content.literal_data_body.data[target];
            data=(unsigned char*) &content_->content.literal_data_body.data[0];
            break;

        default:
            assert(0);
            }

        if (target==0)
            {
            fprintf(stderr,"Nothing in text body to change!!\n");
            break;
            }

        // change a byte somewhere near the middle
        target/=2;
        // remove const-ness so we can change it
        data[target]= ~orig;
        assert(orig!=content_->content.signed_cleartext_body.data[target]);
        break;

    default:
        break;
        }
    return callback_verify(content_,cbinfo);
    }

static void test_rsa_verify_noarmour_fail_bad_sig(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_fail(OPS_UNARMOURED,filename_rsa_noarmour_fail_bad_sig,callback_bad_sig,OPS_E_V_BAD_SIGNATURE);
    }

static void test_rsa_verify_v3sig_fail_bad_sig(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_fail(OPS_UNARMOURED,filename_rsa_v3sig_fail_bad_sig, callback_bad_sig, OPS_E_V_BAD_SIGNATURE);
    }

static void test_rsa_verify_clearsign_fail_bad_sig(void)
    {
    assert(pub_keyring.nkeys);

    test_rsa_verify_fail(OPS_ARMOURED,filename_rsa_clearsign_fail_bad_sig,callback_bad_sig,OPS_E_V_BAD_SIGNATURE);
    }

static void test_rsa_verify_clearsign_fail_malformed_msg(void)
    {
    int i=0;
    assert(pub_keyring.nkeys);

    for (i=0; i<num_malformed; i++)
        {
        char filename[MAXBUF];
        make_filename_malformed(filename,MAXBUF,i);
        test_rsa_verify_fail(OPS_ARMOURED,filename,NULL,OPS_E_R_BAD_FORMAT);
        }
    }

static void test_rsa_verify_clearsign_fail_wellformed_msg(void)
    {
    int i=0;
    assert(pub_keyring.nkeys);

    for (i=0; i<num_wellformed; i++)
        {
        char filename[MAXBUF];
        make_filename_wellformed(filename,MAXBUF,i);
        test_rsa_verify_wellformed(OPS_ARMOURED,filename,NULL);
        }
    }

CU_pSuite suite_rsa_verify()
{
    CU_pSuite suite = NULL;

    suite = CU_add_suite("RSA Verification Suite", init_suite_rsa_verify, clean_suite_rsa_verify);
    if (!suite)
	    return NULL;

    // add tests to suite
    
    if (NULL == CU_add_test(suite, "Clearsigned, no passphrase", test_rsa_verify_clearsign_nopassphrase))
	    return NULL;
    
    if (NULL == CU_add_test(suite, "Clearsigned, passphrase", test_rsa_verify_clearsign_passphrase))
	    return NULL;
    
    if (NULL == CU_add_test(suite, "Armoured, no passphrase", test_rsa_verify_armour_nopassphrase))
	    return NULL;
    
    if (NULL == CU_add_test(suite, "Armoured, passphrase", test_rsa_verify_armour_passphrase))
	    return NULL;
    
    if (NULL == CU_add_test(suite, "Unarmoured, no passphrase", test_rsa_verify_noarmour_nopassphrase))
	    return NULL;
    
    if (NULL == CU_add_test(suite, "Unarmoured, passphrase", test_rsa_verify_noarmour_passphrase))
	    return NULL;

    if (NULL == CU_add_test(suite, "V3 signature verification", test_rsa_verify_v3sig))
	    return NULL;

    if (NULL == CU_add_test(suite, "V3 signature: should fail on bad sig", test_rsa_verify_v3sig_fail_bad_sig))
	    return NULL;

    if (NULL == CU_add_test(suite, "MD5 Hash", test_rsa_verify_hash_md5))
	    return NULL;

    if (NULL == CU_add_test(suite, "SHA256 Hash", test_rsa_verify_hash_sha256))
	    return NULL;

    if (NULL == CU_add_test(suite, "Unarmoured: should fail on bad sig", test_rsa_verify_noarmour_fail_bad_sig))
	    return NULL;
    if (NULL == CU_add_test(suite, "Clearsign: should fail on bad sig", test_rsa_verify_clearsign_fail_bad_sig))
	    return NULL;

    if (NULL == CU_add_test(suite, "Clearsign: should fail on malformed message", test_rsa_verify_clearsign_fail_malformed_msg))
	    return NULL;

    if (NULL == CU_add_test(suite, "Clearsign: should not get BAD FORMAT on wellformed message", test_rsa_verify_clearsign_fail_wellformed_msg))
	    return NULL;

    return suite;
}

// EOF
