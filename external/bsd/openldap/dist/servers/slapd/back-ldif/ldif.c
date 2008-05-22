/* ldif.c - the ldif backend */
/* $OpenLDAP: pkg/ldap/servers/slapd/back-ldif/ldif.c,v 1.48.2.14 2008/04/21 18:53:52 quanah Exp $ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2008 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by Eric Stokes for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"
#include <stdio.h>
#include <ac/string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ac/dirent.h>
#include <fcntl.h>
#include <ac/errno.h>
#include <ac/unistd.h>
#include "slap.h"
#include "lutil.h"
#include "config.h"

typedef struct enumCookie {
	Operation *op;
	SlapReply *rs;
	Entry **entries;
	int elen;
	int eind;
} enumCookie;

struct ldif_info {
	struct berval li_base_path;
	enumCookie li_tool_cookie;
	ID li_tool_current;
	ldap_pvt_thread_rdwr_t  li_rdwr;
};

#ifdef _WIN32
#define mkdir(a,b)	mkdir(a)
#endif


#define LDIF	".ldif"
#define LDIF_FILETYPE_SEP	'.'			/* LDIF[0] */

/*
 * Unsafe/translated characters in the filesystem.
 *
 * LDIF_UNSAFE_CHAR(c) returns true if the character c is not to be used
 * in relative filenames, except it should accept '\\' even if unsafe and
 * need not reject '{' and '}'.  The value should be a constant expression.
 *
 * If '\\' is unsafe, #define LDIF_ESCAPE_CHAR as a safe character.
 *
 * If '{' and '}' are unsafe, #define IX_FSL/IX_FSR as safe characters.
 * (Not digits, '-' or '+'.  IX_FSL == IX_FSR is allowed.)
 *
 * Characters are escaped as LDIF_ESCAPE_CHAR followed by two hex digits,
 * except '\\' is replaced with LDIF_ESCAPE_CHAR and {} with IX_FS[LR].
 * Also some LDIF special chars are hex-escaped.
 *
 * Thus an LDIF filename is a valid normalized RDN (or suffix DN)
 * followed by ".ldif", except with '\\' replaced with LDIF_ESCAPE_CHAR.
 */

#ifndef _WIN32

/*
 * Unix/MacOSX version.  ':' vs '/' can cause confusion on MacOSX so we
 * escape both.  We escape them on Unix so both OS variants get the same
 * filenames.
 */
#define LDIF_ESCAPE_CHAR	'\\'
#define LDIF_UNSAFE_CHAR(c)	((c) == '/' || (c) == ':')

#else /* _WIN32 */

/* Windows version - Microsoft's list of unsafe characters, except '\\' */
#define LDIF_ESCAPE_CHAR	'^'
#define LDIF_UNSAFE_CHAR(c)	\
	((c) == '/' || (c) == ':' || \
	 (c) == '<' || (c) == '>' || (c) == '"' || \
	 (c) == '|' || (c) == '?' || (c) == '*')

#endif /* !_WIN32 */

/*
 * Left and Right "{num}" prefix to ordered RDNs ("olcDatabase={1}bdb").
 * IX_DN* are for LDAP RDNs, IX_FS* for their .ldif filenames.
 */
#define IX_DNL	'{'
#define	IX_DNR	'}'
#ifndef IX_FSL
#define	IX_FSL	IX_DNL
#define IX_FSR	IX_DNR
#endif

/*
 * Test for unsafe chars, as well as chars handled specially by back-ldif:
 * - If the escape char is not '\\', it must itself be escaped.  Otherwise
 *   '\\' and the escape char would map to the same character.
 * - Escape the '.' in ".ldif", so the directory for an RDN that actually
 *   ends with ".ldif" can not conflict with a file of the same name.  And
 *   since some OSes/programs choke on multiple '.'s, escape all of them.
 * - If '{' and '}' are translated to some other characters, those
 *   characters must in turn be escaped when they occur in an RDN.
 */
#ifndef LDIF_NEED_ESCAPE
#define	LDIF_NEED_ESCAPE(c) \
	((LDIF_UNSAFE_CHAR(c)) || \
	 LDIF_MAYBE_UNSAFE(c, LDIF_ESCAPE_CHAR) || \
	 LDIF_MAYBE_UNSAFE(c, LDIF_FILETYPE_SEP) || \
	 LDIF_MAYBE_UNSAFE(c, IX_FSL) || \
	 (IX_FSR != IX_FSL && LDIF_MAYBE_UNSAFE(c, IX_FSR)))
#endif
/*
 * Helper macro for LDIF_NEED_ESCAPE(): Treat character x as unsafe if
 * back-ldif does not already treat is specially.
 */
#define LDIF_MAYBE_UNSAFE(c, x) \
	(!(LDIF_UNSAFE_CHAR(x) || (x) == '\\' || (x) == IX_DNL || (x) == IX_DNR) \
	 && (c) == (x))


#define ENTRY_BUFF_INCREMENT 500

static ConfigTable ldifcfg[] = {
	{ "directory", "dir", 2, 2, 0, ARG_BERVAL|ARG_OFFSET,
		(void *)offsetof(struct ldif_info, li_base_path),
		"( OLcfgDbAt:0.1 NAME 'olcDbDirectory' "
			"DESC 'Directory for database content' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs ldifocs[] = {
	{ "( OLcfgDbOc:2.1 "
		"NAME 'olcLdifConfig' "
		"DESC 'LDIF backend configuration' "
		"SUP olcDatabaseConfig "
		"MUST ( olcDbDirectory ) )", Cft_Database, ldifcfg },
	{ NULL, 0, NULL }
};


/* Set *res = LDIF filename path for the normalized DN */
static void
dn2path( BackendDB *be, struct berval *dn, struct berval *res )
{
	struct ldif_info *li = (struct ldif_info *) be->be_private;
	struct berval *suffixdn = &be->be_nsuffix[0];
	const char *start, *end, *next, *p;
	char ch, *ptr;
	ber_len_t len;
	static const char hex[] = "0123456789ABCDEF";

	assert( dn != NULL );
	assert( !BER_BVISNULL( dn ) );
	assert( suffixdn != NULL );
	assert( !BER_BVISNULL( suffixdn ) );
	assert( dnIsSuffix( dn, suffixdn ) );

	start = dn->bv_val;
	end = start + dn->bv_len;

	/* Room for dir, dirsep, dn, LDIF, "\hexpair"-escaping of unsafe chars */
	len = li->li_base_path.bv_len + dn->bv_len + (1 + STRLENOF( LDIF ));
	for ( p = start; p < end; ) {
		ch = *p++;
		if ( LDIF_NEED_ESCAPE( ch ) )
			len += 2;
	}
	res->bv_val = ch_malloc( len + 1 );

	ptr = lutil_strcopy( res->bv_val, li->li_base_path.bv_val );
	for ( next = end - suffixdn->bv_len; end > start; end = next ) {
		/* Set p = start of DN component, next = &',' or start of DN */
		while ( (p = next) > start ) {
			--next;
			if ( DN_SEPARATOR( *next ) )
				break;
		}
		/* Append <dirsep> <p..end-1: RDN or database-suffix> */
		for ( *ptr++ = LDAP_DIRSEP[0]; p < end; *ptr++ = ch ) {
			ch = *p++;
			if ( LDIF_ESCAPE_CHAR != '\\' && ch == '\\' ) {
				ch = LDIF_ESCAPE_CHAR;
			} else if ( IX_FSL != IX_DNL && ch == IX_DNL ) {
				ch = IX_FSL;
			} else if ( IX_FSR != IX_DNR && ch == IX_DNR ) {
				ch = IX_FSR;
			} else if ( LDIF_NEED_ESCAPE( ch ) ) {
				*ptr++ = LDIF_ESCAPE_CHAR;
				*ptr++ = hex[(ch & 0xFFU) >> 4];
				ch = hex[ch & 0x0FU];
			}
		}
	}
	ptr = lutil_strcopy( ptr, LDIF );
	res->bv_len = ptr - res->bv_val;

	assert( res->bv_len <= len );
}

static char * slurp_file(int fd) {
	int read_chars_total = 0;
	int read_chars = 0;
	int entry_size;
	char * entry;
	char * entry_pos;
	struct stat st;

	fstat(fd, &st);
	entry_size = st.st_size;
	entry = ch_malloc( entry_size+1 );
	entry_pos = entry;
	
	while(1) {
		read_chars = read(fd, (void *) entry_pos, entry_size - read_chars_total);
		if(read_chars == -1) {
			SLAP_FREE(entry);
			return NULL;
		}
		if(read_chars == 0) {
			entry[read_chars_total] = '\0';
			break;
		}
		else {
			read_chars_total += read_chars;
			entry_pos += read_chars;
		}
	}
	return entry;
}

/*
 * return nonnegative for success or -1 for error
 * do not return numbers less than -1
 */
static int spew_file(int fd, char * spew, int len) {
	int writeres = 0;
	
	while(len > 0) {
		writeres = write(fd, spew, len);
		if(writeres == -1) {
			return -1;
		}
		else {
			spew += writeres;
			len -= writeres;
		}
	}
	return writeres;
}

static int
spew_entry( Entry * e, struct berval * path, int dolock, int *save_errnop )
{
	int rs, save_errno = 0;
	int openres;
	int res, spew_res;
	int entry_length;
	char * entry_as_string;
	char *tmpfname = NULL;

	tmpfname = ch_malloc( path->bv_len + STRLENOF( "XXXXXX" ) + 1 );
	AC_MEMCPY( tmpfname, path->bv_val, path->bv_len );
	AC_MEMCPY( &tmpfname[ path->bv_len ], "XXXXXX", STRLENOF( "XXXXXX" ) + 1 );

	openres = mkstemp( tmpfname );
	if ( openres == -1 ) {
		save_errno = errno;
		rs = LDAP_UNWILLING_TO_PERFORM;
		Debug( LDAP_DEBUG_ANY, "could not create tmpfile \"%s\": %s\n",
			tmpfname, STRERROR( save_errno ), 0 );

	} else {
		struct berval rdn;
		int tmp;

		/* Only save the RDN onto disk */
		dnRdn( &e->e_name, &rdn );
		if ( rdn.bv_len != e->e_name.bv_len ) {
			e->e_name.bv_val[rdn.bv_len] = '\0';
			tmp = e->e_name.bv_len;
			e->e_name.bv_len = rdn.bv_len;
			rdn.bv_len = tmp;
		}

		spew_res = -2;
		if ( dolock ) {
			ldap_pvt_thread_mutex_lock(&entry2str_mutex);
		}

		entry_as_string = entry2str(e, &entry_length);
		if ( entry_as_string != NULL ) {
			spew_res = spew_file( openres,
				entry_as_string, entry_length );
			if ( spew_res == -1 ) {
				save_errno = errno;
			}
		}

		if ( dolock ) {
			ldap_pvt_thread_mutex_unlock(&entry2str_mutex);
		}

		/* Restore full DN */
		if ( rdn.bv_len != e->e_name.bv_len ) {
			e->e_name.bv_val[e->e_name.bv_len] = ',';
			e->e_name.bv_len = rdn.bv_len;
		}

		res = close( openres );
		rs = LDAP_UNWILLING_TO_PERFORM;

		if ( spew_res > -2 ) {
			if ( res == -1 || spew_res == -1 ) {
				if ( save_errno == 0 ) {
					save_errno = errno;
				}
				Debug( LDAP_DEBUG_ANY, "write error to tmpfile \"%s\": %s\n",
					tmpfname, STRERROR( save_errno ), 0 );

			} else {
				res = rename( tmpfname, path->bv_val );
				if ( res == 0 ) {
					rs = LDAP_SUCCESS;

				} else {
					save_errno = errno;
					switch ( save_errno ) {
					case ENOENT:
						rs = LDAP_NO_SUCH_OBJECT;
						break;

					default:
						break;
					}
				}
			}
		}

		if ( rs != LDAP_SUCCESS ) {
			unlink( tmpfname );
		}
	}

	ch_free( tmpfname );

	if ( rs != LDAP_SUCCESS && save_errnop != NULL ) {
		*save_errnop = save_errno;
	}

	return rs;
}

static Entry * get_entry_for_fd(int fd,
	struct berval *pdn,
	struct berval *pndn)
{
	char * entry = (char *) slurp_file(fd);
	Entry * ldentry = NULL;
	
	/* error reading file */
	if(entry == NULL) {
		goto return_value;
	}

	ldentry = str2entry(entry);
	if ( ldentry ) {
		struct berval rdn;
		rdn = ldentry->e_name;
		build_new_dn( &ldentry->e_name, pdn, &rdn, NULL );
		ch_free( rdn.bv_val );
		rdn = ldentry->e_nname;
		build_new_dn( &ldentry->e_nname, pndn, &rdn, NULL );
		ch_free( rdn.bv_val );
	}

 return_value:
	if(fd != -1) {
		if(close(fd) != 0) {
			/* log error */
		}
	}
	if(entry != NULL)
		SLAP_FREE(entry);
	return ldentry;
}

static int
get_entry(
	Operation *op,
	Entry **entryp,
	struct berval *pathp )
{
	int rc;
	struct berval path, pdn, pndn;
	int fd;

	dnParent(&op->o_req_dn, &pdn);
	dnParent(&op->o_req_ndn, &pndn);
	dn2path( op->o_bd, &op->o_req_ndn, &path );
	fd = open(path.bv_val, O_RDONLY);
	/* error opening file (mebbe should log error) */
	if ( fd == -1 && ( errno != ENOENT || op->o_tag != LDAP_REQ_ADD ) ) {
		Debug( LDAP_DEBUG_ANY, "failed to open file \"%s\": %s\n",
			path.bv_val, STRERROR(errno), 0 );
	}
	*entryp = fd < 0 ? NULL : get_entry_for_fd( fd, &pdn, &pndn );
	rc = *entryp ? LDAP_SUCCESS : LDAP_NO_SUCH_OBJECT;

	if ( rc == LDAP_SUCCESS && pathp != NULL ) {
		*pathp = path;
	} else {
		SLAP_FREE(path.bv_val);
	}
	return rc;
}

static void fullpath(struct berval *base, struct berval *name, struct berval *res) {
	char *ptr;
	res->bv_len = name->bv_len + base->bv_len + 1;
	res->bv_val = ch_malloc( res->bv_len + 1 );
	strcpy(res->bv_val, base->bv_val);
	ptr = res->bv_val + base->bv_len;
	*ptr++ = LDAP_DIRSEP[0];
	strcpy(ptr, name->bv_val);
}

typedef struct bvlist {
	struct bvlist *next;
	struct berval bv;
	struct berval num;
	int inum;
	int off;
} bvlist;


static int r_enum_tree(enumCookie *ck, struct berval *path, int base,
	struct berval *pdn, struct berval *pndn)
{
	Entry *e = NULL;
	int fd = 0, rc = LDAP_SUCCESS;

	if ( !base ) {
		fd = open( path->bv_val, O_RDONLY );
		if ( fd < 0 ) {
			Debug( LDAP_DEBUG_TRACE,
				"=> ldif_enum_tree: failed to open %s: %s\n",
				path->bv_val, STRERROR(errno), 0 );
			return LDAP_NO_SUCH_OBJECT;
		}

		e = get_entry_for_fd(fd, pdn, pndn);
		if ( !e ) {
			Debug( LDAP_DEBUG_ANY,
				"=> ldif_enum_tree: failed to read entry for %s\n",
				path->bv_val, 0, 0 );
			return LDAP_BUSY;
		}

		if ( ck->op->ors_scope == LDAP_SCOPE_BASE ||
			ck->op->ors_scope == LDAP_SCOPE_SUBTREE ) {
			/* Send right away? */
			if ( ck->rs ) {
				/*
				 * if it's a referral, add it to the list of referrals. only do
				 * this for non-base searches, and don't check the filter
				 * explicitly here since it's only a candidate anyway.
				 */
				if ( !get_manageDSAit( ck->op )
						&& ck->op->ors_scope != LDAP_SCOPE_BASE
						&& is_entry_referral( e ) )
				{
					BerVarray erefs = get_entry_referrals( ck->op, e );
					ck->rs->sr_ref = referral_rewrite( erefs,
							&e->e_name, NULL,
							ck->op->oq_search.rs_scope == LDAP_SCOPE_ONELEVEL
								? LDAP_SCOPE_BASE : LDAP_SCOPE_SUBTREE );
	
					ck->rs->sr_entry = e;
					rc = send_search_reference( ck->op, ck->rs );
					ber_bvarray_free( ck->rs->sr_ref );
					ber_bvarray_free( erefs );
					ck->rs->sr_ref = NULL;
					ck->rs->sr_entry = NULL;
	
				} else if ( test_filter( ck->op, e, ck->op->ors_filter ) == LDAP_COMPARE_TRUE )
				{
					ck->rs->sr_entry = e;
					ck->rs->sr_attrs = ck->op->ors_attrs;
					ck->rs->sr_flags = REP_ENTRY_MODIFIABLE;
					rc = send_search_entry(ck->op, ck->rs);
					ck->rs->sr_entry = NULL;
				}
				fd = 1;
				if ( rc )
					goto done;
			} else {
			/* Queueing up for tool mode */
				if(ck->entries == NULL) {
					ck->entries = (Entry **) ch_malloc(sizeof(Entry *) * ENTRY_BUFF_INCREMENT);
					ck->elen = ENTRY_BUFF_INCREMENT;
				}
				if(ck->eind >= ck->elen) { /* grow entries if necessary */	
					ck->entries = (Entry **) ch_realloc(ck->entries, sizeof(Entry *) * (ck->elen) * 2);
					ck->elen *= 2;
				}
	
				ck->entries[ck->eind++] = e;
				fd = 0;
			}
		} else {
			fd = 1;
		}
	}

	if ( ck->op->ors_scope != LDAP_SCOPE_BASE ) {
		DIR * dir_of_path;
		bvlist *list = NULL, *ptr;

		path->bv_len -= STRLENOF( LDIF );
		path->bv_val[path->bv_len] = '\0';

		dir_of_path = opendir(path->bv_val);
		if(dir_of_path == NULL) { /* can't open directory */
			if ( errno != ENOENT ) {
				/* it shouldn't be treated as an error
				 * only if the directory doesn't exist */
				rc = LDAP_BUSY;
				Debug( LDAP_DEBUG_ANY,
					"=> ldif_enum_tree: failed to opendir %s (%d)\n",
					path->bv_val, errno, 0 );
			}
			goto done;
		}
	
		while(1) {
			struct berval fname, itmp;
			struct dirent * dir;
			bvlist *bvl, **prev;

			dir = readdir(dir_of_path);
			if(dir == NULL) break; /* end of the directory */
			fname.bv_len = strlen( dir->d_name );
			if ( fname.bv_len <= STRLENOF( LDIF ))
				continue;
			if ( strcmp( dir->d_name + (fname.bv_len - STRLENOF(LDIF)), LDIF))
				continue;
			fname.bv_val = dir->d_name;

			bvl = ch_malloc( sizeof(bvlist) );
			ber_dupbv( &bvl->bv, &fname );
			BER_BVZERO( &bvl->num );
			itmp.bv_val = ber_bvchr( &bvl->bv, IX_FSL );
			if ( itmp.bv_val ) {
				char *ptr;
				itmp.bv_val++;
				itmp.bv_len = bvl->bv.bv_len
					- ( itmp.bv_val - bvl->bv.bv_val );
				ptr = ber_bvchr( &itmp, IX_FSR );
				if ( ptr ) {
					itmp.bv_len = ptr - itmp.bv_val;
					ber_dupbv( &bvl->num, &itmp );
					bvl->inum = strtol( itmp.bv_val, NULL, 0 );
					itmp.bv_val[0] = '\0';
					bvl->off = itmp.bv_val - bvl->bv.bv_val;
				}
			}

			for (prev = &list; (ptr = *prev) != NULL; prev = &ptr->next) {
				int cmp = strcmp( bvl->bv.bv_val, ptr->bv.bv_val );
				if ( !cmp && bvl->num.bv_val )
					cmp = bvl->inum - ptr->inum;
				if ( cmp < 0 )
					break;
			}
			*prev = bvl;
			bvl->next = ptr;
				
		}
		closedir(dir_of_path);

		if (ck->op->ors_scope == LDAP_SCOPE_ONELEVEL)
			ck->op->ors_scope = LDAP_SCOPE_BASE;
		else if ( ck->op->ors_scope == LDAP_SCOPE_SUBORDINATE)
			ck->op->ors_scope = LDAP_SCOPE_SUBTREE;

		while ( ( ptr = list ) ) {
			struct berval fpath;

			list = ptr->next;

			if ( rc == LDAP_SUCCESS ) {
				if ( ptr->num.bv_val )
					AC_MEMCPY( ptr->bv.bv_val + ptr->off, ptr->num.bv_val,
						ptr->num.bv_len );
				fullpath( path, &ptr->bv, &fpath );
				rc = r_enum_tree(ck, &fpath, 0,
					e != NULL ? &e->e_name : pdn,
					e != NULL ? &e->e_nname : pndn );
				free(fpath.bv_val);
			}
			if ( ptr->num.bv_val )
				free( ptr->num.bv_val );
			free(ptr->bv.bv_val);
			free(ptr);
		}
	}
done:
	if ( fd ) entry_free( e );
	return rc;
}

static int
enum_tree(
	enumCookie *ck
)
{
	struct berval path;
	struct berval pdn, pndn;
	int rc;

	dnParent( &ck->op->o_req_dn, &pdn );
	dnParent( &ck->op->o_req_ndn, &pndn );
	dn2path( ck->op->o_bd, &ck->op->o_req_ndn, &path );
	rc = r_enum_tree(ck, &path, BER_BVISEMPTY( &ck->op->o_req_ndn ) ? 1 : 0, &pdn, &pndn);
	ch_free( path.bv_val );
	return rc;
}


/* Get the parent directory path, plus the LDIF suffix overwritten by a \0 */
static void
get_parent_path( struct berval *dnpath, struct berval *res )
{
	int dnpathlen = dnpath->bv_len;
	int i;
	
	for(i = dnpathlen;i>0;i--) /* find the first path seperator */
		if(dnpath->bv_val[i] == LDAP_DIRSEP[0])
			break;
	res->bv_len = i;
	res->bv_val = ch_malloc( res->bv_len + 1 + STRLENOF(LDIF) );
	strncpy(res->bv_val, dnpath->bv_val, i);
	strcpy(res->bv_val+i, LDIF);
	res->bv_val[i] = '\0';
}

static int apply_modify_to_entry(Entry * entry,
				Modifications * modlist,
				Operation * op,
				SlapReply * rs)
{
	char textbuf[SLAP_TEXT_BUFLEN];
	int rc = modlist ? LDAP_UNWILLING_TO_PERFORM : LDAP_SUCCESS;
	int is_oc = 0;
	Modification *mods;

	if (!acl_check_modlist(op, entry, modlist)) {
		return LDAP_INSUFFICIENT_ACCESS;
	}

	for (; modlist != NULL; modlist = modlist->sml_next) {
		mods = &modlist->sml_mod;

		if ( mods->sm_desc == slap_schema.si_ad_objectClass ) {
			is_oc = 1;
		}
		switch (mods->sm_op) {
		case LDAP_MOD_ADD:
			rc = modify_add_values(entry, mods,
				   get_permissiveModify(op),
				   &rs->sr_text, textbuf,
				   sizeof( textbuf ) );
			break;
				
		case LDAP_MOD_DELETE:
			rc = modify_delete_values(entry, mods,
				get_permissiveModify(op),
				&rs->sr_text, textbuf,
				sizeof( textbuf ) );
			break;
				
		case LDAP_MOD_REPLACE:
			rc = modify_replace_values(entry, mods,
				 get_permissiveModify(op),
				 &rs->sr_text, textbuf,
				 sizeof( textbuf ) );
			break;

		case LDAP_MOD_INCREMENT:
			rc = modify_increment_values( entry,
				mods, get_permissiveModify(op),
				&rs->sr_text, textbuf,
				sizeof( textbuf ) );
			break;

		case SLAP_MOD_SOFTADD:
			mods->sm_op = LDAP_MOD_ADD;
			rc = modify_add_values(entry, mods,
				   get_permissiveModify(op),
				   &rs->sr_text, textbuf,
				   sizeof( textbuf ) );
			mods->sm_op = SLAP_MOD_SOFTADD;
			if (rc == LDAP_TYPE_OR_VALUE_EXISTS) {
				rc = LDAP_SUCCESS;
			}
			break;
		}
		if(rc != LDAP_SUCCESS) break;
	}

	if(rc == LDAP_SUCCESS) {
		if ( is_oc ) {
			entry->e_ocflags = 0;
		}
		/* check that the entry still obeys the schema */
		rc = entry_schema_check( op, entry, NULL, 0, 0,
			  &rs->sr_text, textbuf, sizeof( textbuf ) );
	}

	return rc;
}

int
ldif_back_referrals( Operation *op, SlapReply *rs )
{
	struct ldif_info	*li = NULL;
	Entry			*entry;
	int			rc = LDAP_SUCCESS;

#if 0
	if ( op->o_tag == LDAP_REQ_SEARCH ) {
		/* let search take care of itself */
		return rc;
	}
#endif

	if ( get_manageDSAit( op ) ) {
		/* let op take care of DSA management */
		return rc;
	}

	if ( BER_BVISEMPTY( &op->o_req_ndn ) ) {
		/* the empty DN cannot be a referral */
		return rc;
	}

	li = (struct ldif_info *)op->o_bd->be_private;
	ldap_pvt_thread_rdwr_rlock( &li->li_rdwr );
	get_entry( op, &entry, NULL );

	/* no object is found for them */
	if ( entry == NULL ) {
		struct berval	odn = op->o_req_dn;
		struct berval	ondn = op->o_req_ndn;
		struct berval	pndn = ondn;
		ber_len_t		min_dnlen = op->o_bd->be_nsuffix[0].bv_len;

		if ( min_dnlen == 0 )
			min_dnlen = 1;	   /* catch empty DN */

		for ( ; entry == NULL; ) {
			dnParent( &pndn, &pndn );
			if ( pndn.bv_len < min_dnlen ) {
				break;
			}

			op->o_req_dn = pndn;
			op->o_req_ndn = pndn;

			get_entry( op, &entry, NULL );
		}

		ldap_pvt_thread_rdwr_runlock( &li->li_rdwr );

		op->o_req_dn = odn;
		op->o_req_ndn = ondn;

		rc = LDAP_SUCCESS;
		rs->sr_matched = NULL;
		if ( entry != NULL ) {
			Debug( LDAP_DEBUG_TRACE,
				"ldif_back_referrals: tag=%lu target=\"%s\" matched=\"%s\"\n",
				(unsigned long) op->o_tag, op->o_req_dn.bv_val, entry->e_name.bv_val );

			if ( is_entry_referral( entry ) ) {
				rc = LDAP_OTHER;
				rs->sr_ref = get_entry_referrals( op, entry );
				if ( rs->sr_ref ) {
					rs->sr_matched = ber_strdup_x(
					entry->e_name.bv_val, op->o_tmpmemctx );
				}
			}

			entry_free(entry);

		} else if ( default_referral != NULL ) {
			rc = LDAP_OTHER;
			rs->sr_ref = referral_rewrite( default_referral,
				NULL, &op->o_req_dn, LDAP_SCOPE_DEFAULT );
		}

		if ( rs->sr_ref != NULL ) {
			/* send referrals */
			rc = rs->sr_err = LDAP_REFERRAL;
			send_ldap_result( op, rs );
			ber_bvarray_free( rs->sr_ref );
			rs->sr_ref = NULL;

		} else if ( rc != LDAP_SUCCESS ) {
			rs->sr_text = rs->sr_matched ? "bad referral object" : NULL;
		}

		if ( rs->sr_matched ) {
			op->o_tmpfree( (char *)rs->sr_matched, op->o_tmpmemctx );
			rs->sr_matched = NULL;
		}

		return rc;
	}

	ldap_pvt_thread_rdwr_runlock( &li->li_rdwr );

	if ( is_entry_referral( entry ) ) {
		/* entry is a referral */
		BerVarray refs = get_entry_referrals( op, entry );
		rs->sr_ref = referral_rewrite(
			refs, &entry->e_name, &op->o_req_dn, LDAP_SCOPE_DEFAULT );

		Debug( LDAP_DEBUG_TRACE,
			"ldif_back_referrals: tag=%lu target=\"%s\" matched=\"%s\"\n",
			(unsigned long) op->o_tag, op->o_req_dn.bv_val, entry->e_name.bv_val );

		rs->sr_matched = entry->e_name.bv_val;
		if ( rs->sr_ref != NULL ) {
			rc = rs->sr_err = LDAP_REFERRAL;
			send_ldap_result( op, rs );
			ber_bvarray_free( rs->sr_ref );
			rs->sr_ref = NULL;

		} else {
			rc = LDAP_OTHER;
			rs->sr_text = "bad referral object";
		}

		rs->sr_matched = NULL;
		ber_bvarray_free( refs );
	}

	entry_free( entry );

	return rc;
}


/* LDAP operations */

static int
ldif_back_bind( Operation *op, SlapReply *rs )
{
	struct ldif_info *li;
	Attribute *a;
	AttributeDescription *password = slap_schema.si_ad_userPassword;
	int return_val;
	Entry *entry;

	switch ( be_rootdn_bind( op, rs ) ) {
	case SLAP_CB_CONTINUE:
		break;

	default:
		/* in case of success, front end will send result;
		 * otherwise, be_rootdn_bind() did */
		return rs->sr_err;
	}

	li = (struct ldif_info *) op->o_bd->be_private;
	ldap_pvt_thread_rdwr_rlock(&li->li_rdwr);
	return_val = get_entry(op, &entry, NULL);

	/* no object is found for them */
	if(return_val != LDAP_SUCCESS) {
		rs->sr_err = return_val = LDAP_INVALID_CREDENTIALS;
		goto return_result;
	}

	/* they don't have userpassword */
	if((a = attr_find(entry->e_attrs, password)) == NULL) {
		rs->sr_err = LDAP_INAPPROPRIATE_AUTH;
		return_val = 1;
		goto return_result;
	}

	/* authentication actually failed */
	if(slap_passwd_check(op, entry, a, &op->oq_bind.rb_cred,
			     &rs->sr_text) != 0) {
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		return_val = 1;
		goto return_result;
	}

	/* let the front-end send success */
	return_val = 0;
	goto return_result;

 return_result:
	ldap_pvt_thread_rdwr_runlock(&li->li_rdwr);
	if(return_val != 0)
		send_ldap_result( op, rs );
	if(entry != NULL)
		entry_free(entry);
	return return_val;
}

static int ldif_back_search(Operation *op, SlapReply *rs)
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	enumCookie ck = { NULL, NULL, NULL, 0, 0 };

	ck.op = op;
	ck.rs = rs;
	ldap_pvt_thread_rdwr_rlock(&li->li_rdwr);
	rs->sr_err = enum_tree( &ck );
	ldap_pvt_thread_rdwr_runlock(&li->li_rdwr);
	send_ldap_result(op, rs);

	return rs->sr_err;
}

