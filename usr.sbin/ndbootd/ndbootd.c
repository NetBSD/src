/*	$NetBSD: ndbootd.c,v 1.4 2001/06/13 21:38:30 fredette Exp $	*/

/* ndbootd.c - the Sun Network Disk (nd) daemon: */

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

/* <<Header: /data/home/fredette/project/THE-WEIGHT-CVS/ndbootd/ndbootd.c,v 1.9 2001/06/13 21:19:11 fredette Exp >> */

/*
 * <<Log: ndbootd.c,v >>
 * Revision 1.9  2001/06/13 21:19:11  fredette
 * (main): Don't assume that a successful, but short, read
 * leaves a zero in errno.  Instead, just check for the short
 * read by looking at the byte count that read returned.
 *
 * Revision 1.8  2001/05/23 02:35:36  fredette
 * Changed many debugging printfs to compile quietly on the
 * alpha.  Patch from Andrew Brown <atatat@atatdot.net>.
 *
 * Revision 1.7  2001/05/22 13:13:20  fredette
 * Ran indent(1) with NetBSD's KNF-approximating profile.
 *
 * Revision 1.6  2001/05/22 12:53:40  fredette
 * [HAVE_STRICT_ALIGNMENT]: Added code to copy packet headers
 * between the buffer and local variables, to satisfy
 * alignment constraints.
 *
 * Revision 1.5  2001/05/15 14:43:24  fredette
 * Now have prototypes for the allocation functions.
 * (main): Now handle boot blocks that aren't an integral
 * multiple of the block size.
 *
 * Revision 1.4  2001/05/09 20:53:38  fredette
 * (main): Now insert a small delay before sending each packet.
 * Sending packets too quickly apparently overwhelms clients.
 * Added new single-letter versions of all options that didn't
 * already have them.  Expanded some debug messages, and fixed
 * others to display Ethernet addresses correctly.
 *
 * Revision 1.3  2001/01/31 17:35:50  fredette
 * (main): Fixed various printf argument lists.
 *
 * Revision 1.2  2001/01/30 15:35:38  fredette
 * Now, ndbootd assembles disk images for clients on-the-fly.
 * Defined many new macros related to this.
 * (main): Added support for the --boot2 option.  Turned the
 * original disk-image filename into the filename of the
 * first-stage boot program.  Now do better multiple-client
 * support, especially when it comes to checking if a client
 * is really ours.  Now assemble client-specific disk images
 * on-the-fly, potentially serving each client a different
 * second-stage boot.
 *
 * Revision 1.1  2001/01/29 15:12:13  fredette
 * Added.
 *
 */

static const char _ndbootd_c_rcsid[] = "<<Id: ndbootd.c,v 1.9 2001/06/13 21:19:11 fredette Exp >>";

/* includes: */
#include "ndbootd.h"

/* the number of blocks that Sun-2 PROMs load, starting from block
   zero: */
#define NDBOOTD_PROM_BLOCK_COUNT (16)

/* the first block number of the (dummy) Sun disklabel: */
#define NDBOOTD_SUNDK_BLOCK_FIRST (0)

/* the number of blocks in the (dummy) Sun disklabel: */
#define NDBOOTD_SUNDK_BLOCK_COUNT (1)

/* the first block number of the first-stage boot program.
   the first-stage boot program begins right after the (dummy)
   Sun disklabel: */
#define NDBOOTD_BOOT1_BLOCK_FIRST (NDBOOTD_SUNDK_BLOCK_FIRST + NDBOOTD_SUNDK_BLOCK_COUNT)

/* the number of blocks in the first-stage boot program: */
#define NDBOOTD_BOOT1_BLOCK_COUNT (NDBOOTD_PROM_BLOCK_COUNT - NDBOOTD_BOOT1_BLOCK_FIRST)

/* the first block number of any second-stage boot program.
   any second-stage boot program begins right after the first-stage boot program: */
#define NDBOOTD_BOOT2_BLOCK_FIRST (NDBOOTD_BOOT1_BLOCK_FIRST + NDBOOTD_BOOT1_BLOCK_COUNT)

/* this macro returns the number of bytes available in an object starting at a given offset: */
#define NDBOOTD_BYTES_AVAIL(block_number, byte_offset, obj_block_first, obj_block_count) \
  ((((ssize_t) (obj_block_count) - (ssize_t) ((block_number) - (obj_block_first))) * NDBOOT_BSIZE) - (ssize_t) (byte_offset))

/* this determines how long we can cache file descriptors and RARP
   information: */
#define NDBOOTD_CLIENT_TTL_SECONDS (10)

/* this determines how long we wait before sending a packet: */
#define NDBOOTD_SEND_DELAY_USECONDS (10000)

/* this macro helps us size a struct ifreq: */
#ifdef HAVE_SOCKADDR_SA_LEN
#define SIZEOF_IFREQ(ifr) (sizeof(ifr->ifr_name) + ifr->ifr_addr.sa_len)
#else				/* !HAVE_SOCKADDR_SA_LEN */
#define SIZEOF_IFREQ(ifr) (sizeof(ifr->ifr_name) + sizeof(struct sockaddr))
#endif				/* !HAVE_SOCKADDR_SA_LEN */

/* prototypes: */
void *ndbootd_malloc _NDBOOTD_P((size_t));
void *ndbootd_malloc0 _NDBOOTD_P((size_t));
void *ndbootd_memdup _NDBOOTD_P((void *, size_t));

/* globals: */
const char *_ndbootd_argv0;
#ifdef _NDBOOTD_DO_DEBUG
int _ndbootd_debug;
#endif				/* _NDBOOTD_DO_DEBUG */

