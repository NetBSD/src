#include "common.h"
#include <openpgpsdk/util.h>
#include <memory.h>
#include <fcntl.h>
#include <assert.h>

#include <openpgpsdk/final.h>

static ops_parse_cb_return_t
callback(const ops_parser_content_t *content,ops_parse_cb_info_t *cbinfo)
    {
    ops_secret_key_t **skey=ops_parse_cb_get_arg(cbinfo);

    if(content->tag == OPS_PTAG_CT_SECRET_KEY)
	{
	*skey=malloc(sizeof **skey);
	memcpy(*skey,&content->content.secret_key,sizeof **skey);
	return OPS_KEEP_MEMORY;
	}

    return OPS_RELEASE_MEMORY;
    }
    
ops_secret_key_t *get_secret_key(const char *keyfile)
    {
    ops_parse_info_t *pinfo;
    ops_secret_key_t *skey;
    int fd;

    pinfo=ops_parse_info_new();

    skey=NULL;
    ops_parse_cb_set(pinfo,callback,&skey);

    fd=open(keyfile,O_RDONLY);
    assert(fd >= 0);
    ops_reader_set_fd(pinfo,fd);

    ops_parse(pinfo);

    return skey;
    }
