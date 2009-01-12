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

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif
#include <sys/stat.h>

#include "CUnit/Basic.h"
#include "openpgpsdk/defs.h"
#include "openpgpsdk/readerwriter.h"
#include "openpgpsdk/std_print.h"
#include "openpgpsdk/packet-show.h"
#include "../src/lib/parse_local.h"

#include "tests.h"

char dir[MAXBUF+1];
char gpgcmd[MAXBUF+1];
ops_keyring_t pub_keyring;
ops_keyring_t sec_keyring;

char alpha_user_id[MAXBUF+1];
char *alpha_name="Alpha";
char *alpha_comment="RSA, no passphrase";
char *alpha_email="alpha@test.com";

const ops_public_key_t *alpha_pkey;
const ops_secret_key_t *alpha_skey;
const ops_keydata_t *alpha_pub_keydata;
const ops_keydata_t *alpha_sec_keydata;
char* alpha_passphrase="";

char bravo_user_id[MAXBUF+1];
char *bravo_name="Bravo";
char *bravo_comment="RSA, passphrase";
char *bravo_email="bravo@test.com";

const ops_public_key_t *bravo_pkey;
const ops_secret_key_t *bravo_skey;
const ops_keydata_t *bravo_pub_keydata;
const ops_keydata_t *bravo_sec_keydata;
char* bravo_passphrase="hello";

char alphadsa_user_id[MAXBUF+1];
char *alphadsa_name="AlphaDSA";
char *alphadsa_comment="DSA, no passphrase";
char *alphadsa_email="alphadsa@test.com";

const ops_public_key_t *alphadsa_pkey;
const ops_secret_key_t *alphadsa_skey;
const ops_keydata_t *alphadsa_pub_keydata;
const ops_keydata_t *alphadsa_sec_keydata;
char* alphadsa_passphrase="";

char bravodsa_user_id[MAXBUF+1];
char *bravodsa_name="BravoDSA";
char *bravodsa_comment="DSA, passphrase";
char *bravodsa_email="bravodsa@test.com";

const ops_public_key_t *bravodsa_pkey;
const ops_secret_key_t *bravodsa_skey;
const ops_keydata_t *bravodsa_pub_keydata;
const ops_keydata_t *bravodsa_sec_keydata;
char* bravodsa_passphrase="hellodsa";

char *charlie_user_id="Charlie (test user) <charlie@test.com>";

const ops_keydata_t *decrypter=NULL;

char logfile[MAXBUF];

static void setup_test_keys();
static void setup_test_keyptrs();
static void setup_test_extra_dsa_keys();
static void setup_test_rsa_keyptrs();
static void setup_test_dsa_keyptrs();
static void read_keyrings();
static void create_logfile();

void setup()
    {
    create_logfile();

    ops_crypto_init();

    // Create temp directory
    if (!mktmpdir())
        return;

    assert(strlen(dir));
    snprintf(gpgcmd,sizeof gpgcmd,"gpg --quiet --no-tty --homedir=%s --openpgp",dir);

    setup_test_keys();
    setup_test_extra_dsa_keys();
    setup_test_keyptrs();

    }

void create_logfile()
    {
    time_t nowtime;
    struct tm now;
    int fd;
    int flags;

    // generate timestamp
    time(&nowtime);
    localtime_r(&nowtime,&now);
    snprintf(logfile,MAXBUF,"logs/logfile_%d%02d%02d_%02d%02d%02d", 1900+now.tm_year, now.tm_mon+1,now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec);

    // open file as new
    flags = O_WRONLY | O_CREAT | O_TRUNC;
#ifdef WIN32
    flags |= O_BINARY;
#endif
    if ((fd=open(logfile,flags,0600)) < 0)
        {
        fprintf(stderr,"Cannot open logfile %s\n", logfile);
        exit(-1);
        }
    close(fd);
    }

void read_keyrings()
    {
    char keyring_name[MAXBUF+1];

    /*
     * read keyrings
     */

    snprintf(keyring_name,sizeof keyring_name,"%s/pubring.gpg", dir);
    ops_keyring_read_from_file(&pub_keyring,OPS_UNARMOURED,keyring_name);

    snprintf(keyring_name,sizeof keyring_name,"%s/secring.gpg", dir);
    ops_keyring_read_from_file(&sec_keyring,OPS_UNARMOURED,keyring_name);
    }

