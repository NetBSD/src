/*	$NetBSD: ldapauth.c,v 1.3 2011/08/16 09:43:03 christos Exp $	*/
/* $Id: ldapauth.c,v 1.3 2011/08/16 09:43:03 christos Exp $
 */

/*
 *
 * Copyright (c) 2005, Eric AUGE <eau@phear.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of the phear.org nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
 * IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, 
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 */
#include "includes.h"
__RCSID("$NetBSD: ldapauth.c,v 1.3 2011/08/16 09:43:03 christos Exp $");

#ifdef WITH_LDAP_PUBKEY
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ldapauth.h"
#include "log.h"

/* filter building infos */
#define FILTER_GROUP_PREFIX "(&(objectclass=posixGroup)"
#define FILTER_OR_PREFIX "(|"
#define FILTER_OR_SUFFIX ")"
#define FILTER_CN_PREFIX "(cn="
#define FILTER_CN_SUFFIX ")"
#define FILTER_UID_FORMAT "(memberUid=%s)"
#define FILTER_GROUP_SUFFIX ")"
#define FILTER_GROUP_SIZE(group) (size_t) (strlen(group)+(ldap_count_group(group)*5)+52)

/* just filter building stuff */
#define REQUEST_GROUP_SIZE(filter, uid) (size_t) (strlen(filter)+strlen(uid)+1)
#define REQUEST_GROUP(buffer, prefilter, pwname) \
    buffer = (char *) calloc(REQUEST_GROUP_SIZE(prefilter, pwname), sizeof(char)); \
    if (!buffer) { \
        perror("calloc()"); \
        return FAILURE; \
    } \
    snprintf(buffer, REQUEST_GROUP_SIZE(prefilter,pwname), prefilter, pwname)
/*
XXX OLD group building macros
#define REQUEST_GROUP_SIZE(grp, uid) (size_t) (strlen(grp)+strlen(uid)+46)
#define REQUEST_GROUP(buffer,pwname,grp) \
    buffer = (char *) calloc(REQUEST_GROUP_SIZE(grp, pwname), sizeof(char)); \
    if (!buffer) { \
        perror("calloc()"); \
        return FAILURE; \
    } \
    snprintf(buffer,REQUEST_GROUP_SIZE(grp,pwname),"(&(objectclass=posixGroup)(cn=%s)(memberUid=%s))",grp,pwname)
    */

/*
XXX stock upstream version without extra filter support
#define REQUEST_USER_SIZE(uid) (size_t) (strlen(uid)+64)
#define REQUEST_USER(buffer, pwname) \
    buffer = (char *) calloc(REQUEST_USER_SIZE(pwname), sizeof(char)); \
    if (!buffer) { \
        perror("calloc()"); \
        return NULL; \
    } \
    snprintf(buffer,REQUEST_USER_SIZE(pwname),"(&(objectclass=posixAccount)(objectclass=ldapPublicKey)(uid=%s))",pwname)
   */

#define REQUEST_USER_SIZE(uid, filter) (size_t) (strlen(uid)+64+(filter != NULL ? strlen(filter) : 0))
#define REQUEST_USER(buffer, pwname, customfilter) \
    buffer = (char *) calloc(REQUEST_USER_SIZE(pwname, customfilter), sizeof(char)); \
    if (!buffer) { \
        perror("calloc()"); \
        return NULL; \
    } \
    snprintf(buffer, REQUEST_USER_SIZE(pwname, customfilter), \
    	"(&(objectclass=posixAccount)(objectclass=ldapPublicKey)(uid=%s)%s)", \
	pwname, (customfilter != NULL ? customfilter : ""))

/* some portable and working tokenizer, lame though */
static int tokenize(char ** o, size_t size, char * input) {
    unsigned int i = 0, num;
    const char * charset = " \t";
    char * ptr = input;

    /* leading white spaces are ignored */
    num = strspn(ptr, charset);
    ptr += num;

    while ((num = strcspn(ptr, charset))) {
        if (i < size-1) {
            o[i++] = ptr;
            ptr += num;
            if (*ptr)
                *ptr++ = '\0';
        }
    }
    o[i] = NULL;
    return SUCCESS;
}

void ldap_close(ldap_opt_t * ldap) {

    if (!ldap)
        return;

    if ( ldap_unbind_ext(ldap->ld, NULL, NULL) < 0)
	ldap_perror(ldap->ld, "ldap_unbind()");

    ldap->ld = NULL;
    FLAG_SET_DISCONNECTED(ldap->flags);

    return;
}

