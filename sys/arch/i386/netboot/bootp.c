/*
 * bootp functions
 *
 *	$Id: bootp.c,v 1.2 1993/08/02 17:52:51 mycroft Exp $
 */

int BootpGetInfo(ipaddr_t *ip_servaddr, ipaddr_t *ip_myaddr, ipaddr_t *ip_gateway, char *file_name) {
  /* zero a packet */
  bzero(xxx);
  /* set dest to 255... */
  xxx = IP_BCASTADDR
  /* set up udp ports */
  /* send it */
  /* wait for reply or timeout */
  /* do exp backoff and retry if timeout */
  /* give up after a minute or so */
  /* return success or failure */
}

/* tbd - set up incoming packet handler to handle bootp replies??? */
