/* sunndd-bpf.c - the Sun Network Disk (nd) daemon BPF component: */

/*
 * Copyright (c) 2001 Matthew Fredette.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *   3. All advertising materials mentioning features or use of this software
 *      must display the following acknowledgement:
 *        This product includes software developed by Matthew Fredette.
 *   4. The name of Matthew Fredette may not be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission. 
 * 
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

/* <<Header: /data/home/fredette/project/THE-WEIGHT-CVS/sunndd/config/sunndd-bpf.c,v 1.2 2001/05/09 20:50:46 fredette Exp >> */

/*
 * <<Log: sunndd-bpf.c,v >>
 * Revision 1.2  2001/05/09 20:50:46  fredette
 * Removed an unnecessary comment.
 *
 * Revision 1.1  2001/01/29 15:12:13  fredette
 * Added.
 *
 */

static const char _sunndd_bpf_c_rcsid[] = "<<Id: sunndd-bpf.c,v 1.2 2001/05/09 20:50:46 fredette Exp >>";

/* includes: */
#include <net/bpf.h>

/* structures: */
struct _sunndd_interface_bpf {

  /* the size of the packet buffer for the interface: */
  size_t _sunndd_interface_bpf_buffer_size;

  /* the packet buffer for the interface: */
  char *_sunndd_interface_bpf_buffer;

  /* the next offset within the packet buffer, and the end
     of the data in the packet buffer: */
  size_t _sunndd_interface_bpf_buffer_offset;
  size_t _sunndd_interface_bpf_buffer_end;
};

/* the BPF program to capture ND packets: */
static struct bpf_insn sunnd_bpf_filter[] = {

  /* drop this packet if its ethertype isn't ETHERTYPE_IP: */
  BPF_STMT(BPF_LD + BPF_H + BPF_ABS, SUNNDD_OFFSETOF(struct ether_header, ether_type)),
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, ETHERTYPE_IP, 0, 9),

  /* drop this packet if its IP protocol isn't IPPROTO_ND: */
  BPF_STMT(BPF_LD + BPF_B + BPF_ABS, sizeof(struct ether_header) + SUNNDD_OFFSETOF(struct ip, ip_p)),
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, IPPROTO_ND, 0, 7),

  /* drop this packet if it's a fragment: */
  BPF_STMT(BPF_LD + BPF_H + BPF_ABS, sizeof(struct ether_header) + SUNNDD_OFFSETOF(struct ip, ip_off)),
  BPF_JUMP(BPF_JMP + BPF_JSET + BPF_K, 0x3fff, 5, 0),

  /* drop this packet if it is carrying data (we only want
     requests, which have no data): */
  BPF_STMT(BPF_LD + BPF_H + BPF_ABS, sizeof(struct ether_header) + SUNNDD_OFFSETOF(struct ip, ip_len)),
  BPF_STMT(BPF_LDX + BPF_B + BPF_MSH, sizeof(struct ether_header)),
  BPF_STMT(BPF_ALU + BPF_SUB + BPF_X, 0),
  BPF_JUMP(BPF_JMP + BPF_JEQ + BPF_K, sizeof(struct sunnd_packet), 0, 1),

  /* accept this packet: */
  BPF_STMT(BPF_RET + BPF_K, (u_int) -1),

  /* drop this packet: */
  BPF_STMT(BPF_RET + BPF_K, 0),
};

