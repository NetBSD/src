/*
 *
 * structure/definitions for the 32 byte id prom found in sun3s
 *
 */


struct idprom {
    unsigned char idp_format;
    unsigned char idp_machtype;
    unsigned char idp_etheraddr;
    long          idp_date;
    unsigned char idp_serialnum[3];
    unsigned char idp_checksum;
    unsigned char ipd_reserved[16];
};

#define IDPROM_VERSION 1
#define IDPROM_SIZE (sizeof(struct idprom))

#define OIDPROM_BASE 0		/* bogus XXX */
int idprom_fetch __P((struct idprom *, int version));