static int ldif_back_add(Operation *op, SlapReply *rs) {
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	Entry * e = op->ora_e;
	struct berval dn = e->e_nname;
	struct berval leaf_path = BER_BVNULL;
	struct stat stats;
	int statres;
	char textbuf[SLAP_TEXT_BUFLEN];

	Debug( LDAP_DEBUG_TRACE, "ldif_back_add: \"%s\"\n", dn.bv_val, 0, 0);

	rs->sr_err = entry_schema_check(op, e, NULL, 0, 1,
		&rs->sr_text, textbuf, sizeof( textbuf ) );
	if ( rs->sr_err != LDAP_SUCCESS ) goto send_res;

	rs->sr_err = slap_add_opattrs( op,
		&rs->sr_text, textbuf, sizeof( textbuf ), 1 );
	if ( rs->sr_err != LDAP_SUCCESS ) goto send_res;

	ldap_pvt_thread_rdwr_wlock(&li->li_rdwr);

	dn2path( op->o_bd, &dn, &leaf_path );

	if(leaf_path.bv_val != NULL) {
		struct berval base = BER_BVNULL;
		/* build path to container and ldif of container */
		get_parent_path(&leaf_path, &base);

		statres = stat(base.bv_val, &stats); /* check if container exists */
		if(statres == -1 && errno == ENOENT) { /* container missing */
			base.bv_val[base.bv_len] = LDIF_FILETYPE_SEP;
			statres = stat(base.bv_val, &stats); /* check for leaf node */
			base.bv_val[base.bv_len] = '\0';
			if(statres == -1 && errno == ENOENT) {
				rs->sr_err = LDAP_NO_SUCH_OBJECT; /* parent doesn't exist */
				rs->sr_text = "Parent does not exist";
			}
			else if(statres != -1) { /* create parent */
				int mkdirres = mkdir(base.bv_val, 0750);
				if(mkdirres == -1) {
					rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
					rs->sr_text = "Could not create parent folder";
					Debug( LDAP_DEBUG_ANY, "could not create folder \"%s\": %s\n",
						base.bv_val, STRERROR( errno ), 0 );
				}
			}
			else
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		}/* container was possibly created, move on to add the entry */
		if(rs->sr_err == LDAP_SUCCESS) {
			statres = stat(leaf_path.bv_val, &stats);
			if(statres == -1 && errno == ENOENT) {
				rs->sr_err = spew_entry(e, &leaf_path, 1, NULL);
			}
			else if ( statres == -1 ) {
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				Debug( LDAP_DEBUG_ANY, "could not stat file \"%s\": %s\n",
					leaf_path.bv_val, STRERROR( errno ), 0 );
			}
			else /* it already exists */
				rs->sr_err = LDAP_ALREADY_EXISTS;
		}
		SLAP_FREE(base.bv_val);
		SLAP_FREE(leaf_path.bv_val);
	}

	ldap_pvt_thread_rdwr_wunlock(&li->li_rdwr);

send_res:
	Debug( LDAP_DEBUG_TRACE, 
			"ldif_back_add: err: %d text: %s\n", rs->sr_err, rs->sr_text ?
				rs->sr_text : "", 0);
	send_ldap_result(op, rs);
	slap_graduate_commit_csn( op );
	return rs->sr_err;
}

