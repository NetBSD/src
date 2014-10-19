/*	$NetBSD: version.h,v 1.13 2014/10/19 16:30:59 christos Exp $	*/
/* $OpenBSD: version.h,v 1.71 2014/04/18 23:52:25 djm Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_6.7"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20141018"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