/* init && bind */
int ldap_connect(ldap_opt_t * ldap) {
    int version = LDAP_VERSION3;

    if (!ldap->servers)
        return FAILURE;

    /* Connection Init and setup */
    ldap->ld = ldap_init(ldap->servers, LDAP_PORT);
    if (!ldap->ld) {
        ldap_perror(ldap->ld, "ldap_init()");
        return FAILURE;
    }

    if ( ldap_set_option(ldap->ld, LDAP_OPT_PROTOCOL_VERSION, &version) != LDAP_OPT_SUCCESS) {
        ldap_perror(ldap->ld, "ldap_set_option(LDAP_OPT_PROTOCOL_VERSION)");
        return FAILURE;
    }

    /* Timeouts setup */
    if (ldap_set_option(ldap->ld, LDAP_OPT_NETWORK_TIMEOUT, &ldap->b_timeout) != LDAP_SUCCESS) {
        ldap_perror(ldap->ld, "ldap_set_option(LDAP_OPT_NETWORK_TIMEOUT)");
    }
    if (ldap_set_option(ldap->ld, LDAP_OPT_TIMEOUT, &ldap->s_timeout) != LDAP_SUCCESS) {
        ldap_perror(ldap->ld, "ldap_set_option(LDAP_OPT_TIMEOUT)");
    }

    /* TLS support */
    if ( (ldap->tls == -1) || (ldap->tls == 1) ) {
        if (ldap_start_tls_s(ldap->ld, NULL, NULL ) != LDAP_SUCCESS) {
            /* failed then reinit the initial connect */
            ldap_perror(ldap->ld, "ldap_connect: (TLS) ldap_start_tls()");
            if (ldap->tls == 1)
                return FAILURE;

            ldap->ld = ldap_init(ldap->servers, LDAP_PORT);
            if (!ldap->ld) { 
                ldap_perror(ldap->ld, "ldap_init()");
                return FAILURE;
            }

            if ( ldap_set_option(ldap->ld, LDAP_OPT_PROTOCOL_VERSION, &version) != LDAP_OPT_SUCCESS) {
                 ldap_perror(ldap->ld, "ldap_set_option()");
                 return FAILURE;
            }
        }
    }


    if ( ldap_simple_bind_s(ldap->ld, ldap->binddn, ldap->bindpw) != LDAP_SUCCESS) {
        ldap_perror(ldap->ld, "ldap_simple_bind_s()");
        return FAILURE;
    }

    /* says it is connected */
    FLAG_SET_CONNECTED(ldap->flags);

    return SUCCESS;
}

/* must free allocated ressource */
static char * ldap_build_host(char *host, int port) {
    unsigned int size = strlen(host)+11;
    char * h = (char *) calloc (size, sizeof(char));
    int rc;
    if (!h)
         return NULL;

    rc = snprintf(h, size, "%s:%d ", host, port);
    if (rc == -1)
        return NULL;
    return h;
}

static int ldap_count_group(const char * input) {
    const char * charset = " \t";
    const char * ptr = input;
    unsigned int count = 0;
    unsigned int num;

    num = strspn(ptr, charset);
    ptr += num;

    while ((num = strcspn(ptr, charset))) {
    count++;
    ptr += num;
    ptr++;
    }

    return count;
}

/* format filter */
char * ldap_parse_groups(const char * groups) {
    unsigned int buffer_size = FILTER_GROUP_SIZE(groups);
    char * buffer = (char *) calloc(buffer_size, sizeof(char));
    char * g = NULL;
    char * garray[32];
    unsigned int i = 0;

    if ((!groups)||(!buffer))
        return NULL;

    g = strdup(groups);
    if (!g) {
        free(buffer);
        return NULL;
    }

    /* first separate into n tokens */
    if ( tokenize(garray, sizeof(garray)/sizeof(*garray), g) < 0) {
        free(g);
        free(buffer);
        return NULL;
    }

    /* build the final filter format */
    strlcat(buffer, FILTER_GROUP_PREFIX, buffer_size);
    strlcat(buffer, FILTER_OR_PREFIX, buffer_size);
    i = 0;
    while (garray[i]) {
        strlcat(buffer, FILTER_CN_PREFIX, buffer_size);
        strlcat(buffer, garray[i], buffer_size);
        strlcat(buffer, FILTER_CN_SUFFIX, buffer_size);
        i++;
    }
    strlcat(buffer, FILTER_OR_SUFFIX, buffer_size);
    strlcat(buffer, FILTER_UID_FORMAT, buffer_size);
    strlcat(buffer, FILTER_GROUP_SUFFIX, buffer_size);

    free(g);
    return buffer;
}

