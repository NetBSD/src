/* Sign a plaintext message with a key, producing a detached signature */

#include "common.h"
#include <openpgpsdk/crypto.h>
#include <openpgpsdk/signature.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

#include <openpgpsdk/final.h>

int main(int argc,char **argv)
    {
    const char *keyfile;
    const char *plainfile;
    const char *user_id;
    const char *hashstr;
    const char *sigfile;
    ops_secret_key_t *skey;
    ops_create_signature_t *sig;
    ops_hash_algorithm_t alg;
    int fd;
    ops_create_info_t *info;
    unsigned char keyid[OPS_KEY_ID_SIZE];

    if(argc != 6)
	{
	fprintf(stderr,"%s <secret key file> <user_id> <hash> <plaintext file>"
		" <signature file>\n",argv[0]);
	exit(1);
	}

    keyfile=argv[1];
    user_id=argv[2];
    hashstr=argv[3];
    plainfile=argv[4];
    sigfile=argv[5];

    ops_init();

    skey=get_secret_key(keyfile);
    assert(skey);

    alg=ops_hash_algorithm_from_text(hashstr);
    if(alg == OPS_HASH_UNKNOWN)
	{
	fprintf(stderr,"Unkonwn hash algorithm: %s\n",hashstr);
	exit(2);
	}

    sig=ops_create_signature_new();
    ops_signature_start_cleartext_signature(sig,skey,alg,OPS_SIG_BINARY);

    fd=open(plainfile,O_RDONLY);
    if(fd < 0)
	{
	perror(plainfile);
	exit(3);
	}

    for( ; ; )
	{
	unsigned char buf[8192];
	int n;
	
	n=read(fd,buf,sizeof buf);
	if(!n)
	    break;
	if(n < 0)
	    {
	    perror(plainfile);
	    exit(4);
	    }
	ops_signature_add_data(sig,buf,n);
	}

    close(fd);

    ops_signature_add_creation_time(sig,time(NULL));

    ops_keyid(keyid,&skey->public_key);
    ops_signature_add_issuer_key_id(sig,keyid);

    ops_signature_hashed_subpackets_end(sig);

    fd=open(sigfile,O_CREAT|O_TRUNC|O_WRONLY,0666);
    if(fd < 0)
	{
	perror(sigfile);
	exit(5);
	}

    info=ops_create_info_new();
    ops_writer_set_fd(info,fd);

    ops_write_signature(sig,&skey->public_key,skey,info);

    ops_secret_key_free(skey);

    return 0;
    }
