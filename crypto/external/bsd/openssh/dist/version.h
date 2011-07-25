/*	$NetBSD: version.h,v 1.6 2011/07/25 03:03:11 christos Exp $	*/
/* $OpenBSD: version.h,v 1.61 2011/02/04 00:44:43 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_5.8"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20110722"
#define SSH_HPN         "-hpn13v11"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
