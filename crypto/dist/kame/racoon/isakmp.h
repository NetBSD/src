/*	$KAME: isakmp.h,v 1.18 2001/03/26 17:27:40 thorpej Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* refer to RFC 2408 */

/* must include <netinet/in.h> first. */
/* must include "isakmp_var.h" first. */

#define INITIATOR	0	/* synonym sender */
#define RESPONDER	1	/* synonym receiver */

#define GENERATE	1
#define VALIDATE	0

/* 3.1 ISAKMP Header Format
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        !                          Initiator                            !
        !                            Cookie                             !
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        !                          Responder                            !
        !                            Cookie                             !
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        !  Next Payload ! MjVer ! MnVer ! Exchange Type !     Flags     !
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        !                          Message ID                           !
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        !                            Length                             !
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct isakmp {
	cookie_t i_ck;		/* Initiator Cookie */
	cookie_t r_ck;		/* Responder Cookie */
	u_int8_t np;		/* Next Payload Type */
	u_int8_t v;
	u_int8_t etype;		/* Exchange Type */
	u_int8_t flags;		/* Flags */
	u_int32_t msgid;
	u_int32_t len;		/* Length */
} __attribute__((__packed__));

/* Next Payload Type */
#define ISAKMP_NPTYPE_NONE	0	/* NONE*/
#define ISAKMP_NPTYPE_SA	1	/* Security Association */
#define ISAKMP_NPTYPE_P		2	/* Proposal */
#define ISAKMP_NPTYPE_T		3	/* Transform */
#define ISAKMP_NPTYPE_KE	4	/* Key Exchange */
#define ISAKMP_NPTYPE_ID	5	/* Identification */
#define ISAKMP_NPTYPE_CERT	6	/* Certificate */
#define ISAKMP_NPTYPE_CR	7	/* Certificate Request */
#define ISAKMP_NPTYPE_HASH	8	/* Hash */
#define ISAKMP_NPTYPE_SIG	9	/* Signature */
#define ISAKMP_NPTYPE_NONCE	10	/* Nonce */
#define ISAKMP_NPTYPE_N		11	/* Notification */
#define ISAKMP_NPTYPE_D		12	/* Delete */
#define ISAKMP_NPTYPE_VID	13	/* Vendor ID */
#define ISAKMP_NPTYPE_MAX	14
			/*	128 - 255 Private Use */

/*
 * The following are valid when the Vendor ID is one of the
 * following:
 *
 *	MD5("A GSS-API Authentication Method for IKE")
 *	MD5("GSSAPI") (recognized by Windows 2000)
 *	MD5("MS NT5 ISAKMPOAKLEY") (sent by Windows 2000)
 *
 * See draft-ietf-ipsec-isakmp-gss-auth-06.txt.
 */
#define ISAKMP_NPTYPE_GSS	129	/* GSS token */

#define ISAKMP_MAJOR_VERSION	1
#define ISAKMP_MINOR_VERSION	0
#define ISAKMP_VERSION_NUMBER	0x10
#define ISAKMP_GETMAJORV(v)	(((v) & 0xf0) >> 4)
#define ISAKMP_SETMAJORV(v, m)	((v) = ((v) & 0x0f) | (((m) << 4) & 0xf0))
#define ISAKMP_GETMINORV(v)	((v) & 0x0f)
#define ISAKMP_SETMINORV(v, m)	((v) = ((v) & 0xf0) | ((m) & 0x0f))

/* Exchange Type */
#define ISAKMP_ETYPE_NONE	0	/* NONE */
#define ISAKMP_ETYPE_BASE	1	/* Base */
#define ISAKMP_ETYPE_IDENT	2	/* Identity Proteciton */
#define ISAKMP_ETYPE_AUTH	3	/* Authentication Only */
#define ISAKMP_ETYPE_AGG	4	/* Aggressive */
#define ISAKMP_ETYPE_INFO	5	/* Informational */
/* Additional Exchange Type */
#define ISAKMP_ETYPE_QUICK	32	/* Quick Mode */
#define ISAKMP_ETYPE_NEWGRP	33	/* New group Mode */
#define ISAKMP_ETYPE_ACKINFO	34	/* Acknowledged Informational */

/* Flags */
#define ISAKMP_FLAG_E 0x01 /* Encryption Bit */
#define ISAKMP_FLAG_C 0x02 /* Commit Bit */
#define ISAKMP_FLAG_A 0x04 /* Authentication Only Bit */

/* 3.2 Payload Generic Header
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        ! Next Payload  !   RESERVED    !         Payload Length        !
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct isakmp_gen {
	u_int8_t np;		/* Next Payload */
	u_int8_t reserved;	/* RESERVED, unused, must set to 0 */
	u_int16_t len;		/* Payload Length */
} __attribute__((__packed__));

