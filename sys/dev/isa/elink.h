#define	ELINK_ID_PORT	0x100
#define	ELINK_RESET	0xc0

#define	ELINK_507_POLY	0xe7
#define	ELINK_509_POLY	0xcf

void elink_reset __P((void));
void elink_idseq __P((u_char p));
