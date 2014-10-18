/*	$NetBSD: support.c,v 1.2 2014/10/18 08:33:23 snj Exp $	*/

/*
 * Portions Copyright (c) 1995-1998 by Trusted Information Systems, Inc.
 *
 * Permission to use, copy modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND TRUSTED INFORMATION SYSTEMS
 * DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS.  IN NO EVENT SHALL
 * TRUSTED INFORMATION SYSTEMS BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT,
 * NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THE SOFTWARE.
 */
#include <sys/cdefs.h>
#if 0
static const char rcsid[] = "Header: /proj/cvs/prod/libbind/dst/support.c,v 1.6 2005/10/11 00:10:13 marka Exp ";
#else
__RCSID("$NetBSD: support.c,v 1.2 2014/10/18 08:33:23 snj Exp $");
#endif

#include "port_before.h"

#include <stdio.h>
#include <unistd.h>
#include <memory.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>

#include "dst_internal.h"

#include "port_after.h"

/*%
 * dst_s_verify_str()
 *     Validate that the input string(*str) is at the head of the input
 *     buffer(**buf).  If so, move the buffer head pointer (*buf) to
 *     the first byte of data following the string(*str).
 * Parameters
 *     buf     Input buffer.
 *     str     Input string.
 * Return
 *	0       *str is not the head of **buff
 *	1       *str is the head of **buff, *buf is is advanced to
 *	the tail of **buf.
 */

int
dst_s_verify_str(const char **buf, const char *str)
{
	size_t b, s;
	if (*buf == NULL)	/*%< error checks */
		return (0);
	if (str == NULL || *str == '\0')
		return (1);

	b = strlen(*buf);	/*%< get length of strings */
	s = strlen(str);
	if (s > b || strncmp(*buf, str, s))	/*%< check if same */
		return (0);	/*%< not a match */
	(*buf) += s;		/*%< advance pointer */
	return (1);
}

/*%
 * dst_s_calculate_bits
 *     Given a binary number represented in a u_char[], determine
 *     the number of significant bits used.
 * Parameters
 *     str       An input character string containing a binary number.
 *     max_bits The maximum possible significant bits.
 * Return
 *     N       The number of significant bits in str.
 */

int
dst_s_calculate_bits(const u_char *str, const int max_bits)
{
	const u_char *p = str;
	u_char i, j = 0x80;
	int bits;
	for (bits = max_bits; *p == 0x00 && bits > 0; p++)
		bits -= 8;
	for (i = *p; (i & j) != j; j >>= 1)
		bits--;
	return (bits);
}

/*%
 * calculates a checksum used in dst for an id.
 * takes an array of bytes and a length.
 * returns a 16  bit checksum.
 */
u_int16_t
dst_s_id_calc(const u_char *key, const int keysize)
{
	u_int32_t ac;
	const u_char *kp = key;
	int size = keysize;

	if (!key || (keysize <= 0))
		return (0xffffU);
 
	for (ac = 0; size > 1; size -= 2, kp += 2)
		ac += ((*kp) << 8) + *(kp + 1);

	if (size > 0)
		ac += ((*kp) << 8);
	ac += (ac >> 16) & 0xffff;

	return (ac & 0xffff);
}

/*%
 * dst_s_dns_key_id() Function to calculate DNSSEC footprint from KEY record
 *   rdata
 * Input:
 *	dns_key_rdata: the raw data in wire format 
 *      rdata_len: the size of the input data 
 * Output:
 *      the key footprint/id calculated from the key data 
 */ 
u_int16_t
dst_s_dns_key_id(const u_char *dns_key_rdata, const int rdata_len)
{
	if (!dns_key_rdata)
		return 0;

	/* compute id */
	if (dns_key_rdata[3] == KEY_RSA)	/*%< Algorithm RSA */
		return dst_s_get_int16((const u_char *)
				       &dns_key_rdata[rdata_len - 3]);
	else if (dns_key_rdata[3] == KEY_HMAC_MD5)
		/* compatibility */
		return 0;
	else
		/* compute a checksum on the key part of the key rr */
		return dst_s_id_calc(dns_key_rdata, rdata_len);
}

/*%
 * dst_s_get_int16
 *     This routine extracts a 16 bit integer from a two byte character
 *     string.  The character string is assumed to be in network byte
 *     order and may be unaligned.  The number returned is in host order.
 * Parameter
 *     buf     A two byte character string.
 * Return
 *     The converted integer value.
 */

u_int16_t
dst_s_get_int16(const u_char *buf)
{
	register u_int16_t a = 0;
	a = ((u_int16_t)(buf[0] << 8)) | ((u_int16_t)(buf[1]));
	return (a);
}

/*%
 * dst_s_get_int32
 *     This routine extracts a 32 bit integer from a four byte character
 *     string.  The character string is assumed to be in network byte
 *     order and may be unaligned.  The number returned is in host order.
 * Parameter
 *     buf     A four byte character string.
 * Return
 *     The converted integer value.
 */

