/* options.c

   DHCP options parsing and reassembly. */

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
"$Id: options.c,v 1.4 2000/06/24 06:50:02 mellon Exp $ Copyright (c) 1995-2000 The Internet Software Consortium.  All rights reserved.\n";
#endif /* not lint */

#define DHCP_OPTION_DATA
#include "dhcpd.h"
#include <omapip/omapip_p.h>

static void do_option_set PROTO ((pair *,
				  struct option_cache *,
				  enum statement_op));

/* Parse all available options out of the specified packet. */

int parse_options (packet)
	struct packet *packet;
{
	int i;
	struct option_cache *op = (struct option_cache *)0;

	/* Allocate a new option state. */
	if (!option_state_allocate (&packet -> options, MDL)) {
		packet -> options_valid = 0;
		return 0;
	}

	/* If we don't see the magic cookie, there's nothing to parse. */
	if (memcmp (packet -> raw -> options, DHCP_OPTIONS_COOKIE, 4)) {
		packet -> options_valid = 0;
		return 1;
	}

	/* Go through the options field, up to the end of the packet
	   or the End field. */
	if (!parse_option_buffer (packet, &packet -> raw -> options [4],
				  (packet -> packet_length -
				   DHCP_FIXED_NON_UDP - 4)))
		return 0;

	/* If we parsed a DHCP Option Overload option, parse more
	   options out of the buffer(s) containing them. */
	if (packet -> options_valid &&
	    (op = lookup_option (&dhcp_universe, packet -> options,
				 DHO_DHCP_OPTION_OVERLOAD))) {
		if (op -> data.data [0] & 1) {
			if (!parse_option_buffer
			    (packet, (unsigned char *)packet -> raw -> file,
			     sizeof packet -> raw -> file))
				return 0;
		}
		if (op -> data.data [0] & 2) {
			if (!parse_option_buffer
			    (packet,
			     (unsigned char *)packet -> raw -> sname,
			     sizeof packet -> raw -> sname))
				return 0;
		}
	}
	return 1;
}

/* Parse options out of the specified buffer, storing addresses of option
   values in packet -> options and setting packet -> options_valid if no
   errors are encountered. */

int parse_option_buffer (packet, buffer, length)
	struct packet *packet;
	unsigned char *buffer;
	unsigned length;
{
	unsigned char *t;
	unsigned char *end = buffer + length;
	int len, offset;
	int code;
	struct option_cache *op = (struct option_cache *)0;
	struct buffer *bp = (struct buffer *)0;

	if (!buffer_allocate (&bp, length, MDL)) {
		log_error ("no memory for option buffer.");
		return 0;
	}
	memcpy (bp -> data, buffer, length);
	
	for (offset = 0; buffer [offset] != DHO_END && offset < length; ) {
		code = buffer [offset];
		/* Pad options don't have a length - just skip them. */
		if (code == DHO_PAD) {
			++offset;
			continue;
		}

		/* All other fields (except end, see above) have a
		   one-byte length. */
		len = buffer [offset + 1];

		/* If the length is outrageous, the options are bad. */
		if (offset + len + 2 > length) {
			log_error ("Client option %s (%d) larger than buffer.",
				   dhcp_options [code].name, len);
			buffer_dereference (&bp, MDL);
			return 0;
		}

		/* If this is a Relay Agent Information option, we must
		   handle it specially. */
		if (code == DHO_DHCP_AGENT_OPTIONS) {
			if (!parse_agent_information_option
			    (packet, len, buffer + offset + 2)) {
				log_error ("bad agent information option.");
				buffer_dereference (&bp, MDL);
				return 0;
			}
		} else {
			if (!option_cache_allocate (&op, MDL)) {
				log_error ("No memory for option %s.",
					   dhcp_options [code].name);
				buffer_dereference (&bp, MDL);
				return 0;
			}

			/* Reference buffer copy to option cache. */
			op -> data.buffer = (struct buffer *)0;
			buffer_reference (&op -> data.buffer, bp, MDL);

			/* Point option cache into buffer. */
			op -> data.data = &bp -> data [offset + 2];
			op -> data.len = len;
			
			/* NUL terminate (we can get away with this
			   because we allocated one more than the
			   buffer size, and because the byte following
			   the end of an option is always the code of
			   the next option, which we're getting out of
			   the *original* buffer. */
			bp -> data [offset + 2 + len] = 0;
			op -> data.terminated = 1;

			op -> option = &dhcp_options [code];
			/* Now store the option. */
			save_option (&dhcp_universe, packet -> options, op);

			/* And let go of our reference. */
			option_cache_dereference (&op, MDL);
		}
		offset += len + 2;
	}
	packet -> options_valid = 1;
	buffer_dereference (&bp, MDL);
	return 1;
}

/* cons options into a big buffer, and then split them out into the
   three seperate buffers if needed.  This allows us to cons up a set
   of vendor options using the same routine. */