void setup_test_keyptrs()
    {
    read_keyrings();
    setup_test_rsa_keyptrs();
    setup_test_dsa_keyptrs();
    }

void setup_test_rsa_keyptrs()
    {
    /*
     * set up key pointers
     */

    assert(pub_keyring.nkeys);

    alpha_pub_keydata=ops_keyring_find_key_by_userid(&pub_keyring, alpha_user_id);
    bravo_pub_keydata=ops_keyring_find_key_by_userid(&pub_keyring, bravo_user_id);
    assert(alpha_pub_keydata);
    assert(bravo_pub_keydata);

    alpha_sec_keydata=ops_keyring_find_key_by_userid(&sec_keyring, alpha_user_id);
    bravo_sec_keydata=ops_keyring_find_key_by_userid(&sec_keyring, bravo_user_id);
    assert(alpha_sec_keydata);
    assert(bravo_sec_keydata);

    alpha_pkey=ops_get_public_key_from_data(alpha_pub_keydata);
    alpha_skey=ops_get_secret_key_from_data(alpha_sec_keydata);
    bravo_pkey=ops_get_public_key_from_data(bravo_pub_keydata);
    bravo_skey=ops_decrypt_secret_key_from_data(bravo_sec_keydata,bravo_passphrase);

    assert(alpha_pkey);
    assert(alpha_skey);
    assert(bravo_pkey);
    assert(bravo_skey); //not yet set because of passphrase
}

void setup_test_dsa_keyptrs()
    {
    /*
     * set up key pointers
     */

    assert(pub_keyring.nkeys);

    alphadsa_pub_keydata=ops_keyring_find_key_by_userid(&pub_keyring, alphadsa_user_id);
    bravodsa_pub_keydata=ops_keyring_find_key_by_userid(&pub_keyring, bravodsa_user_id);
    assert(alphadsa_pub_keydata);
    assert(bravodsa_pub_keydata);

    alphadsa_sec_keydata=ops_keyring_find_key_by_userid(&sec_keyring, alphadsa_user_id);
    bravodsa_sec_keydata=ops_keyring_find_key_by_userid(&sec_keyring, bravodsa_user_id);
    assert(alphadsa_sec_keydata);
    assert(bravodsa_sec_keydata);

    alphadsa_pkey=ops_get_public_key_from_data(alphadsa_pub_keydata);
    alphadsa_skey=ops_get_secret_key_from_data(alphadsa_sec_keydata);
    bravodsa_pkey=ops_get_public_key_from_data(bravodsa_pub_keydata);
    bravodsa_skey=ops_decrypt_secret_key_from_data(bravodsa_sec_keydata,bravodsa_passphrase);

    assert(alphadsa_pkey);
    assert(alphadsa_skey);
    assert(bravodsa_pkey);
    assert(bravodsa_skey); 
}

static void setup_test_keys()
    {
    char keydetails[MAXBUF+1];
    int fd=0;
    char cmd[MAXBUF+1];
    const int keylength=1024;
    const char* RSA="RSA";
    const char* DSA="DSA";
    char* format="Key-Type: %s\nKey-Usage: encrypt, sign\nName-Real: %s\nName-Comment: %s\nName-Email: %s\nKey-Length: %d\n";

    char rsa_nopass[MAXBUF+1];
    snprintf(rsa_nopass,MAXBUF,format,RSA,alpha_name, alpha_comment, alpha_email, keylength);
    snprintf(alpha_user_id, MAXBUF, "%s (%s) <%s>", alpha_name, alpha_comment, alpha_email);

    char rsa_pass[MAXBUF+1];
    snprintf(rsa_pass,MAXBUF,format,RSA,bravo_name, bravo_comment, bravo_email, keylength);
    snprintf(bravo_user_id, MAXBUF, "%s (%s) <%s>", bravo_name, bravo_comment, bravo_email);

    char dsa_nopass[MAXBUF+1];
    snprintf(dsa_nopass,MAXBUF,format, DSA, alphadsa_name, alphadsa_comment, alphadsa_email, keylength);
    snprintf(alphadsa_user_id, MAXBUF, "%s (%s) <%s>", alphadsa_name, alphadsa_comment, alphadsa_email);

    char dsa_pass[MAXBUF+1];
    snprintf(dsa_pass, MAXBUF, format, DSA, bravodsa_name, bravodsa_comment, bravodsa_email, keylength);
    snprintf(bravodsa_user_id, MAXBUF, "%s (%s) <%s>", bravodsa_name, bravodsa_comment, bravodsa_email);

    /*
     * Create a RSA keypair with no passphrase
     */

    snprintf(keydetails,sizeof keydetails,"%s/%s",dir,"keydetails.alpha");

#ifdef WIN32
    if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0600))<0)
