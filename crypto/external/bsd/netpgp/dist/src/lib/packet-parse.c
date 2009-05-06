/*
 * Copyright (c) 2005-2008 Nominet UK (www.nic.uk)
 * All rights reserved.
 * Contributors: Ben Laurie, Rachel Willmer. The Contributors have asserted
 * their moral rights under the UK Copyright Design and Patents Act 1988 to
 * be recorded as the authors of this copyright work.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.
 *
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** \file
 * \brief Parser for OpenPGP packets
 */
#include "config.h"

#ifdef HAVE_OPENSSL_CAST_H
#include <openssl/cast.h>
#endif

#include "packet.h"
#include "packet-parse.h"
#include "keyring.h"
#include "errors.h"
#include "packet-show.h"
#include "create.h"

#include "readerwriter.h"
#include "netpgpdefs.h"
#include "parse_local.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <errno.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#define ERRP(cbinfo, cont, err)	do {					\
	cont.u.error.error = err;					\
	CALLBACK(cbinfo, OPS_PARSER_ERROR, &cont);			\
	return false;							\
	/*NOTREACHED*/							\
} while(/*CONSTCOND*/0)

/**
 * limited_read_data reads the specified amount of the subregion's data
 * into a data_t structure
 *
 * \param data	Empty structure which will be filled with data
 * \param len	Number of octets to read
 * \param subregion
 * \param pinfo	How to parse
 *
 * \return 1 on success, 0 on failure
 */
static int 
limited_read_data(__ops_data_t * data, unsigned int len,
		  __ops_region_t * subregion, __ops_parse_info_t * pinfo)
{
	data->len = len;

	if (subregion->length - subregion->length_read < len) {
		(void) fprintf(stderr, "limited_data_read: bad length\n");
		return 0;
	}

	data->contents = calloc(1, data->len);
	if (!data->contents)
		return 0;

	if (!__ops_limited_read(data->contents, data->len, subregion, &pinfo->errors,
			      &pinfo->rinfo, &pinfo->cbinfo))
		return 0;

	return 1;
}

/**
 * read_data reads the remainder of the subregion's data
 * into a data_t structure
 *
 * \param data
 * \param subregion
 * \param pinfo
 *
 * \return 1 on success, 0 on failure
 */
static int 
read_data(__ops_data_t * data, __ops_region_t * subregion,
	  __ops_parse_info_t * pinfo)
{
	int	len;

	len = subregion->length - subregion->length_read;

	if (len >= 0) {
		return (limited_read_data(data, (unsigned)len, subregion, pinfo));
	}
	return 0;
}

/**
 * Reads the remainder of the subregion as a string.
 * It is the user's responsibility to free the memory allocated here.
 */

static int 
read_unsigned_string(unsigned char **str, __ops_region_t * subregion,
		     __ops_parse_info_t * pinfo)
{
	size_t	len = 0;

	len = subregion->length - subregion->length_read;

	*str = calloc(1, len + 1);
	if (!(*str))
		return 0;

	if (len && !__ops_limited_read(*str, len, subregion, &pinfo->errors,
				     &pinfo->rinfo, &pinfo->cbinfo))
		return 0;

	/* ! ensure the string is NULL-terminated */

	(*str)[len] = '\0';

	return 1;
}

static int 
read_string(char **str, __ops_region_t * subregion, __ops_parse_info_t * pinfo)
{
	return (read_unsigned_string((unsigned char **) str, subregion, pinfo));
}

void 
__ops_init_subregion(__ops_region_t * subregion, __ops_region_t * region)
{
	(void) memset(subregion, 0x0, sizeof(*subregion));
	subregion->parent = region;
}

/*
 * XXX: replace __ops_ptag_t with something more appropriate for limiting reads
 */

/**
 * low-level function to read data from reader function
 *
 * Use this function, rather than calling the reader directly.
 *
 * If the accumulate flag is set in *pinfo, the function
 * adds the read data to the accumulated data, and updates
 * the accumulated length. This is useful if, for example,
 * the application wants access to the raw data as well as the
 * parsed data.
 *
 * This function will also try to read the entire amount asked for, but not
 * if it is over INT_MAX. Obviously many callers will know that they
 * never ask for that much and so can avoid the extra complexity of
 * dealing with return codes and filled-in lengths.
 *
 * \param *dest
 * \param *plength
 * \param flags
 * \param *pinfo
 *
 * \return OPS_R_OK
 * \return OPS_R_PARTIAL_READ
 * \return OPS_R_EOF
 * \return OPS_R_EARLY_EOF
 *
 * \sa #__ops_reader_ret_t for details of return codes
 */

static int 
sub_base_read(void *dest, size_t length, __ops_error_t ** errors,
	      __ops_reader_info_t * rinfo, __ops_callback_data_t * cbinfo)
{
	size_t          n;

	/* reading more than this would look like an error */
	if (length > INT_MAX)
		length = INT_MAX;

	for (n = 0; n < length;) {
		int             r = rinfo->reader((char *) dest + n, length - n, errors, rinfo, cbinfo);

		if (r > (int) (length - n)) {
			(void) fprintf(stderr, "sub_base_read: bad read\n");
			return 0;
		}

		/*
		 * XXX: should we save the error and return what was read so
		 * far?
		 */
		if (r < 0)
			return r;

		if (r == 0)
			break;

		n += r;
	}

	if (n == 0)
		return 0;

	if (rinfo->accumulate) {
		if (rinfo->asize < rinfo->alength) {
			(void) fprintf(stderr, "sub_base_read: bad size\n");
			return 0;
		}
		if (rinfo->alength + n > rinfo->asize) {
			rinfo->asize = rinfo->asize * 2 + n;
			rinfo->accumulated = realloc(rinfo->accumulated, rinfo->asize);
		}
		if (rinfo->asize < rinfo->alength + n) {
			(void) fprintf(stderr, "sub_base_read: bad realloc\n");
			return 0;
		}
		(void) memcpy(rinfo->accumulated + rinfo->alength, dest, n);
	}
	/* we track length anyway, because it is used for packet offsets */
	rinfo->alength += n;
	/* and also the position */
	rinfo->position += n;

	return n;
}

int 
__ops_stacked_read(void *dest, size_t length, __ops_error_t ** errors,
		 __ops_reader_info_t * rinfo, __ops_callback_data_t * cbinfo)
{
	return sub_base_read(dest, length, errors, rinfo->next, cbinfo);
}

/* This will do a full read so long as length < MAX_INT */
static int 
base_read(unsigned char *dest, size_t length,
	  __ops_parse_info_t * pinfo)
{
	return sub_base_read(dest, length, &pinfo->errors, &pinfo->rinfo,
			     &pinfo->cbinfo);
}

/*
 * Read a full size_t's worth. If the return is < than length, then
 * *last_read tells you why - < 0 for an error, == 0 for EOF
 */

static size_t 
full_read(unsigned char *dest, size_t length, int *last_read,
	  __ops_error_t ** errors, __ops_reader_info_t * rinfo,
	  __ops_callback_data_t * cbinfo)
{
	size_t          t;
	int             r = 0;	/* preset in case some loon calls with length
				 * == 0 */

	for (t = 0; t < length;) {
		r = sub_base_read(dest + t, length - t, errors, rinfo, cbinfo);

		if (r <= 0) {
			*last_read = r;
			return t;
		}
		t += r;
	}

	*last_read = r;

	return t;
}



/** Read a scalar value of selected length from reader.
 *
 * Read an unsigned scalar value from reader in Big Endian representation.
 *
 * This function does not know or care about packet boundaries. It
 * also assumes that an EOF is an error.
 *
 * \param *result	The scalar value is stored here
 * \param *reader	Our reader
 * \param length	How many bytes to read
 * \return		true on success, false on failure
 */