static int ldif_back_modify(Operation *op, SlapReply *rs) {
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	Modifications * modlst = op->orm_modlist;
	struct berval path;
	Entry *entry;
	int spew_res;

	slap_mods_opattrs( op, &op->orm_modlist, 1 );

	ldap_pvt_thread_rdwr_wlock(&li->li_rdwr);

	rs->sr_err = get_entry( op, &entry, &path );
	if(entry != NULL) {
		rs->sr_err = apply_modify_to_entry(entry, modlst, op, rs);
		if(rs->sr_err == LDAP_SUCCESS) {
			int save_errno;
			spew_res = spew_entry(entry, &path, 1, &save_errno);
			if(spew_res == -1) {
				Debug( LDAP_DEBUG_ANY,
					"%s ldif_back_modify: could not output entry \"%s\": %s\n",
					op->o_log_prefix, entry->e_name.bv_val, STRERROR( save_errno ) );
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			}
		}

		entry_free( entry );
		SLAP_FREE( path.bv_val );
	}

	rs->sr_text = NULL;
	ldap_pvt_thread_rdwr_wunlock(&li->li_rdwr);
	send_ldap_result(op, rs);
	slap_graduate_commit_csn( op );
	return rs->sr_err;
}

static int ldif_back_delete(Operation *op, SlapReply *rs) {
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	struct berval path;
	int res = 0;

	if ( BER_BVISEMPTY( &op->o_csn )) {
		struct berval csn;
		char csnbuf[LDAP_LUTIL_CSNSTR_BUFSIZE];

		csn.bv_val = csnbuf;
		csn.bv_len = sizeof( csnbuf );
		slap_get_csn( op, &csn, 1 );
	}

	ldap_pvt_thread_rdwr_wlock(&li->li_rdwr);

	dn2path( op->o_bd, &op->o_req_ndn, &path );
	path.bv_val[path.bv_len - STRLENOF(LDIF)] = '\0';
	res = rmdir(path.bv_val);
	path.bv_val[path.bv_len - STRLENOF(LDIF)] = LDIF_FILETYPE_SEP;
	rs->sr_err = LDAP_SUCCESS;
	if ( res ) {
		switch ( errno ) {
		case ENOTEMPTY:
			rs->sr_err = LDAP_NOT_ALLOWED_ON_NONLEAF;
			break;

		case ENOENT:
			/* is leaf, go on */
			res = 0;
			break;

		default:
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			break;
		}
	}

	if ( !res ) {
		res = unlink(path.bv_val);
		if ( res == -1 ) {
			switch ( errno ) {
			case ENOENT:
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
				break;

			default:
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				break;
			}
		}
	}

	SLAP_FREE(path.bv_val);
	ldap_pvt_thread_rdwr_wunlock(&li->li_rdwr);
	send_ldap_result(op, rs);
	slap_graduate_commit_csn( op );
	return rs->sr_err;
}