#else
    if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_EXCL, 0600))<0)
#endif
        {
        fprintf(stderr,"Can't create Alpha key details\n");
        return;
        }

    write(fd,rsa_nopass,strlen(rsa_nopass));
    close(fd);

    snprintf(cmd,sizeof cmd,"%s --openpgp --gen-key --s2k-cipher-algo \"AES\" --expert --batch %s",gpgcmd,keydetails);
    run(cmd);

    /*
     * Create a RSA keypair with passphrase
     */

    snprintf(keydetails,sizeof keydetails,"%s/%s",dir,"keydetails.bravo");

#ifdef WIN32
    if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0600))<0)
#else
    if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_EXCL, 0600))<0)
#endif
        {
        fprintf(stderr,"Can't create Bravo key details\n");
        return;
        }

    write(fd,rsa_pass,strlen(rsa_pass));
    close(fd);

    snprintf(cmd,sizeof cmd,"%s --openpgp --gen-key --s2k-cipher-algo \"AES\" --expert --batch %s",gpgcmd,keydetails);
    run(cmd);
    
    /*
     * Create a DSA keypair with no passphrase
     */

    snprintf(keydetails,sizeof keydetails,"%s/%s",dir,"keydetails.alphadsa");

#ifdef WIN32
    if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0600))<0)
#else
    if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_EXCL, 0600))<0)
#endif
        {
        fprintf(stderr,"Can't create AlphaDSA key details\n");
        return;
        }

    write(fd,dsa_nopass,strlen(dsa_nopass));
    close(fd);

    snprintf(cmd,sizeof cmd,"%s --openpgp --gen-key --s2k-cipher-algo \"AES\" --expert --batch %s",gpgcmd,keydetails);
    run(cmd);

    /*
     * Create a RSA keypair with passphrase
     */

    snprintf(keydetails,sizeof keydetails,"%s/%s",dir,"keydetails.bravodsa");

#ifdef WIN32
    if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_EXCL | O_BINARY, 0600))<0)
#else
    if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_EXCL, 0600))<0)
#endif
        {
        fprintf(stderr,"Can't create Bravo key details\n");
        return;
        }

    write(fd,dsa_pass,strlen(dsa_pass));
    close(fd);

    snprintf(cmd,sizeof cmd,"%s --openpgp --gen-key --s2k-cipher-algo \"AES\" --expert --batch %s",gpgcmd,keydetails);
    run(cmd);
    
    }

static void setup_test_extra_dsa_keys()
    {
    int fd=0;
    char cmd[MAXBUF+1];
    char def[MAXBUF+1];
    char * keydetails="/tmp/keydetails";
    char name[MAXBUF+1];
    char comment[MAXBUF+1];
    char email[MAXBUF+1];

    unsigned i;
    int ret;

    for (i=0; i<sz_dsstests; i++)
        {
        snprintf(name, MAXBUF, "DSA%i", i);
        snprintf(comment, MAXBUF, "Keysize %d, Qsize %d, Hash %s",
                 dsstests[i].keysize,
                 dsstests[i].qsize,
                 ops_show_hash_algorithm(dsstests[i].hashalg));
        snprintf(email, MAXBUF, "dsa%d@testing.com", i);

        //        printf("%s, %s, %s\n", name, comment, email);

        // key definition
        snprintf(def, MAXBUF, 
                 "Key-Type: DSA\nKey-Usage: encrypt, sign\nName-Real: %s\nName-Comment: %s\nName-Email: %s\nKey-Length: %d\n",
                 name, comment, email, dsstests[i].keysize);

#ifdef WIN32
        if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0600))<0)
