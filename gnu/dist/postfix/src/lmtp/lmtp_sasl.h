/*++
/* NAME
/*	lmtp_sasl 3h
/* SUMMARY
/*	Postfix SASL interface for LMTP client
/* SYNOPSIS
/*	#include "lmtp_sasl.h"
/* DESCRIPTION
/* .nf

 /*
 * SASL protocol functions
 */
extern void lmtp_sasl_initialize(void);
extern void lmtp_sasl_connect(LMTP_STATE *);
extern int lmtp_sasl_passwd_lookup(LMTP_STATE *);
extern void lmtp_sasl_start(LMTP_STATE *);
extern int lmtp_sasl_authenticate(LMTP_STATE *, VSTRING *);
extern void lmtp_sasl_cleanup(LMTP_STATE *);

extern void lmtp_sasl_helo_auth(LMTP_STATE *, const char *);
extern int lmtp_sasl_helo_login(LMTP_STATE *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Initial implementation by:
/*	Till Franke
/*	SuSE Rhein/Main AG
/*	65760 Eschborn, Germany
/*
/*	Adopted by:
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/