/* this opens a raw socket using BPF. */
int
sunndd_raw_open(struct sunndd_interface *interface)
{
  int network_fd;
#define DEV_BPF_FORMAT "/dev/bpf%d"
  char dev_bpf_filename[sizeof(DEV_BPF_FORMAT) + (sizeof(int) * 3) + 1];
  int minor;
  int saved_errno;
  u_int bpf_opt;
  struct bpf_version version;
  u_int packet_buffer_size;
  struct bpf_program program;
  struct _sunndd_interface_bpf *interface_bpf;

  /* loop trying to open a /dev/bpf device: */
  for(minor = 0;; minor++) {

    /* form the name of the next device to try, then try opening it.
       if we succeed, we're done: */
    sprintf(dev_bpf_filename, DEV_BPF_FORMAT, minor);
    _SUNNDD_DEBUG((fp, "bpf: trying %s", dev_bpf_filename));
    if ((network_fd = open(dev_bpf_filename, O_RDWR)) >= 0) {
      _SUNNDD_DEBUG((fp, "bpf: opened %s", dev_bpf_filename));
      break;
    }

    /* we failed to open this device.  if this device was simply busy,
       loop: */
    _SUNNDD_DEBUG((fp, "bpf: failed to open %s: %s", dev_bpf_filename, strerror(errno)));
    if (errno == EBUSY) {
      continue;
    }

    /* otherwise, we have failed: */
    return (-1);
  }

  /* this macro helps in closing the BPF socket on error: */
#define _SUNNDD_RAW_OPEN_ERROR(x) saved_errno = errno; x; errno = saved_errno

  /* check the BPF version: */
  if (ioctl(network_fd, BIOCVERSION, &version) < 0) {
    _SUNNDD_DEBUG((fp, "bpf: failed to get the BPF version on %s: %s", 
		   dev_bpf_filename, strerror(errno)));
    _SUNNDD_RAW_OPEN_ERROR(close(network_fd));
    return (-1);
  }
  if (version.bv_major != BPF_MAJOR_VERSION
      || version.bv_minor < BPF_MINOR_VERSION) {
    _SUNNDD_DEBUG((fp, "bpf: kernel BPF version is %d.%d, my BPF version is %d.%d", 
		   version.bv_major, version.bv_minor,
		   BPF_MAJOR_VERSION, BPF_MINOR_VERSION));
    close(network_fd);
    errno = ENXIO;
    return (-1);
  }

  /* put the BPF device into immediate mode: */
  bpf_opt = TRUE;
  if (ioctl(network_fd, BIOCIMMEDIATE, &bpf_opt) < 0) {
    _SUNNDD_DEBUG((fp, "bpf: failed to put %s into immediate mode: %s", 
		   dev_bpf_filename, strerror(errno)));
    _SUNNDD_RAW_OPEN_ERROR(close(network_fd));
    return (-1);
  }

  /* tell the BPF device we're providing complete Ethernet headers: */
  bpf_opt = TRUE;
  if (ioctl(network_fd, BIOCSHDRCMPLT, &bpf_opt) < 0) {
    _SUNNDD_DEBUG((fp, "bpf: failed to put %s into complete-headers mode: %s", 
		   dev_bpf_filename, strerror(errno)));
    _SUNNDD_RAW_OPEN_ERROR(close(network_fd));
    return (-1);
  }

  /* point the BPF device at the interface we're using: */
  if (ioctl(network_fd, BIOCSETIF, interface->sunndd_interface_ifreq) < 0) {
    _SUNNDD_DEBUG((fp, "bpf: failed to point BPF socket at %s: %s", 
		   interface->sunndd_interface_ifreq->ifr_name, strerror(errno)));
    saved_errno = errno;
    close(network_fd);
    errno = saved_errno;
    return (-1);
  }

  /* set the filter on the BPF device: */
  program.bf_len = sizeof(sunnd_bpf_filter) / sizeof(sunnd_bpf_filter[0]);
  program.bf_insns = sunnd_bpf_filter;
  if (ioctl(network_fd, BIOCSETF, &program) < 0) {
    _SUNNDD_DEBUG((fp, "bpf: failed to set the filter on %s: %s", 
		   dev_bpf_filename, strerror(errno)));
    _SUNNDD_RAW_OPEN_ERROR(close(network_fd));
    return (-1);
  }
  
  /* get the BPF read buffer size: */
  if (ioctl(network_fd, BIOCGBLEN, &packet_buffer_size) < 0) {
    _SUNNDD_DEBUG((fp, "bpf: failed to read the buffer size for %s: %s", 
		   dev_bpf_filename, strerror(errno)));
    _SUNNDD_RAW_OPEN_ERROR(close(network_fd));
    return (-1);
  }
  _SUNNDD_DEBUG((fp, "bpf: buffer size for %s is %u", 
		 dev_bpf_filename, packet_buffer_size));

  /* allocate our private interface information and we're done: */
  interface->sunndd_interface_fd = network_fd;
  interface_bpf = sunndd_new0(struct _sunndd_interface_bpf, 1);
  interface_bpf->_sunndd_interface_bpf_buffer_size = packet_buffer_size;
  interface_bpf->_sunndd_interface_bpf_buffer = sunndd_new(char, packet_buffer_size);
  interface->_sunndd_interface_raw_private = interface_bpf;
  return (0);
#undef _SUNNDD_RAW_OPEN_ERROR
}