/* 3.3 Data Attributes
         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        !A!       Attribute Type        !    AF=0  Attribute Length     !
        !F!                             !    AF=1  Attribute Value      !
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
        .                   AF=0  Attribute Value                       .
        .                   AF=1  Not Transmitted                       .
        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
*/
struct isakmp_data {
	u_int16_t type;		/* defined by DOI-spec, and Attribute Format */
	u_int16_t lorv;		/* if f equal 1, Attribute Length */
				/* if f equal 0, Attribute Value */
	/* if f equal 1, Attribute Value */
} __attribute__((__packed__));
#define ISAKMP_GEN_TLV 0x0000
#define ISAKMP_GEN_TV  0x8000
	/* mask for type of attribute format */
#define ISAKMP_GEN_MASK 0x8000

#if 0
/* MAY NOT be used, because of being defined in ipsec-doi. */
/* 3.4 Security Association Payload */
struct isakmp_pl_sa {
	struct isakmp_gen h;
	u_int32_t doi;		/* Domain of Interpretation */
	u_int32_t sit;		/* Situation */
} __attribute__((__packed__));
#endif

/* 3.5 Proposal Payload */
	/*
	The value of the next payload field MUST only contain the value "2"
	or "0".  If there are additional Proposal payloads in the message,
	then this field will be 2.  If the current Proposal payload is the
	last within the security association proposal, then this field will
	be 0.
	*/
struct isakmp_pl_p {
	struct isakmp_gen h;
	u_int8_t p_no;		/* Proposal # */
	u_int8_t proto_id;	/* Protocol */
	u_int8_t spi_size;	/* SPI Size */
	u_int8_t num_t;		/* Number of Transforms */
	/* SPI */
} __attribute__((__packed__));

/* 3.6 Transform Payload */
	/*
	The value of the next payload field MUST only contain the value "3"
	or "0".  If there are additional Transform payloads in the proposal,
	then this field will be 3.  If the current Transform payload is the
	last within the proposal, then this field will be 0.
	*/
struct isakmp_pl_t {
	struct isakmp_gen h;
	u_int8_t t_no;		/* Transform # */
	u_int8_t t_id;		/* Transform-Id */
	u_int16_t reserved;	/* RESERVED2 */
	/* SA Attributes */
} __attribute__((__packed__));

/* 3.7 Key Exchange Payload */
struct isakmp_pl_ke {
	struct isakmp_gen h;
	/* Key Exchange Data */
} __attribute__((__packed__));

#if 0
/* NOTE: MUST NOT use because of being defined in ipsec-doi instead them. */
/* 3.8 Identification Payload */
struct isakmp_pl_id {
	struct isakmp_gen h;
	union {
		u_int8_t id_type;	/* ID Type */
		u_int32_t doi_data;	/* DOI Specific ID Data */
	} d;
	/* Identification Data */
} __attribute__((__packed__));
/* A.4 ISAKMP Identification Type Values */
#define ISAKMP_ID_IPV4_ADDR		0
#define ISAKMP_ID_IPV4_ADDR_SUBNET	1
#define ISAKMP_ID_IPV6_ADDR		2
#define ISAKMP_ID_IPV6_ADDR_SUBNET	3
#endif

/* 3.9 Certificate Payload */
struct isakmp_pl_cert {
	struct isakmp_gen h;
	/*
	 * Encoding type of 1 octet follows immediately,
	 * variable length CERT data follows encoding type.
	 */
} __attribute__((__packed__));

/* Certificate Type */
#define ISAKMP_CERT_NONE	0
#define ISAKMP_CERT_PKCS7	1
#define ISAKMP_CERT_PGP		2
#define ISAKMP_CERT_DNS		3
#define ISAKMP_CERT_X509SIGN	4
#define ISAKMP_CERT_X509KE	5
#define ISAKMP_CERT_KERBEROS	6
#define ISAKMP_CERT_CRL		7
#define ISAKMP_CERT_ARL		8
#define ISAKMP_CERT_SPKI	9
#define ISAKMP_CERT_X509ATTR	10

/* 3.10 Certificate Request Payload */
struct isakmp_pl_cr {
	struct isakmp_gen h;
	u_int8_t num_cert; /* # Cert. Types */
	/*
	Certificate Types (variable length)
	  -- Contains a list of the types of certificates requested,
	  sorted in order of preference.  Each individual certificate
	  type is 1 octet.  This field is NOT required.
	*/
	/* # Certificate Authorities (1 octet) */
	/* Certificate Authorities (variable length) */
} __attribute__((__packed__));