/* allocators: */
void *
ndbootd_malloc(size_t size)
{
	void *buffer;
	if ((buffer = malloc(size)) == NULL) {
		abort();
	}
	return (buffer);
}
void *
ndbootd_malloc0(size_t size)
{
	void *buffer;
	buffer = ndbootd_malloc(size);
	memset(buffer, 0, size);
	return (buffer);
}
void *
ndbootd_memdup(void *buffer0, size_t size)
{
	void *buffer1;
	buffer1 = ndbootd_malloc(size);
	memcpy(buffer1, buffer0, size);
	return (buffer1);
}
#define ndbootd_free free
#define ndbootd_new(t, c) ((t *) ndbootd_malloc(sizeof(t) * (c)))
#define ndbootd_new0(t, c) ((t *) ndbootd_malloc0(sizeof(t) * (c)))
#define ndbootd_dup(t, b, c) ((t *) ndbootd_memdup(b, c))

/* this calculates an IP packet header checksum: */
static void
_ndbootd_ip_cksum(struct ip * ip_packet)
{
	u_int16_t *_word, word;
	u_int32_t checksum;
	unsigned int byte_count, bytes_left;

	/* we assume that the IP packet header is 16-bit aligned: */
	assert((((unsigned long) ip_packet) % sizeof(word)) == 0);

	/* initialize for the checksum: */
	checksum = 0;

	/* sum up the packet contents: */
	_word = (u_int16_t *) ip_packet;
	byte_count = ip_packet->ip_hl << 2;
	for (bytes_left = byte_count; bytes_left >= sizeof(*_word);) {
		checksum += *(_word++);
		bytes_left -= sizeof(*_word);
	}
	word = 0;
	memcpy(&word, _word, bytes_left);
	checksum += word;

	/* finish the checksum: */
	checksum = (checksum >> 16) + (checksum & 0xffff);
	checksum += (checksum >> 16);
	ip_packet->ip_sum = (~checksum);
}
/* this finds a network interface: */
static struct ndbootd_interface *
_ndbootd_find_interface(const char *ifr_name_user)
{
	int saved_errno;
	int dummy_fd;
	char ifreq_buffer[16384];	/* FIXME - magic constant. */
	struct ifconf ifc;
	struct ifreq *ifr;
	struct ifreq *ifr_user;
	size_t ifr_offset;
	struct sockaddr_in saved_ip_address;
	short saved_flags;
#ifdef HAVE_AF_LINK
	struct ifreq *link_ifreqs[20];	/* FIXME - magic constant. */
	size_t link_ifreqs_count;
	size_t link_ifreqs_i;
	struct sockaddr_dl *sadl;
#endif				/* HAVE_AF_LINK */
	struct ndbootd_interface *interface;

	/* make a dummy socket so we can read the interface list: */
	if ((dummy_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		return (NULL);
	}
	/* read the interface list: */
	ifc.ifc_len = sizeof(ifreq_buffer);
	ifc.ifc_buf = ifreq_buffer;
	if (ioctl(dummy_fd, SIOCGIFCONF, &ifc) < 0) {
		saved_errno = errno;
		close(dummy_fd);
		errno = saved_errno;
		return (NULL);
	}
#ifdef HAVE_AF_LINK
	/* start our list of link address ifreqs: */
	link_ifreqs_count = 0;
#endif				/* HAVE_AF_LINK */

	/* walk the interface list: */
	ifr_user = NULL;
	for (ifr_offset = 0;; ifr_offset += SIZEOF_IFREQ(ifr)) {

		/* stop walking if we have run out of space in the buffer.
		 * note that before we can use SIZEOF_IFREQ, we have to make
		 * sure that there is a minimum number of bytes in the buffer
		 * to use it (namely, that there's a whole struct sockaddr
		 * available): */
		ifr = (struct ifreq *) (ifreq_buffer + ifr_offset);
		if ((ifr_offset + sizeof(ifr->ifr_name) + sizeof(struct sockaddr)) > ifc.ifc_len
		    || (ifr_offset + SIZEOF_IFREQ(ifr)) > ifc.ifc_len) {
			errno = ENOENT;
			break;
		}
#ifdef HAVE_AF_LINK
		/* if this is a hardware address, save it: */
		if (ifr->ifr_addr.sa_family == AF_LINK) {
			if (link_ifreqs_count < (sizeof(link_ifreqs) / sizeof(link_ifreqs[0]))) {
				link_ifreqs[link_ifreqs_count++] = ifr;
			}
			continue;
		}
#endif				/* HAVE_AF_LINK */

		/* ignore this interface if it doesn't do IP: */
		if (ifr->ifr_addr.sa_family != AF_INET) {
			continue;
		}
		/* get the interface flags, preserving the IP address in the
		 * struct ifreq across the call: */
		saved_ip_address = *((struct sockaddr_in *) & ifr->ifr_addr);
		if (ioctl(dummy_fd, SIOCGIFFLAGS, ifr) < 0) {
			ifr = NULL;
			break;
		}
		saved_flags = ifr->ifr_flags;
		*((struct sockaddr_in *) & ifr->ifr_addr) = saved_ip_address;

		/* ignore this interface if it isn't up and running: */
		if ((saved_flags & (IFF_UP | IFF_RUNNING)) != (IFF_UP | IFF_RUNNING)) {
			continue;
		}
		/* if we don't have an interface yet, take this one depending
		 * on whether the user asked for an interface by name or not.
		 * if he did, and this is it, take this one.  if he didn't,
		 * and this isn't a loopback interface, take this one: */
		if (ifr_user == NULL
		    && (ifr_name_user != NULL
			? !strncmp(ifr->ifr_name, ifr_name_user, sizeof(ifr->ifr_name))
			: !(ifr->ifr_flags & IFF_LOOPBACK))) {
			ifr_user = ifr;
		}
	}

	/* close the dummy socket: */
	saved_errno = errno;
	close(dummy_fd);
	errno = saved_errno;

	/* if we don't have an interface to return: */
	if (ifr_user == NULL) {
		return (NULL);
	}
	/* start the interface description: */
	interface = ndbootd_new0(struct ndbootd_interface, 1);

#ifdef HAVE_AF_LINK

	/* we must be able to find an AF_LINK ifreq that gives us the
	 * interface's Ethernet address. */
	ifr = NULL;
	for (link_ifreqs_i = 0; link_ifreqs_i < link_ifreqs_count; link_ifreqs_i++) {
		if (!strncmp(link_ifreqs[link_ifreqs_i]->ifr_name,
			ifr_user->ifr_name,
			sizeof(ifr_user->ifr_name))) {
			ifr = link_ifreqs[link_ifreqs_i];
			break;
		}
	}
	if (ifr == NULL) {
		free(interface);
		return (NULL);
	}
	/* copy out the Ethernet address: */
	sadl = (struct sockaddr_dl *) & ifr->ifr_addr;
	memcpy(interface->ndbootd_interface_ether, LLADDR(sadl), sadl->sdl_alen);

#else				/* !HAVE_AF_LINK */
#error "must have AF_LINK for now"
#endif				/* !HAVE_AF_LINK */

	/* finish this interface and return it: */
	interface->ndbootd_interface_ifreq = (struct ifreq *) ndbootd_memdup(ifr_user, SIZEOF_IFREQ(ifr_user));
	interface->ndbootd_interface_fd = -1;
	return (interface);
}

int
main(int argc, char *argv[])
{
	int argv_i;
	int show_usage;
	const char *interface_name;
	const char *boot1_file_name;
	const char *boot2_x_name;
	char *boot2_file_name;
	int boot2_x_name_is_dir;
	time_t last_open_time;
	int boot1_fd;
	int boot2_fd;
	time_t last_rarp_time;
	char last_client_ether[ETHER_ADDR_LEN];
	struct in_addr last_client_ip;
	struct stat stat_buffer;
	int32_t boot1_block_count;
	int32_t boot2_block_count;
	size_t boot1_byte_count;
	size_t boot2_byte_count;
	ssize_t byte_count_read;
	struct ndbootd_interface *interface;
	char pid_buffer[(sizeof(pid_t) * 3) + 2];
	unsigned char packet_buffer[sizeof(struct ether_header) + IP_MAXPACKET];
	unsigned char disk_buffer[NDBOOT_MAX_BYTE_COUNT];
	char hostname_buffer[MAXHOSTNAMELEN + 1];
	struct hostent *the_hostent;
	ssize_t packet_length;
	time_t now;
	struct ether_header *ether_packet;
	struct ip *ip_packet;
	struct ndboot_packet *nd_packet;
#ifdef HAVE_STRICT_ALIGNMENT
	struct ether_header ether_packet_buffer;
	unsigned char ip_packet_buffer[IP_MAXPACKET];
	struct ndboot_packet nd_packet_buffer;
#endif				/* HAVE_STRICT_ALIGNMENT */
	int nd_window_size;
	int nd_window_filled;
	off_t file_offset;
	size_t disk_buffer_offset;
	size_t block_number;
	size_t byte_offset;
	ssize_t byte_count;
	ssize_t byte_count_wanted;
	struct timeval send_delay;
	int fd;

	/* check our command line: */
	if ((_ndbootd_argv0 = strrchr(argv[0], '/')) == NULL)
		_ndbootd_argv0 = argv[0];
	else
		_ndbootd_argv0++;
	show_usage = FALSE;
#ifdef _NDBOOTD_DO_DEBUG
	_ndbootd_debug = FALSE;
#endif				/* _NDBOOTD_DO_DEBUG */
	boot1_file_name = NULL;
	boot2_x_name = NULL;
	interface_name = NULL;
	nd_window_size = NDBOOT_WINDOW_SIZE_DEFAULT;
	for (argv_i = 1; argv_i < argc; argv_i++) {
		if (argv[argv_i][0] != '-'
		    || argv[argv_i][1] == '\0') {
			break;
		} else if (!strcmp(argv[argv_i], "-s")
		    || !strcmp(argv[argv_i], "--boot2")) {
			if (++argv_i < argc) {
				boot2_x_name = argv[argv_i];
			} else {
				show_usage = TRUE;
				break;
			}
		} else if (!strcmp(argv[argv_i], "-i")
		    || !strcmp(argv[argv_i], "--interface")) {
			if (++argv_i < argc) {
				interface_name = argv[argv_i];
			} else {
				show_usage = TRUE;
				break;
			}
		} else if (!strcmp(argv[argv_i], "-w")
		    || !strcmp(argv[argv_i], "--window-size")) {
			if (++argv_i == argc || (nd_window_size = atoi(argv[argv_i])) <= 0) {
				show_usage = TRUE;
				break;
			}
		}
#ifdef _NDBOOTD_DO_DEBUG
		else if (!strcmp(argv[argv_i], "-d")
		    || !strcmp(argv[argv_i], "--debug")) {
			_ndbootd_debug = TRUE;
		}
#endif				/* _NDBOOTD_DO_DEBUG */
		else {
			if (strcmp(argv[argv_i], "-h")
			    && strcmp(argv[argv_i], "--help")) {
				fprintf(stderr, "%s error: unknown switch '%s'\n",
				    _ndbootd_argv0, argv[argv_i]);
			}
			show_usage = TRUE;
			break;
		}
	}
	if (argv_i + 1 == argc) {
		boot1_file_name = argv[argv_i];
	} else {
		show_usage = TRUE;
	}

	if (show_usage) {
		fprintf(stderr, "\
usage: %s [OPTIONS] BOOT1-BIN\n\
where OPTIONS are:\n\
  -s, --boot2 { BOOT2-BIN | DIR }\n\
                          find a second-stage boot program in the file\n\
                          BOOT2-BIN or in the directory DIR\n\
  -i, --interface NAME    use interface NAME\n\
  -w, --window-size COUNT \n\
                          send at most COUNT unacknowledged packets [default=%d]\n",
		    _ndbootd_argv0,
		    NDBOOT_WINDOW_SIZE_DEFAULT);
#ifdef _NDBOOTD_DO_DEBUG
		fprintf(stderr, "\
  -d, --debug             set debug mode\n");
#endif				/* _NDBOOTD_DO_DEBUG */
		exit(1);
	}
	/* if we have been given a name for the second-stage boot, see if it's
	 * a filename or a directory: */
	boot2_x_name_is_dir = FALSE;
	if (boot2_x_name != NULL) {
		if (stat(boot2_x_name, &stat_buffer) < 0) {
			fprintf(stderr, "%s error: could not stat %s: %s\n",
			    _ndbootd_argv0, boot2_x_name, strerror(errno));
			exit(1);
		}
		if (S_ISDIR(stat_buffer.st_mode)) {
			boot2_x_name_is_dir = TRUE;
		} else if (!S_ISREG(stat_buffer.st_mode)) {
			fprintf(stderr, "%s error: %s is neither a regular file nor a directory\n",
			    _ndbootd_argv0, boot2_x_name);
			exit(1);
		}
	}
	/* find the interface we will use: */
	if ((interface = _ndbootd_find_interface(interface_name)) == NULL) {
		fprintf(stderr, "%s error: could not find the interface to use: %s\n",
		    _ndbootd_argv0, strerror(errno));
		exit(1);
	}
	_NDBOOTD_DEBUG((fp, "opening interface %s", interface->ndbootd_interface_ifreq->ifr_name));

	/* open the network interface: */
	if (ndbootd_raw_open(interface)) {
		fprintf(stderr, "%s error: could not open the %s interface: %s\n",
		    _ndbootd_argv0, interface->ndbootd_interface_ifreq->ifr_name, strerror(errno));
		exit(1);
	}
	_NDBOOTD_DEBUG((fp, "opened interface %s (ip %s ether %02x:%02x:%02x:%02x:%02x:%02x)",
		interface->ndbootd_interface_ifreq->ifr_name,
		inet_ntoa(((struct sockaddr_in *) & interface->ndbootd_interface_ifreq->ifr_addr)->sin_addr),
		((unsigned char *) interface->ndbootd_interface_ether)[0],
		((unsigned char *) interface->ndbootd_interface_ether)[1],
		((unsigned char *) interface->ndbootd_interface_ether)[2],
		((unsigned char *) interface->ndbootd_interface_ether)[3],
		((unsigned char *) interface->ndbootd_interface_ether)[4],
		((unsigned char *) interface->ndbootd_interface_ether)[5]));

	/* become a daemon: */
#ifdef _NDBOOTD_DO_DEBUG
	if (!_ndbootd_debug)
#endif				/* _NDBOOTD_DO_DEBUG */
	{

		/* fork and exit: */
		switch (fork()) {
		case 0:
			break;
		case -1:
			fprintf(stderr, "%s error: could not fork: %s\n",
			    _ndbootd_argv0, strerror(errno));
			exit(1);
		default:
			exit(0);
		}

		/* close all file descriptors: */
#ifdef HAVE_GETDTABLESIZE
		fd = getdtablesize();
#else				/* !HAVE_GETDTABLESIZE */
		fd = -1;
#endif				/* !HAVE_GETDTABLESIZE */
		for (; fd >= 0; fd--) {
			if (fd != interface->ndbootd_interface_fd) {
				close(fd);
			}
		}

#ifdef HAVE_SETSID
		/* become our own session: */
		setsid();
#endif				/* HAVE_SETSID */
	}
	/* write the pid file: */
	if ((fd = open(NDBOOTD_PID_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644)) >= 0) {
		sprintf(pid_buffer, "%u\n", getpid());
		write(fd, pid_buffer, strlen(pid_buffer));
		close(fd);
	}
#ifdef HAVE_STRICT_ALIGNMENT
	/* we will be dealing with all packet headers in separate buffers, to
	 * make sure everything is correctly aligned: */
	ether_packet = &ether_packet_buffer;
	ip_packet = (struct ip *) & ip_packet_buffer[0];
	nd_packet = &nd_packet_buffer;
#else				/* !HAVE_STRICT_ALIGNMENT */
	/* we will always find the Ethernet header and the IP packet at the
	 * front of the buffer: */
	ether_packet = (struct ether_header *) packet_buffer;
	ip_packet = (struct ip *) (ether_packet + 1);
#endif				/* !HAVE_STRICT_ALIGNMENT */

	/* initialize our state: */
	last_rarp_time = 0;
	last_open_time = 0;
	boot1_fd = -1;
	boot2_file_name = NULL;
	boot2_fd = -1;

	/* loop processing packets: */
	for (;;) {

		/* receive another packet: */
		packet_length = ndbootd_raw_read(interface, packet_buffer, sizeof(packet_buffer));
		if (packet_length < 0) {
			_NDBOOTD_DEBUG((fp, "failed to receive packet: %s", strerror(errno)));
			exit(1);
			continue;
		}
		now = time(NULL);

		/* check the Ethernet and IP parts of the packet: */
		if (packet_length
		    < (sizeof(struct ether_header)
			+ sizeof(struct ip)
			+ sizeof(struct ndboot_packet))) {
			_NDBOOTD_DEBUG((fp, "ignoring a too-short packet of length %ld", (long) packet_length));
			continue;
		}
#ifdef HAVE_STRICT_ALIGNMENT
		memcpy(ether_packet, packet_buffer, sizeof(struct ether_header));
		memcpy(ip_packet, packet_buffer + sizeof(struct ether_header),
		    (((struct ip *) (packet_buffer + sizeof(struct ether_header)))->ip_hl << 2));
#endif				/* !HAVE_STRICT_ALIGNMENT */
		if (ether_packet->ether_type != htons(ETHERTYPE_IP)
		    || ip_packet->ip_p != IPPROTO_ND) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with the wrong Ethernet or IP protocol"));
			continue;
		}
		_ndbootd_ip_cksum(ip_packet);
		if (ip_packet->ip_sum != 0) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with a bad IP checksum"));
			continue;
		}
		if (packet_length
		    != (sizeof(struct ether_header)
			+ (ip_packet->ip_hl << 2)
			+ sizeof(struct ndboot_packet))) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with bad total length %ld", (long) packet_length));
			continue;
		}
		/* if we need to, refresh our RARP cache: */
		if ((last_rarp_time + NDBOOTD_CLIENT_TTL_SECONDS) < now
		    || memcmp(last_client_ether, ether_packet->ether_shost, ETHER_ADDR_LEN)) {

			/* turn the Ethernet address into a hostname: */
			if (ether_ntohost(hostname_buffer, (struct ether_addr *) ether_packet->ether_shost)) {
				_NDBOOTD_DEBUG((fp, "could not resolve %02x:%02x:%02x:%02x:%02x:%02x into a hostname: %s",
					((unsigned char *) ether_packet->ether_shost)[0],
					((unsigned char *) ether_packet->ether_shost)[1],
					((unsigned char *) ether_packet->ether_shost)[2],
					((unsigned char *) ether_packet->ether_shost)[3],
					((unsigned char *) ether_packet->ether_shost)[4],
					((unsigned char *) ether_packet->ether_shost)[5],
					strerror(errno)));
				continue;
			}
			/* turn the hostname into an IP address: */
			hostname_buffer[sizeof(hostname_buffer) - 1] = '\0';
			if ((the_hostent = gethostbyname(hostname_buffer)) == NULL
			    || the_hostent->h_addrtype != AF_INET) {
				_NDBOOTD_DEBUG((fp, "could not resolve %s into an IP address: %s",
					hostname_buffer,
					strerror(errno)));
				continue;
			}
			/* save these new results in our RARP cache: */
			last_rarp_time = now;
			memcpy(last_client_ether, ether_packet->ether_shost, ETHER_ADDR_LEN);
			memcpy(&last_client_ip, the_hostent->h_addr, sizeof(last_client_ip));
			_NDBOOTD_DEBUG((fp, "IP address for %02x:%02x:%02x:%02x:%02x:%02x is %s",
				((unsigned char *) last_client_ether)[0],
				((unsigned char *) last_client_ether)[1],
				((unsigned char *) last_client_ether)[2],
				((unsigned char *) last_client_ether)[3],
				((unsigned char *) last_client_ether)[4],
				((unsigned char *) last_client_ether)[5],
				inet_ntoa(last_client_ip)));

			/* this will cause the file descriptor cache to be
			 * reloaded, the next time we make it that far: */
			last_open_time = 0;
		}
		/* if this IP packet was broadcast, rewrite the source IP
		 * address to be the client, else, check that the client is
		 * using the correct IP addresses: */
		if (ip_packet->ip_dst.s_addr == htonl(0)) {
			ip_packet->ip_src = last_client_ip;
		} else {
			if (ip_packet->ip_src.s_addr !=
			    last_client_ip.s_addr) {
				_NDBOOTD_DEBUG((fp, "machine %02x:%02x:%02x:%02x:%02x:%02x is using the wrong IP address\n",
					((unsigned char *) ether_packet->ether_shost)[0],
					((unsigned char *) ether_packet->ether_shost)[1],
					((unsigned char *) ether_packet->ether_shost)[2],
					((unsigned char *) ether_packet->ether_shost)[3],
					((unsigned char *) ether_packet->ether_shost)[4],
					((unsigned char *) ether_packet->ether_shost)[5]));
				continue;
			}
			if (ip_packet->ip_dst.s_addr
			    != ((struct sockaddr_in *) & interface->ndbootd_interface_ifreq->ifr_addr)->sin_addr.s_addr) {
				_NDBOOTD_DEBUG((fp, "machine %02x:%02x:%02x:%02x:%02x:%02x is sending to the wrong IP address\n",
					((unsigned char *) ether_packet->ether_shost)[0],
					((unsigned char *) ether_packet->ether_shost)[1],
					((unsigned char *) ether_packet->ether_shost)[2],
					((unsigned char *) ether_packet->ether_shost)[3],
					((unsigned char *) ether_packet->ether_shost)[4],
					((unsigned char *) ether_packet->ether_shost)[5]));
				continue;
			}
		}

		/* if we need to, refresh our "cache" of file descriptors for
		 * the boot programs: */
		if ((last_open_time + NDBOOTD_CLIENT_TTL_SECONDS) < now) {

			/* close any previously opened programs: */
			if (boot1_fd >= 0) {
				close(boot1_fd);
			}
			if (boot2_file_name != NULL) {
				free(boot2_file_name);
			}
			if (boot2_fd >= 0) {
				close(boot2_fd);
			}
			/* open the first-stage boot program: */
			if ((boot1_fd = open(boot1_file_name, O_RDONLY)) < 0) {
				_NDBOOTD_DEBUG((fp, "could not open %s: %s",
					boot1_file_name, strerror(errno)));
				continue;
			}
			if (fstat(boot1_fd, &stat_buffer) < 0) {
				_NDBOOTD_DEBUG((fp, "could not stat %s: %s",
					boot1_file_name, strerror(errno)));
				continue;
			}
			boot1_byte_count = stat_buffer.st_size;
			boot1_block_count = (boot1_byte_count + (NDBOOT_BSIZE - 1)) / NDBOOT_BSIZE;
			if (boot1_block_count > NDBOOTD_BOOT1_BLOCK_COUNT) {
				_NDBOOTD_DEBUG((fp, "first-stage boot program %s has too many blocks (%d, max is %d)",
					boot1_file_name, boot1_block_count, NDBOOTD_BOOT1_BLOCK_COUNT));
			}
			_NDBOOTD_DEBUG((fp, "first-stage boot program %s has %d blocks",
				boot1_file_name, boot1_block_count));

			/* open any second-stage boot program: */
			if (boot2_x_name != NULL) {

				/* determine what the name of the second-stage
				 * boot program will be: */
				if (boot2_x_name_is_dir) {
					if ((boot2_file_name = malloc(strlen(boot2_x_name) + strlen("/00000000.SUN2") + 1)) != NULL) {
						sprintf(boot2_file_name, "%s/%02X%02X%02X%02X.SUN2",
						    boot2_x_name,
						    ((unsigned char *) &last_client_ip)[0],
						    ((unsigned char *) &last_client_ip)[1],
						    ((unsigned char *) &last_client_ip)[2],
						    ((unsigned char *) &last_client_ip)[3]);
					}
				} else {
					boot2_file_name = strdup(boot2_x_name);
				}
				if (boot2_file_name == NULL) {
					abort();
				}
				/* open the second-stage boot program: */
				if ((boot2_fd = open(boot2_file_name, O_RDONLY)) < 0) {
					_NDBOOTD_DEBUG((fp, "could not open %s: %s",
						boot2_file_name, strerror(errno)));
					continue;
				}
				if (fstat(boot2_fd, &stat_buffer) < 0) {
					_NDBOOTD_DEBUG((fp, "could not stat %s: %s",
						boot2_file_name, strerror(errno)));
					continue;
				}
				boot2_byte_count = stat_buffer.st_size;
				boot2_block_count = (boot2_byte_count + (NDBOOT_BSIZE - 1)) / NDBOOT_BSIZE;
				_NDBOOTD_DEBUG((fp, "second-stage boot program %s has %d blocks",
					boot2_file_name, boot2_block_count));
			}
			/* success: */
			last_open_time = now;
		}
		/* check the nd packet: */
