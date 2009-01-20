#include "common.h"
#include <openpgpsdk/create.h>
#include <openpgpsdk/util.h>
#include <openpgpsdk/signature.h>
#include <openpgpsdk/packet-parse.h>
#include <openssl/rsa.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <openpgpsdk/final.h>

// XXX: rename to create-self-signed-key

/*
 * Slightly strange beast that might get replaced later - it needs
 * some other OpenPGP package to generate a key for it to use - this
 * is because we don't have a way to generate our own (yet).
 */

int main(int argc,char **argv)
    {
    ops_create_info_t *info;
    ops_create_signature_t *sig;
    ops_user_id_t id;
    unsigned char keyid[OPS_KEY_ID_SIZE];
    unsigned char *user_id; /* not const coz we use _fast_ */
    ops_secret_key_t skey;
    RSA *rsa;
    const char *secfile;
    const char *pubfile;
    BIGNUM *f4;
    int fd;

    if(argc != 4)
	{
	fprintf(stderr,"%s <user_id> <secret key file> <public key file>\n",
		argv[0]);
	exit(1);
	}

    user_id=(unsigned char *)argv[1];
    secfile=argv[2];
    pubfile=argv[3];

    ops_init();

    f4=BN_new();
    BN_set_word(f4,RSA_F4);

    rsa=RSA_new();
    RSA_generate_key_ex(rsa,1024,f4,NULL);

    info=ops_create_info_new();

    // OpenSSL has p and q reversed relative to OpenPGP
    ops_fast_create_rsa_secret_key(&skey,time(NULL),rsa->d,rsa->q,rsa->p,
				   rsa->iqmp,rsa->n,rsa->e);

    fd=open(secfile,O_CREAT|O_TRUNC|O_WRONLY,0666);
    if(fd < 0)
	{
	perror(secfile);
	exit(2);
	}
    ops_writer_set_fd(info,fd);

    ops_write_struct_secret_key(&skey,info);

    ops_writer_close(info);

    close(fd);

    fd=open(pubfile,O_CREAT|O_TRUNC|O_WRONLY,0666);
    if(fd < 0)
	{
	perror(pubfile);
	exit(2);
	}
    ops_writer_set_fd(info,fd);

    ops_write_struct_public_key(&skey.public_key,info);

    ops_fast_create_user_id(&id,user_id);
    ops_write_struct_user_id(&id,info);

    sig=ops_create_signature_new();

    ops_signature_start_key_signature(sig,&skey.public_key,&id,
				      OPS_CERT_POSITIVE);
    ops_signature_add_creation_time(sig,time(NULL));

    ops_keyid(keyid,&skey.public_key);
    ops_signature_add_issuer_key_id(sig,keyid);

    ops_signature_add_primary_user_id(sig,ops_true);

    ops_signature_hashed_subpackets_end(sig);

    ops_write_signature(sig,&skey.public_key,&skey,info);

    ops_create_signature_delete(sig);
    ops_create_info_delete(info);

    close(fd);

    RSA_free(rsa);
    BN_free(f4);

    ops_finish();

    return 0;
    }