static int
ldif_move_entry(
	Operation *op,
	Entry *entry,
	struct berval *oldpath )
{
	int res;
	int exists_res;
	struct berval newpath;

	dn2path( op->o_bd, &entry->e_nname, &newpath );

	if((entry == NULL || oldpath->bv_val == NULL) || newpath.bv_val == NULL) {
		/* some object doesn't exist */
		res = LDAP_NO_SUCH_OBJECT;
	}
	else { /* do the modrdn */
		exists_res = open(newpath.bv_val, O_RDONLY);
		if(exists_res == -1 && errno == ENOENT) {
			ldap_pvt_thread_mutex_lock( &entry2str_mutex );
			res = spew_entry(entry, &newpath, 0, NULL);
			if(res != -1) {
				/* if this fails we should log something bad */
				res = unlink( oldpath->bv_val );
				oldpath->bv_val[oldpath->bv_len - STRLENOF(".ldif")] = '\0';
				newpath.bv_val[newpath.bv_len - STRLENOF(".ldif")] = '\0';
				res = rename( oldpath->bv_val, newpath.bv_val );
				res = LDAP_SUCCESS;
			}
			else {
				if(errno == ENOENT)
					res = LDAP_NO_SUCH_OBJECT;
				else
					res = LDAP_UNWILLING_TO_PERFORM;
				unlink(newpath.bv_val); /* in case file was created */
			}
			ldap_pvt_thread_mutex_unlock( &entry2str_mutex );
		}
		else if(exists_res) {
			int close_res = close(exists_res);
			res = LDAP_ALREADY_EXISTS;
			if(close_res == -1) {
			/* log heinous error */
			}
		}
		else {
			res = LDAP_UNWILLING_TO_PERFORM;
		}
	}