int cons_options (inpacket, outpacket, lease, mms, in_options, cfg_options,
		  scope, overload, terminate, bootpp, prl)
	struct packet *inpacket;
	struct dhcp_packet *outpacket;
	struct lease *lease;
	int mms;
	struct option_state *in_options;
	struct option_state *cfg_options;
	struct binding_scope *scope;
	int overload;	/* Overload flags that may be set. */
	int terminate;
	int bootpp;
	struct data_string *prl;
{
#define PRIORITY_COUNT 300
	unsigned priority_list [PRIORITY_COUNT];
	int priority_len;
	unsigned char buffer [4096];	/* Really big buffer... */
	unsigned main_buffer_size;
	unsigned mainbufix, bufix, agentix;
	unsigned option_size;
	unsigned length;
	int i;
	struct option_cache *op;
	struct data_string ds;
	pair pp, *hash;

	memset (&ds, 0, sizeof ds);

	/* If there's a Maximum Message Size option in the incoming packet
	   and no alternate maximum message size has been specified, take the
	   one in the packet. */

	if (!mms && inpacket &&
	    (op = lookup_option (&dhcp_universe, inpacket -> options,
				 DHO_DHCP_MAX_MESSAGE_SIZE))) {
		evaluate_option_cache (&ds, inpacket, lease, in_options,
				       cfg_options, scope, op, MDL);
		if (ds.len >= sizeof (u_int16_t))
			mms = getUShort (ds.data);
		data_string_forget (&ds, MDL);
	}

	/* If the client has provided a maximum DHCP message size,
	   use that; otherwise, if it's BOOTP, only 64 bytes; otherwise
	   use up to the minimum IP MTU size (576 bytes). */
	/* XXX if a BOOTP client specifies a max message size, we will
	   honor it. */

	if (mms) {
		main_buffer_size = mms - DHCP_FIXED_LEN;

		/* Enforce a minimum packet size... */
		if (main_buffer_size < (576 - DHCP_FIXED_LEN))
			main_buffer_size = 576 - DHCP_FIXED_LEN;
	} else if (bootpp) {
		if (inpacket) {
			main_buffer_size =
				inpacket -> packet_length - DHCP_FIXED_LEN;
			if (main_buffer_size < 64)
				main_buffer_size = 64;
		} else
			main_buffer_size = 64;
	} else
		main_buffer_size = 576 - DHCP_FIXED_LEN;

	/* Set a hard limit at the size of the output buffer. */
	if (main_buffer_size > sizeof buffer)
		main_buffer_size = sizeof buffer;

	/* Preload the option priority list with mandatory options. */
	priority_len = 0;
	priority_list [priority_len++] = DHO_DHCP_MESSAGE_TYPE;
	priority_list [priority_len++] = DHO_DHCP_SERVER_IDENTIFIER;
	priority_list [priority_len++] = DHO_DHCP_LEASE_TIME;
	priority_list [priority_len++] = DHO_DHCP_MESSAGE;
	priority_list [priority_len++] = DHO_DHCP_REQUESTED_ADDRESS;

	if (prl && prl -> len > 0) {
		data_string_truncate (prl, (PRIORITY_COUNT - priority_len));

		for (i = 0; i < prl -> len; i++)
			priority_list [priority_len++] = prl -> data [i];
	} else {
		/* First, hardcode some more options that ought to be
		   sent first... */
		priority_list [priority_len++] = DHO_SUBNET_MASK;
		priority_list [priority_len++] = DHO_ROUTERS;
		priority_list [priority_len++] = DHO_DOMAIN_NAME_SERVERS;
		priority_list [priority_len++] = DHO_HOST_NAME;

		/* Append a list of the standard DHCP options from the
		   standard DHCP option space.  Actually, if a site
		   option space hasn't been specified, we wind up
		   treating the dhcp option space as the site option
		   space, and the first for loop is skipped, because
		   it's slightly more general to do it this way,
		   taking the 1Q99 DHCP futures work into account. */
		if (cfg_options -> site_code_min) {
		    for (i = 0; i < OPTION_HASH_SIZE; i++) {
			hash = cfg_options -> universes [dhcp_universe.index];
			for (pp = hash [i]; pp; pp = pp -> cdr) {
				op = (struct option_cache *)(pp -> car);
				if (op -> option -> code <
				    cfg_options -> site_code_min &&
				    priority_len < PRIORITY_COUNT)
					priority_list [priority_len++] =
						op -> option -> code;
			}
		    }
		}

		/* Now cycle through the site option space, or if there
		   is no site option space, we'll be cycling through the
		   dhcp option space. */
		for (i = 0; i < OPTION_HASH_SIZE; i++) {
			hash = (cfg_options -> universes
				[cfg_options -> site_universe]);
			for (pp = hash [i]; pp; pp = pp -> cdr) {
				op = (struct option_cache *)(pp -> car);
				if (op -> option -> code >=
				    cfg_options -> site_code_min &&
				    priority_len < PRIORITY_COUNT)
					priority_list [priority_len++] =
						op -> option -> code;
			}
		    }
	}

	/* Copy the options into the big buffer... */
	option_size = store_options (buffer,
				     (main_buffer_size - 7 +
				      ((overload & 1) ? DHCP_FILE_LEN : 0) +
				      ((overload & 2) ? DHCP_SNAME_LEN : 0)),
				     inpacket,
				     lease,
				     in_options, cfg_options, scope,
				     priority_list, priority_len,
				     main_buffer_size,
				     (main_buffer_size +
				      ((overload & 1) ? DHCP_FILE_LEN : 0)),
				     terminate);

