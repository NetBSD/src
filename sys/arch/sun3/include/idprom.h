/*
 *
 * structure/definitions for the 32 byte id prom found in sun3s
 *
 */


struct idprom {
    unsigned char idp_format;
    unsigned char idp_machtype;
    unsigned char idp_etheraddr[6];
    long          idp_date;
    unsigned char idp_serialnum[3];
    unsigned char idp_checksum;
    unsigned char idp_reserved[16];
};

#define IDPROM_VERSION 1
#define IDPROM_SIZE (sizeof(struct idprom))

int idprom_fetch __P((struct idprom *, int));
void idprom_etheraddr __P((u_char *));
