/*	$NetBSD: version.h,v 1.3 2009/12/27 01:40:47 christos Exp $	*/
/* $OpenBSD: version.h,v 1.56 2009/06/30 14:54:40 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_5.3"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20091226"
#define SSH_HPN         "-hpn13v6"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN
