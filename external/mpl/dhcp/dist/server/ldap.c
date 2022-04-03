/*	$NetBSD: ldap.c,v 1.1.1.3 2022/04/03 01:08:44 christos Exp $	*/

/* ldap.c

   Routines for reading the configuration from LDAP */

/*
 * Copyright (c) 2010-2022 by Internet Systems Consortium, Inc. ("ISC")
 * Copyright (c) 2003-2006 Ntelos, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The Internet Software Consortium nor the names
 *    of its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INTERNET SOFTWARE CONSORTIUM AND
 * CONTRIBUTORS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE INTERNET SOFTWARE CONSORTIUM OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This LDAP module was written by Brian Masney <masneyb@ntelos.net>. Its
 * development was sponsored by Ntelos, Inc. (www.ntelos.com).
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: ldap.c,v 1.1.1.3 2022/04/03 01:08:44 christos Exp $");


#include "dhcpd.h"
#if defined(LDAP_CONFIGURATION)
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <netdb.h>
#include <net/if.h>
#if defined(HAVE_IFADDRS_H)
#include <ifaddrs.h>
#endif
#include <string.h>

#if defined(LDAP_CASA_AUTH)
#include "ldap_casa.h"
#endif

#if defined(LDAP_USE_GSSAPI)
#include <sasl/sasl.h>
#include "ldap_krb_helper.h"
#endif

static LDAP * ld = NULL;
static char *ldap_server = NULL, 
            *ldap_username = NULL, 
            *ldap_password = NULL,
            *ldap_base_dn = NULL,
            *ldap_dhcp_server_cn = NULL,
            *ldap_debug_file = NULL;
static int ldap_port = LDAP_PORT,
           ldap_method = LDAP_METHOD_DYNAMIC,
           ldap_referrals = -1,
           ldap_debug_fd = -1,
           ldap_enable_retry = -1,
           ldap_init_retry = -1;
#if defined (LDAP_USE_SSL)
static int ldap_use_ssl = -1,        /* try TLS if possible */
           ldap_tls_reqcert = -1,
           ldap_tls_crlcheck = -1;
static char *ldap_tls_ca_file = NULL,
            *ldap_tls_ca_dir = NULL,
            *ldap_tls_cert = NULL,
            *ldap_tls_key = NULL,
            *ldap_tls_ciphers = NULL,
            *ldap_tls_randfile = NULL;
#endif

#if defined (LDAP_USE_GSSAPI)
static char *ldap_gssapi_keytab = NULL,
            *ldap_gssapi_principal = NULL;

struct ldap_sasl_instance {
    char        *sasl_mech;
    char        *sasl_realm;
    char        *sasl_authz_id;
    char        *sasl_authc_id;
    char        *sasl_password;
};

static struct ldap_sasl_instance *ldap_sasl_inst = NULL;

static int
_ldap_sasl_interact(LDAP *ld, unsigned flags, void *defaults, void *sin) ;
#endif 

static struct ldap_config_stack *ldap_stack = NULL;

typedef struct ldap_dn_node {
    struct ldap_dn_node *next;
    size_t refs;
    char *dn;
} ldap_dn_node;

static ldap_dn_node *ldap_service_dn_head = NULL;
static ldap_dn_node *ldap_service_dn_tail = NULL;

static int ldap_read_function (struct parse *cfile);

static struct parse *
x_parser_init(const char *name)
{
  struct parse *cfile;
  isc_result_t res;
  char *inbuf;

  inbuf = dmalloc (LDAP_BUFFER_SIZE, MDL);
  if (inbuf == NULL)
    return NULL;

  cfile = (struct parse *) NULL;
  res = new_parse (&cfile, -1, inbuf, LDAP_BUFFER_SIZE, name, 0);
  if (res != ISC_R_SUCCESS)
    {
      dfree(inbuf, MDL);
      return NULL;
    }
  /* the buffer is still empty */
  cfile->bufsiz = LDAP_BUFFER_SIZE;
  cfile->buflen = cfile->bufix = 0;
  /* attach ldap read function */
  cfile->read_function = ldap_read_function;
  return cfile;
}

static isc_result_t
x_parser_free(struct parse **cfile)
{
  if (cfile && *cfile)
    {
      if ((*cfile)->inbuf)
          dfree((*cfile)->inbuf, MDL);
      (*cfile)->inbuf = NULL;
      (*cfile)->bufsiz = 0;
      return end_parse(cfile);
    }
  return ISC_R_SUCCESS;
}

static int
x_parser_resize(struct parse *cfile, size_t len)
{
  size_t size;
  char * temp;

  /* grow by len rounded up at LDAP_BUFFER_SIZE */
  size = cfile->bufsiz + (len | (LDAP_BUFFER_SIZE-1)) + 1;

  /* realloc would be better, but there isn't any */
  if ((temp = dmalloc (size, MDL)) != NULL)
    {
#if defined (DEBUG_LDAP)
      log_info ("Reallocated %s buffer from %zu to %zu",
                cfile->tlname, cfile->bufsiz, size);
#endif
      memcpy(temp, cfile->inbuf, cfile->bufsiz);
      dfree(cfile->inbuf, MDL);
      cfile->inbuf  = temp;
      cfile->bufsiz = size;
      return 1;
    }

  /*
   * Hmm... what is worser, consider it as fatal error and
   * bail out completely or discard config data in hope it
   * is "only" an option in dynamic host lookup?
   */
  log_error("Unable to reallocated %s buffer from %zu to %zu",
            cfile->tlname, cfile->bufsiz, size);
  return 0;
}

static char *
x_parser_strcat(struct parse *cfile, const char *str)
{
  size_t cur = strlen(cfile->inbuf);
  size_t len = strlen(str);
  size_t cnt;

  if (cur + len >= cfile->bufsiz && !x_parser_resize(cfile, len))
    return NULL;

  cnt = cfile->bufsiz > cur ? cfile->bufsiz - cur - 1 : 0;
  return strncat(cfile->inbuf, str, cnt);
}

static inline void
x_parser_reset(struct parse *cfile)
{
  cfile->inbuf[0] = '\0';
  cfile->bufix = cfile->buflen = 0;
}

static inline size_t
x_parser_length(struct parse *cfile)
{
  cfile->buflen = strlen(cfile->inbuf);
  return cfile->buflen;
}

static char *
x_strxform(char *dst, const char *src, size_t dst_size,
           int (*xform)(int))
{
  if(dst && src && dst_size)
    {
      size_t len, pos;

      len = strlen(src);
      for(pos=0; pos < len && pos + 1 < dst_size; pos++)
        dst[pos] = xform((int)src[pos]);
      dst[pos] = '\0';

      return dst;
    }
  return NULL;
}

static int
get_host_entry(char *fqdnname, size_t fqdnname_size,
               char *hostaddr, size_t hostaddr_size)
{
#if defined(MAXHOSTNAMELEN)
  char   hname[MAXHOSTNAMELEN+1];
#else
  char   hname[65];
#endif
  struct hostent *hp;

  if (NULL == fqdnname || 1 >= fqdnname_size)
    return -1;

  memset(hname, 0, sizeof(hname));
  if (gethostname(hname, sizeof(hname)-1))
    return -1;

  if (NULL == (hp = gethostbyname(hname)))
    return -1;

  strncpy(fqdnname, hp->h_name, fqdnname_size-1);
  fqdnname[fqdnname_size-1] = '\0';

  if (hostaddr != NULL)
    {
      if (hp->h_addr != NULL)
        {
          struct in_addr *aptr = (struct in_addr *)hp->h_addr;
#if defined(HAVE_INET_NTOP)
          if (hostaddr_size >= INET_ADDRSTRLEN &&
              inet_ntop(AF_INET, aptr, hostaddr, hostaddr_size) != NULL)
            {
              return 0;
            }
#else
          char  *astr = inet_ntoa(*aptr);
          size_t alen = strlen(astr);
          if (astr && alen > 0 && hostaddr_size > alen)
            {
              strncpy(hostaddr, astr, hostaddr_size-1);
              hostaddr[hostaddr_size-1] = '\0';
              return 0;
            }
#endif
        }
      return -1;
    }
  return 0;
}

#if defined(HAVE_IFADDRS_H)
static int
is_iface_address(struct ifaddrs *addrs, struct in_addr *addr)
{
  struct ifaddrs     *ia;
  struct sockaddr_in *sa;
  int                 num = 0;

  if(addrs == NULL || addr == NULL)
    return -1;

  for (ia = addrs; ia != NULL; ia = ia->ifa_next)
    {
      ++num;
      if (ia->ifa_addr && (ia->ifa_flags & IFF_UP) &&
          ia->ifa_addr->sa_family == AF_INET)
      {
        sa = (struct sockaddr_in *)(ia->ifa_addr);
        if (addr->s_addr == sa->sin_addr.s_addr)
          return num;
      }
    }
  return 0;
}

static int
get_host_address(const char *hostname, char *hostaddr, size_t hostaddr_size, struct ifaddrs *addrs)
{
  if (hostname && *hostname && hostaddr && hostaddr_size)
    {
      struct in_addr addr;

#if defined(HAVE_INET_PTON)
      if (inet_pton(AF_INET, hostname, &addr) == 1)
#else
      if (inet_aton(hostname, &addr) != 0)
#endif
        {
          /* it is already IP address string */
          if(strlen(hostname) < hostaddr_size)
            {
              strncpy(hostaddr, hostname, hostaddr_size-1);
              hostaddr[hostaddr_size-1] = '\0';

              if (addrs != NULL && is_iface_address (addrs, &addr) > 0)
                return 1;
              else
                return 0;
            }
        }
      else
        {
          struct hostent *hp;
          if ((hp = gethostbyname(hostname)) != NULL && hp->h_addr != NULL)
            {
              struct in_addr *aptr  = (struct in_addr *)hp->h_addr;
              int             mret = 0;

              if (addrs != NULL)
                {
                  char **h;
                  for (h=hp->h_addr_list; *h; h++)
                    {
                      struct in_addr *haddr = (struct in_addr *)*h;
                      if (is_iface_address (addrs, haddr) > 0)
                        {
                          aptr = haddr;
                          mret = 1;
                        }
                    }
                }

#if defined(HAVE_INET_NTOP)
              if (hostaddr_size >= INET_ADDRSTRLEN &&
                  inet_ntop(AF_INET, aptr, hostaddr, hostaddr_size) != NULL)
                {
                  return mret;
                }
#else
              char  *astr = inet_ntoa(*aptr);
              size_t alen = strlen(astr);
              if (astr && alen > 0 && alen < hostaddr_size)
                {
                  strncpy(hostaddr, astr, hostaddr_size-1);
                  hostaddr[hostaddr_size-1] = '\0';
                  return mret;
                }
#endif
            }
        }
    }
  return -1;
}
#endif /* HAVE_IFADDRS_H */

static void
ldap_parse_class (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv;

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "cn")) == NULL ||
      tempbv[0] == NULL)
    {
      if (tempbv != NULL)
        ldap_value_free_len (tempbv);

      return;
    }

  x_parser_strcat (cfile, "class \"");
  x_parser_strcat (cfile, tempbv[0]->bv_val);
  x_parser_strcat (cfile, "\" {\n");

  item->close_brace = 1;
  ldap_value_free_len (tempbv);
}

static int
is_hex_string(const char *str)
{
  int colon = 1;
  int xdigit = 0;
  size_t i;

  if (!str)
    return 0;

  if (*str == '-')
    str++;

  for (i=0; str[i]; ++i)
    {
      if (str[i] == ':')
        {
          xdigit = 0;
          if(++colon > 1)
            return 0;
        }
      else if(isxdigit((unsigned char)str[i]))
        {
          colon = 0;
          if (++xdigit > 2)
            return 0;
        }
      else
        return 0;
    }
  return i > 0 && !colon;
}

