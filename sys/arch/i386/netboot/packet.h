/*
 * source in this file came from
 * the Mach ethernet boot written by Leendert van Doorn.
 *
 * Packet layout definitions
 *
 *	$Id: packet.h,v 1.2 1993/08/02 17:53:01 mycroft Exp $
 */

/* implementation constants */
#define	PKT_POOLSIZE	5
#define	PKT_DATASIZE	1514

/*
 * Structure of a packet.
 * Each packet can hold exactly one ethernet message.
 */
typedef struct {
  u_short pkt_used;			/* whether this packet it used */
  u_short pkt_len;			/* length of data  */
  u_char *pkt_offset;			/* current offset in data */
  u_char pkt_data[PKT_DATASIZE];	/* packet data */
} packet_t;

void PktInit(void);
packet_t *PktAlloc(u_long);
void PktRelease(packet_t *);