	if(newpath.bv_val != NULL)
		SLAP_FREE(newpath.bv_val);
	return res;
}

static int
ldif_back_modrdn(Operation *op, SlapReply *rs)
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	struct berval new_dn = BER_BVNULL, new_ndn = BER_BVNULL;
	struct berval p_dn, old_path;
	Entry *entry;
	int rc;

	slap_mods_opattrs( op, &op->orr_modlist, 1 );

	ldap_pvt_thread_rdwr_wlock( &li->li_rdwr );

	rc = get_entry( op, &entry, &old_path );
	if ( rc == LDAP_SUCCESS ) {
		/* build new dn, and new ndn for the entry */
		if ( op->oq_modrdn.rs_newSup != NULL ) {
			struct berval	op_dn = op->o_req_dn,
					op_ndn = op->o_req_ndn;
			Entry		*np;

			/* new superior */
			p_dn = *op->oq_modrdn.rs_newSup;
			op->o_req_dn = *op->oq_modrdn.rs_newSup;
			op->o_req_ndn = *op->oq_modrdn.rs_nnewSup;
			rc = get_entry( op, &np, NULL );
			op->o_req_dn = op_dn;
			op->o_req_ndn = op_ndn;
			if ( rc != LDAP_SUCCESS ) {
				goto no_such_object;
			}
			entry_free( np );
		} else {
			dnParent( &entry->e_name, &p_dn );
		}
		build_new_dn( &new_dn, &p_dn, &op->oq_modrdn.rs_newrdn, NULL ); 
		dnNormalize( 0, NULL, NULL, &new_dn, &new_ndn, NULL );
		ber_memfree_x( entry->e_name.bv_val, NULL );
		ber_memfree_x( entry->e_nname.bv_val, NULL );
		entry->e_name = new_dn;
		entry->e_nname = new_ndn;

		/* perform the modifications */
		rc = apply_modify_to_entry( entry, op->orr_modlist, op, rs );
		if ( rc == LDAP_SUCCESS )
			rc = ldif_move_entry( op, entry, &old_path );

no_such_object:;
		entry_free( entry );
		SLAP_FREE( old_path.bv_val );
	}

	rs->sr_text = "";
	ldap_pvt_thread_rdwr_wunlock( &li->li_rdwr );
	rs->sr_err = rc;
	send_ldap_result( op, rs );
	slap_graduate_commit_csn( op );
	return rs->sr_err;
}


