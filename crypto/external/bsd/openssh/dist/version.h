/*	$NetBSD: version.h,v 1.16 2015/08/13 10:33:21 christos Exp $	*/
/* $OpenBSD: version.h,v 1.74 2015/08/02 09:56:42 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_7.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20150812"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