#else
        if ((fd=open(keydetails,O_WRONLY | O_CREAT | O_TRUNC, 0600))<0)
#endif
            {
            fprintf(stderr,"Can't open keydetails file %s\n", keydetails);
            return;
            }

        write(fd,def,strlen(def));
        close(fd);

        // save userids

        snprintf(dsstests[i].userid, MAXBUF, "%s (%s) <%s>", name, comment, email);

        // create DSA keypair with no passphrase

        snprintf(cmd,sizeof cmd,"%s --gen-key --s2k-cipher-algo \"AES\" --expert --batch %s",gpgcmd,keydetails);
        ret=run(cmd);
        if (ret)
            {
            fprintf(stderr, "Command failed: returns %d\n", ret);
            assert(0); 
            }
        }

    }

void cleanup()
    {
    char cmd[MAXBUF];

    ops_crypto_finish();

    if (CU_get_number_of_tests_failed())
	{
	fprintf(stderr,"Test directory %s retained, because one or more tests failed.\n", dir);
	}
    else
        {
        /* Remove test dir and files */
        snprintf(cmd,sizeof cmd,"rm -rf %s", dir);
        if (run(cmd))
            {
            perror("Can't delete test directory ");
            return;
            }
        }
    }

int mktmpdir (void)
    {
    int limit=10; // don't try indefinitely
    long int rnd=0;

#ifdef WIN32
    srand( (unsigned)time( NULL ) );
#endif
    while (limit--) 
        {
        rnd=random();
        snprintf(dir,sizeof dir,"./testdir.%ld",rnd);
        
        // Try to create directory
#ifndef WIN32
        if (!mkdir(dir,0700))
#else
    	if (!_mkdir(dir))
#endif
            {
            // success
            return 1;
            }
        else
            {
            fprintf (stderr,"Couldn't open dir %s, errno=%d\n", dir, errno);
            perror(NULL);
            }
        }
    fprintf(stderr,"Too many temp dirs: please delete them\n");
    exit(1);
    }

char* create_testtext(const char *text, const unsigned int repeats)
    {
    //const unsigned int repeats=10;
    unsigned int i=0;

    const unsigned int maxbuf=1024;
    char buf[maxbuf+1];
    unsigned int sz_one=0;
    unsigned int sz_big=0;
    char* bigbuf=NULL; 

    buf[maxbuf]='\0';
    snprintf(buf,sizeof buf,"%s : Test Text\n", text);

    sz_one=strlen(buf);
    sz_big=sz_one*repeats+1;

    bigbuf=ops_mallocz(sz_big); 

   for (i=0; i<repeats; i++)
        {
        char* ptr=bigbuf+ (i*(sz_one-1));
        snprintf(ptr,sz_one,buf);
        }

   return bigbuf;
    }

void create_testdata(const char *text, unsigned char *buf, const int maxlen)
    {
    char *preamble=" : Test Data :";
    int i=0;

    snprintf((char *)buf,maxlen,"%s%s", text, preamble);

#ifdef WIN32
    srand( (unsigned)time( NULL ) );
#endif
    for (i=strlen(text)+strlen(preamble); i<maxlen; i++)
        {
        buf[i]=(random() & 0xFF);
        }
    }

void create_small_testfile(const char* name)
    {
    const unsigned int repeats=10;
    create_testfile(name, repeats);
    }

void create_large_testfile(const char* name)
    {
    const unsigned int repeats=10^6;
    create_testfile(name, repeats);
    }

void create_testfile(const char *name, const unsigned int repeats)
    {
    char filename[MAXBUF+1];
    char* testtext=NULL;

    int fd=0;
    snprintf(filename,sizeof filename,"%s/%s",dir,name);
#ifdef WIN32
    if ((fd=open(filename,O_WRONLY| O_CREAT | O_EXCL | O_BINARY, 0600))<0)
#else
    if ((fd=open(filename,O_WRONLY| O_CREAT | O_EXCL, 0600))<0)
#endif
	return;

    testtext=create_testtext(name, repeats);
    write(fd,testtext,strlen(testtext));
    close(fd);
    free(testtext);
    }