static void
ldap_parse_subclass (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv, **classdata;
  char *tmp;

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "cn")) == NULL ||
      tempbv[0] == NULL)
    {
      if (tempbv != NULL)
        ldap_value_free_len (tempbv);

      return;
    }

  if ((classdata = ldap_get_values_len (ld, item->ldent, 
                                  "dhcpClassData")) == NULL || 
      classdata[0] == NULL)
    {
      if (classdata != NULL)
        ldap_value_free_len (classdata);
      ldap_value_free_len (tempbv);

      return;
    }

  x_parser_strcat (cfile, "subclass \"");
  x_parser_strcat (cfile, classdata[0]->bv_val);
  if (is_hex_string(tempbv[0]->bv_val))
    {
      x_parser_strcat (cfile, "\" ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, " {\n");
    }
  else
    {
      tmp = quotify_string(tempbv[0]->bv_val, MDL);
      x_parser_strcat (cfile, "\" \"");
      x_parser_strcat (cfile, tmp);
      x_parser_strcat (cfile, "\" {\n");
      dfree(tmp, MDL);
    }

  item->close_brace = 1;
  ldap_value_free_len (tempbv);
  ldap_value_free_len (classdata);
}


static void
ldap_parse_host (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv, **hwaddr;

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "cn")) == NULL ||
      tempbv[0] == NULL)
    {
      if (tempbv != NULL)
        ldap_value_free_len (tempbv);

      return;
    }

  hwaddr = ldap_get_values_len (ld, item->ldent, "dhcpHWAddress");

  x_parser_strcat (cfile, "host ");
  x_parser_strcat (cfile, tempbv[0]->bv_val);
  x_parser_strcat (cfile, " {\n");

  if (hwaddr != NULL)
    {
      if (hwaddr[0] != NULL)
        {
          x_parser_strcat (cfile, "hardware ");
          x_parser_strcat (cfile, hwaddr[0]->bv_val);
          x_parser_strcat (cfile, ";\n");
        }
      ldap_value_free_len (hwaddr);
    }

  item->close_brace = 1;
  ldap_value_free_len (tempbv);
}


static void
ldap_parse_shared_network (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv;

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "cn")) == NULL ||
      tempbv[0] == NULL)
    {
      if (tempbv != NULL)
        ldap_value_free_len (tempbv);

      return;
    }

  x_parser_strcat (cfile, "shared-network \"");
  x_parser_strcat (cfile, tempbv[0]->bv_val);
  x_parser_strcat (cfile, "\" {\n");

  item->close_brace = 1;
  ldap_value_free_len (tempbv);
}


static void
parse_netmask (int netmask, char *netmaskbuf)
{
  unsigned long nm;
  int i;

  nm = 0;
  for (i=1; i <= netmask; i++)
    {
      nm |= 1 << (32 - i);
    }

  sprintf (netmaskbuf, "%d.%d.%d.%d", (int) (nm >> 24) & 0xff, 
                                      (int) (nm >> 16) & 0xff, 
                                      (int) (nm >> 8) & 0xff, 
                                      (int) nm & 0xff);
}


static void
ldap_parse_subnet (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv, **netmaskstr;
  char netmaskbuf[sizeof("255.255.255.255")];
  int i;

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "cn")) == NULL ||
      tempbv[0] == NULL)
    {
      if (tempbv != NULL)
        ldap_value_free_len (tempbv);

      return;
    }

  if ((netmaskstr = ldap_get_values_len (ld, item->ldent, 
                                     "dhcpNetmask")) == NULL || 
      netmaskstr[0] == NULL)
    {
      if (netmaskstr != NULL)
        ldap_value_free_len (netmaskstr);
      ldap_value_free_len (tempbv);

      return;
    }

  x_parser_strcat (cfile, "subnet ");
  x_parser_strcat (cfile, tempbv[0]->bv_val);

  x_parser_strcat (cfile, " netmask ");
  parse_netmask (strtol (netmaskstr[0]->bv_val, NULL, 10), netmaskbuf);
  x_parser_strcat (cfile, netmaskbuf);

  x_parser_strcat (cfile, " {\n");

  ldap_value_free_len (tempbv);
  ldap_value_free_len (netmaskstr);

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpRange")) != NULL)
    {
      for (i=0; tempbv[i] != NULL; i++)
        {
          x_parser_strcat (cfile, "range");
          x_parser_strcat (cfile, " ");
          x_parser_strcat (cfile, tempbv[i]->bv_val);
          x_parser_strcat (cfile, ";\n");
        }
      ldap_value_free_len (tempbv);
    }

  item->close_brace = 1;
}

static void
ldap_parse_subnet6 (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv;
  int i;

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "cn")) == NULL ||
      tempbv[0] == NULL)
    {
      if (tempbv != NULL)
        ldap_value_free_len (tempbv);

      return;
    }

  x_parser_strcat (cfile, "subnet6 ");
  x_parser_strcat (cfile, tempbv[0]->bv_val);

  x_parser_strcat (cfile, " {\n");

  ldap_value_free_len (tempbv);

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpRange6")) != NULL)
    {
      for (i=0; tempbv[i] != NULL; i++)
        {
          x_parser_strcat (cfile, "range6");
          x_parser_strcat (cfile, " ");
          x_parser_strcat (cfile, tempbv[i]->bv_val);
          x_parser_strcat (cfile, ";\n");
        }
      ldap_value_free_len (tempbv);
    }

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpPermitList")) != NULL)
    {
      for (i=0; tempbv[i] != NULL; i++)
        {
          x_parser_strcat (cfile, tempbv[i]->bv_val);
          x_parser_strcat (cfile, ";\n");
        }
      ldap_value_free_len (tempbv);
     }

   item->close_brace = 1;
}

static void
ldap_parse_pool (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv;
  int i;

  x_parser_strcat (cfile, "pool {\n");

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpRange")) != NULL)
    {
      x_parser_strcat (cfile, "range");
      for (i=0; tempbv[i] != NULL; i++)
        {
          x_parser_strcat (cfile, " ");
          x_parser_strcat (cfile, tempbv[i]->bv_val);
        }
      x_parser_strcat (cfile, ";\n");
      ldap_value_free_len (tempbv);
    }

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpPermitList")) != NULL)
    {
      for (i=0; tempbv[i] != NULL; i++)
        {
          x_parser_strcat (cfile, tempbv[i]->bv_val);
          x_parser_strcat (cfile, ";\n");
        }
      ldap_value_free_len (tempbv);
    }

  item->close_brace = 1;
}

static void
ldap_parse_pool6 (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv;
  int i;

  x_parser_strcat (cfile, "pool6 {\n");

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpRange6")) != NULL)
    {
      x_parser_strcat (cfile, "range6");
      for (i=0; tempbv[i] != NULL; i++)
        {
          x_parser_strcat (cfile, " ");
          x_parser_strcat (cfile, tempbv[i]->bv_val);
        }
      x_parser_strcat (cfile, ";\n");
      ldap_value_free_len (tempbv);
    }

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpPermitList")) != NULL)
    {
      for (i=0; tempbv[i] != NULL; i++)
        {
          x_parser_strcat(cfile, tempbv[i]->bv_val);
          x_parser_strcat (cfile, ";\n");
        }
      ldap_value_free_len (tempbv);
    }

  item->close_brace = 1;
}

static void
ldap_parse_group (struct ldap_config_stack *item, struct parse *cfile)
{
  x_parser_strcat (cfile, "group {\n");
  item->close_brace = 1;
}


