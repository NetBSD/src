#include <openpgpsdk/packet-parse.h>
#include <openpgpsdk/util.h>
#include <openpgpsdk/keyring.h>
#include <openpgpsdk/accumulate.h>
#include <openpgpsdk/armour.h>
#include <openpgpsdk/std_print.h>

#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>

#include <openpgpsdk/final.h>

static char *pname;
static ops_keyring_t keyring;

static ops_parse_cb_return_t
callback(const ops_parser_content_t *content_,ops_parse_cb_info_t *cbinfo)
    {
    ops_parser_content_union_t* content=(ops_parser_content_union_t *)&content_->content;
    static ops_boolean_t skipping;
    static const ops_keydata_t *decrypter;

    OPS_USED(cbinfo);

    ops_print_packet(content_);

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

    case OPS_PTAG_CT_ARMOUR_HEADER:
	printf ("OPS_PTAG_CT_ARMOUR_HEADER\n");
	break;

    case OPS_PARSER_PTAG:
	printf ("OPS_PARSER_PTAG\n");
	break;

    case OPS_PTAG_CT_PK_SESSION_KEY:
	printf ("OPS_PTAG_CT_PK_SESSION_KEY\n");
	if(decrypter)
	    break;

	printf("looking for key\n");
	decrypter=ops_keyring_find_key_by_id(&keyring,
					     content->pk_session_key.key_id);
	if(!decrypter)
	    break;
	printf("found key\n");
	break;

    case OPS_PTAG_CT_SE_IP_DATA_HEADER:
	printf("TBD: OPS_PTAG_CT_SE_IP_DATA_HEADER\n");
	break;

    case OPS_PTAG_CT_SE_IP_DATA_BODY:
	printf("TBD: OPS_PTAG_CT_SE_IP_DATA_BODY\n");
	/* decrypt it here */
	break;

    case OPS_PARSER_CMD_GET_SECRET_KEY:
	printf("TBD: OPS_PARSER_CMD_GET_SECRET_KEY\n");
	const ops_keydata_t* key=ops_keyring_find_key_by_id(&keyring,content->get_secret_key.pk_session_key->key_id);
	if (!key || !ops_key_is_secret(key))
	    return 0;

	ops_set_secret_key(content,key);

	break;

    case OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY:
	printf("TBD: OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY\n");
	break;

    case OPS_PTAG_CT_COMPRESSED:
    case OPS_PTAG_CT_LITERAL_DATA_HEADER:
    case OPS_PTAG_CT_LITERAL_DATA_BODY:
	// Ignore these packets 
	// They're handled in ops_parse_one_packet()
	// and nothing else needs to be done
	break;

    default:
	fprintf(stderr,"Unexpected packet tag=%d (0x%x)\n",content_->tag,
		content_->tag);
	assert(0);
	}

    return OPS_RELEASE_MEMORY;
    }

static void usage()
    {
    fprintf(stderr,"%s [-a] -k <keyring> -e <file to decrypt>\n",pname);
    exit(1);
    }

int main(int argc,char **argv)
    {
    ops_parse_info_t *pinfo;
    int fd;
    const char *keyfile=(const char *)NULL;
    const char *encfile=(const char *)NULL;
    ops_boolean_t armour=ops_false;
    int ch;

    pname=argv[0];

    while((ch=getopt(argc,argv,"ak:e:")) != -1)
	switch(ch)
	    {
	case 'a':
	    armour=ops_true;
	    break;

	case 'k':
	    keyfile=optarg;
	    break;
		
	case 'e':
	    encfile=optarg;
	    break;
		
	default:
	    usage();
	    }

    argc-=optind;
    argv+=optind;
	
    if (argc!=0)
	{
	usage();
	exit(1);
	}

    if (!keyfile || !encfile)
	{
	usage();
	exit(1);
	}

    ops_init();

    ops_keyring_read(&keyring,keyfile);

    pinfo=ops_parse_info_new();

    fd=open(encfile,O_RDONLY);
    if(fd < 0)
	{
	perror(encfile);
	exit(2);
	}

    // Now do file
    ops_reader_set_fd(pinfo,fd);

    ops_parse_cb_set(pinfo,callback,NULL);

    if(armour)
	ops_reader_push_dearmour(pinfo,ops_false,ops_false,ops_false);

    ops_parse(pinfo);

    if(armour)
	ops_reader_pop_dearmour(pinfo);

    ops_keyring_free(&keyring);
    ops_finish();

    return 0;
    }