	/* Put the cookie up front... */
	memcpy (outpacket -> options, DHCP_OPTIONS_COOKIE, 4);
	mainbufix = 4;

	/* If we're going to have to overload, store the overload
	   option at the beginning.  If we can, though, just store the
	   whole thing in the packet's option buffer and leave it at
	   that. */
	if (option_size <= main_buffer_size - mainbufix) {
		memcpy (&outpacket -> options [mainbufix],
			buffer, option_size);
		mainbufix += option_size;
		if (mainbufix < main_buffer_size) {
			agentix = mainbufix;
			outpacket -> options [mainbufix++] = DHO_END;
		} else
			agentix = mainbufix;
		length = DHCP_FIXED_NON_UDP + mainbufix;
	} else {
		outpacket -> options [mainbufix++] = DHO_DHCP_OPTION_OVERLOAD;
		outpacket -> options [mainbufix++] = 1;
		if (option_size > main_buffer_size - mainbufix + DHCP_FILE_LEN)
			outpacket -> options [mainbufix++] = 3;
		else
			outpacket -> options [mainbufix++] = 1;

		memcpy (&outpacket -> options [mainbufix],
			buffer, main_buffer_size - mainbufix);
		length = DHCP_FIXED_NON_UDP + main_buffer_size;
		agentix = main_buffer_size;

		bufix = main_buffer_size - mainbufix;
		if (overload & 1) {
			if (option_size - bufix <= DHCP_FILE_LEN) {
				memcpy (outpacket -> file,
					&buffer [bufix], option_size - bufix);
				mainbufix = option_size - bufix;
				if (mainbufix < DHCP_FILE_LEN)
					outpacket -> file [mainbufix++]
						= DHO_END;
				while (mainbufix < DHCP_FILE_LEN)
					outpacket -> file [mainbufix++]
						= DHO_PAD;
			} else {
				memcpy (outpacket -> file,
					&buffer [bufix], DHCP_FILE_LEN);
				bufix += DHCP_FILE_LEN;
			}
		}
		if ((overload & 2) && option_size < bufix) {
			memcpy (outpacket -> sname,
				&buffer [bufix], option_size - bufix);

			mainbufix = option_size - bufix;
			if (mainbufix < DHCP_SNAME_LEN)
				outpacket -> file [mainbufix++]
					= DHO_END;
			while (mainbufix < DHCP_SNAME_LEN)
				outpacket -> file [mainbufix++]
					= DHO_PAD;
		}
	}

	length = cons_agent_information_options (cfg_options,
						 outpacket, agentix, length);
		
	return length;
}

/* Store all the requested options into the requested buffer. */

int store_options (buffer, buflen, packet, lease,
		   in_options, cfg_options, scope, priority_list,
		   priority_len, first_cutoff, second_cutoff, terminate)
	unsigned char *buffer;
	unsigned buflen;
	struct packet *packet;
	struct lease *lease;
	struct option_state *in_options;
	struct option_state *cfg_options;
	struct binding_scope *scope;
	unsigned *priority_list;
	int priority_len;
	unsigned first_cutoff, second_cutoff;
	int terminate;
{
	int bufix = 0;
	int i;
	int ix;
	int tto;
	struct data_string od;
	struct option_cache *oc;

	memset (&od, 0, sizeof od);

	/* Eliminate duplicate options in the parameter request list.
	   There's got to be some clever knuthian way to do this:
	   Eliminate all but the first occurance of a value in an array
	   of values without otherwise disturbing the order of the array. */
	for (i = 0; i < priority_len - 1; i++) {
		tto = 0;
		for (ix = i + 1; ix < priority_len + tto; ix++) {
			if (tto)
				priority_list [ix - tto] =
					priority_list [ix];
			if (priority_list [i] == priority_list [ix]) {
				tto++;
				priority_len--;
			}
		}
	}

	/* Copy out the options in the order that they appear in the
	   priority list... */
	for (i = 0; i < priority_len; i++) {
		/* Code for next option to try to store. */
		unsigned code = priority_list [i];
		int optstart;

		/* Number of bytes left to store (some may already
		   have been stored by a previous pass). */
		int length;

		/* Look up the option in the site option space if the code
		   is above the cutoff, otherwise in the DHCP option space. */
		if (code >= cfg_options -> site_code_min)
			oc = lookup_option
				(universes [cfg_options -> site_universe],
				 cfg_options, code);
		else
			oc = lookup_option (&dhcp_universe, cfg_options, code);

		/* If no data is available for this option, skip it. */
		if (!oc) {
			continue;
		}

		/* Find the value of the option... */
		evaluate_option_cache (&od, packet, lease, in_options,
				       cfg_options, scope, oc, MDL);
		if (!od.len) {
			continue;
		}

		/* We should now have a constant length for the option. */
		length = od.len;

		/* Do we add a NUL? */
		if (terminate && dhcp_options [code].format [0] == 't') {
			length++;
			tto = 1;
		} else {
			tto = 0;
		}

		/* Try to store the option. */

		/* If the option's length is more than 255, we must store it
		   in multiple hunks.   Store 255-byte hunks first.  However,
		   in any case, if the option data will cross a buffer
		   boundary, split it across that boundary. */

		ix = 0;

		optstart = bufix;
		while (length) {
			unsigned char incr = length > 255 ? 255 : length;

			/* If this hunk of the buffer will cross a
			   boundary, only go up to the boundary in this
			   pass. */
			if (bufix < first_cutoff &&
			    bufix + incr > first_cutoff)
				incr = first_cutoff - bufix;
			else if (bufix < second_cutoff &&
				 bufix + incr > second_cutoff)
				incr = second_cutoff - bufix;

			/* If this option is going to overflow the buffer,
			   skip it. */
			if (bufix + 2 + incr > buflen) {
				bufix = optstart;
				break;
			}

			/* Everything looks good - copy it in! */
			buffer [bufix] = code;
			buffer [bufix + 1] = incr;
			if (tto && incr == length) {
				memcpy (buffer + bufix + 2,
					od.data + ix, (unsigned)(incr - 1));
				buffer [bufix + 2 + incr - 1] = 0;
			} else {
				memcpy (buffer + bufix + 2,
					od.data + ix, (unsigned)incr);
			}
			length -= incr;
			ix += incr;
			bufix += 2 + incr;
		}
		data_string_forget (&od, MDL);
	}
	return bufix;
}