/* this reads a raw packet: */
int
sunndd_raw_read(struct sunndd_interface *interface, void *packet_buffer, size_t packet_buffer_size)
{
  struct _sunndd_interface_bpf *interface_bpf;
  ssize_t buffer_end;
  struct bpf_hdr the_bpf_header;
  fd_set fdset_read;

  /* recover our state: */
  interface_bpf = (struct _sunndd_interface_bpf *) interface->_sunndd_interface_raw_private;

  /* loop until we have something to return: */
  for(;;) {

    /* if the buffer is empty, fill it: */
    if (interface_bpf->_sunndd_interface_bpf_buffer_offset
	>= interface_bpf->_sunndd_interface_bpf_buffer_end) {

      /* select on the BPF socket: */
      _SUNNDD_DEBUG((fp, "bpf: calling select"));
      FD_ZERO(&fdset_read);
      FD_SET(interface->sunndd_interface_fd, &fdset_read);
      switch(select(interface->sunndd_interface_fd + 1, &fdset_read, NULL, NULL, NULL)) {
      case 0:
	_SUNNDD_DEBUG((fp, "bpf: select returned zero"));
	continue;
      case 1:
	break;
      default:
	if (errno == EINTR) {
	  _SUNNDD_DEBUG((fp, "bpf: select got EINTR"));
	  continue;
	}
	_SUNNDD_DEBUG((fp, "bpf: select failed: %s", strerror(errno)));
	return (-1);
      }
      assert(FD_ISSET(interface->sunndd_interface_fd, &fdset_read));

      /* read the BPF socket: */
      _SUNNDD_DEBUG((fp, "bpf: calling read"));
      buffer_end = read(interface->sunndd_interface_fd,
			interface_bpf->_sunndd_interface_bpf_buffer,
			interface_bpf->_sunndd_interface_bpf_buffer_size);
      if (buffer_end <= 0) {
	_SUNNDD_DEBUG((fp, "bpf: failed to read packets: %s", strerror(errno)));
	return (-1);
      }
      _SUNNDD_DEBUG((fp, "bpf: read %d bytes of packets", buffer_end));
      interface_bpf->_sunndd_interface_bpf_buffer_offset = 0;
      interface_bpf->_sunndd_interface_bpf_buffer_end = buffer_end;
    }

    /* if there's not enough for a BPF header, flush the buffer: */
    if ((interface_bpf->_sunndd_interface_bpf_buffer_offset
	 + sizeof(the_bpf_header))
	> interface_bpf->_sunndd_interface_bpf_buffer_end) {
      _SUNNDD_DEBUG((fp, "bpf: flushed garbage BPF header bytes"));
      interface_bpf->_sunndd_interface_bpf_buffer_end = 0;
      continue;
    }

    /* get the BPF header and check it: */
    memcpy(&the_bpf_header,
	   interface_bpf->_sunndd_interface_bpf_buffer
	   + interface_bpf->_sunndd_interface_bpf_buffer_offset,
	   sizeof(the_bpf_header));
    interface_bpf->_sunndd_interface_bpf_buffer_offset += the_bpf_header.bh_hdrlen;

    /* if we're missing some part of the packet: */
    if (the_bpf_header.bh_caplen != the_bpf_header.bh_datalen
	|| ((interface_bpf->_sunndd_interface_bpf_buffer_offset + the_bpf_header.bh_datalen)
	    > interface_bpf->_sunndd_interface_bpf_buffer_end)) {
      _SUNNDD_DEBUG((fp, "bpf: flushed truncated BPF packet"));
      interface_bpf->_sunndd_interface_bpf_buffer_offset += the_bpf_header.bh_datalen;
      continue;
    }

    /* silently ignore packets that don't even have Ethernet headers,
       and those packets that we transmitted: */
    if (the_bpf_header.bh_datalen < sizeof(struct ether_header)
	|| !memcmp(((struct ether_header *) 
		    (interface_bpf->_sunndd_interface_bpf_buffer
		     + interface_bpf->_sunndd_interface_bpf_buffer_offset))->ether_shost,
		   interface->sunndd_interface_ether,
		   ETHER_ADDR_LEN)) {
      /* silently ignore packets from us: */
      interface_bpf->_sunndd_interface_bpf_buffer_offset += the_bpf_header.bh_datalen;
      continue;
    }
	      
    /* if the caller hasn't provided a large enough buffer: */
    if (packet_buffer_size < the_bpf_header.bh_datalen) {
      errno = EIO;
      interface_bpf->_sunndd_interface_bpf_buffer_offset += the_bpf_header.bh_datalen;
      return (-1);
    }
    
    /* return this captured packet to the user: */
    memcpy(packet_buffer, 
	   interface_bpf->_sunndd_interface_bpf_buffer
	   + interface_bpf->_sunndd_interface_bpf_buffer_offset,
	   the_bpf_header.bh_datalen);
    interface_bpf->_sunndd_interface_bpf_buffer_offset += the_bpf_header.bh_datalen;
    return (the_bpf_header.bh_datalen);
  }
  /* NOTREACHED */
}

/* this writes a raw packet: */
int
sunndd_raw_write(struct sunndd_interface *interface, void *packet_buffer, size_t packet_buffer_size)
{
  return(write(interface->sunndd_interface_fd, packet_buffer, packet_buffer_size));
}