ops_parse_cb_return_t
callback_general(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    int debug=0;

    ops_parser_content_union_t* content=(ops_parser_content_union_t *)&content_->content;
    
    OPS_USED(cbinfo);
    
    //    ops_print_packet(content_);
    
    switch(content_->tag)
        {
    case OPS_PARSER_PTAG:
    case OPS_PTAG_CT_COMPRESSED: 
        // ignore
        break;
        
    case OPS_PARSER_PACKET_END:
        // print raw packet

        if (debug)
            {
            unsigned i=0;
            fprintf(stderr,"***\n***Raw Packet:\n");
            for (i=0; i<content_->content.packet.length; i++)
                {
                fprintf(stderr,"0x%02x ", content_->content.packet.raw[i]);
                if (!((i+1) % 16))
                    fprintf(stderr,"\n");
                else if (!((i+1) % 8))
                    fprintf(stderr,"  ");
                }
            fprintf(stderr,"\n");
            }
        break;

    case OPS_PARSER_ERROR:
        printf("parse error: %s\n",content->error.error);
        break;
        
    case OPS_PARSER_ERRCODE:
        printf("parse error: %s\n",
               ops_errcode(content->errcode.errcode));
        break;
        
    default:
        fprintf(stderr,"Unexpected packet tag=%d (0x%x)\n",content_->tag,
                content_->tag);
        assert(0);
        }
    
    return OPS_RELEASE_MEMORY;
    }

ops_parse_cb_return_t
test_cb_get_passphrase(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    //    ops_parser_content_union_t* content=(ops_parser_content_union_t *)&content_->content;
    char *passphrase=NULL;

    OPS_USED(cbinfo);

//    ops_print_packet(content_);

    switch(content_->tag)
        {
    case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
        /*
          Doing this so the test can be automated.
        */
        
        if (cbinfo->cryptinfo.keydata==alpha_sec_keydata)
            passphrase=alpha_passphrase;
        else if (cbinfo->cryptinfo.keydata==bravo_sec_keydata)
            passphrase=bravo_passphrase;
        else
            assert(0);
        //        *(content->secret_key_passphrase.passphrase)=ops_malloc_passphrase(no_passphrase);
        cbinfo->cryptinfo.passphrase=ops_malloc_passphrase(passphrase);
        return OPS_KEEP_MEMORY;
        break;
        
    default:
        //        return callback_general(content_,cbinfo);
        break;
	}
    
    return OPS_RELEASE_MEMORY;
    }
 
// move definition to better location
ops_parse_cb_return_t
validate_key_cb(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);
ops_parse_cb_return_t
validate_data_cb(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo);

ops_parse_cb_return_t
callback_data_signature(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    //    ops_parser_content_union_t* content=(ops_parser_content_union_t *)&content_->content;

    OPS_USED(cbinfo);

    //    ops_print_packet(content_);

    switch(content_->tag)
        {
    case OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER:
    case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
    case OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER:

    case OPS_PTAG_CT_ONE_PASS_SIGNATURE:
    case OPS_PTAG_CT_SIGNATURE_HEADER:
    case OPS_PTAG_CT_SIGNATURE_FOOTER:

    case OPS_PTAG_CT_LITERAL_DATA_HEADER:
    case OPS_PTAG_CT_LITERAL_DATA_BODY:

    case OPS_PTAG_CT_SIGNATURE:
        return validate_data_cb(content_,cbinfo);
        break;

    default:
        return callback_general(content_,cbinfo);
        }

    return OPS_RELEASE_MEMORY;
    }
 
ops_parse_cb_return_t
callback_verify(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    switch(content_->tag)
        {
    case OPS_PTAG_RAW_SS:
    case OPS_PTAG_SS_CREATION_TIME:
    case OPS_PTAG_SS_ISSUER_KEY_ID:
        // \todo should free memory?
        return OPS_KEEP_MEMORY;
        
    case OPS_PTAG_CT_ONE_PASS_SIGNATURE:
    case OPS_PTAG_CT_ARMOUR_HEADER:
    case OPS_PTAG_CT_ARMOUR_TRAILER:
        break;

    case OPS_PTAG_CT_UNARMOURED_TEXT:
        break;
        
    case OPS_PTAG_CT_SIGNATURE:
    case OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER:
    case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
    case OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER:
    case OPS_PTAG_CT_SIGNATURE_HEADER:
    case OPS_PTAG_CT_SIGNATURE_FOOTER:
    case OPS_PTAG_CT_LITERAL_DATA_HEADER:
    case OPS_PTAG_CT_LITERAL_DATA_BODY:
        return callback_data_signature(content_, cbinfo);

    default:
        return callback_general(content_,cbinfo);
	}

    return OPS_RELEASE_MEMORY;
    }