/* Format the specified option so that a human can easily read it. */

const char *pretty_print_option (code, data, len, emit_commas, emit_quotes)
	unsigned int code;
	const unsigned char *data;
	unsigned len;
	int emit_commas;
	int emit_quotes;
{
	static char optbuf [32768]; /* XXX */
	int hunksize = 0;
	int numhunk = -1;
	int numelem = 0;
	char fmtbuf [32];
	int i, j, k;
	char *op = optbuf;
	const unsigned char *dp = data;
	struct in_addr foo;
	char comma;

	/* Code should be between 0 and 255. */
	if (code > 255)
		log_fatal ("pretty_print_option: bad code %d\n", code);

	if (emit_commas)
		comma = ',';
	else
		comma = ' ';
	
	/* Figure out the size of the data. */
	for (i = 0; dhcp_options [code].format [i]; i++) {
		if (!numhunk) {
			log_error ("%s: Extra codes in format string: %s\n",
				   dhcp_options [code].name,
				   &(dhcp_options [code].format [i]));
			break;
		}
		numelem++;
		fmtbuf [i] = dhcp_options [code].format [i];
		switch (dhcp_options [code].format [i]) {
		      case 'A':
			--numelem;
			fmtbuf [i] = 0;
			numhunk = 0;
			break;
		      case 'X':
			for (k = 0; k < len; k++) {
				if (!isascii (data [k]) ||
				    !isprint (data [k]))
					break;
			}
			/* If we found no bogus characters, or the bogus
			   character we found is a trailing NUL, it's
			   okay to print this option as text. */
			if (k == len || (k + 1 == len && data [k] == 0)) {
				fmtbuf [i] = 't';
				numhunk = -2;
			} else {
				fmtbuf [i] = 'x';
				hunksize++;
				comma = ':';
				numhunk = 0;
			}
			fmtbuf [i + 1] = 0;
			break;
		      case 't':
			fmtbuf [i] = 't';
			fmtbuf [i + 1] = 0;
			numhunk = -2;
			break;
		      case 'I':
		      case 'l':
		      case 'L':
			hunksize += 4;
			break;
		      case 's':
		      case 'S':
			hunksize += 2;
			break;
		      case 'b':
		      case 'B':
		      case 'f':
			hunksize++;
			break;
		      case 'e':
			break;
		      default:
			log_error ("%s: garbage in format string: %s\n",
			      dhcp_options [code].name,
			      &(dhcp_options [code].format [i]));
			break;
		} 
	}

	/* Check for too few bytes... */
	if (hunksize > len) {
		log_error ("%s: expecting at least %d bytes; got %d",
		      dhcp_options [code].name,
		      hunksize, len);
		return "<error>";
	}
	/* Check for too many bytes... */
	if (numhunk == -1 && hunksize < len)
		log_error ("%s: %d extra bytes",
		      dhcp_options [code].name,
		      len - hunksize);

	/* If this is an array, compute its size. */
	if (!numhunk)
		numhunk = len / hunksize;
	/* See if we got an exact number of hunks. */
	if (numhunk > 0 && numhunk * hunksize < len)
		log_error ("%s: %d extra bytes at end of array\n",
		      dhcp_options [code].name,
		      len - numhunk * hunksize);

	/* A one-hunk array prints the same as a single hunk. */
	if (numhunk < 0)
		numhunk = 1;

	/* Cycle through the array (or hunk) printing the data. */
	for (i = 0; i < numhunk; i++) {
		for (j = 0; j < numelem; j++) {
			switch (fmtbuf [j]) {
			      case 't':
				if (emit_quotes)
					*op++ = '"';
<<<<<<< options.c
				if (len != 0) {
					strcpy (op, (const char *)dp);
					op += strlen ((const char *)dp);
				}
=======
				for (k = 0; dp [k]; k++) {
					if (!isascii (dp [k]) ||
					    !isprint (dp [k])) {
						sprintf (op, "\\%03o",
							 dp [k]);
						op += 4;
					} else if (dp [k] == '"' ||
						   dp [k] == '\'' ||
						   dp [k] == '$' ||
						   dp [k] == '`' ||
						   dp [k] == '\\') {
						*op++ = '\\';
						*op++ = dp [k];
					} else
						*op++ = dp [k];
				}
>>>>>>> 1.1.1.12
				if (emit_quotes)
					*op++ = '"';
				*op = 0;
				break;
			      case 'I':
				foo.s_addr = htonl (getULong (dp));
				strcpy (op, inet_ntoa (foo));
				dp += 4;
				break;
			      case 'l':
				sprintf (op, "%ld", (long)getLong (dp));
				dp += 4;
				break;
			      case 'L':
				sprintf (op, "%ld",
					 (unsigned long)getULong (dp));
				dp += 4;
				break;
			      case 's':
				sprintf (op, "%d", (int)getShort (dp));
				dp += 2;
				break;
			      case 'S':
				sprintf (op, "%d", (unsigned)getUShort (dp));
				dp += 2;
				break;
			      case 'b':
				sprintf (op, "%d", *(const char *)dp++);
				break;
			      case 'B':
				sprintf (op, "%d", *dp++);
				break;
			      case 'x':
				sprintf (op, "%x", *dp++);
				break;
			      case 'f':
				strcpy (op, *dp++ ? "true" : "false");
				break;
			      default:
				log_error ("Unexpected format code %c",
					   fmtbuf [j]);
			}
			op += strlen (op);
			if (j + 1 < numelem && comma != ':')
				*op++ = ' ';
		}
		if (i + 1 < numhunk) {
			*op++ = comma;
		}
		
	}
	return optbuf;
}

