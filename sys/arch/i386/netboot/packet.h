/* netboot
 *
 * $Log: packet.h,v $
 * Revision 1.1  1993/07/08 16:04:08  brezak
 * Diskless boot prom code from Jim McKim (mckim@lerc.nasa.gov)
 *
 * Revision 1.1.1.1  1993/05/28  11:41:08  mckim
 * Initial version.
 *
 *
 * source in this file came from
 * the Mach ethernet boot written by Leendert van Doorn.
 *
 * Packet layout definitions
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
