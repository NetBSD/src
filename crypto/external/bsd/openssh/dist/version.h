/*	$NetBSD: version.h,v 1.7.2.1 2012/05/23 10:07:05 yamt Exp $	*/
/* $OpenBSD: version.h,v 1.64 2012/02/09 20:00:18 markus Exp $ */

#define __OPENSSH_VERSION	"OpenSSH_6.0"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20120501"
#define SSH_HPN         "-hpn13v11"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
