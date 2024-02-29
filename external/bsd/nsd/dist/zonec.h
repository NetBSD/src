/*
 * zonec.h -- zone compiler.
 *
 * Copyright (c) 2001-2006, NLnet Labs. All rights reserved.
 *
 * See LICENSE for the license.
 *
 */

#ifndef ZONEC_H
#define ZONEC_H

#include "namedb.h"

#define	MAXTOKENSLEN	512		/* Maximum number of tokens per entry */
#define	B64BUFSIZE	65535		/* Buffer size for b64 conversion */
#define	ROOT		(const uint8_t *)"\001"

#define NSEC_WINDOW_COUNT     256
#define NSEC_WINDOW_BITS_COUNT 256
#define NSEC_WINDOW_BITS_SIZE  (NSEC_WINDOW_BITS_COUNT / 8)

#define IPSECKEY_NOGATEWAY      0       /* RFC 4025 */
#define IPSECKEY_IP4            1
#define IPSECKEY_IP6            2
#define IPSECKEY_DNAME          3

#define LINEBUFSZ 1024

struct lex_data {
    size_t   len;		/* holds the label length */
    char    *str;		/* holds the data */
};

#define DEFAULT_TTL 3600

/* administration struct */
typedef struct zparser zparser_type;
struct zparser {
	region_type *region;	/* Allocate for parser lifetime data.  */
	region_type *rr_region;	/* Allocate RR lifetime data.  */
	namedb_type *db;

	const char *filename;
	uint32_t default_ttl;
	uint16_t default_class;
	zone_type *current_zone;
	domain_type *origin;
	domain_type *prev_dname;

	int error_occurred;
	unsigned int errors;
	unsigned int line;

	rr_type current_rr;
	rdata_atom_type *temporary_rdatas;
};

extern zparser_type *parser;

/* used in zonec.lex */
extern FILE *yyin;

/*
 * Used to mark bad domains and domain names.  Do not dereference
 * these pointers!
 */
extern const dname_type *error_dname;
extern domain_type *error_domain;

int yyparse(void);
int yylex(void);
int yylex_destroy(void);
/*int yyerror(const char *s);*/
void yyrestart(FILE *);

void zc_warning(const char *fmt, ...) ATTR_FORMAT(printf, 1, 2);
void zc_warning_prev_line(const char *fmt, ...) ATTR_FORMAT(printf, 1, 2);
void zc_error(const char *fmt, ...) ATTR_FORMAT(printf, 1, 2);
void zc_error_prev_line(const char *fmt, ...) ATTR_FORMAT(printf, 1, 2);

void parser_push_stringbuf(char* str);
void parser_pop_stringbuf(void);
void parser_flush(void);

int process_rr(void);
uint16_t *zparser_conv_hex(region_type *region, const char *hex, size_t len);
uint16_t *zparser_conv_hex_length(region_type *region, const char *hex, size_t len);
uint16_t *zparser_conv_time(region_type *region, const char *time);
uint16_t *zparser_conv_services(region_type *region, const char *protostr, char *servicestr);
uint16_t *zparser_conv_serial(region_type *region, const char *periodstr);
uint16_t *zparser_conv_period(region_type *region, const char *periodstr);
uint16_t *zparser_conv_short(region_type *region, const char *text);
uint16_t *zparser_conv_long(region_type *region, const char *text);
uint16_t *zparser_conv_byte(region_type *region, const char *text);
uint16_t *zparser_conv_a(region_type *region, const char *text);
uint16_t *zparser_conv_aaaa(region_type *region, const char *text);
uint16_t *zparser_conv_ilnp64(region_type *region, const char *text);
uint16_t *zparser_conv_eui(region_type *region, const char *text, size_t len);
uint16_t *zparser_conv_text(region_type *region, const char *text, size_t len);
uint16_t *zparser_conv_long_text(region_type *region, const char *text, size_t len);
uint16_t *zparser_conv_tag(region_type *region, const char *text, size_t len);
uint16_t *zparser_conv_dns_name(region_type *region, const uint8_t* name, size_t len);
uint16_t *zparser_conv_b32(region_type *region, const char *b32);
uint16_t *zparser_conv_b64(region_type *region, const char *b64);
uint16_t *zparser_conv_rrtype(region_type *region, const char *rr);
uint16_t *zparser_conv_nxt(region_type *region, uint8_t nxtbits[]);
uint16_t *zparser_conv_nsec(region_type *region, uint8_t nsecbits[NSEC_WINDOW_COUNT][NSEC_WINDOW_BITS_SIZE]);
uint16_t *zparser_conv_loc(region_type *region, char *str);
uint16_t *zparser_conv_algorithm(region_type *region, const char *algstr);
uint16_t *zparser_conv_certificate_type(region_type *region,
					const char *typestr);
uint16_t *zparser_conv_apl_rdata(region_type *region, char *str);
uint16_t *zparser_conv_svcbparam(region_type *region,
	const char *key, size_t key_len, const char *value, size_t value_len);

void parse_unknown_rdata(uint16_t type, uint16_t *wireformat);

uint32_t zparser_ttl2int(const char *ttlstr, int* error);
void zadd_rdata_wireformat(uint16_t *data);
void zadd_rdata_txt_wireformat(uint16_t *data, int first);
void zadd_rdata_txt_clean_wireformat(void);
void zadd_rdata_svcb_check_wireformat(void);
void zadd_rdata_domain(domain_type *domain);

void set_bitnsec(uint8_t  bits[NSEC_WINDOW_COUNT][NSEC_WINDOW_BITS_SIZE],
		 uint16_t index);
uint16_t *alloc_rdata_init(region_type *region, const void *data, size_t size);

/* zparser.y */
zparser_type *zparser_create(region_type *region, region_type *rr_region,
			     namedb_type *db);
void zparser_init(const char *filename, uint32_t ttl, uint16_t klass,
		  const dname_type *origin);

/* parser start and stop to parse a zone */
void zonec_setup_parser(namedb_type* db);
void zonec_desetup_parser(void);
/* parse a zone into memory. name is origin. zonefile is file to read.
 * returns number of errors; failure may have read a partial zone */
unsigned int zonec_read(const char *name, const char *zonefile, zone_type* zone);
/* parse a string into the region. and with given domaintable. global parser
 * is restored afterwards. zone needs apex set. returns last domain name
 * parsed and the number rrs parse. return number of errors, 0 is success.
 * The string must end with a newline after the RR. */
int zonec_parse_string(region_type* region, domain_table_type* domains,
	zone_type* zone, char* str, domain_type** parsed, int* num_rrs);
/** check SSHFP type for failures and emit warnings */
void check_sshfp(void);
void apex_rrset_checks(struct namedb* db, rrset_type* rrset,
	domain_type* domain);

#endif /* ZONEC_H */