/* Return LDAP_SUCCESS IFF we retrieve the specified entry. */
static int
ldif_back_entry_get(
	Operation *op,
	struct berval *ndn,
	ObjectClass *oc,
	AttributeDescription *at,
	int rw,
	Entry **e )
{
	struct ldif_info *li = (struct ldif_info *) op->o_bd->be_private;
	struct berval op_dn = op->o_req_dn, op_ndn = op->o_req_ndn;
	int rc;

	assert( ndn != NULL );
	assert( !BER_BVISNULL( ndn ) );

	ldap_pvt_thread_rdwr_rlock( &li->li_rdwr );
	op->o_req_dn = *ndn;
	op->o_req_ndn = *ndn;
	rc = get_entry( op, e, NULL );
	op->o_req_dn = op_dn;
	op->o_req_ndn = op_ndn;
	ldap_pvt_thread_rdwr_runlock( &li->li_rdwr );

	if ( rc == LDAP_SUCCESS && oc && !is_entry_objectclass_or_sub( *e, oc ) ) {
		rc = LDAP_NO_SUCH_ATTRIBUTE;
		entry_free( *e );
		*e = NULL;
	}

	return rc;
}


/* Slap tools */

static int ldif_tool_entry_open(BackendDB *be, int mode) {
	struct ldif_info *li = (struct ldif_info *) be->be_private;
	li->li_tool_current = 0;
	return 0;
}					

