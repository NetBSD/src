/*	$NetBSD: tlsrpt_wrapper.h,v 1.2 2025/02/25 19:15:50 christos Exp $	*/

#ifndef _TLSRPT_WRAPPER_INCLUDED_
#define _TLSRPT_WRAPPER_INCLUDED_

/*++
/* NAME
/*	tlsrpt_wrapper 3h
/* SUMMARY
/*	TLSRPT support for the SMTP and TLS protocol engines
/* SYNOPSIS
/*	#include <tlsrpt_wrapper.h>
/* DESCRIPTION
/* .nf

 /*
  * System library.
  */
#if defined(USE_TLS)

#if defined(USE_TLSRPT)

#include <tlsrpt.h>

 /*
  * External interface, with convenient setters for each SMTP protocol engine
  * stage. Many functions have multiple arguments of the same type. Include
  * parameter names in function prototypes here, and in call sites include
  * comments before parameter values, to prepare for future clang-tidy
  * bugprone-argument-comment checks.
  */
typedef struct TLSRPT_WRAPPER TLSRPT_WRAPPER;

extern TLSRPT_WRAPPER *trw_create(const char *rpt_socket_name,
				          const char *rpt_policy_domain,
				          const char *rpt_policy_string,
				          int skip_reused_hs);
extern void trw_free(TLSRPT_WRAPPER *trw);
extern void trw_set_tls_policy(TLSRPT_WRAPPER *trw,
			               tlsrpt_policy_type_t tls_policy_type,
			             const char *const * tls_policy_strings,
			               const char *tls_policy_domain,
			            const char *const * mx_policy_patterns);
extern void trw_set_tcp_connection(TLSRPT_WRAPPER *trw,
				           const char *snd_mta_addr,
				           const char *rcv_mta_name,
				           const char *rcv_mta_addr);
extern void trw_set_ehlo_resp(TLSRPT_WRAPPER *trw,
			              const char *rcv_mta_ehlo);
extern int trw_report_failure(TLSRPT_WRAPPER *trw,
			              tlsrpt_failure_t policy_failure,
			              const char *additional_info,
			              const char *failure_reason);
extern int trw_report_success(TLSRPT_WRAPPER *trw);
extern int trw_is_reported(const TLSRPT_WRAPPER *trw);
extern int trw_is_skip_reused_hs(const TLSRPT_WRAPPER *trw);

 /*
  * The internals declarations are also needed for functions that transmit
  * and receive TLSRPT_WRAPPER objects.
  */
#ifdef TLSRPT_WRAPPER_INTERNAL

 /*
  * Utility library.
  */
#include <argv.h>

struct TLSRPT_WRAPPER {
    /* Set with trw_create(). */
    char   *rpt_socket_name;
    char   *rpt_policy_domain;
    char   *rpt_policy_string;
    int     skip_reused_hs;
    /* Set with trw_set_policy(). */
    tlsrpt_policy_type_t tls_policy_type;
    ARGV   *tls_policy_strings;
    char   *tls_policy_domain;
    ARGV   *mx_host_patterns;
    /* Set with trw_set_tcp_connection(). */
    char   *snd_mta_addr;
    char   *rcv_mta_name;
    char   *rcv_mta_addr;
    /* Set with trw_set_ehlo_resp(). */
    char   *rcv_mta_ehlo;
    int     flags;
};

#define TRW_FLAG_HAVE_TLS_POLICY (1<<0)
#define TRW_FLAG_HAVE_TCP_CONN	(1<<1)
#define TRW_FLAG_HAVE_EHLO_RESP	(1<<2)
#define TRW_FLAG_REPORTED	(1<<3)

#define TRW_RPT_SOCKET_NAME	"rpt_socket_name"
#define TRW_RPT_POLICY_DOMAIN	"rpt_policy_domain"
#define TRW_RPT_POLICY_STRING	"rpt_policy_string"
#define TRW_SKIP_REUSED_HS	"skip_reused_hs"
#define TRW_TLS_POLICY_TYPE	"tls_policy_type"
#define TRW_TLS_POLICY_STRINGS	"tls_policy_strings"	/* XXX Not checked */
#define TRW_TLS_POLICY_DOMAIN	"tls_policy_domain"
#define TRW_MX_HOST_PATTERNS	"mx_host_patterns"	/* XXX Not checked */
#define TRW_SRC_MTA_ADDR	"snd_mta_addr"
#define TRW_DST_MTA_NAME	"rcv_mta_name"
#define TRW_DST_MTA_ADDR	"rcv_mta_addr"
#define TRW_DST_MTA_EHLO	"rcv_mta_ehlo"	/* Optional */
#define TRW_FLAGS		"flags"

#endif					/* TLSRPT_WRAPPER_INTERNAL */

extern tlsrpt_policy_type_t convert_tlsrpt_policy_type(const char *policy_type);
extern tlsrpt_failure_t convert_tlsrpt_policy_failure(const char *policy_failure);

#endif					/* USE_TLSRPT */

extern int valid_tlsrpt_policy_type(const char *policy_type);
extern int valid_tlsrpt_policy_failure(const char *policy_failure);

#endif					/* USE_TLS */

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*--*/

#endif					/* _TLSRPT_WRAPPER_INCLUDED_ */
