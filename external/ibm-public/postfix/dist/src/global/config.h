/*	$NetBSD: config.h,v 1.1.1.1 2009/06/23 10:08:45 tron Exp $	*/

#ifndef _CONFIG_H_INCLUDED_
#define _CONFIG_H_INCLUDED_

/*++
/* NAME
/*	config 3h
/* SUMMARY
/*	compatibility
/* SYNOPSIS
/*	#include <config.h>
/* DESCRIPTION
/* .nf

 /*
  * Global library.
  */
#include <mail_conf.h>

 /*
  * Aliases.
  */
#define config_eval			mail_conf_eval
#define config_lookup			mail_conf_lookup
#define config_lookup_eval		mail_conf_lookup_eval
#define config_read			mail_conf_read
#define read_config			mail_conf_update
#define get_config_bool			get_mail_conf_bool
#define get_config_bool_fn		get_mail_conf_bool_fn
#define get_config_bool_fn_table	get_mail_conf_bool_fn_table
#define get_config_bool_table		get_mail_conf_bool_table
#define get_config_int			get_mail_conf_int
#define get_config_int2			get_mail_conf_int2
#define get_config_int_fn		get_mail_conf_int_fn
#define get_config_int_fn_table		get_mail_conf_int_fn_table
#define get_config_int_table		get_mail_conf_int_table
#define get_config_raw			get_mail_conf_raw
#define get_config_raw_fn		get_mail_conf_raw_fn
#define get_config_raw_fn_table		get_mail_conf_raw_fn_table
#define get_config_raw_table		get_mail_conf_raw_table
#define get_config_str			get_mail_conf_str
#define get_config_str_fn		get_mail_conf_str_fn
#define get_config_str_fn_table		get_mail_conf_str_fn_table
#define get_config_str_table		get_mail_conf_str_table
#define set_config_bool			set_mail_conf_bool
#define set_config_int			set_mail_conf_int
#define set_config_str			set_mail_conf_str

/* LICENSE
/* .ad
/* .fi
/*	The Secure Mailer license must be distributed with this software.
/* AUTHOR(S)
/*	Wietse Venema
/*	IBM T.J. Watson Research
/*	P.O. Box 704
/*	Yorktown Heights, NY 10598, USA
/*--*/

#endif
