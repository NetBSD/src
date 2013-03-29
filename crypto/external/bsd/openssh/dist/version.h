/*	$NetBSD: version.h,v 1.11 2013/03/29 16:19:46 christos Exp $	*/
/* $OpenBSD: version.h,v 1.66 2013/02/10 21:19:34 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_6.2"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20130329"
#define SSH_HPN         "-hpn13v14"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
