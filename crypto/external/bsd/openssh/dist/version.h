/*	$NetBSD: version.h,v 1.7 2011/09/07 17:49:19 christos Exp $	*/

#define __OPENSSH_VERSION	"OpenSSH_5.9"
#define __NETBSDSSH_VERSION	"NetBSD_Secure_Shell-20110907"
#define SSH_HPN         "-hpn13v11"
#define SSH_LPK		"-lpk"
/*
 * it is important to retain OpenSSH version identification part, it is
 * used for bug compatibility operation.  present NetBSD SSH version as comment
 */
#define SSH_VERSION	__OPENSSH_VERSION " " __NETBSDSSH_VERSION
#define SSH_RELEASE SSH_VERSION SSH_HPN SSH_LPK
