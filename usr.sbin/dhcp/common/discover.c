/* dispatch.c

   Network input dispatcher... */

/*
 * Copyright (c) 1995-2000 Internet Software Consortium.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This software has been written for the Internet Software Consortium
 * by Ted Lemon in cooperation with Vixie Enterprises and Nominum, Inc.
 * To learn more about the Internet Software Consortium, see
 * ``http://www.isc.org/''.  To learn more about Vixie Enterprises,
 * see ``http://www.vix.com''.   To learn more about Nominum, Inc., see
 * ``http://www.nominum.com''.
 */

#ifndef lint
static char copyright[] =
"$Id: discover.c,v 1.4 2000/06/10 18:17:20 mellon Exp $ Copyright (c) 1995-2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#include "dhcpd.h"
#include <sys/ioctl.h>

struct interface_info *interfaces, *dummy_interfaces, *fallback_interface;
extern int interfaces_invalidated;
int quiet_interface_discovery;
u_int16_t local_port;
u_int16_t remote_port;
int (*dhcp_interface_setup_hook) (struct interface_info *, struct iaddr *);
int (*dhcp_interface_discovery_hook) (struct interface_info *);

struct in_addr limited_broadcast;
struct in_addr local_address;

void (*bootp_packet_handler) PROTO ((struct interface_info *,
				     struct dhcp_packet *, unsigned,
				     unsigned int,
				     struct iaddr, struct hardware *));

omapi_object_type_t *dhcp_type_interface;

OMAPI_OBJECT_ALLOC (interface, struct interface_info, dhcp_type_interface)

/* Use the SIOCGIFCONF ioctl to get a list of all the attached interfaces.
   For each interface that's of type INET and not the loopback interface,
   register that interface with the network I/O software, figure out what
   subnet it's on, and add it to the list of interfaces. */