int hashed_option_get (result, universe, packet, lease,
		       in_options, cfg_options, options, scope, code)
	struct data_string *result;
	struct universe *universe;
	struct packet *packet;
	struct lease *lease;
	struct option_state *in_options;
	struct option_state *cfg_options;
	struct option_state *options;
	struct binding_scope *scope;
	unsigned code;
{
	struct option_cache *oc;

	if (!universe -> lookup_func)
		return 0;
	oc = ((*universe -> lookup_func) (universe, options, code));
	if (!oc)
		return 0;
	if (!evaluate_option_cache (result, packet, lease, in_options,
				    cfg_options, scope, oc, MDL))
		return 0;
	return 1;
}

int agent_option_get (result, universe, packet, lease,
		      in_options, cfg_options, options, scope, code)
	struct data_string *result;
	struct universe *universe;
	struct packet *packet;
	struct lease *lease;
	struct option_state *in_options;
	struct option_state *cfg_options;
	struct option_state *options;
	struct binding_scope *scope;
	unsigned code;
{
	struct agent_options *ao;
	struct option_tag *t;

	/* Make sure there's agent option state. */
	if (universe -> index >= options -> universe_count ||
	    !(options -> universes [universe -> index]))
		return 0;
	ao = (struct agent_options *)options -> universes [universe -> index];

	/* Find the last set of agent options and consider it definitive. */
	for (; ao -> next; ao = ao -> next)
		;
	if (ao) {
		for (t = ao -> first; t; t = t -> next) {
			if (t -> data [0] == code) {
				result -> len = t -> data [1];
				if (!(buffer_allocate (&result -> buffer,
						       result -> len + 1,
						       MDL))) {
					result -> len = 0;
					buffer_dereference
						(&result -> buffer, MDL);
					return 0;
				}
				result -> data = &result -> buffer -> data [0];
				memcpy (result -> buffer -> data,
					&t -> data [2], result -> len);
				result -> buffer -> data [result -> len] = 0;
				result -> terminated = 1;
				return 1;
			}
		}
	}
	return 0;
}

void hashed_option_set (universe, options, option, op)
	struct universe *universe;
	struct option_state *options;
	struct option_cache *option;
	enum statement_op op;
{
	struct option_cache *oc, *noc;

	switch (op) {
	      case if_statement:
	      case add_statement:
	      case eval_statement:
	      case break_statement:
	      default:
		log_error ("bogus statement type in do_option_set.");
		break;

	      case default_option_statement:
		oc = lookup_option (universe, options,
				    option -> option -> code);
		if (oc)
			break;
		save_option (universe, options, option);
		break;

	      case supersede_option_statement:
		/* Install the option, replacing any existing version. */
		save_option (universe, options, option);
		break;

	      case append_option_statement:
	      case prepend_option_statement:
		oc = lookup_option (universe, options,
				    option -> option -> code);
		if (!oc) {
			save_option (universe, options, option);
			break;
		}
		/* If it's not an expression, make it into one. */
		if (!oc -> expression && oc -> data.len) {
			if (!expression_allocate (&oc -> expression, MDL)) {
				log_error ("Can't allocate const expression.");
				break;
			}
			oc -> expression -> op = expr_const_data;
			data_string_copy
				(&oc -> expression -> data.const_data,
				 &oc -> data, MDL);
			data_string_forget (&oc -> data, MDL);
		}
		noc = (struct option_cache *)0;
		if (!option_cache_allocate (&noc, MDL))
			break;
		if (op == append_option_statement) {
			if (!make_concat (&noc -> expression,
					  oc -> expression,
					  option -> expression)) {
				option_cache_dereference (&noc, MDL);
				break;
			}
		} else {
			if (!make_concat (&noc -> expression,
					  option -> expression,
					  oc -> expression)) {
				option_cache_dereference (&noc, MDL);
				break;
			}
		}
		noc -> option = oc -> option;
		save_option (universe, options, noc);
		option_cache_dereference (&noc, MDL);
		break;
	}
}

