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

/**
 \file Command line program to perform openpgp operations
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <assert.h>
#include <libgen.h>
#include <unistd.h>

#include <openpgpsdk/keyring.h>
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/signature.h>
#include <openpgpsdk/validate.h>
#include <openpgpsdk/readerwriter.h>
#include <openpgpsdk/std_print.h>
#include <openpgpsdk/util.h>

#define DEFAULT_NUMBITS 1024

#define MAXBUF 1024

static const char* usage="%s --list-keys | --list-packets | --encrypt | --decrypt | --sign | --clearsign | --verify [--keyring=<keyring>] [--userid=<userid>] [--file=<filename>] [--armour] [--homedir=<homedir>]\n";
static const char* usage_list_keys="%s --list-keys [--keyring=<keyring>]\n";
static const char* usage_find_key="%s --find-key --userid=<userid> [--keyring=<keyring>] \n";
static const char* usage_export_key="%s --export-key --userid=<userid> [--keyring=<keyring>] \n";
static const char* usage_import_key="%s --import-key --file=<filename> [--homedir=<dir>] [--armour]\n";
static const char* usage_generate_key="%s --generate-key --userid=<userid> [--numbits=<numbits>] [--passphrase=<passphrase>]\n";
static const char* usage_encrypt="%s --encrypt --userid=<userid> --file=<filename> [--armour] [--homedir=<homedir>]\n";
static const char* usage_decrypt="%s --decrypt --file=<filename> [--armour] [--homedir=<homedir>]\n";
static const char* usage_sign="%s --sign --userid=<userid> --file=<filename> [--armour] [--homedir=<homedir>]\n";
static const char* usage_clearsign="%s --clearsign --userid=<userid> --file=<filename> [--homedir=<homedir>]\n";
static const char* usage_verify="%s --verify --file=<filename> [--homedir=<homedir>] [--armour]\n";
static const char* usage_list_packets="%s --list-packets --file=<filename> [--homedir=<homedir>] [--armour]\n";

static char* pname;

enum optdefs {
// commands
LIST_KEYS=1,
// \todo LIST_PACKETS,
FIND_KEY,
EXPORT_KEY,
IMPORT_KEY,
GENERATE_KEY,
ENCRYPT,
DECRYPT,
SIGN,
CLEARSIGN,
VERIFY,
LIST_PACKETS,

// options
KEYRING,
USERID,
PASSPHRASE,
FILENAME,
ARMOUR,
HOMEDIR,
NUMBITS,

/* debug */
OPS_DEBUG

};

#define EXIT_ERROR	2

static struct option long_options[]=
    {
    // commands
    { "list-keys", no_argument, NULL, LIST_KEYS },
    { "find-key", no_argument, NULL, FIND_KEY },
    { "export-key", no_argument, NULL, EXPORT_KEY },
    { "import-key", no_argument, NULL, IMPORT_KEY },
    { "generate-key", no_argument, NULL, GENERATE_KEY },

    { "encrypt", no_argument, NULL, ENCRYPT },
    { "decrypt", no_argument, NULL, DECRYPT },
    { "sign", no_argument, NULL, SIGN },
    { "clearsign", no_argument, NULL, CLEARSIGN },
    { "verify", no_argument, NULL, VERIFY },

    { "list-packets", no_argument, NULL, LIST_PACKETS },

    // options
    { "keyring", required_argument, NULL, KEYRING },
    { "userid", required_argument, NULL, USERID },
    { "passphrase", required_argument, NULL, PASSPHRASE },
    { "file", required_argument, NULL, FILENAME },
    { "homedir", required_argument, NULL, HOMEDIR },
    { "armour", no_argument, NULL, ARMOUR },
    { "numbits", required_argument, NULL, NUMBITS },

    /* debug */
    { "debug", required_argument, NULL, OPS_DEBUG },

    { 0,0,0,0},
    };

static void print_usage(const char* usagemsg, char* progname)
    {
    fprintf(stderr, "\nUsage: ");
    fprintf(stderr, usagemsg, basename(progname));
    }

/* wrapper to get a pass phrase from the user */
static void
get_pass_phrase(char *phrase, size_t size)
{
	char	*p;

	if ((p = getpass("openpgp pass phrase: ")) == NULL) {
		exit(EXIT_ERROR);
	}
	(void) snprintf(phrase, size, "%s", p);
}

