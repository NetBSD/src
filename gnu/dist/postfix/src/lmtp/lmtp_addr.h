/*++
/* NAME
/*	lmtp_addr 3h
/* SUMMARY
/*	LMTP server address lookup
/* SYNOPSIS
/*	#include "lmtp_addr.h"
/* DESCRIPTION
/* .nf

 /*
  * DNS library.
  */
#include <dns.h>

 /*
  * Internal interfaces.
  */
extern DNS_RR *lmtp_host_addr(char *, VSTRING *);

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*
/*      Alterations for LMTP by:
/*      Philip A. Prindeville
/*      Mirapoint, Inc.
/*      USA.
/*
/*      Additional work on LMTP by:
/*      Amos Gouaux
/*      University of Texas at Dallas
/*      P.O. Box 830688, MC34
/*      Richardson, TX 75083, USA
/*--*/