struct option_cache *lookup_option (universe, options, code)
	struct universe *universe;
	struct option_state *options;
	unsigned code;
{
	if (universe -> lookup_func)
		return (*universe -> lookup_func) (universe, options, code);
	else
		log_error ("can't look up options in %s space.",
			   universe -> name);
	return (struct option_cache *)0;
}

struct option_cache *lookup_hashed_option (universe, options, code)
	struct universe *universe;
	struct option_state *options;
	unsigned code;
{
	int hashix;
	pair bptr;
	pair *hash;

	/* Make sure there's a hash table. */
	if (universe -> index >= options -> universe_count ||
	    !(options -> universes [universe -> index]))
		return (struct option_cache *)0;

	hash = options -> universes [universe -> index];

	hashix = compute_option_hash (code);
	for (bptr = hash [hashix]; bptr; bptr = bptr -> cdr) {
		if (((struct option_cache *)(bptr -> car)) -> option -> code ==
		    code)
			return (struct option_cache *)(bptr -> car);
	}
	return (struct option_cache *)0;
}

void save_option (universe, options, oc)
	struct universe *universe;
	struct option_state *options;
	struct option_cache *oc;
{
	if (universe -> save_func)
		(*universe -> save_func) (universe, options, oc);
	else
		log_error ("can't store options in %s space.",
			   universe -> name);
}

void save_hashed_option (universe, options, oc)
	struct universe *universe;
	struct option_state *options;
	struct option_cache *oc;
{
	int hashix;
	pair bptr;
	pair *hash = options -> universes [universe -> index];

	/* Compute the hash. */
	hashix = compute_option_hash (oc -> option -> code);

	/* If there's no hash table, make one. */
	if (!hash) {
		hash = (pair *)dmalloc (OPTION_HASH_SIZE * sizeof *hash, MDL);
		if (!hash) {
			log_error ("no memory to store %s.%s",
				   universe -> name, oc -> option -> name);
			return;
		}
		memset (hash, 0, OPTION_HASH_SIZE * sizeof *hash);
		options -> universes [universe -> index] = (VOIDPTR)hash;
	} else {
		/* Try to find an existing option matching the new one. */
		for (bptr = hash [hashix]; bptr; bptr = bptr -> cdr) {
			if (((struct option_cache *)
			     (bptr -> car)) -> option -> code ==
			    oc -> option -> code)
				break;
		}

		/* If we find one, dereference it and put the new one
		   in its place. */
		if (bptr) {
			option_cache_dereference
				((struct option_cache **)&bptr -> car, MDL);
			option_cache_reference
				((struct option_cache **)&bptr -> car,
				 oc, MDL);
			return;
		}
	}

	/* Otherwise, just put the new one at the head of the list. */
	bptr = new_pair (MDL);
	if (!bptr) {
		log_error ("No memory for option_cache reference.");
		return;
	}
	bptr -> cdr = hash [hashix];
	bptr -> car = 0;
	option_cache_reference ((struct option_cache **)&bptr -> car, oc, MDL);
	hash [hashix] = bptr;
}

void delete_option (universe, options, code)
	struct universe *universe;
	struct option_state *options;
	int code;
{
	if (universe -> delete_func)
		(*universe -> delete_func) (universe, options, code);
	else
		log_error ("can't delete options from %s space.",
			   universe -> name);
}

void delete_hashed_option (universe, options, code)
	struct universe *universe;
	struct option_state *options;
	int code;
{
	int hashix;
	pair bptr, prev = (pair)0;
	pair *hash = options -> universes [universe -> index];

	/* There may not be any options in this space. */
	if (!hash)
		return;

	/* Try to find an existing option matching the new one. */
	hashix = compute_option_hash (code);
	for (bptr = hash [hashix]; bptr; bptr = bptr -> cdr) {
		if (((struct option_cache *)(bptr -> car)) -> option -> code
		    == code)
			break;
		prev = bptr;
	}
	/* If we found one, wipe it out... */
	if (bptr) {
		if (prev)
			prev -> cdr = bptr -> cdr;
		else
			hash [hashix] = bptr -> cdr;
		option_cache_dereference
			((struct option_cache **)(&bptr -> car), MDL);
		free_pair (bptr, MDL);
	}
}

extern struct option_cache *free_option_caches; /* XXX */

int option_cache_dereference (ptr, file, line)
	struct option_cache **ptr;
	const char *file;
	int line;
{
	if (!ptr || !*ptr) {
		log_error ("Null pointer in option_cache_dereference: %s(%d)",
			   file, line);
#if defined (POINTER_DEBUG)
		abort ();
#else
		return 0;
#endif
	}

	(*ptr) -> refcnt--;
	rc_register (file, line, ptr, *ptr, (*ptr) -> refcnt);
	if (!(*ptr) -> refcnt) {
		if ((*ptr) -> data.buffer)
			data_string_forget (&(*ptr) -> data, file, line);
		if ((*ptr) -> expression)
			expression_dereference (&(*ptr) -> expression,
						file, line);
		/* Put it back on the free list... */
		(*ptr) -> expression = (struct expression *)free_option_caches;
		free_option_caches = *ptr;
		dmalloc_reuse (free_option_caches, (char *)0, 0, 0);
	}
	if ((*ptr) -> refcnt < 0) {
		log_error ("%s(%d): negative refcnt!", file, line);
#if defined (DEBUG_RC_HISTORY)
		dump_rc_history ();
#endif
#if defined (POINTER_DEBUG)
		abort ();
#else
		*ptr = (struct option_cache *)0;
		return 0;
#endif
	}
	*ptr = (struct option_cache *)0;
	return 1;

}

