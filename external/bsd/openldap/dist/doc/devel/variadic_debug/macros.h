/*	$NetBSD: macros.h,v 1.2 2021/08/14 16:14:54 christos Exp $	*/

#define LDAP_PF_LOCAL_SENDMSG_ARG(x)

#define LDAP_P(x) x
#define LDAP_F(x) extern x
#define LDAP_V(x) extern x

#define LDAP_GCCATTR(x)
#define LDAP_XSTRING(x) ""
#define LDAP_CONCAT(x,y) x

#define LDAP_CONST const
#define LDAP_BEGIN_DECL
#define LDAP_END_DECL

#define SLAP_EVENT_DECL
#define SLAP_EVENT_FNAME

/* contrib/slapd-modules/smbk5pwd/smbk5pwd.c */
#define HDB int*

#define BACKSQL_ARBITRARY_KEY
#define BACKSQL_IDNUMFMT "%llu"
#define BACKSQL_IDFMT "%s"