/* a bit dirty but leak free  */
char * ldap_parse_servers(const char * servers) {
    char * s = NULL;
    char * tmp = NULL, *urls[32];
    unsigned int num = 0 , i = 0 , asize = 0;
    LDAPURLDesc *urld[32];

    if (!servers)
        return NULL;

    /* local copy of the arg */
    s = strdup(servers);
    if (!s)
        return NULL;

    /* first separate into URL tokens */
    if ( tokenize(urls, sizeof(urls)/sizeof(*urls), s) < 0)
        return NULL;

    i = 0;
    while (urls[i]) {
        if (! ldap_is_ldap_url(urls[i]) ||
           (ldap_url_parse(urls[i], &urld[i]) != 0)) {
                return NULL;
        }
        i++;
    }

    /* now free(s) */
    free (s);

    /* how much memory do we need */
    num = i;
    for (i = 0 ; i < num ; i++)
        asize += strlen(urld[i]->lud_host)+11;

    /* alloc */
    s = (char *) calloc( asize+1 , sizeof(char));
    if (!s) {
        for (i = 0 ; i < num ; i++)
            ldap_free_urldesc(urld[i]);
        return NULL;
    }

    /* then build the final host string */
    for (i = 0 ; i < num ; i++) {
        /* built host part */
        tmp = ldap_build_host(urld[i]->lud_host, urld[i]->lud_port);
        strncat(s, tmp, strlen(tmp));
        ldap_free_urldesc(urld[i]);
        free(tmp);
    }

    return s;
}

void ldap_options_print(ldap_opt_t * ldap) {
    debug("ldap options:");
    debug("servers: %s", ldap->servers);
    if (ldap->u_basedn)
        debug("user basedn: %s", ldap->u_basedn);
    if (ldap->g_basedn)
        debug("group basedn: %s", ldap->g_basedn);
    if (ldap->binddn)
        debug("binddn: %s", ldap->binddn);
    if (ldap->bindpw)
        debug("bindpw: %s", ldap->bindpw);
    if (ldap->sgroup)
        debug("group: %s", ldap->sgroup);
    if (ldap->filter)
        debug("filter: %s", ldap->filter);
}

void ldap_options_free(ldap_opt_t * l) {
    if (!l)
        return;
    if (l->servers)
        free(l->servers);
    if (l->u_basedn)
        free(l->u_basedn);
    if (l->g_basedn)
        free(l->g_basedn);
    if (l->binddn)
        free(l->binddn);
    if (l->bindpw)
        free(l->bindpw);
    if (l->sgroup)
        free(l->sgroup);
    if (l->fgroup)
        free(l->fgroup);
    if (l->filter)
        free(l->filter);
    if (l->l_conf)
        free(l->l_conf);
    free(l);
}

/* free keys */
void ldap_keys_free(ldap_key_t * k) {
    ldap_value_free_len(k->keys);
    free(k);
    return;
}

ldap_key_t * ldap_getuserkey(ldap_opt_t *l, const char * user) {
    ldap_key_t * k = (ldap_key_t *) calloc (1, sizeof(ldap_key_t));
    LDAPMessage *res, *e;
    char * filter;
    int i;
    char *attrs[] = {
      l->pub_key_attr,
      NULL
    };

    if ((!k) || (!l))
         return NULL;

    /* Am i still connected ? RETRY n times */
    /* XXX TODO: setup some conf value for retrying */
    if (!(l->flags & FLAG_CONNECTED))
        for (i = 0 ; i < 2 ; i++)
            if (ldap_connect(l) == 0)
                break;

    /* quick check for attempts to be evil */
    if ((strchr(user, '(') != NULL) || (strchr(user, ')') != NULL) ||
        (strchr(user, '*') != NULL) || (strchr(user, '\\') != NULL))
        return NULL;

    /* build  filter for LDAP request */
    REQUEST_USER(filter, user, l->filter);

    if ( ldap_search_st( l->ld,
        l->u_basedn,
        LDAP_SCOPE_SUBTREE,
        filter,
        attrs, 0, &l->s_timeout, &res ) != LDAP_SUCCESS) {
        
        ldap_perror(l->ld, "ldap_search_st()");

        free(filter);
        free(k);

        /* XXX error on search, timeout etc.. close ask for reconnect */
        ldap_close(l);

        return NULL;
    } 

    /* free */
    free(filter);

    /* check if any results */
    i = ldap_count_entries(l->ld,res);
    if (i <= 0) {
        ldap_msgfree(res);
        free(k);
        return NULL;
    }

    if (i > 1)
        debug("[LDAP] duplicate entries, using the FIRST entry returned");

    e = ldap_first_entry(l->ld, res);
    k->keys = ldap_get_values_len(l->ld, e, l->pub_key_attr);
    k->num = ldap_count_values_len(k->keys);

    ldap_msgfree(res);
    return k;
}