int hashed_option_state_dereference (universe, state, file, line)
	struct universe *universe;
	struct option_state *state;
	const char *file;
	int line;
{
	pair *heads;
	pair cp, next;
	int i;

	/* Get the pointer to the array of hash table bucket heads. */
	heads = (pair *)(state -> universes [universe -> index]);
	if (!heads)
		return 0;

	/* For each non-null head, loop through all the buckets dereferencing
	   the attached option cache structures and freeing the buckets. */
	for (i = 0; i < OPTION_HASH_SIZE; i++) {
		for (cp = heads [i]; cp; cp = next) {
			next = cp -> cdr;
			option_cache_dereference
				((struct option_cache **)&cp -> car, file, line);
			free_pair (cp, file, line);
		}
	}

	dfree (heads, file, line);
	state -> universes [universe -> index] = (void *)0;
	return 1;
}

int agent_option_state_dereference (universe, state, file, line)
	struct universe *universe;
	struct option_state *state;
	const char *file;
	int line;
{
	struct agent_options *a, *na;
	struct option_tag *ot, *not;

	if (universe -> index >= state -> universe_count ||
	    !state -> universes [universe -> index])
		return 0;

	/* We can also release the agent options, if any... */
	for (a = (struct agent_options *)(state -> universes
					  [universe -> index]); a; a = na) {
		na = a -> next;
		for (ot = a -> first; ot; ot = not) {
			not = ot -> next;
			dfree (ot, file, line);
		}
	}

	dfree (state -> universes [universe -> index], file, line);
	state -> universes [universe -> index] = (void *)0;
	return 1;
}

int store_option (result, universe, packet, lease,
		  in_options, cfg_options, scope, oc)
	struct data_string *result;
	struct universe *universe;
	struct packet *packet;
	struct lease *lease;
	struct option_state *in_options;
	struct option_state *cfg_options;
	struct binding_scope *scope;
	struct option_cache *oc;
{
	struct data_string d1, d2;

	memset (&d1, 0, sizeof d1);
	memset (&d2, 0, sizeof d2);

	if (evaluate_option_cache (&d2, packet, lease, in_options,
				   cfg_options, scope, oc, MDL)) {
		if (!buffer_allocate (&d1.buffer,
				      (result -> len +
				       universe -> length_size +
				       universe -> tag_size + d2.len), MDL)) {
			data_string_forget (result, MDL);
			data_string_forget (&d2, MDL);
			return 0;
		}
		d1.data = &d1.buffer -> data [0];
		if (result -> len)
			memcpy (d1.buffer -> data,
				result -> data, result -> len);
		d1.len = result -> len;
		(*universe -> store_tag) (&d1.buffer -> data [d1.len],
					  oc -> option -> code);
		d1.len += universe -> tag_size;
		(*universe -> store_length) (&d1.buffer -> data [d1.len],
					     d2.len);
		d1.len += universe -> length_size;
		memcpy (&d1.buffer -> data [d1.len], d2.data, d2.len);
		d1.len += d2.len;
		data_string_forget (&d2, MDL);
		data_string_forget (result, MDL);
		data_string_copy (result, &d1, MDL);
		data_string_forget (&d1, MDL);
		return 1;
	}
	return 0;
}
	
int option_space_encapsulate (result, packet, lease,
			      in_options, cfg_options, scope, name)
	struct data_string *result;
	struct packet *packet;
	struct lease *lease;
	struct option_state *in_options;
	struct option_state *cfg_options;
	struct binding_scope *scope;
	struct data_string *name;
{
	struct universe *u;

	u = (struct universe *)0;
	universe_hash_lookup (&u, universe_hash,
			      (const char *)name -> data, name -> len, MDL);
	if (!u) {
		log_error ("unknown option space %s.", name -> data);
		return 0;
	}

	if (u -> encapsulate)
		return (*u -> encapsulate) (result, packet, lease,
					    in_options, cfg_options, scope, u);
	log_error ("encapsulation requested for %s with no support.",
		   name -> data);
	return 0;
}

int hashed_option_space_encapsulate (result, packet, lease,
				     in_options, cfg_options, scope, universe)
	struct data_string *result;
	struct packet *packet;
	struct lease *lease;
	struct option_state *in_options;
	struct option_state *cfg_options;
	struct binding_scope *scope;
	struct universe *universe;
{
	pair p, *hash;
	int status;
	int i;

	if (universe -> index >= cfg_options -> universe_count)
		return 0;

	hash = cfg_options -> universes [universe -> index];
	if (!hash)
		return 0;

	status = 0;
	for (i = 0; i < OPTION_HASH_SIZE; i++) {
		for (p = hash [i]; p; p = p -> cdr) {
			if (store_option (result, universe, packet, lease,
					  in_options, cfg_options, scope,
					  (struct option_cache *)p -> car))
				status = 1;
		}
	}