#ifdef HAVE_STRICT_ALIGNMENT
		memcpy(nd_packet, packet_buffer + sizeof(struct ether_header) + (ip_packet->ip_hl << 2), sizeof(struct ndboot_packet));
#else				/* !HAVE_STRICT_ALIGNMENT */
		nd_packet = (struct ndboot_packet *) (((char *) ip_packet) + (ip_packet->ip_hl << 2));
#endif				/* !HAVE_STRICT_ALIGNMENT */

		/* dump a bunch of debug information: */
		_NDBOOTD_DEBUG((fp, "recv: op 0x%02x minor 0x%02x error %d vers %d seq %d blk %d bcount %d off %d count %d",
			nd_packet->ndboot_packet_op,
			nd_packet->ndboot_packet_minor,
			nd_packet->ndboot_packet_error,
			nd_packet->ndboot_packet_disk_version,
			(int) ntohl(nd_packet->ndboot_packet_sequence),
			(int) ntohl(nd_packet->ndboot_packet_block_number),
			(int) ntohl(nd_packet->ndboot_packet_byte_count),
			(int) ntohl(nd_packet->ndboot_packet_current_byte_offset),
			(int) ntohl(nd_packet->ndboot_packet_current_byte_count)));

		/* ignore this packet if it has a bad opcode, a bad minor
		 * number, a bad disk version, a bad block number, a bad byte
		 * count, a bad current byte offset, or a bad current byte
		 * count: */
		/* FIXME - for some of these conditions, we probably should
		 * return an NDBOOT_OP_ERROR packet: */
		if ((nd_packet->ndboot_packet_op & NDBOOT_OP_MASK) != NDBOOT_OP_READ) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with bad op %d",
				nd_packet->ndboot_packet_op & NDBOOT_OP_MASK));
			continue;
		}
		if (nd_packet->ndboot_packet_minor != NDBOOT_MINOR_NDP0) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with device minor %d",
				nd_packet->ndboot_packet_minor));
			continue;
		}
		if (nd_packet->ndboot_packet_disk_version != 0) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with disk version %d",
				nd_packet->ndboot_packet_disk_version));
			continue;
		}
		if (ntohl(nd_packet->ndboot_packet_block_number) < 0) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with bad block number %d",
				(int) ntohl(nd_packet->ndboot_packet_block_number)));
			continue;
		}
		if (ntohl(nd_packet->ndboot_packet_byte_count) <= 0 ||
		    ntohl(nd_packet->ndboot_packet_byte_count) > NDBOOT_MAX_BYTE_COUNT) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with bad byte count %d",
				(int) ntohl(nd_packet->ndboot_packet_byte_count)));
			continue;
		}
		if (ntohl(nd_packet->ndboot_packet_current_byte_offset) < 0 ||
		    ntohl(nd_packet->ndboot_packet_current_byte_offset)
		    >= ntohl(nd_packet->ndboot_packet_byte_count)) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with bad current offset %d",
				(int) ntohl(nd_packet->ndboot_packet_current_byte_offset)));
			continue;
		}
		if (ntohl(nd_packet->ndboot_packet_current_byte_count) < 0 ||
		    ntohl(nd_packet->ndboot_packet_current_byte_count)
		    > (ntohl(nd_packet->ndboot_packet_byte_count)
			- ntohl(nd_packet->ndboot_packet_current_byte_offset))) {
			_NDBOOTD_DEBUG((fp, "ignoring a packet with bad current count %d",
				(int) ntohl(nd_packet->ndboot_packet_current_byte_count)));
			continue;
		}
		/* if we were given a current byte count of zero, rewrite it
		 * to be the maximum: */
		if (ntohl(nd_packet->ndboot_packet_current_byte_count) == 0) {
			nd_packet->ndboot_packet_current_byte_count =
			    htonl(ntohl(nd_packet->ndboot_packet_byte_count)
			    - ntohl(nd_packet->ndboot_packet_current_byte_offset));
		}
		/* read the data: */
		disk_buffer_offset = 0;
		block_number = ntohl(nd_packet->ndboot_packet_block_number);
		byte_offset = ntohl(nd_packet->ndboot_packet_current_byte_offset);
		byte_count = ntohl(nd_packet->ndboot_packet_current_byte_count);
		for (; byte_count > 0;) {

			/* adjust the current block number and byte offset
			 * such that the byte offset is always < NDBOOT_BSIZE: */
			block_number += (byte_offset / NDBOOT_BSIZE);
			byte_offset = byte_offset % NDBOOT_BSIZE;

			/* dispatch on the beginning block number: */
			byte_count_read = 0;

			/* the (dummy) Sun disk label: */
			if (block_number >= NDBOOTD_SUNDK_BLOCK_FIRST
			    && block_number < (NDBOOTD_SUNDK_BLOCK_FIRST + NDBOOTD_SUNDK_BLOCK_COUNT)) {
				byte_count_read = MIN(NDBOOTD_BYTES_AVAIL(block_number, byte_offset,
					NDBOOTD_SUNDK_BLOCK_FIRST, NDBOOTD_SUNDK_BLOCK_COUNT),
				    byte_count);
			}
			/* the first-stage boot program: */
			else if (block_number >= NDBOOTD_BOOT1_BLOCK_FIRST
			    && block_number < (NDBOOTD_BOOT1_BLOCK_FIRST + NDBOOTD_BOOT1_BLOCK_COUNT)) {

				/* if any real part of the first-stage boot
				 * program is needed to satisfy the request,
				 * read it (otherwise we return garbage as
				 * padding): */
				byte_count_wanted = MIN(NDBOOTD_BYTES_AVAIL(block_number, byte_offset,
					NDBOOTD_BOOT1_BLOCK_FIRST, boot1_block_count),
				    byte_count);
				if (byte_count_wanted > 0) {

					file_offset = ((block_number - NDBOOTD_BOOT1_BLOCK_FIRST) * NDBOOT_BSIZE) + byte_offset;
					if (lseek(boot1_fd, file_offset, SEEK_SET) < 0) {
						_NDBOOTD_DEBUG((fp, "could not seek %s to block %ld offset %ld: %s",
							boot1_file_name,
							(long) (block_number - NDBOOTD_BOOT1_BLOCK_FIRST),
							(long) byte_offset,
							strerror(errno)));
						break;
					}
					byte_count_read = read(boot1_fd, disk_buffer + disk_buffer_offset, byte_count_wanted);
					/* pretend that the size of the
					 * first-stage boot program is a
					 * multiple of NDBOOT_BSIZE: */
					if (byte_count_read != byte_count_wanted
					    && byte_count_read > 0
					    && file_offset + byte_count_read == boot1_byte_count) {
						byte_count_read = byte_count_wanted;
					}
					if (byte_count_read != byte_count_wanted) {
						_NDBOOTD_DEBUG((fp, "could not read %ld bytes at block %ld offset %ld from %s: %s (read %ld bytes)",
							(long) byte_count_wanted,
							(long) (block_number - NDBOOTD_BOOT1_BLOCK_FIRST),
							(long) byte_offset,
							boot1_file_name,
							strerror(errno),
							(long) byte_count_read));
						break;
					}
				}
				/* the number of bytes we read, including any
				 * padding garbage: */
				byte_count_read = MIN(NDBOOTD_BYTES_AVAIL(block_number, byte_offset,
					NDBOOTD_BOOT1_BLOCK_FIRST, NDBOOTD_BOOT1_BLOCK_COUNT),
				    byte_count);
			}
			/* any second-stage boot program: */
			else if (block_number >= NDBOOTD_BOOT2_BLOCK_FIRST) {

				/* if any real part of any first-stage boot
				 * program is needed to satisfy the request,
				 * read it (otherwise we return garbage as
				 * padding): */
				byte_count_wanted = MIN(NDBOOTD_BYTES_AVAIL(block_number, byte_offset,
					NDBOOTD_BOOT2_BLOCK_FIRST, boot2_block_count),
				    byte_count);
				if (boot2_fd >= 0
				    && byte_count_wanted > 0) {

					file_offset = ((block_number - NDBOOTD_BOOT2_BLOCK_FIRST) * NDBOOT_BSIZE) + byte_offset;
					if (lseek(boot2_fd, file_offset, SEEK_SET) < 0) {
						_NDBOOTD_DEBUG((fp, "could not seek %s to block %ld offset %ld: %s",
							boot2_file_name,
							(long) (block_number - NDBOOTD_BOOT2_BLOCK_FIRST),
							(long) byte_offset,
							strerror(errno)));
						break;
					}
					byte_count_read = read(boot2_fd, disk_buffer + disk_buffer_offset, byte_count_wanted);
					/* pretend that the size of the
					 * second-stage boot program is a
					 * multiple of NDBOOT_BSIZE: */
					if (byte_count_read != byte_count_wanted
					    && byte_count_read > 0
					    && file_offset + byte_count_read == boot2_byte_count) {
						byte_count_read = byte_count_wanted;
					}
					if (byte_count_read != byte_count_wanted) {
						_NDBOOTD_DEBUG((fp, "could not read %ld bytes at block %ld offset %ld from %s: %s (read %ld bytes)",
							(long) byte_count_wanted,
							(long) (block_number - NDBOOTD_BOOT2_BLOCK_FIRST),
							(long) byte_offset,
							boot2_file_name,
							strerror(errno),
							(long) byte_count_read));
						break;
					}
				}
				/* the number of bytes we read, including any
				 * padding garbage: */
				byte_count_read = byte_count;
			}
			/* update for the amount that we read: */
			assert(byte_count_read > 0);
			disk_buffer_offset += byte_count_read;
			byte_offset += byte_count_read;
			byte_count -= byte_count_read;
		}
		if (byte_count > 0) {
			/* an error occurred: */
			continue;
		}
		/* set the Ethernet and IP destination and source addresses,
		 * and the IP TTL: */
		memcpy(ether_packet->ether_dhost, ether_packet->ether_shost, ETHER_ADDR_LEN);
		memcpy(ether_packet->ether_shost, interface->ndbootd_interface_ether, ETHER_ADDR_LEN);