/* -1 if trouble
   0 if user is NOT member of current server group
   1 if user IS MEMBER of current server group 
 */
int ldap_ismember(ldap_opt_t * l, const char * user) {
    LDAPMessage *res;
    char * filter;
    int i;

    if ((!l->sgroup) || !(l->g_basedn))
        return 1;

    /* Am i still connected ? RETRY n times */
    /* XXX TODO: setup some conf value for retrying */
    if (!(l->flags & FLAG_CONNECTED)) 
        for (i = 0 ; i < 2 ; i++)
            if (ldap_connect(l) == 0)
                 break;

    /* quick check for attempts to be evil */
    if ((strchr(user, '(') != NULL) || (strchr(user, ')') != NULL) ||
        (strchr(user, '*') != NULL) || (strchr(user, '\\') != NULL))
        return FAILURE;

    /* build filter for LDAP request */
    REQUEST_GROUP(filter, l->fgroup, user);

    if (ldap_search_st( l->ld, 
        l->g_basedn,
        LDAP_SCOPE_SUBTREE,
        filter,
        NULL, 0, &l->s_timeout, &res) != LDAP_SUCCESS) {
    
        ldap_perror(l->ld, "ldap_search_st()");

        free(filter);

        /* XXX error on search, timeout etc.. close ask for reconnect */
        ldap_close(l);

        return FAILURE;
    }

    free(filter);

    /* check if any results */
    if (ldap_count_entries(l->ld, res) > 0) {
        ldap_msgfree(res);
        return 1;
    }

    ldap_msgfree(res);
    return 0;
}

/*
 * ldap.conf simple parser
 * XXX TODO:  sanity checks
 * must either
 * - free the previous ldap_opt_before replacing entries
 * - free each necessary previously parsed elements
 * ret:
 * -1 on FAILURE, 0 on SUCCESS
 */
int ldap_parse_lconf(ldap_opt_t * l) {
    FILE * lcd; /* ldap.conf descriptor */
    char buf[BUFSIZ];
    char * s = NULL, * k = NULL, * v = NULL;
    int li, len;

    lcd = fopen (l->l_conf, "r");
    if (lcd == NULL) {
        /* debug("Cannot open %s", l->l_conf); */
        perror("ldap_parse_lconf()");
        return FAILURE;
    }
    
    while (fgets (buf, sizeof (buf), lcd) != NULL) {

        if (*buf == '\n' || *buf == '#')
            continue;

        k = buf;
        v = k;
        while (*v != '\0' && *v != ' ' && *v != '\t')
            v++;

        if (*v == '\0')
            continue;

        *(v++) = '\0';

        while (*v == ' ' || *v == '\t')
            v++;

        li = strlen (v) - 1;
        while (v[li] == ' ' || v[li] == '\t' || v[li] == '\n')
            --li;
        v[li + 1] = '\0';

        if (!strcasecmp (k, "uri")) {
            if ((l->servers = ldap_parse_servers(v)) == NULL) {
                fatal("error in ldap servers");
            return FAILURE;
            }

        }
        else if (!strcasecmp (k, "base")) { 
            s = strchr (v, '?');
            if (s != NULL) {
                len = s - v;
                l->u_basedn = malloc (len + 1);
                strncpy (l->u_basedn, v, len);
                l->u_basedn[len] = '\0';
            } else {
                l->u_basedn = strdup (v);
            }
        }
        else if (!strcasecmp (k, "binddn")) {
            l->binddn = strdup (v);
        }
        else if (!strcasecmp (k, "bindpw")) {
            l->bindpw = strdup (v);
        }
        else if (!strcasecmp (k, "timelimit")) {
            l->s_timeout.tv_sec = atoi (v);
                }
        else if (!strcasecmp (k, "bind_timelimit")) {
            l->b_timeout.tv_sec = atoi (v);
        }
        else if (!strcasecmp (k, "ssl")) {
            if (!strcasecmp (v, "start_tls"))
                l->tls = 1;
        }
    }

    fclose (lcd);
    return SUCCESS;
}

#endif /* WITH_LDAP_PUBKEY */