/* 3.11 Hash Payload */
struct isakmp_pl_hash {
	struct isakmp_gen h;
	/* Hash Data */
} __attribute__((__packed__));

/* 3.12 Signature Payload */
struct isakmp_pl_sig {
	struct isakmp_gen h;
	/* Signature Data */
} __attribute__((__packed__));

/* 3.13 Nonce Payload */
struct isakmp_pl_nonce {
	struct isakmp_gen h;
	/* Nonce Data */
} __attribute__((__packed__));

/* 3.14 Notification Payload */
struct isakmp_pl_n {
	struct isakmp_gen h;
	u_int32_t doi;		/* Domain of Interpretation */
	u_int8_t proto_id;	/* Protocol-ID */
	u_int8_t spi_size;	/* SPI Size */
	u_int16_t type;		/* Notify Message Type */
	/* SPI */
	/* Notification Data */
} __attribute__((__packed__));

/* 3.14.1 Notify Message Types */
/* NOTIFY MESSAGES - ERROR TYPES */
#define ISAKMP_NTYPE_INVALID_PAYLOAD_TYPE	1
#define ISAKMP_NTYPE_DOI_NOT_SUPPORTED		2
#define ISAKMP_NTYPE_SITUATION_NOT_SUPPORTED	3
#define ISAKMP_NTYPE_INVALID_COOKIE		4
#define ISAKMP_NTYPE_INVALID_MAJOR_VERSION	5
#define ISAKMP_NTYPE_INVALID_MINOR_VERSION	6
#define ISAKMP_NTYPE_INVALID_EXCHANGE_TYPE	7
#define ISAKMP_NTYPE_INVALID_FLAGS		8
#define ISAKMP_NTYPE_INVALID_MESSAGE_ID		9
#define ISAKMP_NTYPE_INVALID_PROTOCOL_ID	10
#define ISAKMP_NTYPE_INVALID_SPI		11
#define ISAKMP_NTYPE_INVALID_TRANSFORM_ID	12
#define ISAKMP_NTYPE_ATTRIBUTES_NOT_SUPPORTED	13
#define ISAKMP_NTYPE_NO_PROPOSAL_CHOSEN		14
#define ISAKMP_NTYPE_BAD_PROPOSAL_SYNTAX	15
#define ISAKMP_NTYPE_PAYLOAD_MALFORMED		16
#define ISAKMP_NTYPE_INVALID_KEY_INFORMATION	17
#define ISAKMP_NTYPE_INVALID_ID_INFORMATION	18
#define ISAKMP_NTYPE_INVALID_CERT_ENCODING	19
#define ISAKMP_NTYPE_INVALID_CERTIFICATE	20
#define ISAKMP_NTYPE_BAD_CERT_REQUEST_SYNTAX	21
#define ISAKMP_NTYPE_INVALID_CERT_AUTHORITY	22
#define ISAKMP_NTYPE_INVALID_HASH_INFORMATION	23
#define ISAKMP_NTYPE_AUTHENTICATION_FAILED	24
#define ISAKMP_NTYPE_INVALID_SIGNATURE		25
#define ISAKMP_NTYPE_ADDRESS_NOTIFICATION	26
#define ISAKMP_NTYPE_NOTIFY_SA_LIFETIME		27
#define ISAKMP_NTYPE_CERTIFICATE_UNAVAILABLE	28
#define ISAKMP_NTYPE_UNSUPPORTED_EXCHANGE_TYPE	29
#define ISAKMP_NTYPE_UNEQUAL_PAYLOAD_LENGTHS	30
/* NOTIFY MESSAGES - STATUS TYPES */
#define ISAKMP_NTYPE_CONNECTED			16384
/* 4.6.3 IPSEC DOI Notify Message Types */
#define ISAKMP_NTYPE_RESPONDER_LIFETIME		24576
#define ISAKMP_NTYPE_REPLAY_STATUS		24577
#define ISAKMP_NTYPE_INITIAL_CONTACT		24578

/* using only to log */
#define ISAKMP_LOG_RETRY_LIMIT_REACHED		65530

/* XXX means internal error but it's not reserved by any drafts... */
#define ISAKMP_INTERNAL_ERROR			-1

/* 3.15 Delete Payload */
struct isakmp_pl_d {
	struct isakmp_gen h;
	u_int32_t doi;		/* Domain of Interpretation */
	u_int8_t proto_id;	/* Protocol-Id */
	u_int8_t spi_size;	/* SPI Size */
	u_int16_t num_spi;	/* # of SPIs */
	/* SPI(es) */
} __attribute__((__packed__));