#ifdef HAVE_STRICT_ALIGNMENT
		memcpy(packet_buffer, ether_packet, sizeof(struct ether_header));
#endif				/* !HAVE_STRICT_ALIGNMENT */
		ip_packet->ip_dst = ip_packet->ip_src;
		ip_packet->ip_src = ((struct sockaddr_in *) & interface->ndbootd_interface_ifreq->ifr_addr)->sin_addr;
		ip_packet->ip_ttl = 4;

		/* return the data: */
		nd_window_filled = 0;
		disk_buffer_offset = 0;
		byte_count = ntohl(nd_packet->ndboot_packet_current_byte_count);
		for (;;) {

			/* set the byte count on this packet: */
			nd_packet->ndboot_packet_current_byte_count = htonl(MIN(byte_count, NDBOOT_MAX_PACKET_DATA));

			/* set our opcode.  the opcode is always
			 * NDBOOT_OP_READ, ORed with NDBOOT_OP_FLAG_DONE |
			 * NDBOOT_OP_FLAG_WAIT if this packet finishes the
			 * request, or ORed with NDBOOT_OP_FLAG_WAIT if this
			 * packet fills the window: */
			nd_window_filled++;
			nd_packet->ndboot_packet_op =
			    (NDBOOT_OP_READ
			    | ((ntohl(nd_packet->ndboot_packet_current_byte_offset)
				    + ntohl(nd_packet->ndboot_packet_current_byte_count))
				== ntohl(nd_packet->ndboot_packet_byte_count)
				? (NDBOOT_OP_FLAG_DONE
				    | NDBOOT_OP_FLAG_WAIT)
				: (nd_window_filled == nd_window_size
				    ? NDBOOT_OP_FLAG_WAIT
				    : 0)));

			/* copy the data into the packet: */
			memcpy(packet_buffer +
			    sizeof(struct ether_header) + (ip_packet->ip_hl << 2) + sizeof(struct ndboot_packet),
			    disk_buffer + disk_buffer_offset,
			    ntohl(nd_packet->ndboot_packet_current_byte_count));

			/* finish the IP packet and calculate the checksum: */
			ip_packet->ip_len = htons((ip_packet->ip_hl << 2)
			    + sizeof(struct ndboot_packet)
			    + ntohl(nd_packet->ndboot_packet_current_byte_count));
			ip_packet->ip_sum = 0;
			_ndbootd_ip_cksum(ip_packet);

#ifdef HAVE_STRICT_ALIGNMENT
			memcpy(packet_buffer + sizeof(struct ether_header), ip_packet, ip_packet->ip_hl << 2);
			memcpy(packet_buffer + sizeof(struct ether_header) + (ip_packet->ip_hl << 2), nd_packet, sizeof(struct ndboot_packet));
#endif				/* !HAVE_STRICT_ALIGNMENT */

			/* dump a bunch of debug information: */
			_NDBOOTD_DEBUG((fp, "send: op 0x%02x minor 0x%02x error %d vers %d seq %d blk %d bcount %d off %d count %d (win %d)",
				nd_packet->ndboot_packet_op,
				nd_packet->ndboot_packet_minor,
				nd_packet->ndboot_packet_error,
				nd_packet->ndboot_packet_disk_version,
				(int) ntohl(nd_packet->ndboot_packet_sequence),
				(int) ntohl(nd_packet->ndboot_packet_block_number),
				(int) ntohl(nd_packet->ndboot_packet_byte_count),
				(int) ntohl(nd_packet->ndboot_packet_current_byte_offset),
				(int) ntohl(nd_packet->ndboot_packet_current_byte_count),
				nd_window_filled - 1));

			/* delay before sending the packet: */
			send_delay.tv_sec = 0;
			send_delay.tv_usec = NDBOOTD_SEND_DELAY_USECONDS;
			select(0, NULL, NULL, NULL, &send_delay);

			/* transmit the packet: */
			if (ndbootd_raw_write(interface, packet_buffer,
				sizeof(struct ether_header) + (ip_packet->ip_hl << 2) + sizeof(struct ndboot_packet) + ntohl(nd_packet->ndboot_packet_current_byte_count)) < 0) {
				_NDBOOTD_DEBUG((fp, "could not write a packet: %s",
					strerror(errno)));
			}
			/* if we set NDBOOT_OP_FLAG_DONE or
			 * NDBOOT_OP_FLAG_WAIT in the packet we just sent,
			 * we're done sending: */
			if (nd_packet->ndboot_packet_op != NDBOOT_OP_READ) {
				break;
			}
			/* advance to the next packet: */
			byte_count -= ntohl(nd_packet->ndboot_packet_current_byte_count);
			disk_buffer_offset += ntohl(nd_packet->ndboot_packet_current_byte_count);
			nd_packet->ndboot_packet_current_byte_offset =
			    htonl(ntohl(nd_packet->ndboot_packet_current_byte_offset)
			    + ntohl(nd_packet->ndboot_packet_current_byte_count));
		}
	}
	/* NOTREACHED */
}
/* the raw Ethernet access code: */
#include "config/ndbootd-bpf.c"