u_int32_t
dst_s_get_int32(const u_char *buf)
{
	register u_int32_t a = 0;
	a = ((u_int32_t)(buf[0] << 24)) | ((u_int32_t)(buf[1] << 16)) |
		((u_int32_t)(buf[2] << 8)) | ((u_int32_t)(buf[3]));
	return (a);
}

/*%
 * dst_s_put_int16
 *     Take a 16 bit integer and store the value in a two byte
 *     character string.  The integer is assumed to be in network
 *     order and the string is returned in host order.
 *
 * Parameters
 *     buf     Storage for a two byte character string.
 *     val     16 bit integer.
 */

void
dst_s_put_int16(u_int8_t *buf, const u_int16_t val)
{
	buf[0] = (u_int8_t)((uint32_t)val >> 8);
	buf[1] = (u_int8_t)(val);
}

/*%
 * dst_s_put_int32
 *     Take a 32 bit integer and store the value in a four byte
 *     character string.  The integer is assumed to be in network
 *     order and the string is returned in host order.
 *
 * Parameters
 *     buf     Storage for a four byte character string.
 *     val     32 bit integer.
 */

void
dst_s_put_int32(u_int8_t *buf, const u_int32_t val)
{
	buf[0] = (u_int8_t)(val >> 24);
	buf[1] = (u_int8_t)(val >> 16);
	buf[2] = (u_int8_t)(val >> 8);
	buf[3] = (u_int8_t)(val);
}

/*%
 *  dst_s_filename_length
 *
 *	This function returns the number of bytes needed to hold the
 *	filename for a key file.  '/', '\' and ':' are not allowed.
 *	form:  K&lt;keyname&gt;+&lt;alg&gt;+&lt;id&gt;.&lt;suffix&gt;
 *
 *	Returns 0 if the filename would contain either '\', '/' or ':'
 */
size_t
dst_s_filename_length(const char *name, const char *suffix)
{
	if (name == NULL)
		return (0);
	if (strrchr(name, '\\'))
		return (0);
	if (strrchr(name, '/'))
		return (0);
	if (strrchr(name, ':'))
		return (0);
	if (suffix == NULL)
		return (0);
	if (strrchr(suffix, '\\'))
		return (0);
	if (strrchr(suffix, '/'))
		return (0);
	if (strrchr(suffix, ':'))
		return (0);
	return (1 + strlen(name) + 6 + strlen(suffix));
}

/*%
 *  dst_s_build_filename ()
 *	Builds a key filename from the key name, its id, and a
 *	suffix.  '\', '/' and ':' are not allowed. fA filename is of the
 *	form:  K&lt;keyname&gt;&lt;id&gt;.&lt;suffix&gt;
 *	form: K&lt;keyname&gt;+&lt;alg&gt;+&lt;id&gt;.&lt;suffix&gt;
 *
 *	Returns -1 if the conversion fails:
 *	  if the filename would be too long for space allotted
 *	  if the filename would contain a '\', '/' or ':'
 *	Returns 0 on success
 */

int
dst_s_build_filename(char *filename, const char *name, u_int16_t id,
		     int alg, const char *suffix, size_t filename_length)
{
	u_int32_t my_id;
	if (filename == NULL)
		return (-1);
	memset(filename, 0, filename_length);
	if (name == NULL)
		return (-1);
	if (suffix == NULL)
		return (-1);
	if (filename_length < 1 + strlen(name) + 4 + 6 + 1 + strlen(suffix))
		return (-1);
	my_id = id;
	sprintf(filename, "K%s+%03d+%05d.%s", name, alg, my_id,
		(const char *) suffix);
	if (strrchr(filename, '/'))
		return (-1);
	if (strrchr(filename, '\\'))
		return (-1);
	if (strrchr(filename, ':'))
		return (-1);
	return (0);
}

/*%
 *  dst_s_fopen ()
 *     Open a file in the dst_path directory.  If perm is specified, the
 *     file is checked for existence first, and not opened if it exists.
 *  Parameters
 *     filename  File to open
 *     mode       Mode to open the file (passed directly to fopen)
 *     perm       File permission, if creating a new file.
 *  Returns
 *     NULL       Failure
 *     NON-NULL  (FILE *) of opened file.
 */
FILE *
dst_s_fopen(const char *filename, const char *mode, int perm)
{
	FILE *fp;
	char pathname[PATH_MAX];

	if (strlen(filename) + strlen(dst_path) >= sizeof(pathname))
		return (NULL);

	if (*dst_path != '\0') {
		strcpy(pathname, dst_path);
		strcat(pathname, filename);
	} else
		strcpy(pathname, filename);
	
	fp = fopen(pathname, mode);
	if (perm)
		chmod(pathname, (mode_t)perm);
	return (fp);
}

void
dst_s_dump(const int mode, const u_char *data, const int size, 
	    const char *msg)
{
	UNUSED(data);

	if (size > 0) {
#ifdef LONG_TEST
		static u_char scratch[1000];
		int n ;
		n = b64_ntop(data, scratch, size, sizeof(scratch));
		printf("%s: %x %d %s\n", msg, mode, n, scratch);
#else
		printf("%s,%x %d\n", msg, mode, size);
#endif
	}
}

/*! \file */