static bool 
_read_scalar(unsigned *result, unsigned length,
	     __ops_parse_info_t * pinfo)
{
	unsigned        t = 0;

	if (length > sizeof(*result)) {
		(void) fprintf(stderr, "_read_scalar: bad length\n");
		return 0;
	}

	while (length--) {
		unsigned char   c[1];
		int             r;

		r = base_read(c, 1, pinfo);
		if (r != 1)
			return false;
		t = (t << 8) + c[0];
	}

	*result = t;
	return true;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Read bytes from a region within the packet.
 *
 * Read length bytes into the buffer pointed to by *dest.
 * Make sure we do not read over the packet boundary.
 * Updates the Packet Tag's __ops_ptag_t::length_read.
 *
 * If length would make us read over the packet boundary, or if
 * reading fails, we call the callback with an error.
 *
 * Note that if the region is indeterminate, this can return a short
 * read - check region->last_read for the length. EOF is indicated by
 * a success return and region->last_read == 0 in this case (for a
 * region of known length, EOF is an error).
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param dest		The destination buffer
 * \param length	How many bytes to read
 * \param region	Pointer to packet region
 * \param errors    Error stack
 * \param rinfo		Reader info
 * \param cbinfo	Callback info
 * \return		true on success, false on error
 */
bool 
__ops_limited_read(unsigned char *dest, size_t length,
		 __ops_region_t * region, __ops_error_t ** errors,
		 __ops_reader_info_t * rinfo,
		 __ops_callback_data_t * cbinfo)
{
	size_t          r;
	int             lr;

	if (!region->indeterminate && region->length_read + length > region->length) {
		OPS_ERROR(errors, OPS_E_P_NOT_ENOUGH_DATA, "Not enough data");
		return false;
	}
	r = full_read(dest, length, &lr, errors, rinfo, cbinfo);

	if (lr < 0) {
		OPS_ERROR(errors, OPS_E_R_READ_FAILED, "Read failed");
		return false;
	}
	if (!region->indeterminate && r != length) {
		OPS_ERROR(errors, OPS_E_R_READ_FAILED, "Read failed");
		return false;
	}
	region->last_read = r;
	do {
		region->length_read += r;
		if (region->parent && region->length > region->parent->length) {
			(void) fprintf(stderr,
				"ops_limited_read: bad length\n");
			return false;
		}
	} while ((region = region->parent) != NULL);
	return true;
}

/**
   \ingroup Core_ReadPackets
   \brief Call __ops_limited_read on next in stack
*/
bool 
__ops_stacked_limited_read(unsigned char *dest, unsigned length,
			 __ops_region_t * region,
			 __ops_error_t ** errors,
			 __ops_reader_info_t * rinfo,
			 __ops_callback_data_t * cbinfo)
{
	return __ops_limited_read(dest, length, region, errors, rinfo->next, cbinfo);
}

static bool 
limited_read(unsigned char *dest, unsigned length,
	     __ops_region_t * region, __ops_parse_info_t * info)
{
	return __ops_limited_read(dest, length, region, &info->errors,
				&info->rinfo, &info->cbinfo);
}

static bool 
exact_limited_read(unsigned char *dest, unsigned length,
		   __ops_region_t * region,
		   __ops_parse_info_t * pinfo)
{
	bool   ret;

	pinfo->exact_read = true;
	ret = limited_read(dest, length, region, pinfo);
	pinfo->exact_read = false;

	return ret;
}

/** Skip over length bytes of this packet.
 *
 * Calls limited_read() to skip over some data.
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param length	How many bytes to skip
 * \param *region	Pointer to packet region
 * \param *pinfo	How to parse
 * \return		1 on success, 0 on error (calls the cb with OPS_PARSER_ERROR in limited_read()).
 */
static int 
limited_skip(unsigned length, __ops_region_t * region,
	     __ops_parse_info_t * pinfo)
{
	unsigned char   buf[NETPGP_BUFSIZ];

	while (length) {
		unsigned	n = length % NETPGP_BUFSIZ;

		if (!limited_read(buf, n, region, pinfo))
			return 0;
		length -= n;
	}
	return 1;
}

/** Read a scalar.
 *
 * Read a big-endian scalar of length bytes, respecting packet
 * boundaries (by calling limited_read() to read the raw data).
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *dest		The scalar value is stored here
 * \param length	How many bytes make up this scalar (at most 4)
 * \param *region	Pointer to current packet region
 * \param *pinfo	How to parse
 * \param *cb		The callback
 * \return		1 on success, 0 on error (calls the cb with OPS_PARSER_ERROR in limited_read()).
 *
 * \see RFC4880 3.1
 */
static int 
limited_read_scalar(unsigned *dest, unsigned length,
		    __ops_region_t * region,
		    __ops_parse_info_t * pinfo)
{
	unsigned char   c[4] = "";
	unsigned        t;
	unsigned        n;

	if (length > 4) {
		(void) fprintf(stderr, "limited_read_scalar: bad length\n");
		return 0;
	}
	/*LINTED*/
	if (/*CONSTCOND*/sizeof(*dest) < 4) {
		(void) fprintf(stderr, "limited_read_scalar: bad dest\n");
		return 0;
	}
	if (!limited_read(c, length, region, pinfo))
		return 0;

	for (t = 0, n = 0; n < length; ++n)
		t = (t << 8) + c[n];
	*dest = t;

	return 1;
}

/** Read a scalar.
 *
 * Read a big-endian scalar of length bytes, respecting packet
 * boundaries (by calling limited_read() to read the raw data).
 *
 * The value read is stored in a size_t, which is a different size
 * from an unsigned on some platforms.
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *dest		The scalar value is stored here
 * \param length	How many bytes make up this scalar (at most 4)
 * \param *region	Pointer to current packet region
 * \param *pinfo	How to parse
 * \param *cb		The callback
 * \return		1 on success, 0 on error (calls the cb with OPS_PARSER_ERROR in limited_read()).
 *
 * \see RFC4880 3.1
 */
static int 
limited_read_size_t_scalar(size_t * dest, unsigned length,
			   __ops_region_t * region,
			   __ops_parse_info_t * pinfo)
{
	unsigned        tmp;

	/*
	 * Note that because the scalar is at most 4 bytes, we don't care if
	 * size_t is bigger than usigned
	 */
	if (!limited_read_scalar(&tmp, length, region, pinfo))
		return 0;

	*dest = tmp;
	return 1;
}

/** Read a timestamp.
 *
 * Timestamps in OpenPGP are unix time, i.e. seconds since The Epoch (1.1.1970).  They are stored in an unsigned scalar
 * of 4 bytes.
 *
 * This function reads the timestamp using limited_read_scalar().
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *dest		The timestamp is stored here
 * \param *ptag		Pointer to current packet's Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		see limited_read_scalar()
 *
 * \see RFC4880 3.5
 */
static int 
limited_read_time(time_t *dest, __ops_region_t * region,
		  __ops_parse_info_t * pinfo)
{
	unsigned char   c[1];
	time_t          mytime = 0;
	int             i = 0;

	/*
         * Cannot assume that time_t is 4 octets long -
	 * SunOS 5.10 and NetBSD both have 64-bit time_ts.
         */
	if (/* CONSTCOND */sizeof(time_t) == 4) {
		return limited_read_scalar((unsigned *)(void *)dest, 4, region, pinfo);
	}
	for (i = 0; i < 4; i++) {
		if (!limited_read(c, 1, region, pinfo))
			return 0;
		mytime = (mytime << 8) + c[0];
	}
	*dest = mytime;
	return 1;
}

/**
 * \ingroup Core_MPI
 * Read a multiprecision integer.
 *
 * Large numbers (multiprecision integers, MPI) are stored in OpenPGP in two parts.  First there is a 2 byte scalar
 * indicating the length of the following MPI in Bits.  Then follow the bits that make up the actual number, most
 * significant bits first (Big Endian).  The most significant bit in the MPI is supposed to be 1 (unless the MPI is
 * encrypted - then it may be different as the bit count refers to the plain text but the bits are encrypted).
 *
 * Unused bits (i.e. those filling up the most significant byte from the left to the first bits that counts) are
 * supposed to be cleared - I guess. XXX - does anything actually say so?
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param **pgn		return the integer there - the BIGNUM is created by BN_bin2bn() and probably needs to be freed
 * 				by the caller XXX right ben?
 * \param *ptag		Pointer to current packet's Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error (by limited_read_scalar() or limited_read() or if the MPI is not properly formed (XXX
 * 				 see comment below - the callback is called with a OPS_PARSER_ERROR in case of an error)
 *
 * \see RFC4880 3.2
 */
static int 
limited_read_mpi(BIGNUM ** pbn, __ops_region_t * region,
		 __ops_parse_info_t * pinfo)
{
	unsigned char   buf[NETPGP_BUFSIZ] = "";
					/* an MPI has a 2 byte length part.
					 * Length is given in bits, so the
					 * largest we should ever need for
					 * the buffer is NETPGP_BUFSIZ bytes. */
	unsigned        length;
	unsigned        nonzero;
	bool   		ret;

	pinfo->reading_mpi_length = true;
	ret = limited_read_scalar(&length, 2, region, pinfo);

	pinfo->reading_mpi_length = false;
	if (!ret)
		return 0;

	nonzero = length & 7;	/* there should be this many zero bits in the
				 * MS byte */
	if (!nonzero)
		nonzero = 8;
	length = (length + 7) / 8;

	if (length == 0) {
		/* if we try to read a length of 0, then fail */
		if (__ops_get_debug_level(__FILE__)) {
			(void) fprintf(stderr, "limited_read_mpi: 0 length\n");
		}
		return 0;
	}
	if (length > NETPGP_BUFSIZ) {
		(void) fprintf(stderr, "limited_read_mpi: bad length\n");
		return 0;
	}
	if (!limited_read(buf, length, region, pinfo))
		return 0;

	if (((unsigned)buf[0] >> nonzero) != 0 ||
	    !((unsigned)buf[0] & (1U << (nonzero - 1U)))) {
		OPS_ERROR(&pinfo->errors, OPS_E_P_MPI_FORMAT_ERROR, "MPI Format error");
		/* XXX: Ben, one part of
		 * this constraint does
		 * not apply to
		 * encrypted MPIs the
		 * draft says. -- peter */
		return 0;
	}
	*pbn = BN_bin2bn(buf, (int)length, NULL);
	return 1;
}

/** Read some data with a New-Format length from reader.
 *
 * \sa Internet-Draft RFC4880.txt Section 4.2.2
 *
 * \param *length	Where the decoded length will be put
 * \param *pinfo	How to parse
 * \return		true if OK, else false
 *
 */

static bool 
read_new_length(unsigned *length, __ops_parse_info_t * pinfo)
{
	unsigned char   c[1];

	if (base_read(c, 1, pinfo) != 1)
		return false;
	if (c[0] < 192) {
		/* 1. One-octet packet */
		*length = c[0];
		return true;
	} else if (c[0] >= 192 && c[0] <= 223) {
		/* 2. Two-octet packet */
		unsigned        t = (c[0] - 192) << 8;

		if (base_read(c, 1, pinfo) != 1)
			return false;
		*length = t + c[0] + 192;
		return true;
	} else if (c[0] == 255) {
		/* 3. Five-Octet packet */
		return _read_scalar(length, 4, pinfo);
	} else if (c[0] >= 224 && c[0] < 255) {
		/* 4. Partial Body Length */
		/* XXX - agc - gpg multi-recipient encryption uses this */
		OPS_ERROR(&pinfo->errors, OPS_E_UNIMPLEMENTED,
		"New format Partial Body Length fields not yet implemented");
		return false;
	}
	return false;
}

/** Read the length information for a new format Packet Tag.
 *
 * New style Packet Tags encode the length in one to five octets.  This function reads the right amount of bytes and
 * decodes it to the proper length information.
 *
 * This function makes sure to respect packet boundaries.
 *
 * \param *length	return the length here
 * \param *ptag		Pointer to current packet's Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error (by limited_read_scalar() or limited_read() or if the MPI is not properly formed (XXX
 * 				 see comment below)
 *
 * \see RFC4880 4.2.2
 * \see __ops_ptag_t
 */
static int 
limited_read_new_length(unsigned *length, __ops_region_t * region,
			__ops_parse_info_t * pinfo)
{
	unsigned char   c[1] = "";

	if (!limited_read(c, 1, region, pinfo))
		return 0;
	if (c[0] < 192) {
		*length = c[0];
		return 1;
	}
	if (c[0] < 255) {
		unsigned        t = (c[0] - 192) << 8;

		if (!limited_read(c, 1, region, pinfo))
			return 0;
		*length = t + c[0] + 192;
		return 1;
	}
	return limited_read_scalar(length, 4, region, pinfo);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
data_free(__ops_data_t * data)
{
	free(data->contents);
	data->contents = NULL;
	data->len = 0;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
string_free(char **str)
{
	free(*str);
	*str = NULL;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free packet memory, set pointer to NULL */
void 
__ops_subpacket_free(__ops_subpacket_t *packet)
{
	(void) free(packet->raw);
	packet->raw = NULL;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
__ops_headers_free(__ops_headers_t * headers)
{
	unsigned        n;

	for (n = 0; n < headers->nheaders; ++n) {
		free(headers->headers[n].key);
		free(headers->headers[n].value);
	}
	free(headers->headers);
	headers->headers = NULL;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
signed_cleartext_trailer_free(__ops_signed_cleartext_trailer_t * trailer)
{
	free(trailer->hash);
	trailer->hash = NULL;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
__ops_cmd_get_passphrase_free(__ops_seckey_passphrase_t * skp)
{
	if (skp->passphrase && *skp->passphrase) {
		free(*skp->passphrase);
		*skp->passphrase = NULL;
	}
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_userdefined_free(__ops_ss_userdefined_t * ss_userdefined)
{
	data_free(&ss_userdefined->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_reserved_free(__ops_ss_unknown_t * ss_unknown)
{
	data_free(&ss_unknown->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this packet type
*/
static void 
trust_free(__ops_trust_t * trust)
{
	data_free(&trust->data);
}

/**
 * \ingroup Core_Create
 * \brief Free the memory used when parsing a private/experimental PKA signature
 * \param unknown_sig
 */
static void 
free_unknown_sig_pka(__ops_unknown_sig_t * unknown_sig)
{
	data_free(&unknown_sig->data);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
static void 
free_BN(BIGNUM ** pp)
{
	BN_free(*pp);
	*pp = NULL;
}

/**
 * \ingroup Core_Create
 * \brief Free the memory used when parsing a signature
 * \param sig
 */
static void 
sig_free(__ops_sig_t * sig)
{
	switch (sig->info.key_algorithm) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_SIGN_ONLY:
		free_BN(&sig->info.sig.rsa.sig);
		break;

	case OPS_PKA_DSA:
		free_BN(&sig->info.sig.dsa.r);
		free_BN(&sig->info.sig.dsa.s);
		break;

	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		free_BN(&sig->info.sig.elgamal.r);
		free_BN(&sig->info.sig.elgamal.s);
		break;

	case OPS_PKA_PRIVATE00:
	case OPS_PKA_PRIVATE01:
	case OPS_PKA_PRIVATE02:
	case OPS_PKA_PRIVATE03:
	case OPS_PKA_PRIVATE04:
	case OPS_PKA_PRIVATE05:
	case OPS_PKA_PRIVATE06:
	case OPS_PKA_PRIVATE07:
	case OPS_PKA_PRIVATE08:
	case OPS_PKA_PRIVATE09:
	case OPS_PKA_PRIVATE10:
		free_unknown_sig_pka(&sig->info.sig.unknown);
		break;

	default:
		(void) fprintf(stderr, "sig_free: bad sig type\n");
	}
}

/**
 \ingroup Core_Create
 \brief Free the memory used when parsing this signature sub-packet type
 \param ss_preferred_ska
*/
static void 
ss_preferred_ska_free(__ops_ss_preferred_ska_t *ss_preferred_ska)
{
	data_free(&ss_preferred_ska->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
   \param ss_preferred_hash
*/
static void 
ss_preferred_hash_free(__ops_ss_preferred_hash_t *ss_preferred_hash)
{
	data_free(&ss_preferred_hash->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_preferred_compression_free(__ops_ss_preferred_compression_t *ss_preferred_compression)
{
	data_free(&ss_preferred_compression->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_key_flags_free(__ops_ss_key_flags_t *ss_key_flags)
{
	data_free(&ss_key_flags->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_key_server_prefs_free(__ops_ss_key_server_prefs_t *ss_key_server_prefs)
{
	data_free(&ss_key_server_prefs->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_features_free(__ops_ss_features_t *ss_features)
{
	data_free(&ss_features->data);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_notation_data_free(__ops_ss_notation_data_t *ss_notation_data)
{
	data_free(&ss_notation_data->name);
	data_free(&ss_notation_data->value);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this signature sub-packet type */
static void 
ss_regexp_free(__ops_ss_regexp_t *regexp)
{
	string_free(&regexp->text);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this signature sub-packet type */
static void 
ss_policy_url_free(__ops_ss_policy_url_t *policy_url)
{
	string_free(&policy_url->text);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this signature sub-packet type */
static void 
ss_preferred_key_server_free(__ops_ss_preferred_key_server_t *preferred_key_server)
{
	string_free(&preferred_key_server->text);
}

/**
   \ingroup Core_Create
   \brief Free the memory used when parsing this signature sub-packet type
*/
static void 
ss_revocation_reason_free(__ops_ss_revocation_reason_t *ss_revocation_reason)
{
	string_free(&ss_revocation_reason->text);
}

static void 
ss_embedded_sig_free(__ops_ss_embedded_sig_t *ss_embedded_sig)
{
	data_free(&ss_embedded_sig->sig);
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free any memory allocated when parsing the packet content */
void 
__ops_parser_content_free(__ops_packet_t * c)
{
	switch (c->tag) {
	case OPS_PARSER_PTAG:
	case OPS_PTAG_CT_COMPRESSED:
	case OPS_PTAG_SS_CREATION_TIME:
	case OPS_PTAG_SS_EXPIRATION_TIME:
	case OPS_PTAG_SS_KEY_EXPIRATION_TIME:
	case OPS_PTAG_SS_TRUST:
	case OPS_PTAG_SS_ISSUER_KEY_ID:
	case OPS_PTAG_CT_ONE_PASS_SIGNATURE:
	case OPS_PTAG_SS_PRIMARY_USER_ID:
	case OPS_PTAG_SS_REVOCABLE:
	case OPS_PTAG_SS_REVOCATION_KEY:
	case OPS_PTAG_CT_LITERAL_DATA_HEADER:
	case OPS_PTAG_CT_LITERAL_DATA_BODY:
	case OPS_PTAG_CT_SIGNED_CLEARTEXT_BODY:
	case OPS_PTAG_CT_UNARMOURED_TEXT:
	case OPS_PTAG_CT_ARMOUR_TRAILER:
	case OPS_PTAG_CT_SIGNATURE_HEADER:
	case OPS_PTAG_CT_SE_DATA_HEADER:
	case OPS_PTAG_CT_SE_IP_DATA_HEADER:
	case OPS_PTAG_CT_SE_IP_DATA_BODY:
	case OPS_PTAG_CT_MDC:
	case OPS_PARSER_CMD_GET_SECRET_KEY:
		break;

	case OPS_PTAG_CT_SIGNED_CLEARTEXT_HEADER:
		__ops_headers_free(&c->u.signed_cleartext_header.headers);
		break;

	case OPS_PTAG_CT_ARMOUR_HEADER:
		__ops_headers_free(&c->u.armour_header.headers);
		break;

	case OPS_PTAG_CT_SIGNED_CLEARTEXT_TRAILER:
		signed_cleartext_trailer_free(&c->u.signed_cleartext_trailer);
		break;

	case OPS_PTAG_CT_TRUST:
		trust_free(&c->u.trust);
		break;

	case OPS_PTAG_CT_SIGNATURE:
	case OPS_PTAG_CT_SIGNATURE_FOOTER:
		sig_free(&c->u.sig);
		break;

	case OPS_PTAG_CT_PUBLIC_KEY:
	case OPS_PTAG_CT_PUBLIC_SUBKEY:
		__ops_pubkey_free(&c->u.pubkey);
		break;

	case OPS_PTAG_CT_USER_ID:
		__ops_user_id_free(&c->u.user_id);
		break;

	case OPS_PTAG_SS_SIGNERS_USER_ID:
		__ops_user_id_free(&c->u.ss_signers_user_id);
		break;

	case OPS_PTAG_CT_USER_ATTRIBUTE:
		__ops_user_attribute_free(&c->u.user_attribute);
		break;

	case OPS_PTAG_SS_PREFERRED_SKA:
		ss_preferred_ska_free(&c->u.ss_preferred_ska);
		break;

	case OPS_PTAG_SS_PREFERRED_HASH:
		ss_preferred_hash_free(&c->u.ss_preferred_hash);
		break;

	case OPS_PTAG_SS_PREFERRED_COMPRESSION:
		ss_preferred_compression_free(&c->u.ss_preferred_compression);
		break;

	case OPS_PTAG_SS_KEY_FLAGS:
		ss_key_flags_free(&c->u.ss_key_flags);
		break;

	case OPS_PTAG_SS_KEY_SERVER_PREFS:
		ss_key_server_prefs_free(&c->u.ss_key_server_prefs);
		break;

	case OPS_PTAG_SS_FEATURES:
		ss_features_free(&c->u.ss_features);
		break;

	case OPS_PTAG_SS_NOTATION_DATA:
		ss_notation_data_free(&c->u.ss_notation_data);
		break;

	case OPS_PTAG_SS_REGEXP:
		ss_regexp_free(&c->u.ss_regexp);
		break;

	case OPS_PTAG_SS_POLICY_URI:
		ss_policy_url_free(&c->u.ss_policy_url);
		break;

	case OPS_PTAG_SS_PREFERRED_KEY_SERVER:
		ss_preferred_key_server_free(&c->u.ss_preferred_key_server);
		break;

	case OPS_PTAG_SS_USERDEFINED00:
	case OPS_PTAG_SS_USERDEFINED01:
	case OPS_PTAG_SS_USERDEFINED02:
	case OPS_PTAG_SS_USERDEFINED03:
	case OPS_PTAG_SS_USERDEFINED04:
	case OPS_PTAG_SS_USERDEFINED05:
	case OPS_PTAG_SS_USERDEFINED06:
	case OPS_PTAG_SS_USERDEFINED07:
	case OPS_PTAG_SS_USERDEFINED08:
	case OPS_PTAG_SS_USERDEFINED09:
	case OPS_PTAG_SS_USERDEFINED10:
		ss_userdefined_free(&c->u.ss_userdefined);
		break;

	case OPS_PTAG_SS_RESERVED:
		ss_reserved_free(&c->u.ss_unknown);
		break;

	case OPS_PTAG_SS_REVOCATION_REASON:
		ss_revocation_reason_free(&c->u.ss_revocation_reason);
		break;

	case OPS_PTAG_SS_EMBEDDED_SIGNATURE:
		ss_embedded_sig_free(&c->u.ss_embedded_sig);
		break;

	case OPS_PARSER_PACKET_END:
		__ops_subpacket_free(&c->u.packet);
		break;

	case OPS_PARSER_ERROR:
	case OPS_PARSER_ERRCODE:
		break;

	case OPS_PTAG_CT_SECRET_KEY:
	case OPS_PTAG_CT_ENCRYPTED_SECRET_KEY:
		__ops_seckey_free(&c->u.seckey);
		break;

	case OPS_PTAG_CT_PK_SESSION_KEY:
	case OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY:
		__ops_pk_session_key_free(&c->u.pk_session_key);
		break;

	case OPS_PARSER_CMD_GET_SK_PASSPHRASE:
		__ops_cmd_get_passphrase_free(&c->u.skey_passphrase);
		break;

	default:
		fprintf(stderr, "Can't free %d (0x%x)\n", c->tag, c->tag);
	}
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
void 
__ops_pk_session_key_free(__ops_pk_session_key_t * sk)
{
	switch (sk->algorithm) {
	case OPS_PKA_RSA:
		free_BN(&sk->parameters.rsa.encrypted_m);
		break;

	case OPS_PKA_ELGAMAL:
		free_BN(&sk->parameters.elgamal.g_to_k);
		free_BN(&sk->parameters.elgamal.encrypted_m);
		break;

	default:
		(void) fprintf(stderr, "__ops_pk_session_key_free: bad alg\n");
		break;
	}
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing a public key */
void 
__ops_pubkey_free(__ops_pubkey_t * p)
{
	switch (p->algorithm) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		free_BN(&p->key.rsa.n);
		free_BN(&p->key.rsa.e);
		break;

	case OPS_PKA_DSA:
		free_BN(&p->key.dsa.p);
		free_BN(&p->key.dsa.q);
		free_BN(&p->key.dsa.g);
		free_BN(&p->key.dsa.y);
		break;

	case OPS_PKA_ELGAMAL:
	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		free_BN(&p->key.elgamal.p);
		free_BN(&p->key.elgamal.g);
		free_BN(&p->key.elgamal.y);
		break;

	case 0:
		/* nothing to free */
		break;

	default:
		(void) fprintf(stderr, "__ops_pubkey_free: bad alg\n");
	}
}

/**
   \ingroup Core_ReadPackets
*/
static int 
parse_pubkey_data(__ops_pubkey_t * key, __ops_region_t * region,
		      __ops_parse_info_t * pinfo)
{
	unsigned char   c[1] = "";

	if (region->length_read != 0) {
		/* We should not have read anything so far */
		(void) fprintf(stderr, "parse_pubkey_data: bad length\n");
		return 0;
	}

	if (!limited_read(c, 1, region, pinfo)) {
		return 0;
	}
	key->version = c[0];
	if (key->version < 2 || key->version > 4) {
		OPS_ERROR_1(&pinfo->errors, OPS_E_PROTO_BAD_PUBLIC_KEY_VRSN,
			    "Bad public key version (0x%02x)", key->version);
		return 0;
	}
	if (!limited_read_time(&key->birthtime, region, pinfo)) {
		return 0;
	}

	key->days_valid = 0;
	if ((key->version == 2 || key->version == 3) &&
	    !limited_read_scalar(&key->days_valid, 2, region, pinfo)) {
		return 0;
	}

	if (!limited_read(c, 1, region, pinfo))
		return 0;

	key->algorithm = c[0];

	switch (key->algorithm) {
	case OPS_PKA_DSA:
		if (!limited_read_mpi(&key->key.dsa.p, region, pinfo) ||
		    !limited_read_mpi(&key->key.dsa.q, region, pinfo) ||
		    !limited_read_mpi(&key->key.dsa.g, region, pinfo) ||
		    !limited_read_mpi(&key->key.dsa.y, region, pinfo)) {
			return 0;
		}
		break;

	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		if (!limited_read_mpi(&key->key.rsa.n, region, pinfo) ||
		    !limited_read_mpi(&key->key.rsa.e, region, pinfo)) {
			return 0;
		}
		break;

	case OPS_PKA_ELGAMAL:
	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		if (!limited_read_mpi(&key->key.elgamal.p, region, pinfo) ||
		    !limited_read_mpi(&key->key.elgamal.g, region, pinfo) ||
		    !limited_read_mpi(&key->key.elgamal.y, region, pinfo)) {
			return 0;
		}
		break;

	default:
		OPS_ERROR_1(&pinfo->errors,
			OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG,
			"Unsupported Public Key algorithm (%s)",
			__ops_show_pka(key->algorithm));
		return 0;
	}

	return 1;
}


/**
 * \ingroup Core_ReadPackets
 * \brief Parse a public key packet.
 *
 * This function parses an entire v3 (== v2) or v4 public key packet for RSA, ElGamal, and DSA keys.
 *
 * Once the key has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the current Packet Tag.  This function should consume the entire packet.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.5.2
 */
static int 
parse_pubkey(__ops_content_tag_t tag, __ops_region_t * region,
		 __ops_parse_info_t * pinfo)
{
	__ops_packet_t pkt;

	if (!parse_pubkey_data(&pkt.u.pubkey, region, pinfo))
		return 0;

	/* XXX: this test should be done for all packets, surely? */
	if (region->length_read != region->length) {
		OPS_ERROR_1(&pinfo->errors, OPS_E_R_UNCONSUMED_DATA,
			    "Unconsumed data (%d)", region->length - region->length_read);
		return 0;
	}
	CALLBACK(&pinfo->cbinfo, tag, &pkt);

	return 1;
}


/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this packet type */
void 
__ops_user_attribute_free(__ops_user_attribute_t * user_att)
{
	data_free(&user_att->data);
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse one user attribute packet.
 *
 * User attribute packets contain one or more attribute subpackets.
 * For now, handle the whole packet as raw data.
 */

static int 
parse_user_attribute(__ops_region_t * region, __ops_parse_info_t * pinfo)
{

	__ops_packet_t pkt;

	/*
	 * xxx- treat as raw data for now. Could break down further into
	 * attribute sub-packets later - rachel
	 */

	if (region->length_read != 0) {
		/* We should not have read anything so far */
		(void) fprintf(stderr, "parse_user_attribute: bad length\n");
		return 0;
	}

	if (!read_data(&pkt.u.user_attribute.data, region, pinfo))
		return 0;

	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_USER_ATTRIBUTE, &pkt);

	return 1;
}

/**
\ingroup Core_Create
\brief Free allocated memory
*/
/* ! Free the memory used when parsing this packet type */
void 
__ops_user_id_free(__ops_user_id_t * id)
{
	free(id->user_id);
	id->user_id = NULL;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a user id.
 *
 * This function parses an user id packet, which is basically just a char array the size of the packet.
 *
 * The char array is to be treated as an UTF-8 string.
 *
 * The userid gets null terminated by this function.  Freeing it is the responsibility of the caller.
 *
 * Once the userid has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.  This function should consume the entire packet.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.11
 */
static int 
parse_user_id(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	__ops_packet_t pkt;

	 if (region->length_read != 0) {
		/* We should not have read anything so far */
		(void) fprintf(stderr, "parse_user_id: bad length\n");
		return 0;
	}

	pkt.u.user_id.user_id = calloc(1, region->length + 1);	/* XXX should we not
							 * like check malloc's
							 * return value? */

	if (region->length && !limited_read(pkt.u.user_id.user_id, region->length, region,
					    pinfo))
		return 0;

	pkt.u.user_id.user_id[region->length] = '\0';	/* terminate the string */

	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_USER_ID, &pkt);

	return 1;
}

static __ops_hash_t     *
parse_hash_find(__ops_parse_info_t * pinfo,
		    const unsigned char keyid[OPS_KEY_ID_SIZE])
{
	size_t          n;

	for (n = 0; n < pinfo->nhashes; ++n) {
		if (memcmp(pinfo->hashes[n].keyid, keyid, OPS_KEY_ID_SIZE) == 0) {
			return &pinfo->hashes[n].hash;
		}
	}
	return NULL;
}

/**
 * \ingroup Core_Parse
 * \brief Parse a version 3 signature.
 *
 * This function parses an version 3 signature packet, handling RSA and DSA signatures.
 *
 * Once the signature has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.  This function should consume the entire packet.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.2
 */
static int 
parse_v3_sig(__ops_region_t * region,
		   __ops_parse_info_t * pinfo)
{
	__ops_packet_t	pkt;
	unsigned char		c[1] = "";

	/* clear signature */
	(void) memset(&pkt.u.sig, 0x0, sizeof(pkt.u.sig));

	pkt.u.sig.info.version = OPS_V3;

	/* hash info length */
	if (!limited_read(c, 1, region, pinfo)) {
		return 0;
	}
	if (c[0] != 5) {
		ERRP(&pinfo->cbinfo, pkt, "bad hash info length");
	}

	if (!limited_read(c, 1, region, pinfo)) {
		return 0;
	}
	pkt.u.sig.info.type = c[0];
	/* XXX: check signature type */

	if (!limited_read_time(&pkt.u.sig.info.birthtime,
			region, pinfo)) {
		return 0;
	}
	pkt.u.sig.info.birthtime_set = true;

	if (!limited_read(pkt.u.sig.info.signer_id, OPS_KEY_ID_SIZE,
			region, pinfo)) {
		return 0;
	}
	pkt.u.sig.info.signer_id_set = true;

	if (!limited_read(c, 1, region, pinfo)) {
		return 0;
	}
	pkt.u.sig.info.key_algorithm = c[0];
	/* XXX: check algorithm */

	if (!limited_read(c, 1, region, pinfo)) {
		return 0;
	}
	pkt.u.sig.info.hash_algorithm = c[0];
	/* XXX: check algorithm */

	if (!limited_read(pkt.u.sig.hash2, 2, region, pinfo)) {
		return 0;
	}

	switch (pkt.u.sig.info.key_algorithm) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_SIGN_ONLY:
		if (!limited_read_mpi(&pkt.u.sig.info.sig.rsa.sig, region, pinfo)) {
			return 0;
		}
		break;

	case OPS_PKA_DSA:
		if (!limited_read_mpi(&pkt.u.sig.info.sig.dsa.r, region, pinfo) ||
		    !limited_read_mpi(&pkt.u.sig.info.sig.dsa.s, region, pinfo)) {
			return 0;
		}
		break;

	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		if (!limited_read_mpi(&pkt.u.sig.info.sig.elgamal.r, region, pinfo) ||
		    !limited_read_mpi(&pkt.u.sig.info.sig.elgamal.s, region, pinfo)) {
			return 0;
		}
		break;

	default:
		OPS_ERROR_1(&pinfo->errors,
			OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG,
			"Unsupported signature key algorithm (%s)",
			__ops_show_pka(pkt.u.sig.info.key_algorithm));
		return 0;
	}

	if (region->length_read != region->length) {
		OPS_ERROR_1(&pinfo->errors, OPS_E_R_UNCONSUMED_DATA, "Unconsumed data (%d)", region->length - region->length_read);
		return 0;
	}
	if (pkt.u.sig.info.signer_id_set) {
		pkt.u.sig.hash = parse_hash_find(pinfo,
				pkt.u.sig.info.signer_id);
	}

	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_SIGNATURE, &pkt);

	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse one signature sub-packet.
 *
 * Version 4 signatures can have an arbitrary amount of (hashed and unhashed) subpackets.  Subpackets are used to hold
 * optional attributes of subpackets.
 *
 * This function parses one such signature subpacket.
 *
 * Once the subpacket has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.  This function should consume the entire subpacket.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.3
 */
static int 
parse_one_sig_subpacket(__ops_sig_t * sig,
			      __ops_region_t * region,
			      __ops_parse_info_t * pinfo)
{
	__ops_region_t    subregion;
	unsigned char   c[1] = "";
	__ops_packet_t pkt;
	unsigned        t8, t7;
	bool   doread = true;
	unsigned char   bools[1] = "";

	__ops_init_subregion(&subregion, region);
	if (!limited_read_new_length(&subregion.length, region, pinfo))
		return 0;

	if (subregion.length > region->length)
		ERRP(&pinfo->cbinfo, pkt, "Subpacket too long");

	if (!limited_read(c, 1, &subregion, pinfo))
		return 0;

	t8 = (c[0] & 0x7f) / 8;
	t7 = 1 << (c[0] & 7);

	pkt.critical = (unsigned)c[0] >> 7;
	pkt.tag = OPS_PTAG_SIGNATURE_SUBPACKET_BASE + (c[0] & 0x7f);

	/* Application wants it delivered raw */
	if (pinfo->ss_raw[t8] & t7) {
		pkt.u.ss_raw.tag = pkt.tag;
		pkt.u.ss_raw.length = subregion.length - 1;
		pkt.u.ss_raw.raw = calloc(1, pkt.u.ss_raw.length);
		if (!limited_read(pkt.u.ss_raw.raw, pkt.u.ss_raw.length, &subregion, pinfo))
			return 0;
		CALLBACK(&pinfo->cbinfo, OPS_PTAG_RAW_SS, &pkt);
		return 1;
	}
	switch (pkt.tag) {
	case OPS_PTAG_SS_CREATION_TIME:
	case OPS_PTAG_SS_EXPIRATION_TIME:
	case OPS_PTAG_SS_KEY_EXPIRATION_TIME:
		if (!limited_read_time(&pkt.u.ss_time.time, &subregion, pinfo))
			return 0;
		if (pkt.tag == OPS_PTAG_SS_CREATION_TIME) {
			sig->info.birthtime = pkt.u.ss_time.time;
			sig->info.birthtime_set = true;
		}
		break;

	case OPS_PTAG_SS_TRUST:
		if (!limited_read(&pkt.u.ss_trust.level, 1, &subregion, pinfo)
		 || !limited_read(&pkt.u.ss_trust.amount, 1, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_REVOCABLE:
		if (!limited_read(bools, 1, &subregion, pinfo))
			return 0;
		pkt.u.ss_revocable.revocable = !!bools[0];
		break;

	case OPS_PTAG_SS_ISSUER_KEY_ID:
		if (!limited_read(pkt.u.ss_issuer_key_id.key_id, OPS_KEY_ID_SIZE,
				  &subregion, pinfo))
			return 0;
		(void) memcpy(sig->info.signer_id, pkt.u.ss_issuer_key_id.key_id, OPS_KEY_ID_SIZE);
		sig->info.signer_id_set = true;
		break;

	case OPS_PTAG_SS_PREFERRED_SKA:
		if (!read_data(&pkt.u.ss_preferred_ska.data, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_PREFERRED_HASH:
		if (!read_data(&pkt.u.ss_preferred_hash.data, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_PREFERRED_COMPRESSION:
		if (!read_data(&pkt.u.ss_preferred_compression.data, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_PRIMARY_USER_ID:
		if (!limited_read(bools, 1, &subregion, pinfo))
			return 0;
		pkt.u.ss_primary_user_id.primary_user_id = !!bools[0];
		break;

	case OPS_PTAG_SS_KEY_FLAGS:
		if (!read_data(&pkt.u.ss_key_flags.data, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_KEY_SERVER_PREFS:
		if (!read_data(&pkt.u.ss_key_server_prefs.data, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_FEATURES:
		if (!read_data(&pkt.u.ss_features.data, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_SIGNERS_USER_ID:
		if (!read_unsigned_string(&pkt.u.ss_signers_user_id.user_id, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_EMBEDDED_SIGNATURE:
		/* \todo should do something with this sig? */
		if (!read_data(&pkt.u.ss_embedded_sig.sig, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_NOTATION_DATA:
		if (!limited_read_data(&pkt.u.ss_notation_data.flags, 4, &subregion, pinfo))
			return 0;
		if (!limited_read_size_t_scalar(&pkt.u.ss_notation_data.name.len, 2,
						&subregion, pinfo))
			return 0;
		if (!limited_read_size_t_scalar(&pkt.u.ss_notation_data.value.len, 2,
						&subregion, pinfo))
			return 0;
		if (!limited_read_data(&pkt.u.ss_notation_data.name,
			    pkt.u.ss_notation_data.name.len, &subregion, pinfo))
			return 0;
		if (!limited_read_data(&pkt.u.ss_notation_data.value,
			   pkt.u.ss_notation_data.value.len, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_POLICY_URI:
		if (!read_string(&pkt.u.ss_policy_url.text, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_REGEXP:
		if (!read_string(&pkt.u.ss_regexp.text, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_PREFERRED_KEY_SERVER:
		if (!read_string(&pkt.u.ss_preferred_key_server.text, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_USERDEFINED00:
	case OPS_PTAG_SS_USERDEFINED01:
	case OPS_PTAG_SS_USERDEFINED02:
	case OPS_PTAG_SS_USERDEFINED03:
	case OPS_PTAG_SS_USERDEFINED04:
	case OPS_PTAG_SS_USERDEFINED05:
	case OPS_PTAG_SS_USERDEFINED06:
	case OPS_PTAG_SS_USERDEFINED07:
	case OPS_PTAG_SS_USERDEFINED08:
	case OPS_PTAG_SS_USERDEFINED09:
	case OPS_PTAG_SS_USERDEFINED10:
		if (!read_data(&pkt.u.ss_userdefined.data, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_RESERVED:
		if (!read_data(&pkt.u.ss_unknown.data, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_REVOCATION_REASON:
		/* first byte is the machine-readable code */
		if (!limited_read(&pkt.u.ss_revocation_reason.code, 1, &subregion, pinfo))
			return 0;

		/* the rest is a human-readable UTF-8 string */
		if (!read_string(&pkt.u.ss_revocation_reason.text, &subregion, pinfo))
			return 0;
		break;

	case OPS_PTAG_SS_REVOCATION_KEY:
		/* octet 0 = class. Bit 0x80 must be set */
		if (!limited_read(&pkt.u.ss_revocation_key.class, 1, &subregion, pinfo))
			return 0;
		if (!(pkt.u.ss_revocation_key.class & 0x80)) {
			printf("Warning: OPS_PTAG_SS_REVOCATION_KEY class: "
			       "Bit 0x80 should be set\n");
			return 0;
		}
		/* octet 1 = algid */
		if (!limited_read(&pkt.u.ss_revocation_key.algid, 1, &subregion, pinfo))
			return 0;

		/* octets 2-21 = fingerprint */
		if (!limited_read(&pkt.u.ss_revocation_key.fingerprint[0], 20, &subregion,
				  pinfo))
			return 0;
		break;

	default:
		if (pinfo->ss_parsed[t8] & t7)
			OPS_ERROR_1(&pinfo->errors, OPS_E_PROTO_UNKNOWN_SS,
				    "Unknown signature subpacket type (%d)", c[0] & 0x7f);
		doread = false;
		break;
	}

	/* Application doesn't want it delivered parsed */
	if (!(pinfo->ss_parsed[t8] & t7)) {
		if (pkt.critical)
			OPS_ERROR_1(&pinfo->errors, OPS_E_PROTO_CRITICAL_SS_IGNORED,
				"Critical signature subpacket ignored (%d)",
				    c[0] & 0x7f);
		if (!doread && !limited_skip(subregion.length - 1, &subregion, pinfo))
			return 0;
		/*
		 * printf("skipped %d length
		 * %d\n",c[0]&0x7f,subregion.length);
		 */
		if (doread)
			__ops_parser_content_free(&pkt);
		return 1;
	}
	if (doread && subregion.length_read != subregion.length) {
		OPS_ERROR_1(&pinfo->errors, OPS_E_R_UNCONSUMED_DATA,
			    "Unconsumed data (%d)",
			    subregion.length - subregion.length_read);
		return 0;
	}
	CALLBACK(&pinfo->cbinfo, pkt.tag, &pkt);

	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse several signature subpackets.
 *
 * Hashed and unhashed subpacket sets are preceded by an octet count that specifies the length of the complete set.
 * This function parses this length and then calls parse_one_sig_subpacket() for each subpacket until the
 * entire set is consumed.
 *
 * This function does not call the callback directly, parse_one_sig_subpacket() does for each subpacket.
 *
 * \param *ptag		Pointer to the Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.3
 */
static int 
parse_sig_subpackets(__ops_sig_t * sig,
			   __ops_region_t * region,
			   __ops_parse_info_t * pinfo)
{
	__ops_packet_t	pkt;
	__ops_region_t		subregion;

	__ops_init_subregion(&subregion, region);
	if (!limited_read_scalar(&subregion.length, 2, region, pinfo)) {
		return 0;
	}

	if (subregion.length > region->length) {
		ERRP(&pinfo->cbinfo, pkt, "Subpacket set too long");
	}

	while (subregion.length_read < subregion.length) {
		if (!parse_one_sig_subpacket(sig, &subregion, pinfo)) {
			return 0;
		}
	}

	if (subregion.length_read != subregion.length) {
		if (!limited_skip(subregion.length - subregion.length_read,
				&subregion, pinfo)) {
			ERRP(&pinfo->cbinfo, pkt,
				"Read failed while recovering from subpacket length mismatch");
		}
		ERRP(&pinfo->cbinfo, pkt, "Subpacket length mismatch");
	}
	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a version 4 signature.
 *
 * This function parses a version 4 signature including all its hashed and unhashed subpackets.
 *
 * Once the signature packet has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 *
 * \see RFC4880 5.2.3
 */
static int 
parse_v4_sig(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	unsigned char   c[1] = "";
	__ops_packet_t pkt;

	/* debug=1; */
	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "\nparse_v4_sig\n");
	}
	/* clear signature */
	(void) memset(&pkt.u.sig, 0x0, sizeof(pkt.u.sig));

	/*
	 * We need to hash the packet data from version through the hashed
	 * subpacket data
	 */

	pkt.u.sig.v4_hashed_data_start = pinfo->rinfo.alength - 1;

	/* Set version,type,algorithms */

	pkt.u.sig.info.version = OPS_V4;

	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.sig.info.type = c[0];
	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "signature type=%d (%s)\n",
			pkt.u.sig.info.type,
			__ops_show_sig_type(pkt.u.sig.info.type));
	}
	/* XXX: check signature type */

	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.sig.info.key_algorithm = c[0];
	/* XXX: check algorithm */
	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "key_algorithm=%d (%s)\n",
			pkt.u.sig.info.key_algorithm,
			__ops_show_pka(pkt.u.sig.info.key_algorithm));
	}
	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.sig.info.hash_algorithm = c[0];
	/* XXX: check algorithm */
	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "hash_algorithm=%d %s\n",
			pkt.u.sig.info.hash_algorithm,
		  __ops_show_hash_algorithm(pkt.u.sig.info.hash_algorithm));
	}
	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_SIGNATURE_HEADER, &pkt);


	if (!parse_sig_subpackets(&pkt.u.sig, region, pinfo))
		return 0;

	pkt.u.sig.info.v4_hashed_data_length = pinfo->rinfo.alength
		- pkt.u.sig.v4_hashed_data_start;

	/* copy hashed subpackets */
	if (pkt.u.sig.info.v4_hashed_data)
		free(pkt.u.sig.info.v4_hashed_data);
	pkt.u.sig.info.v4_hashed_data = calloc(1, pkt.u.sig.info.v4_hashed_data_length);

	if (!pinfo->rinfo.accumulate) {
		/* We must accumulate, else we can't check the signature */
		fprintf(stderr, "*** ERROR: must set accumulate to true\n");
		return 0;
	}
	(void) memcpy(pkt.u.sig.info.v4_hashed_data,
	       pinfo->rinfo.accumulated + pkt.u.sig.v4_hashed_data_start,
	       pkt.u.sig.info.v4_hashed_data_length);

	if (!parse_sig_subpackets(&pkt.u.sig, region, pinfo))
		return 0;

	if (!limited_read(pkt.u.sig.hash2, 2, region, pinfo))
		return 0;

	switch (pkt.u.sig.info.key_algorithm) {
	case OPS_PKA_RSA:
		if (!limited_read_mpi(&pkt.u.sig.info.sig.rsa.sig, region, pinfo))
			return 0;
		break;

	case OPS_PKA_DSA:
		if (!limited_read_mpi(&pkt.u.sig.info.sig.dsa.r, region, pinfo)) {
			/*
			 * usually if this fails, it just means we've reached
			 * the end of the keyring
			 */
			if (__ops_get_debug_level(__FILE__)) {
				(void) fprintf(stderr, "Error reading DSA r field in signature");
			}
			return 0;
		}
		if (!limited_read_mpi(&pkt.u.sig.info.sig.dsa.s, region, pinfo))
			ERRP(&pinfo->cbinfo, pkt, "Error reading DSA s field in signature");
		break;

	case OPS_PKA_ELGAMAL_ENCRYPT_OR_SIGN:
		if (!limited_read_mpi(&pkt.u.sig.info.sig.elgamal.r, region, pinfo)
		    || !limited_read_mpi(&pkt.u.sig.info.sig.elgamal.s, region, pinfo))
			return 0;
		break;

	case OPS_PKA_PRIVATE00:
	case OPS_PKA_PRIVATE01:
	case OPS_PKA_PRIVATE02:
	case OPS_PKA_PRIVATE03:
	case OPS_PKA_PRIVATE04:
	case OPS_PKA_PRIVATE05:
	case OPS_PKA_PRIVATE06:
	case OPS_PKA_PRIVATE07:
	case OPS_PKA_PRIVATE08:
	case OPS_PKA_PRIVATE09:
	case OPS_PKA_PRIVATE10:
		if (!read_data(&pkt.u.sig.info.sig.unknown.data, region, pinfo))
			return 0;
		break;

	default:
		OPS_ERROR_1(&pinfo->errors, OPS_E_ALG_UNSUPPORTED_SIGNATURE_ALG,
			    "Bad v4 signature key algorithm (%s)",
			    __ops_show_pka(pkt.u.sig.info.key_algorithm));
		return 0;
	}

	if (region->length_read != region->length) {
		OPS_ERROR_1(&pinfo->errors, OPS_E_R_UNCONSUMED_DATA,
			    "Unconsumed data (%d)",
			    region->length - region->length_read);
		return 0;
	}
	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_SIGNATURE_FOOTER, &pkt);

	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a signature subpacket.
 *
 * This function calls the appropriate function to handle v3 or v4 signatures.
 *
 * Once the signature packet has been parsed successfully, it is passed to the callback.
 *
 * \param *ptag		Pointer to the Packet Tag.
 * \param *reader	Our reader
 * \param *cb		The callback
 * \return		1 on success, 0 on error
 */
static int 
parse_sig(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	unsigned char   c[1] = "";
	__ops_packet_t pkt;

	if (region->length_read != 0) {
		/* We should not have read anything so far */
		(void) fprintf(stderr, "parse_sig: bad length\n");
		return 0;
	}

	(void) memset(&pkt, 0x0, sizeof(pkt));

	if (!limited_read(c, 1, region, pinfo))
		return 0;

	if (c[0] == 2 || c[0] == 3)
		return parse_v3_sig(region, pinfo);
	else if (c[0] == 4)
		return parse_v4_sig(region, pinfo);

	OPS_ERROR_1(&pinfo->errors, OPS_E_PROTO_BAD_SIGNATURE_VRSN,
		    "Bad signature version (%d)", c[0]);
	return 0;
}

/**
 \ingroup Core_ReadPackets
 \brief Parse Compressed packet
*/
static int 
parse_compressed(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	__ops_packet_t	pkt;
	unsigned char		c[1] = "";

	if (!limited_read(c, 1, region, pinfo)) {
		return 0;
	}

	pkt.u.compressed.type = c[0];

	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_COMPRESSED, &pkt);

	/*
	 * The content of a compressed data packet is more OpenPGP packets
	 * once decompressed, so recursively handle them
	 */

	return __ops_decompress(region, pinfo, pkt.u.compressed.type);
}

/* XXX: this could be improved by sharing all hashes that are the */
/* same, then duping them just before checking the signature. */
static void 
parse_hash_init(__ops_parse_info_t * pinfo, __ops_hash_algorithm_t type,
		    const unsigned char *keyid)
{
	__ops_parse_hash_info_t *hash;

	pinfo->hashes = realloc(pinfo->hashes,
			      (pinfo->nhashes + 1) * sizeof(*pinfo->hashes));
	hash = &pinfo->hashes[pinfo->nhashes++];

	__ops_hash_any(&hash->hash, type);
	hash->hash.init(&hash->hash);
	(void) memcpy(hash->keyid, keyid, sizeof(hash->keyid));
}

/**
   \ingroup Core_ReadPackets
   \brief Parse a One Pass Signature packet
*/
static int 
parse_one_pass(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	unsigned char   c[1] = "";
	__ops_packet_t pkt;

	if (!limited_read(&pkt.u.one_pass_sig.version, 1, region, pinfo))
		return 0;
	if (pkt.u.one_pass_sig.version != 3) {
		OPS_ERROR_1(&pinfo->errors, OPS_E_PROTO_BAD_ONE_PASS_SIG_VRSN,
			    "Bad one-pass signature version (%d)",
			    pkt.u.one_pass_sig.version);
		return 0;
	}
	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.one_pass_sig.sig_type = c[0];

	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.one_pass_sig.hash_algorithm = c[0];

	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.one_pass_sig.key_algorithm = c[0];

	if (!limited_read(pkt.u.one_pass_sig.keyid,
			  sizeof(pkt.u.one_pass_sig.keyid), region, pinfo))
		return 0;

	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.one_pass_sig.nested = !!c[0];

	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_ONE_PASS_SIGNATURE, &pkt);

	/* XXX: we should, perhaps, let the app choose whether to hash or not */
	parse_hash_init(pinfo, pkt.u.one_pass_sig.hash_algorithm,
			    pkt.u.one_pass_sig.keyid);

	return 1;
}

/**
 \ingroup Core_ReadPackets
 \brief Parse a Trust packet
*/
static int
parse_trust(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	__ops_packet_t pkt;

	if (!read_data(&pkt.u.trust.data, region, pinfo))
		return 0;

	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_TRUST, &pkt);

	return 1;
}

static void 
parse_hash_data(__ops_parse_info_t * pinfo, const void *data,
		    size_t length)
{
	size_t          n;

	for (n = 0; n < pinfo->nhashes; ++n) {
		pinfo->hashes[n].hash.add(&pinfo->hashes[n].hash, data, length);
	}
}

/**
   \ingroup Core_ReadPackets
   \brief Parse a Literal Data packet
*/
static int 
parse_litdata(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	__ops_packet_t	 pkt;
	__ops_memory_t		*mem;
	unsigned char		 c[1] = "";

	if (!limited_read(c, 1, region, pinfo)) {
		return 0;
	}
	pkt.u.litdata_header.format = c[0];

	if (!limited_read(c, 1, region, pinfo)) {
		return 0;
	}
	if (!limited_read((unsigned char *)pkt.u.litdata_header.filename,
			(unsigned)c[0], region, pinfo)) {
		return 0;
	}
	pkt.u.litdata_header.filename[c[0]] = '\0';

	if (!limited_read_time(&pkt.u.litdata_header.modification_time,
			region, pinfo)) {
		return 0;
	}

	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_LITERAL_DATA_HEADER, &pkt);

	mem = pkt.u.litdata_body.mem = __ops_memory_new();
	__ops_memory_init(pkt.u.litdata_body.mem,
			(unsigned)(region->length * 1.01) + 12);
	pkt.u.litdata_body.data = mem->buf;

	while (region->length_read < region->length) {
		unsigned        readc = region->length - region->length_read;

		if (!limited_read(mem->buf, readc, region, pinfo)) {
			return 0;
		}
		pkt.u.litdata_body.length = readc;
		parse_hash_data(pinfo, pkt.u.litdata_body.data,
			region->length);
		CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_LITERAL_DATA_BODY,
			&pkt);
	}

	/* XXX - get rid of mem here? */

	return 1;
}

/**
 * \ingroup Core_Create
 *
 * __ops_seckey_free() frees the memory associated with "key". Note that
 * the key itself is not freed.
 *
 * \param key
 */

void 
__ops_seckey_free(__ops_seckey_t * key)
{
	switch (key->pubkey.algorithm) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		free_BN(&key->key.rsa.d);
		free_BN(&key->key.rsa.p);
		free_BN(&key->key.rsa.q);
		free_BN(&key->key.rsa.u);
		break;

	case OPS_PKA_DSA:
		free_BN(&key->key.dsa.x);
		break;

	default:
		(void) fprintf(stderr,
			"__ops_seckey_free: Unknown algorithm: %d (%s)\n",
			key->pubkey.algorithm,
			__ops_show_pka(key->pubkey.algorithm));
	}

	__ops_pubkey_free(&key->pubkey);
}

static int 
consume_packet(__ops_region_t * region, __ops_parse_info_t * pinfo,
	       bool warn)
{
	__ops_packet_t	pkt;
	__ops_data_t		remainder;

	if (region->indeterminate) {
		ERRP(&pinfo->cbinfo, pkt, "Can't consume indeterminate packets");
	}

#if 0
	if (read_data(&remainder, region, pinfo)) {
		/* now throw it away */
		data_free(&remainder);
		if (warn) {
			OPS_ERROR(&pinfo->errors, OPS_E_P_PACKET_CONSUMED,
				"Warning: packet consumer");
		}
	} else if (warn) {
		OPS_ERROR(&pinfo->errors, OPS_E_P_PACKET_NOT_CONSUMED,
			"Warning: Packet was not consumed");
	} else {
		OPS_ERROR(&pinfo->errors, OPS_E_P_PACKET_NOT_CONSUMED,
			"Packet was not consumed");
		return 0;
	}
	return 1;
#else
	if (read_data(&remainder, region, pinfo)) {
		/* now throw it away */
		data_free(&remainder);
		if (warn) {
			OPS_ERROR(&pinfo->errors, OPS_E_P_PACKET_CONSUMED,
				"Warning: packet consumer");
		}
		return 1;
	}
	OPS_ERROR(&pinfo->errors, OPS_E_P_PACKET_NOT_CONSUMED,
			(warn) ? "Warning: Packet was not consumed" :
				"Packet was not consumed");
	return warn;
#endif
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse a secret key
 */
static int 
parse_seckey(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	__ops_packet_t	pkt;
	__ops_region_t		encregion;
	__ops_region_t	       *saved_region = NULL;
	unsigned char		c[1] = "";
	__ops_crypt_t		decrypt;
	__ops_hash_t		checkhash;
	unsigned		blocksize;
#if 0
	size_t			checksum_length = 2;
#endif
	bool			crypted;
	int			ret = 1;

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "\n---------\nparse_seckey:\n");
		fprintf(stderr, "region length=%d, length_read=%d, remainder=%d\n", region->length, region->length_read, region->length - region->length_read);
	}
	(void) memset(&pkt, 0x0, sizeof(pkt));
	if (!parse_pubkey_data(&pkt.u.seckey.pubkey, region, pinfo))
		return 0;

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "parse_seckey: public key parsed\n");
		__ops_print_pubkey(&pkt.u.seckey.pubkey);
	}
	pinfo->reading_v3_secret = pkt.u.seckey.pubkey.version != OPS_V4;

	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.seckey.s2k_usage = c[0];
#if 0
	if (pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED)
		checksum_length = 20;
#endif

	if (pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED ||
	    pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED) {
		if (!limited_read(c, 1, region, pinfo)) {
			return 0;
		}
		pkt.u.seckey.algorithm = c[0];

		if (!limited_read(c, 1, region, pinfo)) {
			return 0;
		}
		pkt.u.seckey.s2k_specifier = c[0];

		switch (pkt.u.seckey.s2k_specifier) {
		case OPS_S2KS_SIMPLE:
		case OPS_S2KS_SALTED:
		case OPS_S2KS_ITERATED_AND_SALTED:
			break;
		default:
			(void) fprintf(stderr,
				"parse_seckey: bad seckey\n");
			return 0;
		}

		if (!limited_read(c, 1, region, pinfo)) {
			return 0;
		}
		pkt.u.seckey.hash_algorithm = c[0];

		if (pkt.u.seckey.s2k_specifier != OPS_S2KS_SIMPLE &&
		    !limited_read(pkt.u.seckey.salt, 8, region,
		    		pinfo)) {
			return 0;
		}
		if (pkt.u.seckey.s2k_specifier ==
					OPS_S2KS_ITERATED_AND_SALTED) {
			if (!limited_read(c, 1, region, pinfo)) {
				return 0;
			}
			pkt.u.seckey.octet_count =
				(16 + ((unsigned)c[0] & 15)) <<
						(((unsigned)c[0] >> 4) + 6);
		}
	} else if (pkt.u.seckey.s2k_usage != OPS_S2KU_NONE) {
		/* this is V3 style, looks just like a V4 simple hash */
		pkt.u.seckey.algorithm = c[0];
		pkt.u.seckey.s2k_usage = OPS_S2KU_ENCRYPTED;
		pkt.u.seckey.s2k_specifier = OPS_S2KS_SIMPLE;
		pkt.u.seckey.hash_algorithm = OPS_HASH_MD5;
	}
	crypted = pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED ||
		pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED;

	if (crypted) {
		int             n;
		__ops_packet_t seckey;
		char           *passphrase;
		unsigned char   key[OPS_MAX_KEY_SIZE + OPS_MAX_HASH_SIZE];
		__ops_hash_t	hashes[(OPS_MAX_KEY_SIZE + OPS_MIN_HASH_SIZE - 1) / OPS_MIN_HASH_SIZE];
		size_t          passlen;
		int             keysize;
		int             hashsize;

		blocksize = __ops_block_size(pkt.u.seckey.algorithm);
		if (blocksize == 0 || blocksize > OPS_MAX_BLOCK_SIZE) {
			(void) fprintf(stderr,
				"parse_seckey: bad blocksize\n");
			return 0;
		}

		if (!limited_read(pkt.u.seckey.iv, blocksize, region,
				pinfo)) {
			return 0;
		}
		(void) memset(&seckey, 0x0, sizeof(seckey));
		passphrase = NULL;
		seckey.u.skey_passphrase.passphrase = &passphrase;
		seckey.u.skey_passphrase.seckey =
				&pkt.u.seckey;
		CALLBACK(&pinfo->cbinfo, OPS_PARSER_CMD_GET_SK_PASSPHRASE,
				&seckey);
		if (!passphrase) {
			if (__ops_get_debug_level(__FILE__)) {
				/* \todo make into proper error */
				(void) fprintf(stderr,
				"parse_seckey: can't get passphrase\n");
			}
			if (!consume_packet(region, pinfo, false)) {
				return 0;
			}

			CALLBACK(&pinfo->cbinfo,
				OPS_PTAG_CT_ENCRYPTED_SECRET_KEY, &pkt);

			return 1;
		}
		keysize = __ops_key_size(pkt.u.seckey.algorithm);
		if (keysize == 0 || keysize > OPS_MAX_KEY_SIZE) {
			(void) fprintf(stderr,
				"parse_seckey: bad keysize\n");
			return 0;
		}

		hashsize = __ops_hash_size(pkt.u.seckey.hash_algorithm);
		if (hashsize == 0 || hashsize > OPS_MAX_HASH_SIZE) {
			(void) fprintf(stderr,
				"parse_seckey: bad hashsize\n");
			return 0;
		}

		for (n = 0; n * hashsize < keysize; ++n) {
			int             i;

			__ops_hash_any(&hashes[n],
				pkt.u.seckey.hash_algorithm);
			hashes[n].init(&hashes[n]);
			/* preload hashes with zeroes... */
			for (i = 0; i < n; ++i) {
				hashes[n].add(&hashes[n],
					(const unsigned char *) "", 1);
			}
		}

		passlen = strlen(passphrase);

		for (n = 0; n * hashsize < keysize; ++n) {
			unsigned        i;

			switch (pkt.u.seckey.s2k_specifier) {
			case OPS_S2KS_SALTED:
				hashes[n].add(&hashes[n],
					pkt.u.seckey.salt,
					OPS_SALT_SIZE);
				/* FALLTHROUGH */
			case OPS_S2KS_SIMPLE:
				hashes[n].add(&hashes[n],
					(unsigned char *) passphrase, passlen);
				break;

			case OPS_S2KS_ITERATED_AND_SALTED:
				for (i = 0; i < pkt.u.seckey.octet_count; i += passlen + OPS_SALT_SIZE) {
					unsigned	j = passlen + OPS_SALT_SIZE;

					if (i + j > pkt.u.seckey.octet_count && i != 0) {
						j = pkt.u.seckey.octet_count - i;
					}
					hashes[n].add(&hashes[n],
						pkt.u.seckey.salt,
						(unsigned)(j > OPS_SALT_SIZE) ?
							OPS_SALT_SIZE : j);
					if (j > OPS_SALT_SIZE) {
						hashes[n].add(&hashes[n],
						(unsigned char *) passphrase,
						j - OPS_SALT_SIZE);
					}
				}

			}
		}

		for (n = 0; n * hashsize < keysize; ++n) {
			int	r;

			r = hashes[n].finish(&hashes[n], key + n * hashsize);
			if (r != hashsize) {
				(void) fprintf(stderr,
					"parse_seckey: bad r\n");
				return 0;
			}
		}

		(void) memset(passphrase, 0x0, passlen);
		(void) free(passphrase);

		__ops_crypt_any(&decrypt, pkt.u.seckey.algorithm);
		if (__ops_get_debug_level(__FILE__)) {
			unsigned int    i = 0;
			fprintf(stderr, "\nREADING:\niv=");
			for (i = 0; i < __ops_block_size(pkt.u.seckey.algorithm); i++) {
				fprintf(stderr, "%02x ", pkt.u.seckey.iv[i]);
			}
			fprintf(stderr, "\n");
			fprintf(stderr, "key=");
			for (i = 0; i < CAST_KEY_LENGTH; i++) {
				fprintf(stderr, "%02x ", key[i]);
			}
			fprintf(stderr, "\n");
		}
		decrypt.set_iv(&decrypt, pkt.u.seckey.iv);
		decrypt.set_key(&decrypt, key);

		/* now read encrypted data */

		__ops_reader_push_decrypt(pinfo, &decrypt, region);

		/*
		 * Since all known encryption for PGP doesn't compress, we
		 * can limit to the same length as the current region (for
		 * now).
		 */
		__ops_init_subregion(&encregion, NULL);
		encregion.length = region->length - region->length_read;
		if (pkt.u.seckey.pubkey.version != OPS_V4) {
			encregion.length -= 2;
		}
		saved_region = region;
		region = &encregion;
	}
	if (pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED) {
		__ops_hash_sha1(&checkhash);
		__ops_reader_push_hash(pinfo, &checkhash);
	} else {
		__ops_reader_push_sum16(pinfo);
	}

	switch (pkt.u.seckey.pubkey.algorithm) {
	case OPS_PKA_RSA:
	case OPS_PKA_RSA_ENCRYPT_ONLY:
	case OPS_PKA_RSA_SIGN_ONLY:
		if (!limited_read_mpi(&pkt.u.seckey.key.rsa.d, region,
					pinfo) ||
		    !limited_read_mpi(&pkt.u.seckey.key.rsa.p, region,
		    			pinfo) ||
		    !limited_read_mpi(&pkt.u.seckey.key.rsa.q, region,
		    			pinfo) ||
		    !limited_read_mpi(&pkt.u.seckey.key.rsa.u, region,
		    			pinfo)) {
			ret = 0;
		}
		break;

	case OPS_PKA_DSA:

		if (!limited_read_mpi(&pkt.u.seckey.key.dsa.x, region,
				pinfo)) {
			ret = 0;
		}
		break;

	default:
		OPS_ERROR_2(&pinfo->errors,
			OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG,
			"Unsupported Public Key algorithm %d (%s)",
			pkt.u.seckey.pubkey.algorithm,
			__ops_show_pka(pkt.u.seckey.pubkey.algorithm));
		ret = 0;
	}

	if (__ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr, "4 MPIs read\n");
	}
	pinfo->reading_v3_secret = false;

	if (pkt.u.seckey.s2k_usage == OPS_S2KU_ENCRYPTED_AND_HASHED) {
		unsigned char   hash[20];

		__ops_reader_pop_hash(pinfo);
		checkhash.finish(&checkhash, hash);

		if (crypted &&
		    pkt.u.seckey.pubkey.version != OPS_V4) {
			__ops_reader_pop_decrypt(pinfo);
			region = saved_region;
		}
		if (ret) {
			if (!limited_read(pkt.u.seckey.checkhash,
				20, region, pinfo)) {
				return 0;
			}

			if (memcmp(hash, pkt.u.seckey.checkhash, 20)) {
				ERRP(&pinfo->cbinfo, pkt,
					"Hash mismatch in secret key");
			}
		}
	} else {
		unsigned short  sum;

		sum = __ops_reader_pop_sum16(pinfo);
		if (crypted &&
		    pkt.u.seckey.pubkey.version != OPS_V4) {
			__ops_reader_pop_decrypt(pinfo);
			region = saved_region;
		}
		if (ret) {
			if (!limited_read_scalar(&pkt.u.seckey.checksum, 2, region,
						 pinfo))
				return 0;

			if (sum != pkt.u.seckey.checksum)
				ERRP(&pinfo->cbinfo, pkt, "Checksum mismatch in secret key");
		}
	}

	if (crypted && pkt.u.seckey.pubkey.version == OPS_V4) {
		__ops_reader_pop_decrypt(pinfo);
	}
	if (ret && region->length_read != region->length) {
		(void) fprintf(stderr, "parse_seckey: bad length\n");
		return 0;
	}

	if (!ret)
		return 0;

	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_SECRET_KEY, &pkt);

	if (__ops_get_debug_level(__FILE__)) {
		fprintf(stderr, "--- end of parse_seckey\n\n");
	}
	return 1;
}

/**
   \ingroup Core_ReadPackets
   \brief Parse a Public Key Session Key packet
*/
static int 
parse_pk_session_key(__ops_region_t * region,
		     __ops_parse_info_t * pinfo)
{
	const __ops_seckey_t	*secret;
	__ops_packet_t		 sesskey;
	__ops_packet_t		 pkt;
	unsigned char			*iv;
	unsigned char		   	 c[1] = "";
	unsigned char			 cs[2];
	unsigned			 k;
	BIGNUM				*enc_m;
	int				 n;

	/* Can't rely on it being CAST5 */
	/* \todo FIXME RW */
	/* const size_t sz_unencoded_m_buf=CAST_KEY_LENGTH+1+2; */
	unsigned char   unencoded_m_buf[1024];

	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.pk_session_key.version = c[0];
	if (pkt.u.pk_session_key.version != OPS_PKSK_V3) {
		OPS_ERROR_1(&pinfo->errors, OPS_E_PROTO_BAD_PKSK_VRSN,
			"Bad public-key encrypted session key version (%d)",
			    pkt.u.pk_session_key.version);
		return 0;
	}
	if (!limited_read(pkt.u.pk_session_key.key_id,
			  sizeof(pkt.u.pk_session_key.key_id), region, pinfo)) {
		return 0;
	}
	if (__ops_get_debug_level(__FILE__)) {
		int             i;
		int             x = sizeof(pkt.u.pk_session_key.key_id);
		printf("session key: public key id: x=%d\n", x);
		for (i = 0; i < x; i++)
			printf("%2x ", pkt.u.pk_session_key.key_id[i]);
		printf("\n");
	}
	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.pk_session_key.algorithm = c[0];
	switch (pkt.u.pk_session_key.algorithm) {
	case OPS_PKA_RSA:
		if (!limited_read_mpi(&pkt.u.pk_session_key.parameters.rsa.encrypted_m,
				      region, pinfo)) {
			return 0;
		}
		enc_m = pkt.u.pk_session_key.parameters.rsa.encrypted_m;
		break;

	case OPS_PKA_ELGAMAL:
		if (!limited_read_mpi(&pkt.u.pk_session_key.parameters.elgamal.g_to_k,
				      region, pinfo)
		    || !limited_read_mpi(&pkt.u.pk_session_key.parameters.elgamal.encrypted_m,
					 region, pinfo)) {
			return 0;
		}
		enc_m = pkt.u.pk_session_key.parameters.elgamal.encrypted_m;
		break;

	default:
		OPS_ERROR_1(&pinfo->errors, OPS_E_ALG_UNSUPPORTED_PUBLIC_KEY_ALG,
			 "Unknown public key algorithm in session key (%s)",
			    __ops_show_pka(pkt.u.pk_session_key.algorithm));
		return 0;
	}

	(void) memset(&sesskey, 0x0, sizeof(sesskey));
	secret = NULL;
	sesskey.u.get_seckey.seckey = &secret;
	sesskey.u.get_seckey.pk_session_key = &pkt.u.pk_session_key;

	CALLBACK(&pinfo->cbinfo, OPS_PARSER_CMD_GET_SECRET_KEY, &sesskey);

	if (!secret) {
		CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_ENCRYPTED_PK_SESSION_KEY, &pkt);

		return 1;
	}
	/* n=__ops_decrypt_mpi(buf,sizeof(buf),enc_m,secret); */
	n = __ops_decrypt_and_unencode_mpi(unencoded_m_buf,
		sizeof(unencoded_m_buf), enc_m, secret);

	if (n < 1) {
		ERRP(&pinfo->cbinfo, pkt, "decrypted message too short");
		return 0;
	}
	/* PKA */
	pkt.u.pk_session_key.symmetric_algorithm = unencoded_m_buf[0];

	if (!__ops_is_sa_supported(pkt.u.pk_session_key.symmetric_algorithm)) {
		/* ERR1P */
		OPS_ERROR_1(&pinfo->errors, OPS_E_ALG_UNSUPPORTED_SYMMETRIC_ALG,
			    "Symmetric algorithm %s not supported",
			    __ops_show_symmetric_algorithm(pkt.u.pk_session_key.symmetric_algorithm));
		return 0;
	}
	k = __ops_key_size(pkt.u.pk_session_key.symmetric_algorithm);

	if ((unsigned) n != k + 3) {
		OPS_ERROR_2(&pinfo->errors, OPS_E_PROTO_DECRYPTED_MSG_WRONG_LEN,
		      "decrypted message wrong length (got %d expected %d)",
			    n, k + 3);
		return 0;
	}
	if (k > sizeof(pkt.u.pk_session_key.key)) {
		(void) fprintf(stderr, "parse_pk_session_key: bad keylength\n");
		return 0;
	}

	(void) memcpy(pkt.u.pk_session_key.key, unencoded_m_buf + 1, k);

	if (__ops_get_debug_level(__FILE__)) {
		unsigned int    j;
		printf("session key recovered (len=%d):\n", k);
		for (j = 0; j < k; j++)
			printf("%2x ", pkt.u.pk_session_key.key[j]);
		printf("\n");
	}
	pkt.u.pk_session_key.checksum = unencoded_m_buf[k + 1] + (unencoded_m_buf[k + 2] << 8);
	if (__ops_get_debug_level(__FILE__)) {
		printf("session key checksum: %2x %2x\n", unencoded_m_buf[k + 1], unencoded_m_buf[k + 2]);
	}
	/* Check checksum */

	__ops_calc_session_key_checksum(&pkt.u.pk_session_key, &cs[0]);
	if (unencoded_m_buf[k + 1] != cs[0] || unencoded_m_buf[k + 2] != cs[1]) {
		OPS_ERROR_4(&pinfo->errors, OPS_E_PROTO_BAD_SK_CHECKSUM,
		"Session key checksum wrong: expected %2x %2x, got %2x %2x",
			    cs[0], cs[1], unencoded_m_buf[k + 1], unencoded_m_buf[k + 2]);
		return 0;
	}
	/* all is well */
	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_PK_SESSION_KEY, &pkt);

	__ops_crypt_any(&pinfo->decrypt, pkt.u.pk_session_key.symmetric_algorithm);
	iv = calloc(1, pinfo->decrypt.blocksize);
	pinfo->decrypt.set_iv(&pinfo->decrypt, iv);
	pinfo->decrypt.set_key(&pinfo->decrypt, pkt.u.pk_session_key.key);
	__ops_encrypt_init(&pinfo->decrypt);
	(void) free(iv);
	return 1;
}

static int 
__ops_decrypt_se_data(__ops_content_tag_t tag, __ops_region_t * region,
		    __ops_parse_info_t * pinfo)
{
	int             r = 1;
	__ops_crypt_t    *decrypt = __ops_parse_get_decrypt(pinfo);

	if (decrypt) {
		unsigned char   buf[OPS_MAX_BLOCK_SIZE + 2] = "";
		size_t          b = decrypt->blocksize;
		/* __ops_packet_t pkt; */
		__ops_region_t    encregion;


		__ops_reader_push_decrypt(pinfo, decrypt, region);

		__ops_init_subregion(&encregion, NULL);
		encregion.length = b + 2;

		if (!exact_limited_read(buf, b + 2, &encregion, pinfo))
			return 0;

		if (buf[b - 2] != buf[b] || buf[b - 1] != buf[b + 1]) {
			__ops_reader_pop_decrypt(pinfo);
			OPS_ERROR_4(&pinfo->errors, OPS_E_PROTO_BAD_SYMMETRIC_DECRYPT,
			     "Bad symmetric decrypt (%02x%02x vs %02x%02x)",
				buf[b - 2], buf[b - 1], buf[b], buf[b + 1]);
			return 0;
		}
		if (tag == OPS_PTAG_CT_SE_DATA_BODY) {
			decrypt->decrypt_resync(decrypt);
			decrypt->block_encrypt(decrypt, decrypt->civ, decrypt->civ);
		}
		r = __ops_parse(pinfo, 0);

		__ops_reader_pop_decrypt(pinfo);
	} else {
		__ops_packet_t pkt;

		while (region->length_read < region->length) {
			unsigned        len = region->length - region->length_read;

			if (len > sizeof(pkt.u.se_data_body.data))
				len = sizeof(pkt.u.se_data_body.data);

			if (!limited_read(pkt.u.se_data_body.data, len, region, pinfo))
				return 0;

			pkt.u.se_data_body.length = len;

			CALLBACK(&pinfo->cbinfo, tag, &pkt);
		}
	}

	return r;
}

static int 
__ops_decrypt_se_ip_data(__ops_content_tag_t tag, __ops_region_t * region,
		       __ops_parse_info_t * pinfo)
{
	int             r = 1;
	__ops_crypt_t    *decrypt = __ops_parse_get_decrypt(pinfo);

	if (decrypt) {
		__ops_reader_push_decrypt(pinfo, decrypt, region);
		__ops_reader_push_se_ip_data(pinfo, decrypt, region);

		r = __ops_parse(pinfo, 0);

		__ops_reader_pop_se_ip_data(pinfo);
		__ops_reader_pop_decrypt(pinfo);
	} else {
		__ops_packet_t pkt;

		while (region->length_read < region->length) {
			unsigned        len = region->length - region->length_read;

			if (len > sizeof(pkt.u.se_data_body.data)) {
				len = sizeof(pkt.u.se_data_body.data);
			}

			if (!limited_read(pkt.u.se_data_body.data,
					len, region, pinfo)) {
				return 0;
			}

			pkt.u.se_data_body.length = len;

			CALLBACK(&pinfo->cbinfo, tag, &pkt);
		}
	}

	return r;
}

/**
   \ingroup Core_ReadPackets
   \brief Read a Symmetrically Encrypted packet
*/
static int 
parse_se_data(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	__ops_packet_t pkt;

	/* there's no info to go with this, so just announce it */
	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_SE_DATA_HEADER, &pkt);

	/*
	 * The content of an encrypted data packet is more OpenPGP packets
	 * once decrypted, so recursively handle them
	 */
	return __ops_decrypt_se_data(OPS_PTAG_CT_SE_DATA_BODY, region, pinfo);
}

/**
   \ingroup Core_ReadPackets
   \brief Read a Symmetrically Encrypted Integrity Protected packet
*/
static int 
parse_se_ip_data(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	unsigned char   c[1] = "";
	__ops_packet_t pkt;

	if (!limited_read(c, 1, region, pinfo))
		return 0;
	pkt.u.se_ip_data_header.version = c[0];

	if (pkt.u.se_ip_data_header.version != OPS_SE_IP_V1) {
		(void) fprintf(stderr, "parse_se_ip_data: bad version\n");
		return 0;
	}

	/*
	 * The content of an encrypted data packet is more OpenPGP packets
	 * once decrypted, so recursively handle them
	 */
	return __ops_decrypt_se_ip_data(OPS_PTAG_CT_SE_IP_DATA_BODY, region, pinfo);
}

/**
   \ingroup Core_ReadPackets
   \brief Read a MDC packet
*/
static int 
parse_mdc(__ops_region_t * region, __ops_parse_info_t * pinfo)
{
	__ops_packet_t pkt;

	if (!limited_read((unsigned char *)(void *)&pkt.u.mdc,
			OPS_SHA1_HASH_SIZE, region, pinfo))
		return 0;

	CALLBACK(&pinfo->cbinfo, OPS_PTAG_CT_MDC, &pkt);

	return 1;
}

/**
 * \ingroup Core_ReadPackets
 * \brief Parse one packet.
 *
 * This function parses the packet tag.  It computes the value of the
 * content tag and then calls the appropriate function to handle the
 * content.
 *
 * \param *pinfo	How to parse
 * \param *pktlen	On return, will contain number of bytes in packet
 * \return 1 on success, 0 on error, -1 on EOF */
static int 
__ops_parse_packet(__ops_parse_info_t * pinfo, unsigned long *pktlen)
{
	__ops_packet_t	pkt;
	__ops_region_t		region;
	unsigned char		ptag[1];
	bool			indeterminate = false;
	int			ret;

	pkt.u.ptag.position = pinfo->rinfo.position;

	ret = base_read(ptag, 1, pinfo);

	if (__ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr,
			"__ops_parse_packet: base_read returned %d\n",
			ret);
	}

	/* errors in the base read are effectively EOF. */
	if (ret <= 0) {
		return -1;
	}

	*pktlen = 0;

	if (!(*ptag & OPS_PTAG_ALWAYS_SET)) {
		pkt.u.error.error = "Format error (ptag bit not set)";
		CALLBACK(&pinfo->cbinfo, OPS_PARSER_ERROR, &pkt);
		return 0;
	}
	pkt.u.ptag.new_format = !!(*ptag & OPS_PTAG_NEW_FORMAT);
	if (pkt.u.ptag.new_format) {
		pkt.u.ptag.type =
			*ptag & OPS_PTAG_NF_CONTENT_TAG_MASK;
		pkt.u.ptag.length_type = 0;
		if (!read_new_length(&pkt.u.ptag.length, pinfo)) {
			return 0;
		}
	} else {
		bool   rb;

		rb = false;
		pkt.u.ptag.type = ((unsigned)*ptag &
				OPS_PTAG_OF_CONTENT_TAG_MASK)
			>> OPS_PTAG_OF_CONTENT_TAG_SHIFT;
		pkt.u.ptag.length_type =
			*ptag & OPS_PTAG_OF_LENGTH_TYPE_MASK;
		switch (pkt.u.ptag.length_type) {
		case OPS_PTAG_OLD_LEN_1:
			rb = _read_scalar(&pkt.u.ptag.length, 1, pinfo);
			break;

		case OPS_PTAG_OLD_LEN_2:
			rb = _read_scalar(&pkt.u.ptag.length, 2, pinfo);
			break;

		case OPS_PTAG_OLD_LEN_4:
			rb = _read_scalar(&pkt.u.ptag.length, 4, pinfo);
			break;

		case OPS_PTAG_OLD_LEN_INDETERMINATE:
			pkt.u.ptag.length = 0;
			indeterminate = true;
			rb = true;
			break;
		}
		if (!rb) {
			return 0;
		}
	}

	CALLBACK(&pinfo->cbinfo, OPS_PARSER_PTAG, &pkt);

	__ops_init_subregion(&region, NULL);
	region.length = pkt.u.ptag.length;
	region.indeterminate = indeterminate;
	if (__ops_get_debug_level(__FILE__)) {
		(void) fprintf(stderr, "__ops_parse_packet: type %d\n",
			       pkt.u.ptag.type);
	}
	switch (pkt.u.ptag.type) {
	case OPS_PTAG_CT_SIGNATURE:
		ret = parse_sig(&region, pinfo);
		break;

	case OPS_PTAG_CT_PUBLIC_KEY:
	case OPS_PTAG_CT_PUBLIC_SUBKEY:
		ret = parse_pubkey(pkt.u.ptag.type, &region,
				pinfo);
		break;

	case OPS_PTAG_CT_TRUST:
		ret = parse_trust(&region, pinfo);
		break;

	case OPS_PTAG_CT_USER_ID:
		ret = parse_user_id(&region, pinfo);
		break;

	case OPS_PTAG_CT_COMPRESSED:
		ret = parse_compressed(&region, pinfo);
		break;

	case OPS_PTAG_CT_ONE_PASS_SIGNATURE:
		ret = parse_one_pass(&region, pinfo);
		break;

	case OPS_PTAG_CT_LITERAL_DATA:
		ret = parse_litdata(&region, pinfo);
		break;

	case OPS_PTAG_CT_USER_ATTRIBUTE:
		ret = parse_user_attribute(&region, pinfo);
		break;

	case OPS_PTAG_CT_SECRET_KEY:
		ret = parse_seckey(&region, pinfo);
		break;

	case OPS_PTAG_CT_SECRET_SUBKEY:
		ret = parse_seckey(&region, pinfo);
		break;

	case OPS_PTAG_CT_PK_SESSION_KEY:
		ret = parse_pk_session_key(&region, pinfo);
		break;

	case OPS_PTAG_CT_SE_DATA:
		ret = parse_se_data(&region, pinfo);
		break;

	case OPS_PTAG_CT_SE_IP_DATA:
		ret = parse_se_ip_data(&region, pinfo);
		break;

	case OPS_PTAG_CT_MDC:
		ret = parse_mdc(&region, pinfo);
		break;

	default:
		OPS_ERROR_1(&pinfo->errors, OPS_E_P_UNKNOWN_TAG,
			    "Unknown content tag 0x%x",
			    pkt.u.ptag.type);
		ret = 0;
	}

	/* Ensure that the entire packet has been consumed */

	if (region.length != region.length_read && !region.indeterminate) {
		if (!consume_packet(&region, pinfo, false)) {
			ret = -1;
		}
	}

	/* also consume it if there's been an error? */
	/* \todo decide what to do about an error on an */
	/* indeterminate packet */
	if (ret == 0) {
		if (!consume_packet(&region, pinfo, false)) {
			ret = -1;
		}
	}
	/* set pktlen */

	*pktlen = pinfo->rinfo.alength;

	/* do callback on entire packet, if desired and there was no error */

	if (ret > 0 && pinfo->rinfo.accumulate) {
		pkt.u.packet.length = pinfo->rinfo.alength;
		pkt.u.packet.raw = pinfo->rinfo.accumulated;
		pinfo->rinfo.accumulated = NULL;
		pinfo->rinfo.asize = 0;
		CALLBACK(&pinfo->cbinfo, OPS_PARSER_PACKET_END, &pkt);
	}
	pinfo->rinfo.alength = 0;

	return (ret < 0) ? -1 : (ret) ? 1 : 0;
}

/**
 * \ingroup Core_ReadPackets
 *
 * \brief Parse packets from an input stream until EOF or error.
 *
 * \details Setup the necessary parsing configuration in "pinfo"
 * before calling __ops_parse().
 *
 * That information includes :
 *
 * - a "reader" function to be used to get the data to be parsed
 *
 * - a "callback" function to be called when this library has identified
 * a parseable object within the data
 *
 * - whether the calling function wants the signature subpackets
 * returned raw, parsed or not at all.
 *
 * After returning, pinfo->errors holds any errors encountered while parsing.
 *
 * \param pinfo	Parsing configuration
 * \return		1 on success in all packets, 0 on error in any packet
 *
 * \sa CoreAPI Overview
 *
 * \sa __ops_print_errors()
 *
 */

int 
__ops_parse(__ops_parse_info_t *pinfo, int perrors)
{
	unsigned long   pktlen;
	int             r;

	if (pinfo->synthlit) {
#if 0
		r = __ops_parse_packet(pinfo->synthsig, &pktlen);
		r = __ops_parse_packet(pinfo->synthlit, &pktlen);
#endif
	}
	do {
		r = __ops_parse_packet(pinfo, &pktlen);
	} while (r != -1);
	if (perrors) {
		__ops_print_errors(pinfo->errors);
	}
	return (pinfo->errors == NULL);
}

/**
 * \ingroup Core_ReadPackets
 *
 * \brief Specifies whether one or more signature
 * subpacket types should be returned parsed; or raw; or ignored.
 *
 * \param	pinfo	Pointer to previously allocated structure
 * \param	tag	Packet tag. OPS_PTAG_SS_ALL for all SS tags; or one individual signature subpacket tag
 * \param	type	Parse type
 * \todo Make all packet types optional, not just subpackets */
void 
__ops_parse_options(__ops_parse_info_t * pinfo,
		  __ops_content_tag_t tag,
		  __ops_parse_type_t type)
{
	int             t8, t7;

	if (tag == OPS_PTAG_SS_ALL) {
		int             n;

		for (n = 0; n < 256; ++n)
			__ops_parse_options(pinfo, OPS_PTAG_SIGNATURE_SUBPACKET_BASE + n,
					  type);
		return;
	}
	if (tag < OPS_PTAG_SIGNATURE_SUBPACKET_BASE
	       || tag > OPS_PTAG_SIGNATURE_SUBPACKET_BASE + NTAGS - 1) {
		(void) fprintf(stderr, "__ops_parse_options: bad tag\n");
		return;
	}
	t8 = (tag - OPS_PTAG_SIGNATURE_SUBPACKET_BASE) / 8;
	t7 = 1 << ((tag - OPS_PTAG_SIGNATURE_SUBPACKET_BASE) & 7);
	switch (type) {
	case OPS_PARSE_RAW:
		pinfo->ss_raw[t8] |= t7;
		pinfo->ss_parsed[t8] &= ~t7;
		break;

	case OPS_PARSE_PARSED:
		pinfo->ss_raw[t8] &= ~t7;
		pinfo->ss_parsed[t8] |= t7;
		break;

	case OPS_PARSE_IGNORE:
		pinfo->ss_raw[t8] &= ~t7;
		pinfo->ss_parsed[t8] &= ~t7;
		break;
	}
}

/**
\ingroup Core_ReadPackets
\brief Creates a new zero-ed __ops_parse_info_t struct
\sa __ops_parse_info_delete()
*/
__ops_parse_info_t *
__ops_parse_info_new(void)
{
	return calloc(1, sizeof(__ops_parse_info_t));
}

/**
\ingroup Core_ReadPackets
\brief Free __ops_parse_info_t struct and its contents
\sa __ops_parse_info_new()
*/
void 
__ops_parse_info_delete(__ops_parse_info_t * pinfo)
{
	__ops_callback_data_t *cbinfo, *next;

	for (cbinfo = pinfo->cbinfo.next; cbinfo; cbinfo = next) {
		next = cbinfo->next;
		free(cbinfo);
	}
	if (pinfo->rinfo.destroyer)
		pinfo->rinfo.destroyer(&pinfo->rinfo);
	__ops_free_errors(pinfo->errors);
	if (pinfo->rinfo.accumulated)
		free(pinfo->rinfo.accumulated);
	free(pinfo);
}

/**
\ingroup Core_ReadPackets
\brief Returns the parse_info's reader_info
\return Pointer to the reader_info inside the parse_info
*/
__ops_reader_info_t *
__ops_parse_get_rinfo(__ops_parse_info_t * pinfo)
{
	return &pinfo->rinfo;
}

/**
\ingroup Core_ReadPackets
\brief Sets the parse_info's callback
This is used when adding the first callback in a stack of callbacks.
\sa __ops_parse_cb_push()
*/

void 
__ops_parse_cb_set(__ops_parse_info_t * pinfo, __ops_parse_cb_t * cb, void *arg)
{
	pinfo->cbinfo.cb = cb;
	pinfo->cbinfo.arg = arg;
	pinfo->cbinfo.errors = &pinfo->errors;
}

/**
\ingroup Core_ReadPackets
\brief Adds a further callback to a stack of callbacks
\sa __ops_parse_cb_set()
*/
void 
__ops_parse_cb_push(__ops_parse_info_t * pinfo, __ops_parse_cb_t * cb, void *arg)
{
	__ops_callback_data_t *cbinfo = calloc(1, sizeof(*cbinfo));

	*cbinfo = pinfo->cbinfo;
	pinfo->cbinfo.next = cbinfo;
	__ops_parse_cb_set(pinfo, cb, arg);
}

/**
\ingroup Core_ReadPackets
\brief Returns callback's arg
*/
void           *
__ops_parse_cb_get_arg(__ops_callback_data_t * cbinfo)
{
	return cbinfo->arg;
}

/**
\ingroup Core_ReadPackets
\brief Returns callback's errors
*/
void           *
__ops_parse_cb_get_errors(__ops_callback_data_t * cbinfo)
{
	return cbinfo->errors;
}

/**
\ingroup Core_ReadPackets
\brief Calls the parse_cb_info's callback if present
\return Return value from callback, if present; else OPS_FINISHED
*/
__ops_parse_cb_return_t 
__ops_parse_cb(const __ops_packet_t * pkt,
	     __ops_callback_data_t * cbinfo)
{
	if (cbinfo->cb)
		return cbinfo->cb(pkt, cbinfo);
	else
		return OPS_FINISHED;
}

/**
\ingroup Core_ReadPackets
\brief Calls the next callback  in the stack
\return Return value from callback
*/
__ops_parse_cb_return_t 
__ops_parse_stacked_cb(const __ops_packet_t * pkt,
		     __ops_callback_data_t * cbinfo)
{
	return __ops_parse_cb(pkt, cbinfo->next);
}

/**
\ingroup Core_ReadPackets
\brief Returns the parse_info's errors
\return parse_info's errors
*/
__ops_error_t    *
__ops_parse_info_get_errors(__ops_parse_info_t * pinfo)
{
	return pinfo->errors;
}

__ops_crypt_t    *
__ops_parse_get_decrypt(__ops_parse_info_t * pinfo)
{
	if (pinfo->decrypt.algorithm)
		return &pinfo->decrypt;
	return NULL;
}