	return status;
}

int nwip_option_space_encapsulate (result, packet, lease,
				   in_options, cfg_options, scope, universe)
	struct data_string *result;
	struct packet *packet;
	struct lease *lease;
	struct option_state *in_options;
	struct option_state *cfg_options;
	struct binding_scope *scope;
	struct universe *universe;
{
	pair p, *hash;
	int status;
	int i;
	static struct option_cache *no_nwip;
	struct data_string ds;

	if (universe -> index >= cfg_options -> universe_count)
		return 0;

	hash = cfg_options -> universes [universe -> index];
	status = 0;
	for (i = 0; hash && i < OPTION_HASH_SIZE; i++) {
		for (p = hash [i]; p; p = p -> cdr) {
			if (store_option (result, universe, packet, lease,
					  in_options, cfg_options, scope,
					  (struct option_cache *)p -> car))
				status = 1;
		}
	}

	/* If there's no data, the nwip suboption is supposed to contain
	   a suboption saying there's no data. */
	if (!status) {
		if (!no_nwip) {
			static unsigned char nni [] = { 1, 0 };
			memset (&ds, 0, sizeof ds);
			ds.data = nni;
			ds.len = 2;
			if (option_cache_allocate (&no_nwip, MDL))
				data_string_copy (&no_nwip -> data, &ds, MDL);
			no_nwip -> option = nwip_universe.options [1];
		}
		if (no_nwip) {
			if (store_option (result, universe, packet, lease,
					  in_options, cfg_options,
					  scope, no_nwip))
				status = 1;
		}
	} else {
		/* If we have nwip options, the first one has to be the
		   nwip-exists-in-option-area option. */
		if (!buffer_allocate (&ds.buffer, result -> len + 2, MDL)) {
			data_string_forget (result, MDL);
			return 0;
		}
		ds.data = &ds.buffer -> data [0];
		ds.buffer -> data [0] = 2;
		ds.buffer -> data [1] = 0;
		memcpy (&ds.buffer -> data [2], result -> data, result -> len);
		data_string_forget (result, MDL);
		data_string_copy (result, &ds, MDL);
		data_string_forget (&ds, MDL);
	}

	return status;
}

void do_packet (interface, packet, len, from_port, from, hfrom)
	struct interface_info *interface;
	struct dhcp_packet *packet;
	unsigned len;
	unsigned int from_port;
	struct iaddr from;
	struct hardware *hfrom;
{
	int i;
	struct option_cache *op;
	struct packet *decoded_packet;
#if defined (DEBUG_MEMORY_LEAKAGE)
	unsigned long previous_outstanding = dmalloc_outstanding;
#endif

	decoded_packet = (struct packet *)0;
	if (!packet_allocate (&decoded_packet, MDL)) {
		log_error ("do_packet: no memory for incoming packet!");
		return;
	}
	decoded_packet -> raw = packet;
	decoded_packet -> packet_length = len;
	decoded_packet -> client_port = from_port;
	decoded_packet -> client_addr = from;
	interface_reference (&decoded_packet -> interface, interface, MDL);
	decoded_packet -> haddr = hfrom;
	
	if (packet -> hlen > sizeof packet -> chaddr) {
		packet_dereference (&decoded_packet, MDL);
		log_info ("Discarding packet with bogus hlen.");
		return;
	}

	/* If there's an option buffer, try to parse it. */
	if (decoded_packet -> packet_length >= DHCP_FIXED_NON_UDP + 4) {
		if (!parse_options (decoded_packet)) {
			if (decoded_packet -> options)
				option_state_dereference
					(&decoded_packet -> options, MDL);
			packet_dereference (&decoded_packet, MDL);
			return;
		}

		if (decoded_packet -> options_valid &&
		    (op = lookup_option (&dhcp_universe,
					 decoded_packet -> options, 
					 DHO_DHCP_MESSAGE_TYPE))) {
			struct data_string dp;
			memset (&dp, 0, sizeof dp);
			evaluate_option_cache (&dp, decoded_packet,
					       (struct lease *)0,
					       decoded_packet -> options,
					       (struct option_state *)0,
					       (struct binding_scope *)0,
					       op, MDL);
			if (dp.len > 0)
				decoded_packet -> packet_type = dp.data [0];
			else
				decoded_packet -> packet_type = 0;
			data_string_forget (&dp, MDL);
		}
	}
		
	if (decoded_packet -> packet_type)
		dhcp (decoded_packet);
	else
		bootp (decoded_packet);

	/* If the caller kept the packet, they'll have upped the refcnt. */
	packet_dereference (&decoded_packet, MDL);

#if defined (DEBUG_MEMORY_LEAKAGE)
	log_info ("generation %ld: %ld new, %ld outstanding, %ld long-term",
		  dmalloc_generation,
		  dmalloc_outstanding - previous_outstanding,
		  dmalloc_outstanding, dmalloc_longterm);
#endif
#if defined (DEBUG_MEMORY_LEAKAGE) || defined (DEBUG_MALLOC_POOL)
	dmalloc_dump_outstanding ();
#endif
#if defined (DEBUG_RC_HISTORY_EXHAUSTIVELY)
	dump_rc_history ();
#endif
}