static int ldif_tool_entry_close(BackendDB * be) {
	struct ldif_info *li = (struct ldif_info *) be->be_private;

	SLAP_FREE(li->li_tool_cookie.entries);
	return 0;
}

static ID ldif_tool_entry_next(BackendDB *be)
{
	struct ldif_info *li = (struct ldif_info *) be->be_private;
	if(li->li_tool_current >= li->li_tool_cookie.eind)
		return NOID;
	else
		return ++li->li_tool_current;
}

static ID
ldif_tool_entry_first(BackendDB *be)
{
	struct ldif_info *li = (struct ldif_info *) be->be_private;

	if(li->li_tool_cookie.entries == NULL) {
		Operation op = {0};

		op.o_bd = be;
		op.o_req_dn = *be->be_suffix;
		op.o_req_ndn = *be->be_nsuffix;
		op.ors_scope = LDAP_SCOPE_SUBTREE;
		li->li_tool_cookie.op = &op;
		(void)enum_tree( &li->li_tool_cookie );
		li->li_tool_cookie.op = NULL;
	}
	return ldif_tool_entry_next( be );
}

static Entry * ldif_tool_entry_get(BackendDB * be, ID id) {
	struct ldif_info *li = (struct ldif_info *) be->be_private;
	Entry * e;

	if(id > li->li_tool_cookie.eind || id < 1)
		return NULL;
	else {
		e = li->li_tool_cookie.entries[id - 1];
		li->li_tool_cookie.entries[id - 1] = NULL;
		return e;
	}
}

static ID ldif_tool_entry_put(BackendDB * be, Entry * e, struct berval *text) {
	struct berval leaf_path = BER_BVNULL;
	struct stat stats;
	int statres;
	int res = LDAP_SUCCESS;

	dn2path( be, &e->e_nname, &leaf_path );

	if(leaf_path.bv_val != NULL) {
		struct berval base = BER_BVNULL;
		/* build path to container, and path to ldif of container */
		get_parent_path(&leaf_path, &base);

		statres = stat(base.bv_val, &stats); /* check if container exists */
		if(statres == -1 && errno == ENOENT) { /* container missing */
			base.bv_val[base.bv_len] = LDIF_FILETYPE_SEP;
			statres = stat(base.bv_val, &stats); /* check for leaf node */
			base.bv_val[base.bv_len] = '\0';
			if(statres == -1 && errno == ENOENT) {
				res = LDAP_NO_SUCH_OBJECT; /* parent doesn't exist */
			}
			else if(statres != -1) { /* create parent */
				int mkdirres = mkdir(base.bv_val, 0750);
				if(mkdirres == -1) {
					res = LDAP_UNWILLING_TO_PERFORM;
				}
			}
			else
				res = LDAP_UNWILLING_TO_PERFORM;
		}/* container was possibly created, move on to add the entry */
		if(res == LDAP_SUCCESS) {
			statres = stat(leaf_path.bv_val, &stats);
			if(statres == -1 && errno == ENOENT) {
				res = spew_entry(e, &leaf_path, 0, NULL);
			}
			else /* it already exists */
				res = LDAP_ALREADY_EXISTS;
		}
		SLAP_FREE(base.bv_val);
		SLAP_FREE(leaf_path.bv_val);
	}

	if(res == LDAP_SUCCESS) {
		return 1;
	}
	else
		return NOID;
}


/* Setup */

static int
ldif_back_db_init( BackendDB *be, ConfigReply *cr )
{
	struct ldif_info *li;

	li = ch_calloc( 1, sizeof(struct ldif_info) );
	be->be_private = li;
	be->be_cf_ocs = ldifocs;
	ldap_pvt_thread_rdwr_init(&li->li_rdwr);
	SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_ONE_SUFFIX;
	return 0;
}

static int
ldif_back_db_destroy( Backend *be, ConfigReply *cr )
{
	struct ldif_info *li = be->be_private;

	ch_free(li->li_base_path.bv_val);
	ldap_pvt_thread_rdwr_destroy(&li->li_rdwr);
	free( be->be_private );
	return 0;
}

static int
ldif_back_db_open( Backend *be, ConfigReply *cr)
{
	struct ldif_info *li = (struct ldif_info *) be->be_private;
	if( BER_BVISEMPTY(&li->li_base_path)) {/* missing base path */
		Debug( LDAP_DEBUG_ANY, "missing base path for back-ldif\n", 0, 0, 0);
		return 1;
	}
	return 0;
}

int
ldif_back_initialize(
			   BackendInfo	*bi
			   )
{
	static char *controls[] = {
		LDAP_CONTROL_MANAGEDSAIT,
		NULL
	};
	int rc;

	bi->bi_flags |=
		SLAP_BFLAG_INCREMENT |
		SLAP_BFLAG_REFERRALS;

	bi->bi_controls = controls;

	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = ldif_back_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = ldif_back_db_open;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = ldif_back_db_destroy;

	bi->bi_op_bind = ldif_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = ldif_back_search;
	bi->bi_op_compare = 0;
	bi->bi_op_modify = ldif_back_modify;
	bi->bi_op_modrdn = ldif_back_modrdn;
	bi->bi_op_add = ldif_back_add;
	bi->bi_op_delete = ldif_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = ldif_back_referrals;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	bi->bi_entry_get_rw = ldif_back_entry_get;

#if 0	/* NOTE: uncomment to completely disable access control */
	bi->bi_access_allowed = slap_access_always_allowed;
#endif

	bi->bi_tool_entry_open = ldif_tool_entry_open;
	bi->bi_tool_entry_close = ldif_tool_entry_close;
	bi->bi_tool_entry_first = ldif_tool_entry_first;
	bi->bi_tool_entry_next = ldif_tool_entry_next;
	bi->bi_tool_entry_get = ldif_tool_entry_get;
	bi->bi_tool_entry_put = ldif_tool_entry_put;
	bi->bi_tool_entry_reindex = 0;
	bi->bi_tool_sync = 0;
	
	bi->bi_tool_dn2id_get = 0;
	bi->bi_tool_entry_modify = 0;

	bi->bi_cf_ocs = ldifocs;

	rc = config_register_schema( ldifcfg, ldifocs );
	if ( rc ) return rc;
	return 0;
}