int main(int argc, char **argv)
    {
    int optindex=0;
    int ch=0;
    int cmd=0;
    int armour=0;
    int fd=0;

    pname=argv[0];
    char opt_keyring[MAXBUF+1]="";
    char opt_userid[MAXBUF+1]="";
    char opt_passphrase[MAXBUF+1]="";
    char opt_filename[MAXBUF+1]="";
    char opt_homedir[MAXBUF+1]="";

    int got_homedir=0;
    int got_keyring=0;
    int got_userid=0;
    int got_passphrase=0;
    int got_filename=0;
    int got_numbits=0;
    int numbits=DEFAULT_NUMBITS;
    int ex;
    char outputfilename[MAXBUF+1]="";
    ops_keyring_t* myring=NULL;
    char myring_name[MAXBUF+1]="";
    ops_keyring_t* pubring=NULL;
    char pubring_name[MAXBUF+1]="";
    ops_keyring_t* secring=NULL;
    char secring_name[MAXBUF+1]="";
    const ops_keydata_t* keydata=NULL;
    char *suffix=NULL;
    char *dir=NULL;
    char default_homedir[MAXBUF+1]="";
    ops_boolean_t overwrite=ops_true;
    ops_keydata_t* mykeydata=NULL;
    ops_create_info_t * cinfo=NULL;
    ops_memory_t* mem=NULL;
    ops_validate_result_t *validate_result=NULL;
    ops_user_id_t uid;
    ops_secret_key_t* skey=NULL;


    if (argc<2)
        {
        print_usage(usage,pname);
        exit(EXIT_ERROR);
        }
    
    // what does the user want to do?

    while((ch=getopt_long(argc,argv,"",long_options,&optindex   )) != -1)
        {
        
        // read options and commands
        
        switch(long_options[optindex].val)
            {
            // commands

        case LIST_KEYS:
            cmd=LIST_KEYS;
            break;
            
        case FIND_KEY:
            cmd=FIND_KEY;
            break;
            
        case EXPORT_KEY:
            cmd=EXPORT_KEY;
            break;

        case IMPORT_KEY:
            cmd=IMPORT_KEY;
            break;

        case GENERATE_KEY:
            cmd=GENERATE_KEY;
            break;


        case ENCRYPT:
            cmd=ENCRYPT;
            break;

        case DECRYPT:
            cmd=DECRYPT;
            break;

        case SIGN:
            cmd=SIGN;
            break;

        case CLEARSIGN:
            cmd=CLEARSIGN;
            break;

        case VERIFY:
            cmd=VERIFY;
            break;

        case LIST_PACKETS:
            cmd=LIST_PACKETS;
            break;

            // option

        case KEYRING:
            assert(optarg);
            snprintf(opt_keyring,MAXBUF,"%s",optarg);
            got_keyring=1;
            break;
            
        case USERID:
            assert(optarg);
	    if (ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr, "user_id is '%s'\n", optarg);
	    }
            snprintf(opt_userid,MAXBUF,"%s",optarg);
            got_userid=1;
            break;
            
        case PASSPHRASE:
            assert(optarg);
            snprintf(opt_passphrase,MAXBUF,"%s",optarg);
            got_passphrase=1;
            break;
            
        case FILENAME:
            assert(optarg);
            snprintf(opt_filename,MAXBUF,"%s",optarg);
            got_filename=1;
            break;
            
        case ARMOUR:
            armour=1;
            break;
            
        case HOMEDIR:
            assert(optarg);
            snprintf(opt_homedir, MAXBUF, "%s", optarg);
            got_homedir=1;
            break;


        case NUMBITS:
            assert(optarg);
            numbits=atoi(optarg);
            got_numbits=1;
            break;

	case OPS_DEBUG:
		ops_set_debug_level(optarg);
		break;

        default:
            printf("shouldn't be here: option=%d\n", long_options[optindex].val);
            break;
            }
        }

    /*
     * Read keyrings.
     * read public and secret from homedir.
     * (assumed names are pubring.gpg and secring.gpg).
     * Also read named keyring, if given.
     *
     * We will then have variables pubring, secring and myring.
     */

    if (got_homedir)
        dir=opt_homedir;
    else
        {
        snprintf(default_homedir,MAXBUF,"%s/.gnupg",getenv("HOME"));
        printf("dir: %s\n", default_homedir);
        dir=default_homedir;
        }

    snprintf(pubring_name, MAXBUF, "%s/pubring.gpg", dir);
    pubring=ops_mallocz(sizeof *pubring);
    if (!ops_keyring_read_from_file(pubring,ops_false,pubring_name))
        {
        fprintf(stderr, "Cannot read keyring %s\n", pubring_name);
        exit(EXIT_ERROR);
        }
    snprintf(secring_name, MAXBUF, "%s/secring.gpg", dir);
    secring=ops_mallocz(sizeof *secring);
    if (!ops_keyring_read_from_file(secring,ops_false,secring_name))
        {
        fprintf(stderr, "Cannot read keyring %s\n", secring_name);
        exit(EXIT_ERROR);
        }

    if (got_keyring)
        {
        snprintf(myring_name, MAXBUF, "%s/%s", opt_homedir, opt_keyring);
        myring=ops_mallocz(sizeof *myring);
        if (!ops_keyring_read_from_file(myring,ops_false,myring_name))
            {
            fprintf(stderr, "Cannot read keyring %s\n", myring_name);
            exit(EXIT_ERROR);
            }
        }

    // now do the required action
    
    switch(cmd)
        {
    case LIST_KEYS:
	ops_keyring_list((got_keyring) ? myring : pubring);
        break;
        
        //case LIST_PACKETS:

    case FIND_KEY:
        if (!got_userid)
            {
            print_usage(usage_find_key,pname);
            exit(EXIT_ERROR);
            }
        
	if (ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr,"userid: %s\n", opt_userid);
	}
        keydata = ops_keyring_find_key_by_userid((got_keyring) ?
				myring : pubring, opt_userid);
	exit((keydata) ? EXIT_FAILURE : EXIT_SUCCESS);
        //        ops_keyring_free(&keyring);
        break;

    case EXPORT_KEY:
        if (!got_keyring || !got_userid)
            {
            print_usage(usage_export_key,pname);
            exit(EXIT_ERROR);
            }
        
        if (got_keyring)
            keydata=ops_keyring_find_key_by_userid(myring, opt_userid);
        else
            keydata=ops_keyring_find_key_by_userid(pubring, opt_userid);
        if (!keydata)
            {
            fprintf(stderr,"Cannot find key in keyring\n");
            exit(EXIT_ERROR);
            }

        ops_setup_memory_write(&cinfo, &mem, 128);
        if (ops_get_keydata_content_type(keydata)==OPS_PTAG_CT_PUBLIC_KEY)
            ops_write_transferable_public_key(keydata, ops_true, cinfo);
        else
            ops_write_transferable_secret_key(keydata, (unsigned char *)opt_passphrase, strlen(opt_passphrase), ops_true, cinfo);
        fprintf(stdout,"%s",(char *)ops_memory_get_data(mem));
        ops_teardown_memory_write(cinfo,mem);

        break;

    case IMPORT_KEY:
        if (!got_filename)
            {
            print_usage(usage_import_key, pname);
            exit(EXIT_ERROR);
            }
        fprintf(stderr,"before:\n");
        ops_keyring_list(pubring);

        // read new key
        if (!ops_keyring_read_from_file(pubring, armour, opt_filename))
            {
            fprintf(stderr,"Cannot import key from file %s\n", opt_filename);
            exit(EXIT_ERROR);
            }

        fprintf(stderr,"after:\n");
        ops_keyring_list(pubring);
        
        break;

    case GENERATE_KEY:
        if (!got_userid)
            {
            print_usage(usage_generate_key,pname);
            exit(EXIT_ERROR);
            }
        
        uid.user_id=(unsigned char *)opt_userid;
        mykeydata=ops_rsa_create_selfsigned_keypair(numbits,65537,&uid);
        if (!mykeydata)
            {
            fprintf(stderr,"Cannot generate key\n");
            exit(EXIT_ERROR);
            }

        // write public key
        // append to keyrings
        fd=ops_setup_file_append(&cinfo, pubring_name);
        ops_write_transferable_public_key(mykeydata, ops_false, cinfo);
        ops_teardown_file_write(cinfo,fd);

        ops_keyring_free(pubring);
        if (!ops_keyring_read_from_file(pubring,ops_false,pubring_name))
            {
            fprintf(stderr, "Cannot re-read keyring %s\n", pubring_name);
            exit(EXIT_ERROR);
            }

        fd=ops_setup_file_append(&cinfo, secring_name);
        ops_write_transferable_secret_key(mykeydata, NULL, 0, ops_false, cinfo);
        ops_teardown_file_write(cinfo,fd);
        ops_keyring_free(secring);
        if (!ops_keyring_read_from_file(secring,ops_false,secring_name))
            {
            fprintf(stderr, "Cannot re-read keyring %s\n", secring_name);
            exit(EXIT_ERROR);
            }

        ops_keydata_free(mykeydata);
        break;

    case ENCRYPT:
        if (!got_filename)
            {
            print_usage(usage_encrypt,pname);
            exit(EXIT_ERROR);
            }

        if (!got_userid)
            {
            print_usage(usage_encrypt,pname);
            exit(EXIT_ERROR);
            }

        suffix=armour ? ".asc" : ".gpg";
        keydata=ops_keyring_find_key_by_userid(pubring,opt_userid);
        if (!keydata)
            {
            fprintf(stderr,"Userid '%s' not found in keyring\n",
                    opt_userid);
            exit(EXIT_ERROR);
            }

        // outputfilename
        snprintf(outputfilename,MAXBUF,"%s%s", opt_filename,suffix);

        overwrite=ops_true;
        ops_encrypt_file(opt_filename, outputfilename, keydata, armour,overwrite);
        break;

    case DECRYPT:
        if (!got_filename)
            {
            print_usage(usage_decrypt,pname);
            exit(EXIT_ERROR);
            }

        overwrite=ops_true;
        ops_decrypt_file(opt_filename, NULL, secring, armour,overwrite,callback_cmd_get_passphrase_from_cmdline);
        break;

    case SIGN:
        if (!got_filename || !got_userid)
            {
            print_usage(usage_sign, pname);
            exit(EXIT_ERROR);
            }

        // get key with which to sign
        keydata=ops_keyring_find_key_by_userid(secring,opt_userid);
        if (!keydata)
            {
            fprintf(stderr,"Userid '%s' not found in keyring\n",
                    opt_userid);
            exit(EXIT_ERROR);
            }

	/* get the passphrase */
	if (opt_passphrase[0] == 0x0) {
		get_pass_phrase(opt_passphrase, sizeof(opt_passphrase));
	}

        // now decrypt key
        skey=ops_decrypt_secret_key_from_data(keydata,opt_passphrase);
        assert(skey);

        // sign file
        overwrite=ops_true;
        ops_sign_file(opt_filename, NULL, skey, armour, overwrite);
        break;

    case CLEARSIGN:
        if (!got_filename || !got_userid)
            {
            print_usage(usage_clearsign, pname);
            exit(EXIT_ERROR);
            }

        // get key with which to sign
        keydata=ops_keyring_find_key_by_userid(secring,opt_userid);
        if (!keydata)
            {
            fprintf(stderr,"Userid '%s' not found in keyring\n",
                    opt_userid);
            exit(EXIT_ERROR);
            }
        skey=ops_decrypt_secret_key_from_data(keydata,opt_passphrase);
        assert(skey);

        // sign file
        overwrite=ops_true;
        ops_sign_file_as_cleartext(opt_filename, NULL, skey, overwrite);
        break;

    case VERIFY:
        if (!got_filename)
            {
            print_usage(usage_verify, pname);
            exit(EXIT_ERROR);
            }

        validate_result=ops_mallocz(sizeof (ops_validate_result_t));

        if (ops_validate_file(validate_result, opt_filename, armour, pubring)==ops_true)
            {
            printf("Good signature for \"%s\"\n", opt_filename);
	    ex = EXIT_SUCCESS;
            }
        else
            {
            printf("\"%s\": verification failure: %d invalid signatures, %d unknown signatures\n", opt_filename, validate_result->invalid_count, validate_result->unknown_signer_count);
	    ex = EXIT_FAILURE;
            }
        ops_validate_result_free(validate_result);
	exit(ex);
        break;

    case LIST_PACKETS:
        if (!got_filename)
            {
            print_usage(usage_list_packets, pname);
            exit(EXIT_ERROR);
            }
        ops_list_packets(opt_filename, armour, pubring, callback_cmd_get_passphrase_from_cmdline);
        break;

    default:
        print_usage(usage,pname);
        exit(EXIT_ERROR);
        ;
        }
    
    if (pubring)
        {
        ops_keyring_free(pubring);
        pubring=NULL;
        }
    if (secring)
        {
        ops_keyring_free(secring);
        secring=NULL;
        }
    if (myring)
        {
        ops_keyring_free(myring);
        myring=NULL;
        }

    exit(EXIT_SUCCESS);
    }