ops_parse_cb_return_t
callback_verify_example(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    ops_parse_cb_return_t rtn;

    // run standard callback
    rtn=callback_verify(content_,cbinfo);

    // do extra work here, if needed
    switch(content_->tag)
        {
    case OPS_PTAG_CT_SIGNATURE_FOOTER:

        // When we get this tag, we know that the signature
        // has been parsed. The signature struct has been filled in 
        // with everything known about the signature.

        // example: print out the signature creation time
        if (content_->content.signature.info.creation_time_set)
            {
            printf("\nsignature creation time : %s", 
                   ctime(&content_->content.signature.info.creation_time));
            }
        break;

    default:
        break;
        }

    return rtn;
	}

void reset_vars()
    {
    //    ops_memory_init(mem_literal_data,0);

    if (decrypter)
        {
        //        free (decrypter);
        decrypter=NULL;
        }
    }

int file_compare(char* file1, char* file2)
    {
    FILE *fp1=NULL;
    FILE *fp2=NULL;
    char ch1, ch2;
    int err=0;

    // open files
    if ((fp1=fopen(file1,"rb"))==NULL)
        {
        fprintf(stderr,"file_compare: cannot open file %s\n",file1);
        return -1;
        }
    if ((fp2=fopen(file2,"rb"))==NULL)
        {
        fprintf(stderr,"file_compare: cannot open file %s\n",file2);
        fclose(fp1);
        return -1;
        }

    while(!feof(fp1))
        {
        ch1 = fgetc(fp1);
        if (ferror(fp1))
            {
            fprintf(stderr,"file_compare: error reading from file %s\n",file1);
            err = -1;
            break;
            }
        ch2 = fgetc(fp2);
        if (ferror(fp2))
            {
            fprintf(stderr,"file_compare: error reading from file %s\n",file2);
            err = -1;
            break;
            }
        if (ch1 != ch2)
            {
            printf("Files %s and %s differ\n",file1,file2);
            err = 1;
            break;
            }
        }
    fclose(fp1);
    fclose(fp2);
    return err;
    }

int run(const char* cmd)
    {
    char fullcmd[MAXBUF];

    const char *format="(echo; echo %s; %s) >> %s 2>&1";
    assert(strlen(format)-2-2+strlen(cmd)+strlen(logfile) < MAXBUF);
    snprintf (fullcmd, sizeof fullcmd, format, cmd, cmd, logfile);

    return(system(fullcmd));
    }

dsatest_t dsstests [] = 
    {
    { 1024, 160, OPS_HASH_SHA1, "", ""},
    { 1024, 160, OPS_HASH_SHA224, "", ""},
    { 1024, 160, OPS_HASH_SHA256, "", ""},
    { 1024, 160, OPS_HASH_SHA384, "", ""},
    { 1024, 160, OPS_HASH_SHA512, "", ""},
    /*
      GPG only allows generation of 1024-bit DSA keys
    { 2048, 224, OPS_HASH_SHA224, "", ""},
    { 2048, 224, OPS_HASH_SHA256, "", ""},
    { 2048, 224, OPS_HASH_SHA384, "", ""},
    { 2048, 224, OPS_HASH_SHA512, "", ""},

    { 2048, 256, OPS_HASH_SHA256, "", ""},
    { 2048, 256, OPS_HASH_SHA384, "", ""},
    { 2048, 256, OPS_HASH_SHA512, "", ""},

    { 3072, 256, OPS_HASH_SHA256, "", ""},
    { 3072, 256, OPS_HASH_SHA384, "", ""},
    { 3072, 256, OPS_HASH_SHA512, "", ""},
*/
    };
const unsigned sz_dsstests = sizeof(dsstests) / sizeof(dsatest_t);

// EOF
