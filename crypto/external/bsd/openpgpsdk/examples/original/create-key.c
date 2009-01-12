#include <openpgpsdk/create.h>
#include <openpgpsdk/util.h>
#include <stdio.h>

#include <openpgpsdk/final.h>

int main(int argc,char **argv)
    {
    ops_create_info_t *info;
    const unsigned char *id;
    const char *nstr;
    const char *estr;
    BIGNUM *n=NULL;
    BIGNUM *e=NULL;

    if(argc != 4)
	{
	fprintf(stderr,"%s <n> <e> <user id>\n",argv[0]);
	exit(1);
	}
    
    nstr=argv[1];
    estr=argv[2];
    id=(unsigned char *)argv[3];

    BN_hex2bn(&n,nstr);
    BN_hex2bn(&e,estr);

    info=ops_create_info_new();
    ops_writer_set_fd(info,1);

    ops_write_rsa_public_key(time(NULL),n,e,info);
    ops_write_user_id(id,info);

    ops_create_info_delete(info);

    return 0;
    }