static void
ldap_parse_key (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv;

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "cn")) != NULL)
    {
      x_parser_strcat (cfile, "key ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, " {\n");
      ldap_value_free_len (tempbv);
    }

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpKeyAlgorithm")) != NULL)
    {
      x_parser_strcat (cfile, "algorithm ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
      ldap_value_free_len (tempbv);
    }

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpKeySecret")) != NULL)
    {
      x_parser_strcat (cfile, "secret ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
      ldap_value_free_len (tempbv);
    }

  item->close_brace = 1;
}


static void
ldap_parse_zone (struct ldap_config_stack *item, struct parse *cfile)
{
  char *cnFindStart, *cnFindEnd;
  struct berval **tempbv;
  char *keyCn;
  size_t len;

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "cn")) != NULL)
    {
      x_parser_strcat (cfile, "zone ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, " {\n");
      ldap_value_free_len (tempbv);
    }

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpDnsZoneServer")) != NULL)
    {
      x_parser_strcat (cfile, "primary ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);

      x_parser_strcat (cfile, ";\n");
      ldap_value_free_len (tempbv);
    }

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpKeyDN")) != NULL)
    {
      cnFindStart = strchr(tempbv[0]->bv_val,'=');
      if (cnFindStart != NULL)
        cnFindEnd = strchr(++cnFindStart,',');
      else
        cnFindEnd = NULL;

      if (cnFindEnd != NULL && cnFindEnd > cnFindStart)
        {
          len = cnFindEnd - cnFindStart;
          keyCn = dmalloc (len + 1, MDL);
        }
      else
        {
          len = 0;
          keyCn = NULL;
        }

      if (keyCn != NULL)
        {
          strncpy (keyCn, cnFindStart, len);
          keyCn[len] = '\0';

          x_parser_strcat (cfile, "key ");
          x_parser_strcat (cfile, keyCn);
          x_parser_strcat (cfile, ";\n");

          dfree (keyCn, MDL);
        }

      ldap_value_free_len (tempbv);
     }

  item->close_brace = 1;
}

#if defined(HAVE_IFADDRS_H)
static void
ldap_parse_failover (struct ldap_config_stack *item, struct parse *cfile)
{
  struct berval **tempbv, **peername;
  struct ifaddrs *addrs = NULL;
  char srvaddr[2][64] = {"\0", "\0"};
  int primary, split = 0, match;

  if ((peername = ldap_get_values_len (ld, item->ldent, "cn")) == NULL ||
      peername[0] == NULL)
    {
      if (peername != NULL)
        ldap_value_free_len (peername);

      // ldap with disabled schema checks? fail to avoid syntax error.
      log_error("Unable to find mandatory failover peering name attribute");
      return;
    }

  /* Get all interface addresses */
  getifaddrs(&addrs);

  /*
  ** when dhcpFailOverPrimaryServer or dhcpFailOverSecondaryServer
  ** matches one of our IP address, the following valiables are set:
  ** - primary is 1 when we are primary or 0 when we are secondary
  ** - srvaddr[0] contains ip address of the primary
  ** - srvaddr[1] contains ip address of the secondary
  */
  primary = -1;
  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpFailOverPrimaryServer")) != NULL &&
      tempbv[0] != NULL)
    {
      match = get_host_address (tempbv[0]->bv_val, srvaddr[0], sizeof(srvaddr[0]), addrs);
      if (match >= 0)
        {
          /* we are the primary */
          if (match > 0)
            primary = 1;
        }
      else
        {
          log_info("Can't resolve address of the primary failover '%s' server %s",
                   peername[0]->bv_val, tempbv[0]->bv_val);
          ldap_value_free_len (tempbv);
          ldap_value_free_len (peername);
          if (addrs)
            freeifaddrs(addrs);
          return;
        }
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpFailOverSecondaryServer")) != NULL &&
      tempbv[0] != NULL)
    {
      match = get_host_address (tempbv[0]->bv_val, srvaddr[1], sizeof(srvaddr[1]), addrs);
      if (match >= 0)
        {
          if (match > 0)
            {
              if (primary == 1)
                {
                  log_info("Both, primary and secondary failover '%s' server"
                           " attributes match our local address", peername[0]->bv_val);
                  ldap_value_free_len (tempbv);
                  ldap_value_free_len (peername);
                  if (addrs)
                    freeifaddrs(addrs);
                  return;
                }

              /* we are the secondary */
              primary = 0;
            }
        }
      else
        {
          log_info("Can't resolve address of the secondary failover '%s' server %s",
                   peername[0]->bv_val, tempbv[0]->bv_val);
          ldap_value_free_len (tempbv);
          ldap_value_free_len (peername);
          if (addrs)
            freeifaddrs(addrs);
          return;
        }
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);


  if (primary == -1 || *srvaddr[0] == '\0' || *srvaddr[1] == '\0')
    {
      log_error("Could not decide if the server type is primary"
                " or secondary for failover peering '%s'.", peername[0]->bv_val);
      ldap_value_free_len (peername);
      if (addrs)
        freeifaddrs(addrs);
      return;
    }

  x_parser_strcat (cfile, "failover peer \"");
  x_parser_strcat (cfile, peername[0]->bv_val);
  x_parser_strcat (cfile, "\" {\n");

  if (primary)
    x_parser_strcat (cfile, "primary;\n");
  else
    x_parser_strcat (cfile, "secondary;\n");

  x_parser_strcat (cfile, "address ");
  if (primary)
    x_parser_strcat (cfile, srvaddr[0]);
  else
    x_parser_strcat (cfile, srvaddr[1]);
  x_parser_strcat (cfile, ";\n");

  x_parser_strcat (cfile, "peer address ");
  if (primary)
    x_parser_strcat (cfile, srvaddr[1]);
  else
    x_parser_strcat (cfile, srvaddr[0]);
  x_parser_strcat (cfile, ";\n");

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpFailOverPrimaryPort")) != NULL &&
      tempbv[0] != NULL)
    {
      if (primary)
        x_parser_strcat (cfile, "port ");
      else
        x_parser_strcat (cfile, "peer port ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpFailOverSecondaryPort")) != NULL &&
      tempbv[0] != NULL)
    {
      if (primary)
        x_parser_strcat (cfile, "peer port ");
      else
        x_parser_strcat (cfile, "port ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpFailOverResponseDelay")) != NULL &&
      tempbv[0] != NULL)
    {
      x_parser_strcat (cfile, "max-response-delay ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpFailOverUnackedUpdates")) != NULL &&
      tempbv[0] != NULL)
    {
      x_parser_strcat (cfile, "max-unacked-updates ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);

  if ((tempbv = ldap_get_values_len (ld, item->ldent, "dhcpFailOverLoadBalanceTime")) != NULL &&
      tempbv[0] != NULL)
    {
      x_parser_strcat (cfile, "load balance max seconds ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);

  tempbv = NULL;
  if (primary &&
      (tempbv = ldap_get_values_len (ld, item->ldent, "dhcpMaxClientLeadTime")) != NULL &&
      tempbv[0] != NULL)
    {
      x_parser_strcat (cfile, "mclt ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);

  tempbv = NULL;
  if (primary &&
      (tempbv = ldap_get_values_len (ld, item->ldent, "dhcpFailOverSplit")) != NULL &&
      tempbv[0] != NULL)
    {
      x_parser_strcat (cfile, "split ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
      split = 1;
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);

  tempbv = NULL;
  if (primary && !split &&
      (tempbv = ldap_get_values_len (ld, item->ldent, "dhcpFailOverHashBucketAssignment")) != NULL &&
      tempbv[0] != NULL)
    {
      x_parser_strcat (cfile, "hba ");
      x_parser_strcat (cfile, tempbv[0]->bv_val);
      x_parser_strcat (cfile, ";\n");
    }
  if (tempbv != NULL)
    ldap_value_free_len (tempbv);

  item->close_brace = 1;
}
#endif /* HAVE_IFADDRS_H */

static void
add_to_config_stack (LDAPMessage * res, LDAPMessage * ent)
{
  struct ldap_config_stack *ns;

  ns = dmalloc (sizeof (*ns), MDL);
  if (!ns) {
    log_fatal ("no memory for add_to_config_stack()");
  }

  ns->res = res;
  ns->ldent = ent;
  ns->close_brace = 0;
  ns->processed = 0;
  ns->next = ldap_stack;
  ldap_stack = ns;
}

static void
ldap_stop()
{
  struct sigaction old, new;

  if (ld == NULL)
    return;

  /*
   ** ldap_unbind after a LDAP_SERVER_DOWN result
   ** causes a SIGPIPE and dhcpd gets terminated,
   ** since it doesn't handle it...
   */

  new.sa_flags   = 0;
  new.sa_handler = SIG_IGN;
  sigemptyset (&new.sa_mask);
  sigaction (SIGPIPE, &new, &old);

  ldap_unbind_ext_s (ld, NULL, NULL);
  ld = NULL;

  sigaction (SIGPIPE, &old, &new);
}


static char *
_do_lookup_dhcp_string_option (struct option_state *options, int option_name)
{
  struct option_cache *oc;
  struct data_string db;
  char *ret;

  memset (&db, 0, sizeof (db));
  oc = lookup_option (&server_universe, options, option_name);
  if (oc &&
      evaluate_option_cache (&db, (struct packet*) NULL,
                             (struct lease *) NULL,
                             (struct client_state *) NULL, options,
                             (struct option_state *) NULL,
                             &global_scope, oc, MDL) &&
      db.data != NULL && *db.data != '\0')

    {
      ret = dmalloc (db.len + 1, MDL);
      if (ret == NULL)
        log_fatal ("no memory for ldap option %d value", option_name);

      memcpy (ret, db.data, db.len);
      ret[db.len] = 0;
      data_string_forget (&db, MDL);
    }
  else
    ret = NULL;

  return (ret);
}


static int
_do_lookup_dhcp_int_option (struct option_state *options, int option_name)
{
  struct option_cache *oc;
  struct data_string db;
  int ret = 0;

  memset (&db, 0, sizeof (db));
  oc = lookup_option (&server_universe, options, option_name);
  if (oc &&
      evaluate_option_cache (&db, (struct packet*) NULL,
                             (struct lease *) NULL,
                             (struct client_state *) NULL, options,
                             (struct option_state *) NULL,
                             &global_scope, oc, MDL) &&
      db.data != NULL)
    {
      if (db.len == 4) {
         ret = getULong(db.data);
      } 

      data_string_forget (&db, MDL);
    }

  return (ret);
}


static int
_do_lookup_dhcp_enum_option (struct option_state *options, int option_name)
{
  struct option_cache *oc;
  struct data_string db;
  int ret = -1;

  memset (&db, 0, sizeof (db));
  oc = lookup_option (&server_universe, options, option_name);
  if (oc &&
      evaluate_option_cache (&db, (struct packet*) NULL,
                             (struct lease *) NULL,
                             (struct client_state *) NULL, options,
                             (struct option_state *) NULL,
                             &global_scope, oc, MDL) &&
      db.data != NULL && *db.data != '\0')
    {
      if (db.len == 1) 
        ret = db.data [0];
      else
        log_fatal ("invalid option name %d", option_name);

      data_string_forget (&db, MDL);
    }
  else
    ret = 0;

  return (ret);
}

int
ldap_rebind_cb (LDAP *ld, LDAP_CONST char *url, ber_tag_t request, ber_int_t msgid, void *parms)
{
  int ret;
  LDAPURLDesc *ldapurl = NULL;
  char *who = NULL;
  struct berval creds;

  log_info("LDAP rebind to '%s'", url);
  if ((ret = ldap_url_parse(url, &ldapurl)) != LDAP_SUCCESS)
    {
      log_error ("Error: Can not parse ldap rebind url '%s': %s",
                 url, ldap_err2string(ret));
      return ret;
    }


#if defined (LDAP_USE_SSL)
  if (strcasecmp(ldapurl->lud_scheme, "ldaps") == 0)
    {
      int opt = LDAP_OPT_X_TLS_HARD;
      if ((ret = ldap_set_option (ld, LDAP_OPT_X_TLS, &opt)) != LDAP_SUCCESS)
        {
          log_error ("Error: Cannot init LDAPS session to %s:%d: %s",
                    ldapurl->lud_host, ldapurl->lud_port, ldap_err2string (ret));
          ldap_free_urldesc(ldapurl);
          return ret;
        }
      else
        {
          log_info ("LDAPS session successfully enabled to %s", ldap_server);
        }
    }
  else
  if (strcasecmp(ldapurl->lud_scheme, "ldap") == 0 &&
      ldap_use_ssl != LDAP_SSL_OFF)
    {
      if ((ret = ldap_start_tls_s (ld, NULL, NULL)) != LDAP_SUCCESS)
        {
          log_error ("Error: Cannot start TLS session to %s:%d: %s",
                     ldapurl->lud_host, ldapurl->lud_port, ldap_err2string (ret));
          ldap_free_urldesc(ldapurl);
          return ret;
        }
      else
        {
          log_info ("TLS session successfully started to %s:%d",
                    ldapurl->lud_host, ldapurl->lud_port);
        }
    }
#endif

#if defined(LDAP_USE_GSSAPI)
    if (ldap_gssapi_principal != NULL) {
      krb5_get_tgt(ldap_gssapi_principal, ldap_gssapi_keytab);
      if ((ret = ldap_sasl_interactive_bind_s(ld, NULL, ldap_sasl_inst->sasl_mech,
                                              NULL, NULL, LDAP_SASL_AUTOMATIC,
                                              _ldap_sasl_interact, ldap_sasl_inst)
          ) != LDAP_SUCCESS)
      {
        log_error ("Error: Cannot SASL bind to ldap server %s:%d: %s",
                   ldap_server, ldap_port, ldap_err2string (ret));
        char *msg=NULL;
        ldap_get_option( ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*)&msg);
        log_error ("\tAdditional info: %s", msg);
        ldap_memfree(msg);
        ldap_stop();
      }

      ldap_free_urldesc(ldapurl);
      return ret;
    } 
#endif

  if (ldap_username != NULL && *ldap_username != '\0' && ldap_password != NULL)
    {
      who = ldap_username;
      creds.bv_val = strdup(ldap_password);
      if (creds.bv_val == NULL)
          log_fatal ("Error: Unable to allocate memory to duplicate ldap_password");

      creds.bv_len = strlen(ldap_password);

      if ((ret = ldap_sasl_bind_s (ld, who, LDAP_SASL_SIMPLE, &creds,
                               NULL, NULL, NULL)) != LDAP_SUCCESS) 
        {
          log_error ("Error: Cannot login into ldap server %s:%d: %s",
                     ldapurl->lud_host, ldapurl->lud_port, ldap_err2string (ret));
        }

      if (creds.bv_val)
        free(creds.bv_val); 
    } 

  ldap_free_urldesc(ldapurl);
  return ret;
}

static int
_do_ldap_retry(int ret, const char *server, int port)
{
  static int inform = 1;

  if (ldap_enable_retry > 0 && ret == LDAP_SERVER_DOWN && ldap_init_retry > 0)
    {
      if (inform || (ldap_init_retry % 10) == 0)
        {
          inform = 0;
          log_info ("Can't contact LDAP server %s:%d: retrying for %d sec",
                    server, port, ldap_init_retry);
        }
      sleep(1);
      return ldap_init_retry--;
    }
  return 0;
}

static struct berval *
_do_ldap_str2esc_filter_bv(const char *str, ber_len_t len, struct berval *bv_o)
{
  struct berval bv_i;

  if (!str || !bv_o || (ber_str2bv(str, len, 0, &bv_i) == NULL) ||
     (ldap_bv2escaped_filter_value(&bv_i, bv_o) != 0))
    return NULL;
  return bv_o;
}

static void
ldap_start (void)
{
  struct option_state *options;
  int ret, version;
  char *uri = NULL;
  struct berval creds;
#if defined(LDAP_USE_GSSAPI)
  char *gssapi_realm = NULL;
  char *gssapi_user = NULL;
  char *running = NULL;
  const char *gssapi_delim = "@";
#endif

  if (ld != NULL)
    return;

  if (ldap_server == NULL)
    {
      options = NULL;
      option_state_allocate (&options, MDL);

      execute_statements_in_scope (NULL, NULL, NULL, NULL, NULL,
				   options, &global_scope, root_group,
				   NULL, NULL);

      ldap_server = _do_lookup_dhcp_string_option (options, SV_LDAP_SERVER);
      ldap_dhcp_server_cn = _do_lookup_dhcp_string_option (options,
                                                      SV_LDAP_DHCP_SERVER_CN);
      ldap_port = _do_lookup_dhcp_int_option (options, SV_LDAP_PORT);
      ldap_base_dn = _do_lookup_dhcp_string_option (options, SV_LDAP_BASE_DN);
      ldap_method = _do_lookup_dhcp_enum_option (options, SV_LDAP_METHOD);
      ldap_debug_file = _do_lookup_dhcp_string_option (options,
                                                       SV_LDAP_DEBUG_FILE);
      ldap_referrals = _do_lookup_dhcp_enum_option (options, SV_LDAP_REFERRALS);
      ldap_init_retry = _do_lookup_dhcp_int_option (options, SV_LDAP_INIT_RETRY);

#if defined (LDAP_USE_SSL)
      ldap_use_ssl = _do_lookup_dhcp_enum_option (options, SV_LDAP_SSL);
      if( ldap_use_ssl != LDAP_SSL_OFF)
        {
          ldap_tls_reqcert = _do_lookup_dhcp_enum_option (options, SV_LDAP_TLS_REQCERT);
          ldap_tls_ca_file = _do_lookup_dhcp_string_option (options, SV_LDAP_TLS_CA_FILE);
          ldap_tls_ca_dir = _do_lookup_dhcp_string_option (options, SV_LDAP_TLS_CA_DIR);
          ldap_tls_cert = _do_lookup_dhcp_string_option (options, SV_LDAP_TLS_CERT);
          ldap_tls_key = _do_lookup_dhcp_string_option (options, SV_LDAP_TLS_KEY);
          ldap_tls_crlcheck = _do_lookup_dhcp_enum_option (options, SV_LDAP_TLS_CRLCHECK);
          ldap_tls_ciphers = _do_lookup_dhcp_string_option (options, SV_LDAP_TLS_CIPHERS);
          ldap_tls_randfile = _do_lookup_dhcp_string_option (options, SV_LDAP_TLS_RANDFILE);
        }
#endif

#if defined (LDAP_USE_GSSAPI)
      ldap_gssapi_principal = _do_lookup_dhcp_string_option (options,
                                                             SV_LDAP_GSSAPI_PRINCIPAL);

      if (ldap_gssapi_principal == NULL) {
          log_info("ldap-gssapi-principal is not set,"
                   "GSSAPI Authentication for LDAP will not be used");
      } else {        
        ldap_gssapi_keytab = _do_lookup_dhcp_string_option (options,
                                                          SV_LDAP_GSSAPI_KEYTAB);
        if (ldap_gssapi_keytab == NULL) {
          log_fatal("ldap-gssapi-keytab must be specified");
        } 

      	running = strdup(ldap_gssapi_principal);
      	if (running == NULL)
          log_fatal("Could not allocate memory  to duplicate gssapi principal");

        gssapi_user = strtok(running, gssapi_delim);
        if (!gssapi_user || strlen(gssapi_user) == 0) {
          log_fatal ("GSSAPI principal must specify user: user@realm");
        }

        gssapi_realm = strtok(NULL, gssapi_delim);
        if (!gssapi_realm || strlen(gssapi_realm) == 0) {
          log_fatal ("GSSAPI principal must specify realm: user@realm");
      	}

        ldap_sasl_inst = malloc(sizeof(struct ldap_sasl_instance));
        if (ldap_sasl_inst == NULL)
          log_fatal("Could not allocate memory for sasl instance! Can not run!");

        ldap_sasl_inst->sasl_mech = ber_strdup("GSSAPI");
        if (ldap_sasl_inst->sasl_mech == NULL)
          log_fatal("Could not allocate memory to duplicate gssapi mechanism");

        ldap_sasl_inst->sasl_realm = ber_strdup(gssapi_realm);
        if (ldap_sasl_inst->sasl_realm == NULL)
          log_fatal("Could not allocate memory  to duplicate gssapi realm");

        ldap_sasl_inst->sasl_authz_id = ber_strdup(gssapi_user);
        if (ldap_sasl_inst->sasl_authz_id == NULL)
          log_fatal("Could not allocate memory  to duplicate gssapi user");

        ldap_sasl_inst->sasl_authc_id = NULL;
        ldap_sasl_inst->sasl_password = NULL; //"" before
        free(running);
      }
#endif

#if defined (LDAP_CASA_AUTH)
      if (!load_uname_pwd_from_miCASA(&ldap_username,&ldap_password))
        {
#if defined (DEBUG_LDAP)
          log_info ("Authentication credential taken from file");
#endif
#endif

      ldap_username = _do_lookup_dhcp_string_option (options, SV_LDAP_USERNAME);
      ldap_password = _do_lookup_dhcp_string_option (options, SV_LDAP_PASSWORD);

#if defined (LDAP_CASA_AUTH)
      }
#endif

      option_state_dereference (&options, MDL);
    }

  if (ldap_server == NULL || ldap_base_dn == NULL)
    {
      log_info ("Not searching LDAP since ldap-server, ldap-port and ldap-base-dn were not specified in the config file");
      ldap_method = LDAP_METHOD_STATIC;
      return;
    }

  if (ldap_debug_file != NULL && ldap_debug_fd == -1)
    {
      if ((ldap_debug_fd = open (ldap_debug_file, O_CREAT | O_TRUNC | O_WRONLY,
                                 S_IRUSR | S_IWUSR)) < 0)
        log_error ("Error opening debug LDAP log file %s: %s", ldap_debug_file,
                   strerror (errno));
    }

#if defined (DEBUG_LDAP)
  log_info ("Connecting to LDAP server %s:%d", ldap_server, ldap_port);
#endif

#if defined (LDAP_USE_SSL)
  if (ldap_use_ssl == -1)
    {
      /*
      ** There was no "ldap-ssl" option in dhcpd.conf (also not "off").
      ** Let's try, if we can use an anonymous TLS session without to
      ** verify the server certificate -- if not continue without TLS.
      */
      int opt = LDAP_OPT_X_TLS_ALLOW;
      if ((ret = ldap_set_option (NULL, LDAP_OPT_X_TLS_REQUIRE_CERT,
                                  &opt)) != LDAP_SUCCESS)
        {
          log_error ("Warning: Cannot set LDAP TLS require cert option to 'allow': %s",
                     ldap_err2string (ret));
        }
    }

  if (ldap_use_ssl != LDAP_SSL_OFF)
    {
      if (ldap_tls_reqcert != -1)
        {
          if ((ret = ldap_set_option (NULL, LDAP_OPT_X_TLS_REQUIRE_CERT,
                                      &ldap_tls_reqcert)) != LDAP_SUCCESS)
            {
              log_error ("Cannot set LDAP TLS require cert option: %s",
                         ldap_err2string (ret));
            }
        }

      if( ldap_tls_ca_file != NULL)
        {
          if ((ret = ldap_set_option (NULL, LDAP_OPT_X_TLS_CACERTFILE,
                                      ldap_tls_ca_file)) != LDAP_SUCCESS)
            {
              log_error ("Cannot set LDAP TLS CA certificate file %s: %s",
                         ldap_tls_ca_file, ldap_err2string (ret));
            }
        }
      if( ldap_tls_ca_dir != NULL)
        {
          if ((ret = ldap_set_option (NULL, LDAP_OPT_X_TLS_CACERTDIR,
                                      ldap_tls_ca_dir)) != LDAP_SUCCESS)
            {
              log_error ("Cannot set LDAP TLS CA certificate dir %s: %s",
                         ldap_tls_ca_dir, ldap_err2string (ret));
            }
        }
      if( ldap_tls_cert != NULL)
        {
          if ((ret = ldap_set_option (NULL, LDAP_OPT_X_TLS_CERTFILE,
                                      ldap_tls_cert)) != LDAP_SUCCESS)
            {
              log_error ("Cannot set LDAP TLS client certificate file %s: %s",
                         ldap_tls_cert, ldap_err2string (ret));
            }
        }
      if( ldap_tls_key != NULL)
        {
          if ((ret = ldap_set_option (NULL, LDAP_OPT_X_TLS_KEYFILE,
                                      ldap_tls_key)) != LDAP_SUCCESS)
            {
              log_error ("Cannot set LDAP TLS certificate key file %s: %s",
                         ldap_tls_key, ldap_err2string (ret));
            }
        }
      if( ldap_tls_crlcheck != -1)
        {
          int opt = ldap_tls_crlcheck;
          if ((ret = ldap_set_option (NULL, LDAP_OPT_X_TLS_CRLCHECK,
                                      &opt)) != LDAP_SUCCESS)
            {
              log_error ("Cannot set LDAP TLS crl check option: %s",
                         ldap_err2string (ret));
            }
        }
      if( ldap_tls_ciphers != NULL)
        {
          if ((ret = ldap_set_option (NULL, LDAP_OPT_X_TLS_CIPHER_SUITE,
                                      ldap_tls_ciphers)) != LDAP_SUCCESS)
            {
              log_error ("Cannot set LDAP TLS cipher suite %s: %s",
                         ldap_tls_ciphers, ldap_err2string (ret));
            }
        }
      if( ldap_tls_randfile != NULL)
        {
          if ((ret = ldap_set_option (NULL, LDAP_OPT_X_TLS_RANDOM_FILE,
                                      ldap_tls_randfile)) != LDAP_SUCCESS)
            {
              log_error ("Cannot set LDAP TLS random file %s: %s",
                         ldap_tls_randfile, ldap_err2string (ret));
            }
        }
    }
#endif

  /* enough for 'ldap://+ + hostname + ':' + port number */
  uri = malloc(strlen(ldap_server) + 16);
  if (uri == NULL)
    {
      log_error ("Cannot build ldap init URI %s:%d", ldap_server, ldap_port);
      return;
    }

  sprintf(uri, "ldap://%s:%d", ldap_server, ldap_port);
  ldap_initialize(&ld, uri);

  if (ld == NULL)
    {
      log_error ("Cannot init ldap session to %s:%d", ldap_server, ldap_port);
      return;
    }

  free(uri);

  version = LDAP_VERSION3;
  if ((ret = ldap_set_option (ld, LDAP_OPT_PROTOCOL_VERSION, &version)) != LDAP_OPT_SUCCESS)
    {
      log_error ("Cannot set LDAP version to %d: %s", version,
                 ldap_err2string (ret));
    }

  if (ldap_referrals != -1)
    {
      if ((ret = ldap_set_option (ld, LDAP_OPT_REFERRALS, ldap_referrals ?
                                  LDAP_OPT_ON : LDAP_OPT_OFF)) != LDAP_OPT_SUCCESS)
        {
          log_error ("Cannot %s LDAP referrals option: %s",
                     (ldap_referrals ? "enable" : "disable"),
                     ldap_err2string (ret));
        }
    }

  if ((ret = ldap_set_rebind_proc(ld, ldap_rebind_cb, NULL)) != LDAP_SUCCESS)
    {
      log_error ("Warning: Cannot set ldap rebind procedure: %s",
                 ldap_err2string (ret));
    }

#if defined (LDAP_USE_SSL)
  if (ldap_use_ssl == LDAP_SSL_LDAPS ||
     (ldap_use_ssl == LDAP_SSL_ON && ldap_port == LDAPS_PORT))
    {
      int opt = LDAP_OPT_X_TLS_HARD;
      if ((ret = ldap_set_option (ld, LDAP_OPT_X_TLS, &opt)) != LDAP_SUCCESS)
        {
          log_error ("Error: Cannot init LDAPS session to %s:%d: %s",
                    ldap_server, ldap_port, ldap_err2string (ret));
          ldap_stop();
          return;
        }
      else
        {
          log_info ("LDAPS session successfully enabled to %s:%d",
                    ldap_server, ldap_port);
        }
    }
  else if (ldap_use_ssl != LDAP_SSL_OFF)
    {
      do
        {
          ret = ldap_start_tls_s (ld, NULL, NULL);
        }
      while(_do_ldap_retry(ret, ldap_server, ldap_port) > 0);

      if (ret != LDAP_SUCCESS)
        {
          log_error ("Error: Cannot start TLS session to %s:%d: %s",
                     ldap_server, ldap_port, ldap_err2string (ret));
          ldap_stop();
          return;
        }
      else
        {
          log_info ("TLS session successfully started to %s:%d",
                    ldap_server, ldap_port);
        }
    }
#endif

#if defined(LDAP_USE_GSSAPI)
    if (ldap_gssapi_principal != NULL) {
      krb5_get_tgt(ldap_gssapi_principal, ldap_gssapi_keytab);
      if ((ret = ldap_sasl_interactive_bind_s(ld, NULL, ldap_sasl_inst->sasl_mech,
                                              NULL, NULL, LDAP_SASL_AUTOMATIC,
                                              _ldap_sasl_interact, ldap_sasl_inst)
          ) != LDAP_SUCCESS)
        {
        log_error ("Error: Cannot SASL bind to ldap server %s:%d: %s",
                   ldap_server, ldap_port, ldap_err2string (ret));
        char *msg=NULL;
        ldap_get_option( ld, LDAP_OPT_DIAGNOSTIC_MESSAGE, (void*)&msg);
          log_error ("\tAdditional info: %s", msg);
          ldap_memfree(msg);
          ldap_stop();
          return;
        }
      } else
#endif

  if (ldap_username != NULL && *ldap_username != '\0' && ldap_password != NULL)
    {
      creds.bv_val = strdup(ldap_password);
      if (creds.bv_val == NULL)
          log_fatal ("Error: Unable to allocate memory to duplicate ldap_password");

      creds.bv_len = strlen(ldap_password);

      do
        {
          ret = ldap_sasl_bind_s (ld, ldap_username, LDAP_SASL_SIMPLE,
                                  &creds, NULL, NULL, NULL);
        }
      while(_do_ldap_retry(ret, ldap_server, ldap_port) > 0);
      free(creds.bv_val);

      if (ret != LDAP_SUCCESS)
        {
          log_error ("Error: Cannot login into ldap server %s:%d: %s",
                     ldap_server, ldap_port, ldap_err2string (ret));
          ldap_stop();
          return;
        }
    }

#if defined (DEBUG_LDAP)
  log_info ("Successfully logged into LDAP server %s", ldap_server);
#endif
}


static void
parse_external_dns (LDAPMessage * ent)
{
  char *search[] = {"dhcpOptionsDN", "dhcpSharedNetworkDN", "dhcpSubnetDN",
                    "dhcpGroupDN", "dhcpHostDN", "dhcpClassesDN",
                    "dhcpPoolDN", "dhcpZoneDN", "dhcpFailOverPeerDN", NULL};

  /* TODO: dhcpKeyDN can't be added. It is referenced in dhcpDnsZone to
     retrive the key name (cn). Adding keyDN will reflect adding a key
     declaration inside the zone configuration.

     dhcpSubClassesDN cant be added. It is also similar to the above.
     Needs schema change.
   */
  LDAPMessage * newres, * newent;
  struct berval **tempbv;
  int i, j, ret;
#if defined (DEBUG_LDAP)
  char *dn;

  dn = ldap_get_dn (ld, ent);
  if (dn != NULL)
    {
      log_info ("Parsing external DNs for '%s'", dn);
      ldap_memfree (dn);
    }
#endif

  if (ld == NULL)
    ldap_start ();
  if (ld == NULL)
    return;

  for (i=0; search[i] != NULL; i++)
    {
      if ((tempbv = ldap_get_values_len (ld, ent, search[i])) == NULL)
        continue;

      for (j=0; tempbv[j] != NULL; j++)
        {
          if (*tempbv[j]->bv_val == '\0')
            continue;

          if ((ret = ldap_search_ext_s(ld, tempbv[j]->bv_val, LDAP_SCOPE_BASE,
                                       "objectClass=*", NULL, 0, NULL,
                                       NULL, NULL, 0, &newres)) != LDAP_SUCCESS)
            {
              ldap_value_free_len (tempbv);
              ldap_stop();
              return;
            }
    
#if defined (DEBUG_LDAP)
          log_info ("Adding contents of subtree '%s' to config stack from '%s' reference", tempbv[j]->bv_val, search[i]);
#endif
          for (newent = ldap_first_entry (ld, newres);
               newent != NULL;
               newent = ldap_next_entry (ld, newent))
            {
#if defined (DEBUG_LDAP)
              dn = ldap_get_dn (ld, newent);
              if (dn != NULL)
                {
                  log_info ("Adding LDAP result set starting with '%s' to config stack", dn);
                  ldap_memfree (dn);
                }
#endif

              add_to_config_stack (newres, newent);
              /* don't free newres here */
            }
        }

      ldap_value_free_len (tempbv);
    }
}


static void
free_stack_entry (struct ldap_config_stack *item)
{
  struct ldap_config_stack *look_ahead_pointer = item;
  int may_free_msg = 1;

  while (look_ahead_pointer->next != NULL)
    {
      look_ahead_pointer = look_ahead_pointer->next;
      if (look_ahead_pointer->res == item->res)
        {
          may_free_msg = 0;
          break;
        }
    }

  if (may_free_msg) 
    ldap_msgfree (item->res);

  dfree (item, MDL);
}


static void
next_ldap_entry (struct parse *cfile)
{
  struct ldap_config_stack *temp_stack;

  if (ldap_stack != NULL && ldap_stack->close_brace)
    {
      x_parser_strcat (cfile, "}\n");
      ldap_stack->close_brace = 0;
    }

  while (ldap_stack != NULL && 
         (ldap_stack->ldent == NULL || ( ldap_stack->processed &&
          (ldap_stack->ldent = ldap_next_entry (ld, ldap_stack->ldent)) == NULL)))
    {
      if (ldap_stack->close_brace)
        {
          x_parser_strcat (cfile, "}\n");
          ldap_stack->close_brace = 0;
        }

      temp_stack = ldap_stack;
      ldap_stack = ldap_stack->next;
      free_stack_entry (temp_stack);
    }

  if (ldap_stack != NULL && ldap_stack->close_brace)
    {
      x_parser_strcat (cfile, "}\n");
      ldap_stack->close_brace = 0;
    }
}


static char
check_statement_end (const char *statement)
{
  char *ptr;

  if (statement == NULL || *statement == '\0')
    return ('\0');

  /*
  ** check if it ends with "}", e.g.:
  **   "zone my.domain. { ... }"
  ** optionally followed by spaces
  */
  ptr = strrchr (statement, '}');
  if (ptr != NULL)
    {
      /* skip following white-spaces */
      for (++ptr; isspace ((int)*ptr); ptr++);

      /* check if we reached the end */
      if (*ptr == '\0')
        return ('}'); /* yes, block end */
      else
        return (*ptr);
    }

  /*
  ** this should not happen, but...
  ** check if it ends with ";", e.g.:
  **   "authoritative;"
  ** optionally followed by spaces
  */
  ptr = strrchr (statement, ';');
  if (ptr != NULL)
    {
      /* skip following white-spaces */
      for (++ptr; isspace ((int)*ptr); ptr++);

      /* check if we reached the end */
      if (*ptr == '\0')
        return (';'); /* ends with a ; */
      else
        return (*ptr);
    }

  return ('\0');
}


static isc_result_t
ldap_parse_entry_options (LDAPMessage *ent, struct parse *cfile,
                          int *lease_limit)
{
  struct berval **tempbv;
  int i;

  if (ent == NULL || cfile == NULL)
    return (ISC_R_FAILURE);

  if ((tempbv = ldap_get_values_len (ld, ent, "dhcpStatements")) != NULL)
    {
      for (i=0; tempbv[i] != NULL; i++)
        {
          if (lease_limit != NULL &&
              strncasecmp ("lease limit ", tempbv[i]->bv_val, 12) == 0)
            {
              *lease_limit = (int) strtol ((tempbv[i]->bv_val) + 12, NULL, 10);
              continue;
            }

          x_parser_strcat (cfile, tempbv[i]->bv_val);

          switch((int) check_statement_end (tempbv[i]->bv_val))
            {
              case '}':
              case ';':
                x_parser_strcat (cfile, "\n");
                break;
              default:
                x_parser_strcat (cfile, ";\n");
                break;
            }
        }
      ldap_value_free_len (tempbv);
    }

  if ((tempbv = ldap_get_values_len (ld, ent, "dhcpOption")) != NULL)
    {
      for (i=0; tempbv[i] != NULL; i++)
        {
          x_parser_strcat (cfile, "option ");
          x_parser_strcat (cfile, tempbv[i]->bv_val);
          switch ((int) check_statement_end (tempbv[i]->bv_val))
            {
              case ';':
                x_parser_strcat (cfile, "\n");
                break;
              default:
                x_parser_strcat (cfile, ";\n");
                break;
            }
        }
      ldap_value_free_len (tempbv);
    }

  return (ISC_R_SUCCESS);
}


static void
ldap_generate_config_string (struct parse *cfile)
{
  struct berval **objectClass;
  char *dn;
  struct ldap_config_stack *entry;
  LDAPMessage * ent, * res, *entfirst, *resfirst;
  int i, ignore, found;
  int ret, parsedn = 1;
  size_t len = cfile->buflen;

  if (ld == NULL)
    ldap_start ();
  if (ld == NULL)
    return;

  entry = ldap_stack;
  if ((objectClass = ldap_get_values_len (ld, entry->ldent, 
                                      "objectClass")) == NULL)
    return;

  entry->processed = 1;
  ignore = 0;
  found = 1;
  for (i=0; objectClass[i] != NULL; i++)
    {
      if (strcasecmp (objectClass[i]->bv_val, "dhcpSharedNetwork") == 0)
        ldap_parse_shared_network (entry, cfile);
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpClass") == 0)
        ldap_parse_class (entry, cfile);
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpSubnet") == 0)
        ldap_parse_subnet (entry, cfile);
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpSubnet6") == 0)
        ldap_parse_subnet6 (entry, cfile);
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpPool") == 0)
        ldap_parse_pool (entry, cfile);
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpPool6") == 0)
        ldap_parse_pool6 (entry, cfile);
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpGroup") == 0)
        ldap_parse_group (entry, cfile);
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpTSigKey") == 0)
        ldap_parse_key (entry, cfile);
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpDnsZone") == 0)
        ldap_parse_zone (entry, cfile);
#if defined(HAVE_IFADDRS_H)
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpFailOverPeer") == 0)
        ldap_parse_failover (entry, cfile);
#endif
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpHost") == 0)
        {
          if (ldap_method == LDAP_METHOD_STATIC)
            ldap_parse_host (entry, cfile);
          else
            {
              ignore = 1;
              break;
            }
        }
      else if (strcasecmp (objectClass[i]->bv_val, "dhcpSubClass") == 0)
        {
          if (ldap_method == LDAP_METHOD_STATIC)
            ldap_parse_subclass (entry, cfile);
          else
            {
              ignore = 1;
              break;
            }
        }
      else
        found = 0;

      if (found && x_parser_length(cfile) <= len)
        {
          ignore = 1;
          break;
        }
    }

  ldap_value_free_len (objectClass);

  if (ignore)
    {
      next_ldap_entry (cfile);
      return;
    }

  ldap_parse_entry_options(entry->ldent, cfile, NULL);

  dn = ldap_get_dn (ld, entry->ldent);
  if (dn == NULL)
    {
      ldap_stop();
      return;
    }
#if defined(DEBUG_LDAP)
  log_info ("Found LDAP entry '%s'", dn);
#endif

  if ((ret = ldap_search_ext_s (ld, dn, LDAP_SCOPE_ONELEVEL,
                                "(!(|(|(objectClass=dhcpTSigKey)(objectClass=dhcpClass)) (objectClass=dhcpFailOverPeer)))",
                                NULL, 0, NULL, NULL,
                                NULL, 0, &res)) != LDAP_SUCCESS)
    {
      ldap_memfree (dn);

      ldap_stop();
      return;
    }

  if ((ret = ldap_search_ext_s (ld, dn, LDAP_SCOPE_ONELEVEL,
                                "(|(|(objectClass=dhcpTSigKey)(objectClass=dhcpClass)) (objectClass=dhcpFailOverPeer))",
                                NULL, 0, NULL, NULL,
                                NULL, 0, &resfirst)) != LDAP_SUCCESS)
    {
      ldap_memfree (dn);
      ldap_msgfree (res);

      ldap_stop();
      return;
    }

  ldap_memfree (dn);

  ent = ldap_first_entry(ld, res);
  entfirst = ldap_first_entry(ld, resfirst);

  if (ent == NULL && entfirst == NULL)
    {
      parse_external_dns (entry->ldent);
      next_ldap_entry (cfile);
    }

  if (ent != NULL)
    {
      add_to_config_stack (res, ent);
      parse_external_dns (entry->ldent);
      parsedn = 0;
    }
  else
    ldap_msgfree (res);

  if (entfirst != NULL)
    {
      add_to_config_stack (resfirst, entfirst);
      if(parsedn)
        parse_external_dns (entry->ldent);

    }
  else
    ldap_msgfree (resfirst);
}


static void
ldap_close_debug_fd()
{
  if (ldap_debug_fd != -1)
    {
      close (ldap_debug_fd);
      ldap_debug_fd = -1;
    }
}


static void
ldap_write_debug (const void *buff, size_t size)
{
  if (ldap_debug_fd != -1)
    {
      if (write (ldap_debug_fd, buff, size) < 0)
        {
          log_error ("Error writing to LDAP debug file %s: %s."
                     " Disabling log file.", ldap_debug_file,
                     strerror (errno));
          ldap_close_debug_fd();
        }
    }
}

static int
ldap_read_function (struct parse *cfile)
{
  size_t len;

  /* append when in saved state */
  if (cfile->saved_state == NULL)
    {
      cfile->inbuf[0] = '\0';
      cfile->bufix = 0;
      cfile->buflen = 0;
    }
  len = cfile->buflen;

  while (ldap_stack != NULL && x_parser_length(cfile) <= len)
    ldap_generate_config_string (cfile);

  if (x_parser_length(cfile) <= len && ldap_stack == NULL)
    return (EOF);

  if (cfile->buflen > len)
    ldap_write_debug (cfile->inbuf + len, cfile->buflen - len);
#if defined (DEBUG_LDAP)
  log_info ("Sending config portion '%s'", cfile->inbuf + len);
#endif

  return (cfile->inbuf[cfile->bufix++]);
}


static char *
ldap_get_host_name (LDAPMessage * ent)
{
  struct berval **name;
  char *ret;

  ret = NULL;
  if ((name = ldap_get_values_len (ld, ent, "cn")) == NULL || name[0] == NULL)
    {
      if (name != NULL)
        ldap_value_free_len (name);

#if defined (DEBUG_LDAP)
      ret = ldap_get_dn (ld, ent);
      if (ret != NULL)
        {
          log_info ("Cannot get cn attribute for LDAP entry %s", ret);
          ldap_memfree(ret);
        }
#endif
      return (NULL);
    }

  ret = dmalloc (strlen (name[0]->bv_val) + 1, MDL);
  strcpy (ret, name[0]->bv_val);
  ldap_value_free_len (name);

  return (ret);
}


isc_result_t
ldap_read_config (void)
{
  LDAPMessage * ldres, * hostres, * ent, * hostent;
  char hfilter[1024], sfilter[1024], fqdn[257];
  char *hostdn;
  ldap_dn_node *curr = NULL;
  struct parse *cfile;
  struct utsname unme;
  isc_result_t res;
  size_t length;
  int ret, cnt;
  struct berval **tempbv = NULL;
  struct berval bv_o[2];

  cfile = x_parser_init("LDAP");
  if (cfile == NULL)
    return (ISC_R_NOMEMORY);

  ldap_enable_retry = 1;
  if (ld == NULL)
    ldap_start ();
  ldap_enable_retry = 0;

  if (ld == NULL)
    {
      x_parser_free(&cfile);
      return (ldap_server == NULL ? ISC_R_SUCCESS : ISC_R_FAILURE);
    }

  uname (&unme);
  if (ldap_dhcp_server_cn != NULL)
    {
      if (_do_ldap_str2esc_filter_bv(ldap_dhcp_server_cn, 0, &bv_o[0]) == NULL)
        {
          log_error ("Cannot escape ldap filter value %s: %m", ldap_dhcp_server_cn);
          x_parser_free(&cfile);
          return (ISC_R_FAILURE);
        }

     snprintf (hfilter, sizeof (hfilter),
                "(&(objectClass=dhcpServer)(cn=%s))", bv_o[0].bv_val);

     ber_memfree(bv_o[0].bv_val);
    }
  else
    {
      if (_do_ldap_str2esc_filter_bv(unme.nodename, 0, &bv_o[0]) == NULL)
        {
          log_error ("Cannot escape ldap filter value %s: %m", unme.nodename);
          x_parser_free(&cfile);
          return (ISC_R_FAILURE);
        }

      *fqdn ='\0';
      if(0 == get_host_entry(fqdn, sizeof(fqdn), NULL, 0))
        {
          if (_do_ldap_str2esc_filter_bv(fqdn, 0, &bv_o[1]) == NULL)
            {
              log_error ("Cannot escape ldap filter value %s: %m", fqdn);
              ber_memfree(bv_o[0].bv_val);
              x_parser_free(&cfile);
              return (ISC_R_FAILURE);
            }
        }

       // If we have fqdn and it isn't the same as nodename, use it in filter
       // otherwise just use nodename
       if ((*fqdn) && (strcmp(unme.nodename, fqdn))) {
          snprintf (hfilter, sizeof (hfilter),
                    "(&(objectClass=dhcpServer)(|(cn=%s)(cn=%s)))", 
                    bv_o[0].bv_val, bv_o[1].bv_val);

          ber_memfree(bv_o[1].bv_val);
        }
      else
        {
          snprintf (hfilter, sizeof (hfilter),
                    "(&(objectClass=dhcpServer)(cn=%s))",
                    bv_o[0].bv_val);
        }

      ber_memfree(bv_o[0].bv_val);
    }

  ldap_enable_retry = 1;
  do
    {
      hostres = NULL;
      ret = ldap_search_ext_s (ld, ldap_base_dn, LDAP_SCOPE_SUBTREE,
                                hfilter, NULL, 0, NULL, NULL, NULL, 0,
                                &hostres);
    }
  while(_do_ldap_retry(ret, ldap_server, ldap_port) > 0);
  ldap_enable_retry = 0;

  if(ret != LDAP_SUCCESS)
    {
      log_error ("Cannot find host LDAP entry %s %s",
                 ((ldap_dhcp_server_cn == NULL)?(unme.nodename):(ldap_dhcp_server_cn)), hfilter);
      if(NULL != hostres)
        ldap_msgfree (hostres);
      ldap_stop();
      x_parser_free(&cfile);
      return (ISC_R_FAILURE);
    }

  if ((hostent = ldap_first_entry (ld, hostres)) == NULL)
    {
      log_error ("Error: Cannot find LDAP entry matching %s", hfilter);
      ldap_msgfree (hostres);
      ldap_stop();
      x_parser_free(&cfile);
      return (ISC_R_FAILURE);
    }

  hostdn = ldap_get_dn (ld, hostent);
#if defined(DEBUG_LDAP)
  if (hostdn != NULL)
    log_info ("Found dhcpServer LDAP entry '%s'", hostdn);
#endif

  if (hostdn == NULL ||
      (tempbv = ldap_get_values_len (ld, hostent, "dhcpServiceDN")) == NULL ||
      tempbv[0] == NULL)
    {
      log_error ("Error: No dhcp service is associated with the server %s %s",
                 (hostdn ? "dn" : "name"), (hostdn ? hostdn :
                 (ldap_dhcp_server_cn ? ldap_dhcp_server_cn : unme.nodename)));

      if (tempbv != NULL)
        ldap_value_free_len (tempbv);

      if (hostdn)
        ldap_memfree (hostdn);
      ldap_msgfree (hostres);
      ldap_stop();
      x_parser_free(&cfile);
      return (ISC_R_FAILURE);
    }

#if defined(DEBUG_LDAP)
  log_info ("LDAP: Parsing dhcpServer options '%s' ...", hostdn);
#endif

  res = ldap_parse_entry_options(hostent, cfile, NULL);
  if (res != ISC_R_SUCCESS)
    {
      ldap_value_free_len (tempbv);
      ldap_msgfree (hostres);
      ldap_memfree (hostdn);
      ldap_stop();
      x_parser_free(&cfile);
      return res;
    }

  if (x_parser_length(cfile) > 0)
    {
      ldap_write_debug(cfile->inbuf, cfile->buflen);

      res = conf_file_subparse (cfile, root_group, ROOT_GROUP);
      if (res != ISC_R_SUCCESS)
        {
          log_error ("LDAP: cannot parse dhcpServer entry '%s'", hostdn);
          ldap_value_free_len (tempbv);
          ldap_msgfree (hostres);
          ldap_memfree (hostdn);
          ldap_stop();
          x_parser_free(&cfile);
          return res;
        }
      x_parser_reset(cfile);
    }
  ldap_msgfree (hostres);

  res = ISC_R_SUCCESS;
  for (cnt=0; tempbv[cnt] != NULL; cnt++)
    {

      if (_do_ldap_str2esc_filter_bv(hostdn, 0, &bv_o[0]) == NULL)
        {
          log_error ("Cannot escape ldap filter value %s: %m", hostdn);
          res = ISC_R_FAILURE;
          break;
        }

      snprintf(sfilter, sizeof(sfilter), "(&(objectClass=dhcpService)"
                        "(|(|(dhcpPrimaryDN=%s)(dhcpSecondaryDN=%s))(dhcpServerDN=%s)))",
                        bv_o[0].bv_val, bv_o[0].bv_val, bv_o[0].bv_val);

      ber_memfree(bv_o[0].bv_val);

      ldres = NULL;
      if ((ret = ldap_search_ext_s (ld, tempbv[cnt]->bv_val, LDAP_SCOPE_BASE,
                                    sfilter, NULL, 0, NULL, NULL, NULL,
                                    0, &ldres)) != LDAP_SUCCESS)
        {
          log_error ("Error searching for dhcpServiceDN '%s': %s. Please update the LDAP entry '%s'",
                     tempbv[cnt]->bv_val, ldap_err2string (ret), hostdn);
          if(NULL != ldres)
            ldap_msgfree(ldres);
          res = ISC_R_FAILURE;
          break;
        }

      if ((ent = ldap_first_entry (ld, ldres)) == NULL)
        {
          log_error ("Error: Cannot find dhcpService DN '%s' with server reference. Please update the LDAP server entry '%s'",
                     tempbv[cnt]->bv_val, hostdn);

          ldap_msgfree(ldres);
          res = ISC_R_FAILURE;
          break;
        }

      /*
      ** FIXME: how to free the remembered dn's on exit?
      **        This should be OK if dmalloc registers the
      **        memory it allocated and frees it on exit..
      */

      curr = dmalloc (sizeof (*curr), MDL);
      if (curr != NULL)
        {
          length = strlen (tempbv[cnt]->bv_val);
          curr->dn = dmalloc (length + 1, MDL);
          if (curr->dn == NULL)
            {
              dfree (curr, MDL);
              curr = NULL;
            }
          else
            strcpy (curr->dn, tempbv[cnt]->bv_val);
        }

      if (curr != NULL)
        {
          curr->refs++;

          /* append to service-dn list */
          if (ldap_service_dn_tail != NULL)
            ldap_service_dn_tail->next = curr;
          else
            ldap_service_dn_head = curr;

          ldap_service_dn_tail = curr;
        }
      else
        log_fatal ("no memory to remember ldap service dn");

#if defined (DEBUG_LDAP)
      log_info ("LDAP: Parsing dhcpService DN '%s' ...", tempbv[cnt]->bv_val);
#endif
      add_to_config_stack (ldres, ent);
      res = conf_file_subparse (cfile, root_group, ROOT_GROUP);
      if (res != ISC_R_SUCCESS)
        {
          log_error ("LDAP: cannot parse dhcpService entry '%s'", tempbv[cnt]->bv_val);
          break;
        }
    }

  x_parser_free(&cfile);
  ldap_close_debug_fd();

  ldap_memfree (hostdn);
  ldap_value_free_len (tempbv);

  if (res != ISC_R_SUCCESS)
    {
      struct ldap_config_stack *temp_stack;

      while ((curr = ldap_service_dn_head) != NULL)
        {
          ldap_service_dn_head = curr->next;
          dfree (curr->dn, MDL);
          dfree (curr, MDL);
        }

      ldap_service_dn_tail = NULL;

      while ((temp_stack = ldap_stack) != NULL)
        {
          ldap_stack = temp_stack->next;
          free_stack_entry (temp_stack);
        }

      ldap_stop();
    }

  /* Unbind from ldap immediately after reading config in static mode. */
  if (ldap_method == LDAP_METHOD_STATIC)
    ldap_stop();

  return (res);
}


/* This function will parse the dhcpOption and dhcpStatements field in the LDAP
   entry if it exists. Right now, type will be either HOST_DECL or CLASS_DECL.
   If we are parsing a HOST_DECL, this always returns 0. If we are parsing a 
   CLASS_DECL, this will return what the current lease limit is in LDAP. If
   there is no lease limit specified, we return 0 */

static int
ldap_parse_options (LDAPMessage * ent, struct group *group,
                         int type, struct host_decl *host,
                         struct class **class)
{
  int declaration, lease_limit;
  enum dhcp_token token;
  struct parse *cfile;
  isc_result_t res;
  const char *val;

  lease_limit = 0;
  cfile = x_parser_init(type == HOST_DECL ? "LDAP-HOST" : "LDAP-SUBCLASS");
  if (cfile == NULL)
    return (lease_limit);

  /* This block of code will try to find the parent of the host, and
     if it is a group object, fetch the options and apply to the host. */
  if (type == HOST_DECL) 
    {
      char *hostdn, *basedn, *temp1, *temp2, filter[1024];
      LDAPMessage *groupdn, *entry;
      int ret;

      hostdn = ldap_get_dn (ld, ent);
      if( hostdn != NULL)
        {
          basedn = NULL;

          temp1 = strchr (hostdn, '=');
          if (temp1 != NULL)
            temp1 = strchr (++temp1, '=');
          if (temp1 != NULL)
            temp2 = strchr (++temp1, ',');
          else
            temp2 = NULL;

          if (temp2 != NULL)
            {
              struct berval bv_o;

              if (_do_ldap_str2esc_filter_bv(temp1, (temp2 - temp1), &bv_o) == NULL)
                {
                  log_error ("Cannot escape ldap filter value %.*s: %m",
                              (int)(temp2 - temp1), temp1);
                  filter[0] = '\0';
                }
              else
                {
                  snprintf (filter, sizeof(filter),
                            "(&(cn=%s)(objectClass=dhcpGroup))",
                            bv_o.bv_val);

                  ber_memfree(bv_o.bv_val); 
                }

              basedn = strchr (temp1, ',');
              if (basedn != NULL)
                ++basedn;
            }

          if (basedn != NULL && *basedn != '\0' && filter[0] != '\0')
            {
              ret = ldap_search_ext_s (ld, basedn, LDAP_SCOPE_SUBTREE, filter,
                                       NULL, 0, NULL, NULL, NULL, 0, &groupdn);
              if (ret == LDAP_SUCCESS)
                {
                  if ((entry = ldap_first_entry (ld, groupdn)) != NULL)
                    {
                      res = ldap_parse_entry_options (entry, cfile, &lease_limit);
                      if (res != ISC_R_SUCCESS)
                        {
                          /* reset option buffer discarding any results */
                          x_parser_reset(cfile);
                          lease_limit = 0;
                        }
                    }
                  ldap_msgfree( groupdn);
                }
            }
          ldap_memfree( hostdn);
        }
    }

  res = ldap_parse_entry_options (ent, cfile, &lease_limit);
  if (res != ISC_R_SUCCESS)
    {
      x_parser_free(&cfile);
      return (lease_limit);
    }

  if (x_parser_length(cfile) == 0)
    {
      x_parser_free(&cfile);
      return (lease_limit);
    }

  declaration = 0;
  do
    {
      token = peek_token (&val, NULL, cfile);
      if (token == END_OF_FILE)
        break;
       declaration = parse_statement (cfile, group, type, host, declaration);
    } while (1);

  x_parser_free(&cfile);

  return (lease_limit);
}



int
find_haddr_in_ldap (struct host_decl **hp, int htype, unsigned hlen,
                    const unsigned char *haddr, const char *file, int line)
{
  char buf[128], *type_str;
  LDAPMessage * res, *ent;
  struct host_decl * host;
  isc_result_t status;
  ldap_dn_node *curr;
  char up_hwaddr[20];
  char lo_hwaddr[20];
  int ret;
  struct berval bv_o[2];

  *hp = NULL;


  if (ldap_method == LDAP_METHOD_STATIC)
    return (0);

  if (ld == NULL)
    ldap_start ();
  if (ld == NULL)
    return (0);

  switch (htype)
    {
      case HTYPE_ETHER:
        type_str = "ethernet";
        break;
      case HTYPE_IEEE802:
        type_str = "token-ring";
        break;
      case HTYPE_FDDI:
        type_str = "fddi";
        break;
      default:
        log_info ("Ignoring unknown type %d", htype);
        return (0);
    }

  /*
  ** FIXME: It is not guaranteed, that the dhcpHWAddress attribute
  **        contains _exactly_ "type addr" with one space between!
  */
  snprintf(lo_hwaddr, sizeof(lo_hwaddr), "%s",
           print_hw_addr (htype, hlen, haddr));
  x_strxform(up_hwaddr, lo_hwaddr, sizeof(up_hwaddr), toupper);

  if (_do_ldap_str2esc_filter_bv(lo_hwaddr, 0, &bv_o[0]) == NULL)
    {
      log_error ("Cannot escape ldap filter value %s: %m", lo_hwaddr);
      return (0);
    }
  if (_do_ldap_str2esc_filter_bv(up_hwaddr, 0, &bv_o[1]) == NULL)
    {
      log_error ("Cannot escape ldap filter value %s: %m", up_hwaddr);
      ber_memfree(bv_o[0].bv_val);
      return (0);
    }

  snprintf (buf, sizeof (buf),
            "(&(objectClass=dhcpHost)(|(dhcpHWAddress=%s %s)(dhcpHWAddress=%s %s)))",
            type_str, bv_o[0].bv_val, type_str, bv_o[1].bv_val);

  ber_memfree(bv_o[0].bv_val);
  ber_memfree(bv_o[1].bv_val);

  res = ent = NULL;
  for (curr = ldap_service_dn_head;
       curr != NULL && *curr->dn != '\0';
       curr = curr->next)
    {
#if defined (DEBUG_LDAP)
      log_info ("Searching for %s in LDAP tree %s", buf, curr->dn);
#endif
      ret = ldap_search_ext_s (ld, curr->dn, LDAP_SCOPE_SUBTREE, buf, NULL, 0,
                               NULL, NULL, NULL, 0, &res);

      if(ret == LDAP_SERVER_DOWN)
        {
          log_info ("LDAP server was down, trying to reconnect...");

          ldap_stop();
          ldap_start();
          if(ld == NULL)
            {
              log_info ("LDAP reconnect failed - try again later...");
              return (0);
            }

          ret = ldap_search_ext_s (ld, curr->dn, LDAP_SCOPE_SUBTREE, buf, NULL,
                                   0, NULL, NULL, NULL, 0, &res);
        }

      if (ret == LDAP_SUCCESS)
        {
          ent = ldap_first_entry (ld, res);
#if defined (DEBUG_LDAP)
          if (ent == NULL) {
            log_info ("No host entry for %s in LDAP tree %s",
                      buf, curr->dn);
	  }
#endif
          while (ent != NULL) {
#if defined (DEBUG_LDAP)
            char *dn = ldap_get_dn (ld, ent);
            if (dn != NULL)
              {
                log_info ("Found dhcpHWAddress LDAP entry %s", dn);
                ldap_memfree(dn);
              }
#endif

            host = (struct host_decl *)0;
            status = host_allocate (&host, MDL);
            if (status != ISC_R_SUCCESS)
              {
                log_fatal ("can't allocate host decl struct: %s", 
                           isc_result_totext (status)); 
                ldap_msgfree (res);
                return (0);
              }

            host->name = ldap_get_host_name (ent);
            if (host->name == NULL)
              {
                host_dereference (&host, MDL);
                ldap_msgfree (res);
                return (0);
              }

            if (!clone_group (&host->group, root_group, MDL))
              {
                log_fatal ("can't clone group for host %s", host->name);
                host_dereference (&host, MDL);
                ldap_msgfree (res);
                return (0);
              }

            ldap_parse_options (ent, host->group, HOST_DECL, host, NULL);

            host->n_ipaddr = *hp;
            *hp = host;
            ent = ldap_next_entry (ld, ent);
          }
          if(res)
            {
              ldap_msgfree (res);
              res = NULL;
            }
          return (*hp != NULL);
        }
      else
        {
          if(res)
            {
              ldap_msgfree (res);
              res = NULL;
            }

          if (ret != LDAP_NO_SUCH_OBJECT && ret != LDAP_SUCCESS)
            {
              log_error ("Cannot search for %s in LDAP tree %s: %s", buf, 
                         curr->dn, ldap_err2string (ret));
              ldap_stop();
              return (0);
            }
#if defined (DEBUG_LDAP)
          else
            {
              log_info ("ldap_search_ext_s returned %s when searching for %s in %s",
                        ldap_err2string (ret), buf, curr->dn);
            }
#endif
        }
    }

  return (0);
}


int
find_subclass_in_ldap (struct class *class, struct class **newclass, 
                       struct data_string *data)
{
  LDAPMessage * res, * ent;
  int ret, lease_limit;
  isc_result_t status;
  ldap_dn_node *curr;
  char buf[2048];
  struct berval bv_class;
  struct berval bv_cdata;
  char *hex_1;

  if (ldap_method == LDAP_METHOD_STATIC)
    return (0);

  if (ld == NULL)
    ldap_start ();
  if (ld == NULL)
    return (0);

  hex_1 = print_hex_1 (data->len, data->data, 1024);
  if (*hex_1 == '"')
    {
      /* result is a quotted not hex string: ldap escape the original string */
      if (_do_ldap_str2esc_filter_bv((const char*)data->data, data->len, &bv_cdata) == NULL)
        {
          log_error ("Cannot escape ldap filter value %s: %m", hex_1);
          return (0);
        }
        hex_1 = NULL;
    }
  if (_do_ldap_str2esc_filter_bv(class->name, strlen (class->name), &bv_class) == NULL)
    {
      log_error ("Cannot escape ldap filter value %s: %m", class->name);
      if (hex_1 == NULL)
        ber_memfree(bv_cdata.bv_val);
      return (0);
    }

  snprintf (buf, sizeof (buf),
            "(&(objectClass=dhcpSubClass)(cn=%s)(dhcpClassData=%s))",
            (hex_1 == NULL ? bv_cdata.bv_val : hex_1), bv_class.bv_val);

  if (hex_1 == NULL)
    ber_memfree(bv_cdata.bv_val);
  ber_memfree(bv_class.bv_val);

#if defined (DEBUG_LDAP)
  log_info ("Searching LDAP for %s", buf);
#endif

  res = ent = NULL;
  for (curr = ldap_service_dn_head;
       curr != NULL && *curr->dn != '\0';
       curr = curr->next)
    {
#if defined (DEBUG_LDAP)
      log_info ("Searching for %s in LDAP tree %s", buf, curr->dn);
#endif
      ret = ldap_search_ext_s (ld, curr->dn, LDAP_SCOPE_SUBTREE, buf, NULL, 0,
                               NULL, NULL, NULL, 0, &res);

      if(ret == LDAP_SERVER_DOWN)
        {
          log_info ("LDAP server was down, trying to reconnect...");

          ldap_stop();
          ldap_start();

          if(ld == NULL)
            {
              log_info ("LDAP reconnect failed - try again later...");
              return (0);
            }

          ret = ldap_search_ext_s (ld, curr->dn, LDAP_SCOPE_SUBTREE, buf,
                                   NULL, 0, NULL, NULL, NULL, 0, &res);
        }

      if (ret == LDAP_SUCCESS)
        {
          if( (ent = ldap_first_entry (ld, res)) != NULL)
            break; /* search OK and have entry */

#if defined (DEBUG_LDAP)
          log_info ("No subclass entry for %s in LDAP tree %s",
                    buf, curr->dn);
#endif
          if(res)
            {
              ldap_msgfree (res);
              res = NULL;
            }
        }
      else
        {
          if(res)
            {
              ldap_msgfree (res);
              res = NULL;
            }

          if (ret != LDAP_NO_SUCH_OBJECT && ret != LDAP_SUCCESS)
            {
              log_error ("Cannot search for %s in LDAP tree %s: %s", buf, 
                         curr->dn, ldap_err2string (ret));
              ldap_stop();
              return (0);
            }
#if defined (DEBUG_LDAP)
          else
            {
              log_info ("ldap_search_ext_s returned %s when searching for %s in %s",
                        ldap_err2string (ret), buf, curr->dn);
            }
#endif
        }
    }

  if (res && ent)
    {
#if defined (DEBUG_LDAP)
      char *dn = ldap_get_dn (ld, ent);
      if (dn != NULL)
        {
          log_info ("Found subclass LDAP entry %s", dn);
          ldap_memfree(dn);
        }
#endif

      status = class_allocate (newclass, MDL);
      if (status != ISC_R_SUCCESS)
        {
          log_error ("Cannot allocate memory for a new class");
          ldap_msgfree (res);
          return (0);
        }

      group_reference (&(*newclass)->group, class->group, MDL);
      class_reference (&(*newclass)->superclass, class, MDL);
      lease_limit = ldap_parse_options (ent, (*newclass)->group, 
                                        CLASS_DECL, NULL, newclass);
      if (lease_limit == 0)
        (*newclass)->lease_limit = class->lease_limit; 
      else
        class->lease_limit = lease_limit;

      if ((*newclass)->lease_limit) 
        {
          (*newclass)->billed_leases = 
              dmalloc ((*newclass)->lease_limit * sizeof (struct lease *), MDL);
          if (!(*newclass)->billed_leases) 
            {
              log_error ("no memory for billing");
              class_dereference (newclass, MDL);
              ldap_msgfree (res);
              return (0);
            }
          memset ((*newclass)->billed_leases, 0, 
		  ((*newclass)->lease_limit * sizeof (struct lease *)));
        }

      data_string_copy (&(*newclass)->hash_string, data, MDL);

      ldap_msgfree (res);
      return (1);
    }

  if(res) ldap_msgfree (res);
  return (0);
}

int find_client_in_ldap (struct host_decl **hp, struct packet *packet,
                         struct option_state *state, const char *file, int line)
{
  LDAPMessage * res, * ent;
  ldap_dn_node *curr;
  struct host_decl * host;
  isc_result_t status;
  struct data_string client_id;
  char buf[1024], buf1[1024];
  int ret;

  if (ldap_method == LDAP_METHOD_STATIC)
    return (0);

  if (ld == NULL)
    ldap_start ();
  if (ld == NULL)
    return (0);

  memset(&client_id, 0, sizeof(client_id));
  if (get_client_id(packet, &client_id) != ISC_R_SUCCESS)
    return (0);
  snprintf(buf, sizeof(buf),
           "(&(objectClass=dhcpHost)(dhcpClientId=%s))",
           print_hw_addr(0, client_id.len, client_id.data));

  /* log_info ("Searching LDAP for %s (%s)", buf, packet->interface->shared_network->name); */

  res = ent = NULL;
  for (curr = ldap_service_dn_head;
       curr != NULL && *curr->dn != '\0';
       curr = curr->next)
    {
      snprintf(buf1, sizeof(buf1), "cn=%s,%s", packet->interface->shared_network->name, curr->dn);
#if defined (DEBUG_LDAP)
      log_info ("Searching for %s in LDAP tree %s", buf, buf1);
#endif
      ret = ldap_search_ext_s (ld, buf1, LDAP_SCOPE_SUBTREE, buf, NULL, 0,
                               NULL, NULL, NULL, 0, &res);

      if(ret == LDAP_SERVER_DOWN)
        {
          log_info ("LDAP server was down, trying to reconnect...");

          ldap_stop();
          ldap_start();

          if(ld == NULL)
            {
              log_info ("LDAP reconnect failed - try again later...");
              return (0);
            }

          ret = ldap_search_ext_s (ld, buf1, LDAP_SCOPE_SUBTREE, buf,
                                   NULL, 0, NULL, NULL, NULL, 0, &res);
        }

      if (ret == LDAP_SUCCESS)
        {
          if( (ent = ldap_first_entry (ld, res)) != NULL) {
            log_info ("found entry in search %s", buf1);
            break; /* search OK and have entry */
        }

#if defined (DEBUG_LDAP)
          log_info ("No subclass entry for %s in LDAP tree %s", buf, curr->dn);
#endif
          if(res)
            {
              ldap_msgfree (res);
              res = NULL;
            }
        }
      else
        {
          if(res)
            {
              ldap_msgfree (res);
              res = NULL;
            }

          if (ret != LDAP_NO_SUCH_OBJECT && ret != LDAP_SUCCESS)
            {
              log_error ("Cannot search for %s in LDAP tree %s: %s", buf,
                         curr->dn, ldap_err2string (ret));
              ldap_stop();
              return (0);
            }
          else
            {
              log_info ("did not find: %s", buf);
            }
        }
    }

  if (res && ent)
    {
#if defined (DEBUG_LDAP)
      log_info ("ldap_get_dn %s", curr->dn);
      char *dn = ldap_get_dn (ld, ent);
      if (dn != NULL)
        {
          log_info ("Found subclass LDAP entry %s", dn);
          ldap_memfree(dn);
        } else {
          log_info ("DN is null %s", dn);
        }
#endif

      host = (struct host_decl *)0;
      status = host_allocate (&host, MDL);
      if (status != ISC_R_SUCCESS)
        {
          log_fatal ("can't allocate host decl struct: %s",
                     isc_result_totext (status));
          ldap_msgfree (res);
          return (0);
        }

      host->name = ldap_get_host_name (ent);
      if (host->name == NULL)
        {
          host_dereference (&host, MDL);
          ldap_msgfree (res);
          return (0);
        }
      /* log_info ("Host name %s", host->name); */

      if (!clone_group (&host->group, root_group, MDL))
        {
          log_fatal ("can't clone group for host %s", host->name);
          host_dereference (&host, MDL);
          ldap_msgfree (res);
          return (0);
        }

      ldap_parse_options (ent, host->group, HOST_DECL, host, NULL);

      *hp = host;
      ldap_msgfree (res);
      return (1);
    }
    else
    {
       log_info ("did not find clientid: %s", buf);
    }

  if(res) ldap_msgfree (res);
  return (0);

}

#if defined(LDAP_USE_GSSAPI)
static int
_ldap_sasl_interact(LDAP *ld, unsigned flags, void *defaults, void *sin) 
{
  sasl_interact_t *in;
  struct ldap_sasl_instance *ldap_inst = defaults;
  int ret = LDAP_OTHER;
  size_t size;

  if (ld == NULL || sin == NULL)
    return LDAP_PARAM_ERROR;

  log_info("doing interactive bind");
  for (in = sin; in != NULL && in->id != SASL_CB_LIST_END; in++) {
    switch (in->id) {
      case SASL_CB_USER:
        log_info("got request for SASL_CB_USER %s", ldap_inst->sasl_authz_id);
        size = strlen(ldap_inst->sasl_authz_id);
        in->result = ldap_inst->sasl_authz_id;
        in->len = size;
        ret = LDAP_SUCCESS;
        break;
      case SASL_CB_GETREALM:
        log_info("got request for SASL_CB_GETREALM %s", ldap_inst->sasl_realm);
        size = strlen(ldap_inst->sasl_realm);
        in->result = ldap_inst->sasl_realm;
        in->len = size;
        ret = LDAP_SUCCESS;
        break;
      case SASL_CB_AUTHNAME:
        log_info("got request for SASL_CB_AUTHNAME %s", ldap_inst->sasl_authc_id);
        size = strlen(ldap_inst->sasl_authc_id);
        in->result = ldap_inst->sasl_authc_id;
        in->len = size;
        ret = LDAP_SUCCESS;
        break;
      case SASL_CB_PASS:
        log_info("got request for SASL_CB_PASS %s", ldap_inst->sasl_password);
        size = strlen(ldap_inst->sasl_password);
        in->result = ldap_inst->sasl_password;
        in->len = size;
        ret = LDAP_SUCCESS;
        break;
      default:
        goto cleanup;
    }
  }
  return ret;

cleanup:
  in->result = NULL;
  in->len = 0;
  return LDAP_OTHER;
}
#endif /* LDAP_USE_GSSAPI */


#endif