void discover_interfaces (state)
	int state;
{
	struct interface_info *tmp, *ip;
	struct interface_info *last, *next;
	char buf [8192];
	struct ifconf ic;
	struct ifreq ifr;
	int i;
	int sock;
	int address_count = 0;
	struct subnet *subnet;
	struct shared_network *share;
	struct sockaddr_in foo;
	int ir;
	struct ifreq *tif;
#ifdef ALIAS_NAMES_PERMUTED
	char *s;
#endif
	isc_result_t status;
	static int setup_fallback = 0;

	if (!dhcp_type_interface) {
		status = omapi_object_type_register
			(&dhcp_type_interface, "interface",
			 interface_set_value, interface_get_value,
			 interface_destroy, interface_signal_handler,
			 interface_stuff_values, 0, 0, 0, 0, 0,
			 sizeof (struct interface_info));
		if (status != ISC_R_SUCCESS)
			log_fatal ("Can't create interface object type: %s",
				   isc_result_totext (status));
	}

	/* Create an unbound datagram socket to do the SIOCGIFADDR ioctl on. */
	if ((sock = socket (AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		log_fatal ("Can't create addrlist socket");

	/* Get the interface configuration information... */
	ic.ifc_len = sizeof buf;
	ic.ifc_ifcu.ifcu_buf = (caddr_t)buf;
	i = ioctl(sock, SIOCGIFCONF, &ic);

	if (i < 0)
		log_fatal ("ioctl: SIOCGIFCONF: %m");

	/* If we already have a list of interfaces, and we're running as
	   a DHCP server, the interfaces were requested. */
	if (interfaces && (state == DISCOVER_SERVER ||
			   state == DISCOVER_RELAY ||
			   state == DISCOVER_REQUESTED))
		ir = 0;
	else if (state == DISCOVER_UNCONFIGURED)
		ir = INTERFACE_REQUESTED | INTERFACE_AUTOMATIC;
	else
		ir = INTERFACE_REQUESTED;

	/* Cycle through the list of interfaces looking for IP addresses. */
	for (i = 0; i < ic.ifc_len;) {
		struct ifreq *ifp = (struct ifreq *)((caddr_t)ic.ifc_req + i);
#ifdef HAVE_SA_LEN
		if (ifp -> ifr_addr.sa_len > sizeof (struct sockaddr))
			i += (sizeof ifp -> ifr_name) + ifp -> ifr_addr.sa_len;
		else
#endif
			i += sizeof *ifp;

#ifdef ALIAS_NAMES_PERMUTED
		if ((s = strrchr (ifp -> ifr_name, ':'))) {
			*s = 0;
		}
#endif

#ifdef SKIP_DUMMY_INTERFACES
		if (!strncmp (ifp -> ifr_name, "dummy", 5))
			continue;
#endif


		/* See if this is the sort of interface we want to
		   deal with. */
		strcpy (ifr.ifr_name, ifp -> ifr_name);
		if (ioctl (sock, SIOCGIFFLAGS, &ifr) < 0)
			log_fatal ("Can't get interface flags for %s: %m",
			       ifr.ifr_name);

		/* See if we've seen an interface that matches this one. */
		for (tmp = interfaces; tmp; tmp = tmp -> next)
			if (!strcmp (tmp -> name, ifp -> ifr_name))
				break;

		/* Skip loopback, point-to-point and down interfaces,
		   except don't skip down interfaces if we're trying to
		   get a list of configurable interfaces. */
		if (((ifr.ifr_flags & IFF_LOOPBACK ||
		      ifr.ifr_flags & IFF_POINTOPOINT) && !tmp) ||
		    (!(ifr.ifr_flags & IFF_UP) &&
		     state != DISCOVER_UNCONFIGURED))
			continue;
		
		/* If there isn't already an interface by this name,
		   allocate one. */
		if (!tmp) {
			tmp = (struct interface_info *)0;
			status = interface_allocate (&tmp, MDL);
			if (status != ISC_R_SUCCESS)
				log_fatal ("Error allocating interface %s: %s",
					   ifp -> ifr_name,
					   isc_result_totext (status));
			strcpy (tmp -> name, ifp -> ifr_name);
			tmp -> circuit_id = (u_int8_t *)tmp -> name;
			tmp -> circuit_id_len = strlen (tmp -> name);
			tmp -> remote_id = 0;
			tmp -> remote_id_len = 0;
			tmp -> flags = ir;
			if (interfaces) {
				interface_reference (&tmp -> next,
						     interfaces, MDL);
				interface_dereference (&interfaces, MDL);
			}
			interface_reference (&interfaces, tmp, MDL);
			interface_dereference (&tmp, MDL);
			tmp = interfaces;
		}

		if (dhcp_interface_discovery_hook)
			(*dhcp_interface_discovery_hook) (tmp);

		/* If we have the capability, extract link information
		   and record it in a linked list. */
#ifdef HAVE_AF_LINK
		if (ifp -> ifr_addr.sa_family == AF_LINK) {
			struct sockaddr_dl *foo = ((struct sockaddr_dl *)
						   (&ifp -> ifr_addr));
#if defined (HAVE_SIN_LEN)
			tmp -> hw_address.hlen = foo -> sdl_alen;
#else
			tmp -> hw_address.hlen = 6; /* XXX!!! */
#endif
			if (foo -> sdl_type == IFT_ETHER) {
				tmp -> hw_address.hbuf [0] = HTYPE_ETHER;
#if defined (DEC_FDDI) || defined(NETBSD_FDDI)
			} else if (foo -> sdl_type == IFT_FDDI) {
				tmp -> hw_address.hbuf [0] = HTYPE_FDDI;
#endif
			} else {
				continue;
			}
			memcpy (&tmp -> hw_address.hbuf [1],
				LLADDR (foo), tmp -> hw_address.hlen);
			tmp -> hw_address.hlen++;	/* for type. */
		} else
#endif /* AF_LINK */

		if (ifp -> ifr_addr.sa_family == AF_INET) {
			struct iaddr addr;
			void *ptr;

			/* Get a pointer to the address... */
			ptr = &ifp -> ifr_addr;
			memcpy (&foo, ptr, sizeof ifp -> ifr_addr);

			/* We don't want the loopback interface. */
			if (foo.sin_addr.s_addr == htonl (INADDR_LOOPBACK) &&
			    ((tmp -> flags & INTERFACE_AUTOMATIC) &&
			     state == DISCOVER_SERVER))
			    continue;


			/* If this is the first real IP address we've
			   found, keep a pointer to ifreq structure in
			   which we found it. */
			if (!tmp -> ifp) {
#ifdef HAVE_SA_LEN
				unsigned len = ((sizeof ifp -> ifr_name) +
						ifp -> ifr_addr.sa_len);
#else
				unsigned len = sizeof *ifp;
#endif
				tif = (struct ifreq *)dmalloc (len, MDL);
				if (!tif)
					log_fatal ("no space for ifp.");
				memcpy (tif, ifp, len);
				tmp -> ifp = tif;
				tmp -> primary_address = foo.sin_addr;
			}

			/* Grab the address... */
			addr.len = 4;
			memcpy (addr.iabuf, &foo.sin_addr.s_addr,
				addr.len);
			if (dhcp_interface_setup_hook)
				(*dhcp_interface_setup_hook) (tmp, &addr);
		}
	}

#if defined (LINUX_SLASHPROC_DISCOVERY)
	/* On Linux, interfaces that don't have IP addresses don't
	   show up in the SIOCGIFCONF syscall.  This only matters for
	   the DHCP client, of course - the relay agent and server
	   should only care about interfaces that are configured with
	   IP addresses anyway.

	   The PROCDEV_DEVICE (/proc/net/dev) is a kernel-supplied file
	   that, when read, prints a human readable network status.   We
	   extract the names of the network devices by skipping the first
	   two lines (which are header) and then parsing off everything
	   up to the colon in each subsequent line - these lines start
	   with the interface name, then a colon, then a bunch of
	   statistics. */

	if (state == DISCOVER_UNCONFIGURED) {
		FILE *proc_dev;
		char buffer [256];
		int skip = 2;

		proc_dev = fopen (PROCDEV_DEVICE, "r");
		if (!proc_dev)
			log_fatal ("%s: %m", PROCDEV_DEVICE);

		while (fgets (buffer, sizeof buffer, proc_dev)) {
			char *name = buffer;
			char *sep;

			/* Skip the first two blocks, which are header
			   lines. */
			if (skip) {
				--skip;
				continue;
			}

			sep = strrchr (buffer, ':');
			if (sep)
				*sep = '\0';
			while (*name == ' ')
				name++;

			/* See if we've seen an interface that matches
			   this one. */
			for (tmp = interfaces; tmp; tmp = tmp -> next)
				if (!strcmp (tmp -> name, name))
					break;

			/* If we found one, nothing more to do.. */
			if (tmp)
				continue;

			/* Otherwise, allocate one. */
			tmp = (struct interface_info *)0;
			status = interface_allocate (&tmp, MDL);
			if (status != ISC_R_SUCCESS)
				log_fatal ("Can't allocate interface %s: %s",
					   name, isc_result_totext (status));
			tmp -> flags = ir;
			interface_reference (&tmp -> next, interfaces, MDL);
			interface_dereference (&interfaces, MDL);
			interface_reference (&interfaces, tmp, MDL);
			interface_dereference (&tmp, MDL);
			tmp = interfaces;

			if (dhcp_interface_discovery_hook)
				(*dhcp_interface_discovery_hook) (tmp);

		}
		fclose (proc_dev);
	}
#endif

	/* Now cycle through all the interfaces we found, looking for
	   hardware addresses. */
#if defined (HAVE_SIOCGIFHWADDR) && !defined (HAVE_AF_LINK)
	for (tmp = interfaces; tmp; tmp = tmp -> next) {
		struct ifreq ifr;
		struct sockaddr sa;
		int b, sk;
		
		if (!tmp -> ifp) {
			/* Make up an ifreq structure. */
			tif = (struct ifreq *)dmalloc (sizeof (struct ifreq),
						       MDL);
			if (!tif)
				log_fatal ("no space to remember ifp.");
			memset (tif, 0, sizeof (struct ifreq));
			strcpy (tif -> ifr_name, tmp -> name);
			tmp -> ifp = tif;
		}

		/* Read the hardware address from this interface. */
		ifr = *tmp -> ifp;
		if (ioctl (sock, SIOCGIFHWADDR, &ifr) < 0)
			continue;
		
		sa = *(struct sockaddr *)&ifr.ifr_hwaddr;
		
		switch (sa.sa_family) {
#ifdef HAVE_ARPHRD_TUNNEL
		      case ARPHRD_TUNNEL:
			/* ignore tunnel interfaces. */
#endif
#ifdef HAVE_ARPHRD_ROSE
		      case ARPHRD_ROSE:
#endif
#ifdef HAVE_ARPHRD_LOOPBACK
		      case ARPHRD_LOOPBACK:
			/* ignore loopback interface */
			break;
#endif

		      case ARPHRD_ETHER:
			tmp -> hw_address.hlen = 7;
			tmp -> hw_address.hbuf [0] = ARPHRD_ETHER;
			memcpy (&tmp -> hw_address.hbuf [1], sa.sa_data, 6);
			break;

#ifndef HAVE_ARPHRD_IEEE802
# define ARPHRD_IEEE802 HTYPE_IEEE802
#endif
		      case ARPHRD_IEEE802:
			tmp -> hw_address.hlen = 7;
			tmp -> hw_address.hbuf [0] = ARPHRD_IEEE802;
			memcpy (&tmp -> hw_address.hbuf [1], sa.sa_data, 6);
			break;

#ifndef HAVE_ARPHRD_FDDI
# define ARPHRD_FDDI HTYPE_FDDI
#endif
		      case ARPHRD_FDDI:
			tmp -> hw_address.hlen = 17;
			tmp -> hw_address.hbuf [0] = HTYPE_FDDI; /* XXX */
			memcpy (&tmp -> hw_address.hbuf [1], sa.sa_data, 16);
			break;

#ifdef HAVE_ARPHRD_METRICOM
		      case ARPHRD_METRICOM:
			tmp -> hw_address.hlen = 7;
			tmp -> hw_address.hbuf [0] = ARPHRD_METRICOM;
			memcpy (&tmp -> hw_address.hbuf [0], sa.sa_data, 6);
			break;
#endif

#ifdef HAVE_ARPHRD_AX25
		      case ARPHRD_AX25:
			tmp -> hw_address.hlen = 7;
			tmp -> hw_address.hbuf [0] = ARPHRD_AX25;
			memcpy (&tmp -> hw_address.hbuf [1], sa.sa_data, 6);
			break;
#endif

#ifdef HAVE_ARPHRD_NETROM
		      case ARPHRD_NETROM:
			tmp -> hw_address.hlen = 7;
			tmp -> hw_address.hbuf [0] = ARPHRD_NETROM;
			memcpy (&tmp -> hw_address.hbuf [1], sa.sa_data, 6);
			break;
#endif

		      default:
			log_error ("%s: unknown hardware address type %d",
				   ifr.ifr_name, sa.sa_family);
			break;
		}
	}
#endif /* defined (HAVE_SIOCGIFHWADDR) && !defined (HAVE_AF_LINK) */

	/* If we're just trying to get a list of interfaces that we might
	   be able to configure, we can quit now. */
	if (state == DISCOVER_UNCONFIGURED) {
		close (sock);
		return;
	}

	/* Weed out the interfaces that did not have IP addresses. */
	tmp = last = next = (struct interface_info *)0;
	if (interfaces)
		interface_reference (&tmp, interfaces, MDL);
	while (tmp) {
		if (next)
			interface_dereference (&next, MDL);
		if (tmp -> next)
			interface_reference (&next, tmp -> next, MDL);
		/* skip interfaces that are running already */
		if (tmp -> flags & INTERFACE_RUNNING)
			continue;
		if ((tmp -> flags & INTERFACE_AUTOMATIC) &&
		    state == DISCOVER_REQUESTED)
			tmp -> flags &= ~(INTERFACE_AUTOMATIC |
					  INTERFACE_REQUESTED);
		if (!tmp -> ifp || !(tmp -> flags & INTERFACE_REQUESTED)) {
			if ((tmp -> flags & INTERFACE_REQUESTED) != ir)
				log_fatal ("%s: not found", tmp -> name);
			if (!last) {
				if (interfaces)
					interface_dereference (&interfaces,
							       MDL);
				interface_reference (&interfaces, next, MDL);
			} else {
				interface_dereference (&last -> next, MDL);
				interface_reference (&last -> next, next, MDL);
			}
			if (tmp -> next)
				interface_dereference (&tmp -> next, MDL);

			/* Remember the interface in case we need to know
			   about it later. */
			if (dummy_interfaces) {
				interface_reference (&tmp -> next,
						     dummy_interfaces, MDL);
				interface_dereference (&dummy_interfaces, MDL);
			}
			interface_reference (&dummy_interfaces, tmp, MDL);
			interface_dereference (&tmp, MDL);
			if (next)
				interface_reference (&tmp, next, MDL);
			continue;
		}
		last = tmp;

		memcpy (&foo, &tmp -> ifp -> ifr_addr,
			sizeof tmp -> ifp -> ifr_addr);

		/* We must have a subnet declaration for each interface. */
		if (!tmp -> shared_network && (state == DISCOVER_SERVER)) {
			log_error ("No subnet declaration for %s (%s).",
				   tmp -> name, inet_ntoa (foo.sin_addr));
			log_error ("Please write a subnet declaration in %s",
				   "your dhcpd.conf file for the");
			log_fatal ("network segment to which interface %s %s",
				   tmp -> name, "is attached.");
		}

		/* Find subnets that don't have valid interface
		   addresses... */
		for (subnet = (tmp -> shared_network
			       ? tmp -> shared_network -> subnets
			       : (struct subnet *)0);
		     subnet; subnet = subnet -> next_sibling) {
			if (!subnet -> interface_address.len) {
				/* Set the interface address for this subnet
				   to the first address we found. */
				subnet -> interface_address.len = 4;
				memcpy (subnet -> interface_address.iabuf,
					&foo.sin_addr.s_addr, 4);
			}
		}

		/* Register the interface... */
		if_register_receive (tmp);
		if_register_send (tmp);
#if defined (HAVE_SETFD)
		if (fcntl (tmp -> rfdesc, F_SETFD, 1) < 0)
			log_error ("Can't set close-on-exec on %s: %m",
				   tmp -> name);
		if (tmp -> rfdesc != tmp -> wfdesc) {
			if (fcntl (tmp -> wfdesc, F_SETFD, 1) < 0)
				log_error ("Can't set close-on-exec on %s: %m",
					   tmp -> name);
		}
#endif
		interface_dereference (&tmp, MDL);
		if (next)
			interface_reference (&tmp, next, MDL);
	}

	/* Now register all the remaining interfaces as protocols. */
	for (tmp = interfaces; tmp; tmp = tmp -> next) {
		/* not if it's been registered before */
		if (tmp -> flags & INTERFACE_RUNNING)
			continue;
		status = omapi_register_io_object ((omapi_object_t *)tmp,
						   if_readsocket, 0,
						   got_one, 0, 0);
		if (status != ISC_R_SUCCESS)
			log_fatal ("Can't register I/O handle for %s: %s",
				   tmp -> name, isc_result_totext (status));
	}

	close (sock);

	if (!setup_fallback) {
		setup_fallback = 1;
		maybe_setup_fallback ();
	}

#if defined (HAVE_SETFD)
	if (fallback_interface) {
	    if (fcntl (fallback_interface -> rfdesc, F_SETFD, 1) < 0)
		log_error ("Can't set close-on-exec on fallback: %m");
	    if (fallback_interface -> rfdesc != fallback_interface -> wfdesc) {
		if (fcntl (fallback_interface -> wfdesc, F_SETFD, 1) < 0)
		    log_error ("Can't set close-on-exec on fallback: %m");
	    }
	}
#endif
}

int if_readsocket (h)
	omapi_object_t *h;
{
	struct interface_info *ip;

	if (h -> type != dhcp_type_interface)
		return -1;
	ip = (struct interface_info *)h;
	return ip -> rfdesc;
}

int setup_fallback (struct interface_info **fp, const char *file, int line)
{
	isc_result_t status;

	status = interface_allocate (&fallback_interface, file, line);
	if (status != ISC_R_SUCCESS)
		log_fatal ("Error allocating fallback interface: %s",
			   isc_result_totext (status));
	strcpy (fallback_interface -> name, "fallback");
	if (dhcp_interface_setup_hook)
		(*dhcp_interface_setup_hook) (fallback_interface,
					      (struct iaddr *)0);
	status = interface_reference (fp, fallback_interface, file, line);
	return status == ISC_R_SUCCESS;
}

void reinitialize_interfaces ()
{
	struct interface_info *ip;

	for (ip = interfaces; ip; ip = ip -> next) {
		if_reinitialize_receive (ip);
		if_reinitialize_send (ip);
	}

	if (fallback_interface)
		if_reinitialize_send (fallback_interface);

	interfaces_invalidated = 1;
}

isc_result_t got_one (h)
	omapi_object_t *h;
{
	struct sockaddr_in from;
	struct hardware hfrom;
	struct iaddr ifrom;
	int result;
	union {
		unsigned char packbuf [4095]; /* Packet input buffer.
					 	 Must be as large as largest
						 possible MTU. */
		struct dhcp_packet packet;
	} u;
	struct interface_info *ip;

	if (h -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	ip = (struct interface_info *)h;

	if ((result =
	     receive_packet (ip, u.packbuf, sizeof u, &from, &hfrom)) < 0) {
		log_error ("receive_packet failed on %s: %m", ip -> name);
		return ISC_R_UNEXPECTED;
	}
	if (result == 0)
		return ISC_R_UNEXPECTED;

	/* If we didn't at least get the fixed portion of the BOOTP
	   packet, drop the packet.  We're allowing packets with no
	   sname or filename, because we're aware of at least one
	   client that sends such packets, but this definitely falls
	   into the category of being forgiving. */
	if (result < DHCP_FIXED_NON_UDP - DHCP_SNAME_LEN - DHCP_FILE_LEN)
		return ISC_R_UNEXPECTED;

	if (bootp_packet_handler) {
		ifrom.len = 4;
		memcpy (ifrom.iabuf, &from.sin_addr, ifrom.len);

		(*bootp_packet_handler) (ip, &u.packet, (unsigned)result,
					 from.sin_port, ifrom, &hfrom);
	}
	return ISC_R_SUCCESS;
}

isc_result_t interface_set_value (omapi_object_t *h,
				  omapi_object_t *id,
				  omapi_data_string_t *name,
				  omapi_typed_data_t *value)
{
	if (h -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> set_value)
		return (*(h -> inner -> type -> set_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t interface_get_value (omapi_object_t *h,
				  omapi_object_t *id,
				  omapi_data_string_t *name,
				  omapi_value_t **value)
{
	if (h -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> get_value)
		return (*(h -> inner -> type -> get_value))
			(h -> inner, id, name, value);
	return ISC_R_NOTFOUND;
}

isc_result_t interface_stuff_values (omapi_object_t *c,
				     omapi_object_t *id,
				     omapi_object_t *m)
{
	if (m -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	
	if (m -> inner && m -> inner -> type -> stuff_values)
		return (*(m -> inner -> type -> stuff_values)) (c, id,
								m -> inner);
	return ISC_R_NOTFOUND;
}

isc_result_t interface_destroy (omapi_object_t *h, const char *file, int line)
{
	int i;

	struct interface_info *p;
	if (h -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	/* Nothing to do yet, AFAIK - interfaces should never be
	   destroyed.   Revisit this later when we handle interface
	   detection/deletion on the fly. */
	return ISC_R_SUCCESS;
}

isc_result_t interface_signal_handler (omapi_object_t *h,
				       const char *name, va_list ap)
{
	if (h -> type != dhcp_type_interface)
		return ISC_R_INVALIDARG;
	
	if (h -> inner && h -> inner -> type -> signal_handler)
		return (*(h -> inner -> type -> signal_handler)) (h -> inner,
								  name, ap);
	return ISC_R_NOTFOUND;
}

