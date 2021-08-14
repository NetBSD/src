/*	$NetBSD: homedir.c,v 1.2 2021/08/14 16:15:02 christos Exp $	*/

/* homedir.c - create/remove user home directories */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2009-2010 The OpenLDAP Foundation.
 * Portions copyright 2009-2010 Symas Corporation.
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
 * This work was initially developed by Emily Backes at Symas
 * Corp. for inclusion in OpenLDAP Software.
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: homedir.c,v 1.2 2021/08/14 16:15:02 christos Exp $");

#include "portable.h"

#ifdef SLAPD_OVER_HOMEDIR

#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <fcntl.h>

#include <ac/string.h>
#include <ac/ctype.h>
#include <ac/errno.h>
#include <sys/stat.h>
#include <ac/unistd.h>
#include <ac/dirent.h>
#include <ac/time.h>

#include "slap.h"
#include "slap-config.h"

#define DEFAULT_MIN_UID ( 100 )
#define DEFAULT_SKEL ( LDAP_DIRSEP "etc" LDAP_DIRSEP "skel" )

typedef struct homedir_regexp {
	char *match;
	char *replace;
	regex_t compiled;
	struct homedir_regexp *next;
} homedir_regexp;

typedef enum {
	DEL_IGNORE,
	DEL_DELETE,
	DEL_ARCHIVE
} delete_style;

typedef struct homedir_data {
	char *skeleton_path;
	unsigned min_uid;
	AttributeDescription *home_ad;
	AttributeDescription *uidn_ad;
	AttributeDescription *gidn_ad;
	homedir_regexp *regexps;
	delete_style style;
	char *archive_path;
} homedir_data;

typedef struct homedir_cb_data {
	slap_overinst *on;
	Entry *entry;
} homedir_cb_data;

typedef struct name_list {
	char *name;
	struct stat st;
	struct name_list *next;
} name_list;

typedef struct name_list_list {
	name_list *list;
	struct name_list_list *next;
} name_list_list;

typedef enum {
	TRAVERSE_CB_CONTINUE,
	TRAVERSE_CB_DONE,
	TRAVERSE_CB_FAIL
} traverse_cb_ret;

/* private, file info, context */
typedef traverse_cb_ret (*traverse_cb_func)(
		void *,
		const char *,
		const struct stat *,
		void * );
typedef struct traverse_cb {
	traverse_cb_func pre_func;
	traverse_cb_func post_func;
	void *pre_private;
	void *post_private;
} traverse_cb;

typedef struct copy_private {
	int source_prefix_len;
	const char *dest_prefix;
	int dest_prefix_len;
	uid_t uidn;
	gid_t gidn;
} copy_private;

typedef struct chown_private {
	uid_t old_uidn;
	uid_t new_uidn;
	gid_t old_gidn;
	gid_t new_gidn;
} chown_private;

typedef struct ustar_header {
	char name[100];
	char mode[8];
	char uid[8];
	char gid[8];
	char size[12];
	char mtime[12];
	char checksum[8];
	char typeflag[1];
	char linkname[100];
	char magic[6];
	char version[2];
	char uname[32];
	char gname[32];
	char devmajor[8];
	char devminor[8];
	char prefix[155];
	char pad[12];
} ustar_header;

typedef struct tar_private {
	FILE *file;
	const char *name;
} tar_private;

/* FIXME: This mutex really needs to be executable-global, but this
 * will have to do for now.
 */
static ldap_pvt_thread_mutex_t readdir_mutex;
static ConfigDriver homedir_regexp_cfg;
static ConfigDriver homedir_style_cfg;
static slap_overinst homedir;

static ConfigTable homedircfg[] = {
	{ "homedir-skeleton-path", "pathname", 2, 2, 0,
		ARG_STRING|ARG_OFFSET,
		(void *)offsetof(homedir_data, skeleton_path),
		"( OLcfgCtAt:8.1 "
			"NAME 'olcSkeletonPath' "
			"DESC 'Pathname for home directory skeleton template' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, { .v_string = DEFAULT_SKEL }
	},

	{ "homedir-min-uidnumber", "uid number", 2, 2, 0,
		ARG_UINT|ARG_OFFSET,
		(void *)offsetof(homedir_data, min_uid),
		"( OLcfgCtAt:8.2 "
			"NAME 'olcMinimumUidNumber' "
			"DESC 'Minimum uidNumber attribute to consider' "
			"SYNTAX OMsInteger "
			"SINGLE-VALUE )",
		NULL, { .v_uint = DEFAULT_MIN_UID }
	},

	{ "homedir-regexp", "regexp> <path", 3, 3, 0,
		ARG_MAGIC,
		homedir_regexp_cfg,
		"( OLcfgCtAt:8.3 "
			"NAME 'olcHomedirRegexp' "
			"DESC 'Regular expression for matching and transforming paths' "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
		NULL, NULL
	},

	{ "homedir-delete-style", "style", 2, 2, 0,
		ARG_MAGIC,
		homedir_style_cfg,
		"( OLcfgCtAt:8.4 "
			"NAME 'olcHomedirDeleteStyle' "
			"DESC 'Action to perform when removing a home directory' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},

	{ "homedir-archive-path", "pathname", 2, 2, 0,
		ARG_STRING|ARG_OFFSET,
		(void *)offsetof(homedir_data, archive_path),
		"( OLcfgCtAt:8.5 "
			"NAME 'olcHomedirArchivePath' "
			"DESC 'Pathname for home directory archival' "
			"SYNTAX OMsDirectoryString "
			"SINGLE-VALUE )",
		NULL, NULL
	},

	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs homedirocs[] = {
	{ "( OLcfgCtOc:8.1 "
		"NAME 'olcHomedirConfig' "
			"DESC 'Homedir configuration' "
			"SUP olcOverlayConfig "
			"MAY ( olcSkeletonPath $ olcMinimumUidNumber "
				"$ olcHomedirRegexp $ olcHomedirDeleteStyle "
				"$ olcHomedirArchivePath ) )",
		Cft_Overlay, homedircfg },

	{ NULL, 0, NULL }
};

static int
homedir_regexp_cfg( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	homedir_data *data = (homedir_data *)on->on_bi.bi_private;
	int rc = ARG_BAD_CONF;

	assert( data != NULL );

	switch ( c->op ) {
		case SLAP_CONFIG_EMIT: {
			int i;
			homedir_regexp *r;
			struct berval bv;
			char buf[4096];

			bv.bv_val = buf;
			for ( i = 0, r = data->regexps; r != NULL; ++i, r = r->next ) {
				bv.bv_len = snprintf( buf, sizeof(buf), "{%d}%s %s", i,
						r->match, r->replace );
				if ( bv.bv_len >= sizeof(buf) ) {
					Debug( LDAP_DEBUG_ANY, "homedir_regexp_cfg: "
							"emit serialization failed: size %lu\n",
							(unsigned long)bv.bv_len );
					return ARG_BAD_CONF;
				}
				value_add_one( &c->rvalue_vals, &bv );
			}
			rc = 0;
		} break;

		case LDAP_MOD_DELETE:
			if ( c->valx < 0 ) { /* delete all values */
				homedir_regexp *r, *rnext;

				for ( r = data->regexps; r != NULL; r = rnext ) {
					rnext = r->next;
					ch_free( r->match );
					ch_free( r->replace );
					regfree( &r->compiled );
					ch_free( r );
				}
				data->regexps = NULL;
				rc = 0;

			} else { /* delete value by index*/
				homedir_regexp **rp, *r;
				int i;

				for ( i = 0, rp = &data->regexps; i < c->valx;
						++i, rp = &(*rp)->next )
					;

				r = *rp;
				*rp = r->next;
				ch_free( r->match );
				ch_free( r->replace );
				regfree( &r->compiled );
				ch_free( r );

				rc = 0;
			}
			break;

		case LDAP_MOD_ADD:		/* fallthrough */
		case SLAP_CONFIG_ADD: { /* add values */
			char *match = c->argv[1];
			char *replace = c->argv[2];
			regex_t compiled;
			homedir_regexp **rp, *r;

			memset( &compiled, 0, sizeof(compiled) );
			rc = regcomp( &compiled, match, REG_EXTENDED );
			if ( rc ) {
				regerror( rc, &compiled, c->cr_msg, sizeof(c->cr_msg) );
				regfree( &compiled );
				return ARG_BAD_CONF;
			}

			r = ch_calloc( 1, sizeof(homedir_regexp) );
			r->match = strdup( match );
			r->replace = strdup( replace );
			r->compiled = compiled;

			if ( c->valx == -1 ) { /* append */
				for ( rp = &data->regexps; ( *rp ) != NULL;
						rp = &(*rp)->next )
					;
				*rp = r;

			} else { /* insert at valx */
				int i;
				for ( i = 0, rp = &data->regexps; i < c->valx;
						rp = &(*rp)->next, ++i )
					;
				r->next = *rp;
				*rp = r;
			}
			rc = 0;
			break;
		}
		default:
			abort();
	}

	return rc;
}

static int
homedir_style_cfg( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	homedir_data *data = (homedir_data *)on->on_bi.bi_private;
	int rc = ARG_BAD_CONF;
	struct berval bv;

	assert( data != NULL );

	switch ( c->op ) {
		case SLAP_CONFIG_EMIT:
			bv.bv_val = data->style == DEL_IGNORE ? "IGNORE" :
					data->style == DEL_DELETE	  ? "DELETE" :
													"ARCHIVE";
			bv.bv_len = strlen( bv.bv_val );
			rc = value_add_one( &c->rvalue_vals, &bv );
			if ( rc != 0 ) return ARG_BAD_CONF;
			break;

		case LDAP_MOD_DELETE:
			data->style = DEL_IGNORE;
			rc = 0;
			break;

		case LDAP_MOD_ADD:	  /* fallthrough */
		case SLAP_CONFIG_ADD: /* add values */
			if ( strcasecmp( c->argv[1], "IGNORE" ) == 0 )
				data->style = DEL_IGNORE;
			else if ( strcasecmp( c->argv[1], "DELETE" ) == 0 )
				data->style = DEL_DELETE;
			else if ( strcasecmp( c->argv[1], "ARCHIVE" ) == 0 )
				data->style = DEL_ARCHIVE;
			else {
				Debug( LDAP_DEBUG_ANY, "homedir_style_cfg: "
						"unrecognized style keyword\n" );
				return ARG_BAD_CONF;
			}
			rc = 0;
			break;

		default:
			abort();
	}

	return rc;
}

#define HOMEDIR_NULLWRAP(x) ( ( x ) == NULL ? "unknown" : (x) )
static void
report_errno( const char *parent_func, const char *func, const char *filename )
{
	int save_errno = errno;
	char ebuf[1024];

	Debug( LDAP_DEBUG_ANY, "homedir: "
			"%s: %s: \"%s\": %d (%s)\n",
			HOMEDIR_NULLWRAP(parent_func), HOMEDIR_NULLWRAP(func),
			HOMEDIR_NULLWRAP(filename), save_errno,
			AC_STRERROR_R( save_errno, ebuf, sizeof(ebuf) ) );
}

static int
copy_link(
		const char *dest_file,
		const char *source_file,
		const struct stat *st,
		uid_t uidn,
		gid_t gidn,
		void *ctx )
{
	char *buf = NULL;
	int rc;

	assert( dest_file != NULL );
	assert( source_file != NULL );
	assert( st != NULL );
	assert( (st->st_mode & S_IFMT) == S_IFLNK );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"copy_link: %s to %s\n",
			source_file, dest_file );
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"copy_link: %s uid %ld gid %ld\n",
			dest_file, (long)uidn, (long)gidn );

	/* calloc +1 for terminator */
	buf = ber_memcalloc_x( 1, st->st_size + 1, ctx );
	if ( buf == NULL ) {
		Debug( LDAP_DEBUG_ANY, "homedir: "
				"copy_link: alloc failed\n" );
		return 1;
	}
	rc = readlink( source_file, buf, st->st_size );
	if ( rc == -1 ) {
		report_errno( "copy_link", "readlink", source_file );
		goto fail;
	}
	rc = symlink( buf, dest_file );
	if ( rc ) {
		report_errno( "copy_link", "symlink", dest_file );
		goto fail;
	}
	rc = lchown( dest_file, uidn, gidn );
	if ( rc ) {
		report_errno( "copy_link", "lchown", dest_file );
		goto fail;
	}
	goto out;

fail:
	rc = 1;

out:
	if ( buf != NULL ) ber_memfree_x( buf, ctx );
	return rc;
}

static int
copy_blocks(
		FILE *source,
		FILE *dest,
		const char *source_file,
		const char *dest_file )
{
	char buf[4096];
	size_t nread = 0;
	int done = 0;

	while ( !done ) {
		nread = fread( buf, 1, sizeof(buf), source );
		if ( nread == 0 ) {
			if ( feof( source ) ) {
				done = 1;
			} else if ( ferror( source ) ) {
				if ( source_file != NULL )
					Debug( LDAP_DEBUG_ANY, "homedir: "
							"read error on %s\n",
							source_file );
				goto fail;
			}
		} else {
			size_t nwritten = 0;
			nwritten = fwrite( buf, 1, nread, dest );
			if ( nwritten < nread ) {
				if ( dest_file != NULL )
					Debug( LDAP_DEBUG_ANY, "homedir: "
							"write error on %s\n",
							dest_file );
				goto fail;
			}
		}
	}
	return 0;
fail:
	return 1;
}

static int
copy_file(
		const char *dest_file,
		const char *source_file,
		uid_t uid,
		gid_t gid,
		int mode )
{
	FILE *source = NULL;
	FILE *dest = NULL;
	int rc;

	assert( dest_file != NULL );
	assert( source_file != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"copy_file: %s to %s mode 0%o\n",
			source_file, dest_file, mode );
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"copy_file: %s uid %ld gid %ld\n",
			dest_file, (long)uid, (long)gid );

	source = fopen( source_file, "rb" );
	if ( source == NULL ) {
		report_errno( "copy_file", "fopen", source_file );
		goto fail;
	}
	dest = fopen( dest_file, "wb" );
	if ( dest == NULL ) {
		report_errno( "copy_file", "fopen", dest_file );
		goto fail;
	}

	rc = copy_blocks( source, dest, source_file, dest_file );
	if ( rc != 0 ) goto fail;

	fclose( source );
	source = NULL;
	rc = fclose( dest );
	dest = NULL;
	if ( rc != 0 ) {
		report_errno( "copy_file", "fclose", dest_file );
		goto fail;
	}

	/* set owner/permission */
	rc = lchown( dest_file, uid, gid );
	if ( rc != 0 ) {
		report_errno( "copy_file", "lchown", dest_file );
		goto fail;
	}
	rc = chmod( dest_file, mode );
	if ( rc != 0 ) {
		report_errno( "copy_file", "chmod", dest_file );
		goto fail;
	}

	rc = 0;
	goto out;
fail:
	rc = 1;
out:
	if ( source != NULL ) fclose( source );
	if ( dest != NULL ) fclose( dest );
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"copy_file: %s to %s exit %d\n",
			source_file, dest_file, rc );
	return rc;
}

static void
free_name_list( name_list *names, void *ctx )
{
	name_list *next;

	while ( names != NULL ) {
		next = names->next;
		if ( names->name != NULL ) ber_memfree_x( names->name, ctx );
		ber_memfree_x( names, ctx );
		names = next;
	}
}

static int
grab_names( const char *dir_path, name_list **names, void *ctx )
{
	int locked = 0;
	DIR *dir = NULL;
	struct dirent *entry = NULL;
	name_list **tail = NULL;
	int dir_path_len = 0;
	int rc = 0;

	assert( dir_path != NULL );
	assert( names != NULL );
	assert( *names == NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"grab_names: %s\n", dir_path );

	tail = names;
	dir_path_len = strlen( dir_path );
	ldap_pvt_thread_mutex_lock( &readdir_mutex );
	locked = 1;

	dir = opendir( dir_path );
	if ( dir == NULL ) {
		report_errno( "grab_names", "opendir", dir_path );
		goto fail;
	}

	while ( ( entry = readdir( dir ) ) != NULL ) {
		/* no d_namelen in ac/dirent.h */
		int d_namelen = strlen( entry->d_name );
		int full_len;

		/* Skip . and .. */
		if ( ( d_namelen == 1 && entry->d_name[0] == '.' ) ||
				( d_namelen == 2 && entry->d_name[0] == '.' &&
						entry->d_name[1] == '.' ) ) {
			continue;
		}

		*tail = ber_memcalloc_x( 1, sizeof(**tail), ctx );
		if ( *tail == NULL ) {
			Debug( LDAP_DEBUG_ANY, "homedir: "
					"grab_names: list alloc failed\n" );
			goto fail;
		}
		(*tail)->next = NULL;

		/* +1 for dirsep, +1 for term */
		full_len = dir_path_len + 1 + d_namelen + 1;
		(*tail)->name = ber_memalloc_x( full_len, ctx );
		if ( (*tail)->name == NULL ) {
			Debug( LDAP_DEBUG_ANY, "homedir: "
					"grab_names: name alloc failed\n" );
			goto fail;
		}
		snprintf( (*tail)->name, full_len, "%s" LDAP_DIRSEP "%s",
				dir_path, entry->d_name );
		Debug( LDAP_DEBUG_TRACE, "homedir: "
				"grab_names: found \"%s\"\n",
				(*tail)->name );

		rc = lstat( (*tail)->name, &(*tail)->st );
		if ( rc ) {
			report_errno( "grab_names", "lstat", (*tail)->name );
			goto fail;
		}

		tail = &(*tail)->next;
	}
	closedir( dir );
	ldap_pvt_thread_mutex_unlock( &readdir_mutex );
	locked = 0;

	dir = NULL;
	goto success;

success:
	rc = 0;
	goto out;
fail:
	rc = 1;
	goto out;
out:
	if ( dir != NULL ) closedir( dir );
	if ( locked ) ldap_pvt_thread_mutex_unlock( &readdir_mutex );
	if ( rc != 0 && *names != NULL ) {
		free_name_list( *names, ctx );
		*names = NULL;
	}
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"grab_names: %s exit %d\n",
			dir_path, rc );
	return rc;
}

static int
traverse( const char *path, const traverse_cb *cb, void *ctx )
{
	name_list *next_name = NULL;
	name_list_list *dir_stack = NULL;
	name_list_list *next_dir;
	int rc = 0;

	assert( path != NULL );
	assert( cb != NULL );
	assert( cb->pre_func || cb->post_func );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse: %s\n", path );

	dir_stack = ber_memcalloc_x( 1, sizeof(*dir_stack), ctx );
	if ( dir_stack == NULL ) goto alloc_fail;
	dir_stack->next = NULL;
	dir_stack->list = ber_memcalloc_x( 1, sizeof(name_list), ctx );
	if ( dir_stack->list == NULL ) goto alloc_fail;
	rc = lstat( path, &dir_stack->list->st );
	if ( rc != 0 ) {
		report_errno( "traverse", "lstat", path );
		goto fail;
	}
	dir_stack->list->next = NULL;
	dir_stack->list->name = ber_strdup_x( path, ctx );
	if ( dir_stack->list->name == NULL ) goto alloc_fail;

	while ( dir_stack != NULL ) {
		while ( dir_stack->list != NULL ) {
			Debug( LDAP_DEBUG_TRACE, "homedir: "
					"traverse: top of loop with \"%s\"\n",
					dir_stack->list->name );

			if ( cb->pre_func != NULL ) {
				traverse_cb_ret cb_rc;
				cb_rc = cb->pre_func( cb->pre_private, dir_stack->list->name,
						&dir_stack->list->st, ctx );

				if ( cb_rc == TRAVERSE_CB_DONE ) goto cb_done;
				if ( cb_rc == TRAVERSE_CB_FAIL ) goto cb_fail;
			}
			if ( (dir_stack->list->st.st_mode & S_IFMT) == S_IFDIR ) {
				/* push dir onto stack */
				next_dir = dir_stack;
				dir_stack = ber_memalloc_x( sizeof(*dir_stack), ctx );
				if ( dir_stack == NULL ) {
					dir_stack = next_dir;
					goto alloc_fail;
				}
				dir_stack->list = NULL;
				dir_stack->next = next_dir;
				rc = grab_names(
						dir_stack->next->list->name, &dir_stack->list, ctx );
				if ( rc != 0 ) {
					Debug( LDAP_DEBUG_ANY, "homedir: "
							"traverse: grab_names %s failed\n",
							dir_stack->next->list->name );
					goto fail;
				}
			} else {
				/* just a file */
				if ( cb->post_func != NULL ) {
					traverse_cb_ret cb_rc;
					cb_rc = cb->post_func( cb->post_private,
							dir_stack->list->name, &dir_stack->list->st, ctx );

					if ( cb_rc == TRAVERSE_CB_DONE ) goto cb_done;
					if ( cb_rc == TRAVERSE_CB_FAIL ) goto cb_fail;
				}
				next_name = dir_stack->list->next;
				ber_memfree_x( dir_stack->list->name, ctx );
				ber_memfree_x( dir_stack->list, ctx );
				dir_stack->list = next_name;
			}
		}
		/* Time to pop a directory off the stack */
		next_dir = dir_stack->next;
		ber_memfree_x( dir_stack, ctx );
		dir_stack = next_dir;
		if ( dir_stack != NULL ) {
			if ( cb->post_func != NULL ) {
				traverse_cb_ret cb_rc;
				cb_rc = cb->post_func( cb->post_private, dir_stack->list->name,
						&dir_stack->list->st, ctx );

				if ( cb_rc == TRAVERSE_CB_DONE ) goto cb_done;
				if ( cb_rc == TRAVERSE_CB_FAIL ) goto cb_fail;
			}
			next_name = dir_stack->list->next;
			ber_memfree_x( dir_stack->list->name, ctx );
			ber_memfree_x( dir_stack->list, ctx );
			dir_stack->list = next_name;
		}
	}

	goto success;

cb_done:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse: cb signaled completion\n" );
success:
	rc = 0;
	goto out;

cb_fail:
	Debug( LDAP_DEBUG_ANY, "homedir: "
			"traverse: cb signaled failure\n" );
	goto fail;
alloc_fail:
	Debug( LDAP_DEBUG_ANY, "homedir: "
			"traverse: allocation failed\n" );
fail:
	rc = 1;
	goto out;

out:
	while ( dir_stack != NULL ) {
		free_name_list( dir_stack->list, ctx );
		next_dir = dir_stack->next;
		ber_memfree_x( dir_stack, ctx );
		dir_stack = next_dir;
	}
	return rc;
}

static traverse_cb_ret
traverse_copy_pre(
		void *private,
		const char *name,
		const struct stat *st,
		void *ctx )
{
	copy_private *cp = private;
	char *dest_name = NULL;
	int source_name_len;
	int dest_name_len;
	int rc;

	assert( private != NULL );
	assert( name != NULL );
	assert( st != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_copy_pre: %s entering\n",
			name );

	assert( cp->source_prefix_len >= 0 );
	assert( cp->dest_prefix != NULL );
	assert( cp->dest_prefix_len > 1 );

	source_name_len = strlen( name );
	assert( source_name_len >= cp->source_prefix_len );
	/* +1 for terminator */
	dest_name_len =
			source_name_len + cp->dest_prefix_len - cp->source_prefix_len + 1;
	dest_name = ber_memalloc_x( dest_name_len, ctx );
	if ( dest_name == NULL ) goto alloc_fail;

	snprintf( dest_name, dest_name_len, "%s%s", cp->dest_prefix,
			name + cp->source_prefix_len );

	switch ( st->st_mode & S_IFMT ) {
		case S_IFDIR:
			rc = mkdir( dest_name, st->st_mode & 06775 );
			if ( rc ) {
				int save_errno = errno;
				switch ( save_errno ) {
					case EEXIST:
						/* directory already present; nothing to do */
						goto exists;
						break;
					case ENOENT:
						/* FIXME: should mkdir -p here */
						/* fallthrough for now */
					default:
						report_errno( "traverse_copy_pre", "mkdir", dest_name );
						goto fail;
				}
			}
			rc = lchown( dest_name, cp->uidn, cp->gidn );
			if ( rc ) {
				report_errno( "traverse_copy_pre", "lchown", dest_name );
				goto fail;
			}
			rc = chmod( dest_name, st->st_mode & 07777 );
			if ( rc ) {
				report_errno( "traverse_copy_pre", "chmod", dest_name );
				goto fail;
			}
			break;
		case S_IFREG:
			rc = copy_file(
					dest_name, name, cp->uidn, cp->gidn, st->st_mode & 07777 );
			if ( rc ) goto fail;
			break;
		case S_IFIFO:
			rc = mkfifo( dest_name, 0700 );
			if ( rc ) {
				report_errno( "traverse_copy_pre", "mkfifo", dest_name );
				goto fail;
			}
			rc = lchown( dest_name, cp->uidn, cp->gidn );
			if ( rc ) {
				report_errno( "traverse_copy_pre", "lchown", dest_name );
				goto fail;
			}
			rc = chmod( dest_name, st->st_mode & 07777 );
			if ( rc ) {
				report_errno( "traverse_copy_pre", "chmod", dest_name );
				goto fail;
			}
			break;
		case S_IFLNK:
			rc = copy_link( dest_name, name, st, cp->uidn, cp->gidn, ctx );
			if ( rc ) goto fail;
			break;
		default:
			Debug( LDAP_DEBUG_TRACE, "homedir: "
					"traverse_copy_pre: skipping special: %s\n",
					name );
	}

	goto success;

alloc_fail:
	Debug( LDAP_DEBUG_ANY, "homedir: "
			"traverse_copy_pre: allocation failed\n" );
fail:
	rc = TRAVERSE_CB_FAIL;
	goto out;

exists:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_copy_pre: \"%s\" already exists,"
			" skipping the rest\n",
			dest_name );
	rc = TRAVERSE_CB_DONE;
	goto out;

success:
	rc = TRAVERSE_CB_CONTINUE;
out:
	if ( dest_name != NULL ) ber_memfree_x( dest_name, ctx );
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_copy_pre: exit %d\n", rc );
	return rc;
}

static int
copy_tree(
		const char *dest_path,
		const char *source_path,
		uid_t uidn,
		gid_t gidn,
		void *ctx )
{
	traverse_cb cb;
	copy_private cp;
	int rc;

	assert( dest_path != NULL );
	assert( source_path != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"copy_tree: %s to %s entering\n",
			source_path, dest_path );

	cb.pre_func = traverse_copy_pre;
	cb.post_func = NULL;
	cb.pre_private = &cp;
	cb.post_private = NULL;

	cp.source_prefix_len = strlen( source_path );
	cp.dest_prefix = dest_path;
	cp.dest_prefix_len = strlen( dest_path );
	cp.uidn = uidn;
	cp.gidn = gidn;

	if ( cp.source_prefix_len <= cp.dest_prefix_len &&
			strncmp( source_path, dest_path, cp.source_prefix_len ) == 0 &&
			( cp.source_prefix_len == cp.dest_prefix_len ||
					dest_path[cp.source_prefix_len] == LDAP_DIRSEP[0] ) ) {
		Debug( LDAP_DEBUG_ANY, "homedir: "
				"copy_tree: aborting: %s contains %s\n",
				source_path, dest_path );
		return 1;
	}

	rc = traverse( source_path, &cb, ctx );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"copy_tree: %s exit %d\n", source_path,
			rc );

	return rc;
}

static int
homedir_provision(
		const char *dest_path,
		const char *skel_path,
		uid_t uidn,
		gid_t gidn,
		void *ctx )
{
	int rc;

	assert( dest_path != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_provision: %s from skeleton %s\n",
			dest_path, skel_path == NULL ? "(none)" : skel_path );
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_provision: %s uidn %ld gidn %ld\n",
			dest_path, (long)uidn, (long)gidn );

	if ( skel_path == NULL ) {
		rc = mkdir( dest_path, 0700 );
		if ( rc ) {
			int save_errno = errno;
			switch ( save_errno ) {
				case EEXIST:
					/* directory already present; nothing to do */
					/* but down chown either */
					rc = 0;
					goto out;
					break;
				default:
					report_errno( "provision_homedir", "mkdir", dest_path );
					goto fail;
			}
		}
		rc = lchown( dest_path, uidn, gidn );
		if ( rc ) {
			report_errno( "provision_homedir", "lchown", dest_path );
			goto fail;
		}

	} else {
		rc = copy_tree( dest_path, skel_path, uidn, gidn, ctx );
	}

	goto out;

fail:
	rc = 1;
	goto out;
out:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_provision: %s to %s exit %d\n",
			skel_path, dest_path, rc );
	return rc;
}

/* traverse func for rm -rf */
static traverse_cb_ret
traverse_remove_post(
		void *private,
		const char *name,
		const struct stat *st,
		void *ctx )
{
	int rc;

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_remove_post: %s entering\n",
			name );

	if ( (st->st_mode & S_IFMT) == S_IFDIR ) {
		rc = rmdir( name );
		if ( rc != 0 ) {
			report_errno( "traverse_remove_post", "rmdir", name );
			goto fail;
		}
	} else {
		rc = unlink( name );
		if ( rc != 0 ) {
			report_errno( "traverse_remove_post", "unlink", name );
			goto fail;
		}
	}

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_remove_post: %s exit continue\n",
			name );
	return TRAVERSE_CB_CONTINUE;

fail:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_remove_post: %s exit failure\n",
			name );
	return TRAVERSE_CB_FAIL;
}

static int
delete_tree( const char *path, void *ctx )
{
	const static traverse_cb cb = { NULL, traverse_remove_post, NULL, NULL };
	int rc;

	assert( path != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"delete_tree: %s entering\n", path );

	rc = traverse( path, &cb, ctx );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"delete_tree: %s exit %d\n", path, rc );

	return rc;
}

static int
get_tar_name(
		const char *path,
		const char *tar_path,
		char *tar_name,
		int name_size )
{
	int rc = 0;
	const char *ch;
	int fd = -1;
	int counter = 0;
	time_t now;

	assert( path != NULL );
	assert( tar_path != NULL );
	assert( tar_name != NULL );

	for ( ch = path + strlen( path );
			*ch != LDAP_DIRSEP[0] && ch > path;
			--ch )
		;
	if ( ch <= path || strlen( ch ) < 2 ) {
		Debug( LDAP_DEBUG_ANY, "homedir: "
				"get_tar_name: unable to construct a tar name from input "
				"path \"%s\"\n",
				path );
		goto fail;
	}
	++ch; /* skip past sep */
	time( &now );

	while ( fd < 0 ) {
		snprintf( tar_name, name_size, "%s" LDAP_DIRSEP "%s-%ld-%d.tar",
				tar_path, ch, (long)now, counter );
		fd = open( tar_name, O_WRONLY|O_CREAT|O_EXCL, 0600 );
		if ( fd < 0 ) {
			int save_errno = errno;
			if ( save_errno != EEXIST ) {
				report_errno( "get_tar_name", "open", tar_name );
				goto fail;
			}
			++counter;
		}
	}

	rc = 0;
	goto out;

fail:
	rc = 1;
	*tar_name = '\0';
out:
	if ( fd >= 0 ) close( fd );
	return rc;
}

/* traverse func for rechown */
static traverse_cb_ret
traverse_chown_pre(
		void *private,
		const char *name,
		const struct stat *st,
		void *ctx )
{
	int rc;
	chown_private *cp = private;
	uid_t set_uidn = -1;
	gid_t set_gidn = -1;

	assert( private != NULL );
	assert( name != NULL );
	assert( st != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_chown_pre: %s entering\n",
			name );

	if ( st->st_uid == cp->old_uidn ) set_uidn = cp->new_uidn;
	if ( st->st_gid == cp->old_gidn ) set_gidn = cp->new_gidn;

	if ( set_uidn != (uid_t)-1 || set_gidn != (gid_t)-1 ) {
		rc = lchown( name, set_uidn, set_gidn );
		if ( rc ) {
			report_errno( "traverse_chown_pre", "lchown", name );
			goto fail;
		}
	}

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_chown_pre: %s exit continue\n",
			name );
	return TRAVERSE_CB_CONTINUE;

fail:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_chown_pre: %s exit failure\n",
			name );
	return TRAVERSE_CB_FAIL;
}

static int
chown_tree(
		const char *path,
		uid_t old_uidn,
		uid_t new_uidn,
		gid_t old_gidn,
		gid_t new_gidn,
		void *ctx )
{
	traverse_cb cb;
	chown_private cp;
	int rc;

	assert( path != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"chown_tree: %s entering\n", path );

	cb.pre_func = traverse_chown_pre;
	cb.post_func = NULL;
	cb.pre_private = &cp;
	cb.post_private = NULL;

	cp.old_uidn = old_uidn;
	cp.new_uidn = new_uidn;
	cp.old_gidn = old_gidn;
	cp.new_gidn = new_gidn;

	rc = traverse( path, &cb, ctx );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"chown_tree: %s exit %d\n", path, rc );

	return rc;
}

static int
homedir_rename( const char *source_path, const char *dest_path )
{
	int rc = 0;

	assert( source_path != NULL );
	assert( dest_path != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_rename: %s to %s\n",
			source_path, dest_path );
	rc = rename( source_path, dest_path );
	if ( rc != 0 ) {
		char ebuf[1024];
		int save_errno = errno;

		Debug( LDAP_DEBUG_ANY, "homedir: "
				"homedir_rename: rename(\"%s\", \"%s\"): (%s)\n",
				source_path, dest_path,
				AC_STRERROR_R( save_errno, ebuf, sizeof(ebuf) ) );
	}

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_rename: %s to %s exit %d\n",
			source_path, dest_path, rc );
	return rc;
}

/* FIXME: This assumes ASCII; needs fixing for z/OS */
static int
tar_set_header( ustar_header *tar, const struct stat *st, const char *name )
{
	int name_len;
	int rc;
	const char *ch, *end;

	assert( tar != NULL );
	assert( st != NULL );
	assert( name != NULL );
	assert( sizeof(*tar) == 512 );
	assert( sizeof(tar->name) == 100 );
	assert( sizeof(tar->prefix) == 155 );
	assert( sizeof(tar->checksum) == 8 );

	memset( tar, 0, sizeof(*tar) );

	assert( name[0] == LDAP_DIRSEP[0] );
	name += 1; /* skip leading / */

	name_len = strlen( name );

	/* fits in tar->name? */
	/* Yes, name and prefix do not need a trailing nul. */
	if ( name_len <= 100 ) {
		strncpy( tar->name, name, 100 );

		/* try fit in tar->name + tar->prefix */
	} else {
		/* try to find something to stick into tar->name */
		for ( ch = name + name_len - 100, end = name + name_len;
				ch < end && *ch != LDAP_DIRSEP[0];
				++ch )
			;
		if ( end - ch > 0 ) /* +1 skip past sep */
			ch++;
		else {
			/* reset; name too long for UStar */
			Debug( LDAP_DEBUG_ANY, "homedir: "
					"tar_set_header: name too long: \"%s\"\n",
					name );
			ch = name + name_len - 100;
		}
		strncpy( tar->name, ch + 1, 100 );
		{
			int prefix_len = ( ch - 1 ) - name;
			if ( prefix_len > 155 ) prefix_len = 155;
			strncpy( tar->prefix, name, prefix_len );
		}
	}

	snprintf( tar->mode, 8, "%06lo ", (long)st->st_mode & 07777 );
	snprintf( tar->uid, 8, "%06lo ", (long)st->st_uid );
	snprintf( tar->gid, 8, "%06lo ", (long)st->st_gid );
	snprintf( tar->mtime, 12, "%010lo ", (long)st->st_mtime );
	snprintf( tar->size, 12, "%010lo ", (long)0 );
	switch ( st->st_mode & S_IFMT ) {
		case S_IFREG:
			tar->typeflag[0] = '0';
			snprintf( tar->size, 12, "%010lo ", (long)st->st_size );
			break;
		case S_IFLNK:
			tar->typeflag[0] = '2';
			rc = readlink( name - 1, tar->linkname, 99 );
			if ( rc == -1 ) {
				report_errno( "tar_set_header", "readlink", name );
				goto fail;
			}
			break;
		case S_IFCHR:
			tar->typeflag[0] = '3';
			/* FIXME: this is probably wrong but shouldn't likely be an issue */
			snprintf( tar->devmajor, 8, "%06lo ", (long)st->st_rdev >> 16 );
			snprintf( tar->devminor, 8, "%06lo ", (long)st->st_rdev & 0xffff );
			break;
		case S_IFBLK:
			tar->typeflag[0] = '4';
			/* FIXME: this is probably wrong but shouldn't likely be an issue */
			snprintf( tar->devmajor, 8, "%06lo ", (long)st->st_rdev >> 16 );
			snprintf( tar->devminor, 8, "%06lo ", (long)st->st_rdev & 0xffff );
			break;
		case S_IFDIR:
			tar->typeflag[0] = '5';
			break;
		case S_IFIFO:
			tar->typeflag[0] = '6';
			break;
		default:
			goto fail;
	}
	snprintf( tar->magic, 6, "ustar" );
	tar->version[0] = '0';
	tar->version[1] = '0';

	{
		unsigned char *uch = (unsigned char *)tar;
		unsigned char *uend = uch + 512;
		unsigned long sum = 0;

		memset( &tar->checksum, ' ', sizeof(tar->checksum) );

		for ( ; uch < uend; ++uch )
			sum += *uch;

		/* zero-padded, six octal digits, followed by NUL then space (!) */
		/* Yes, that's terminated exactly reverse of the others. */
		snprintf( tar->checksum, sizeof(tar->checksum) - 1, "%06lo", sum );
	}

	return 0;
fail:
	return 1;
}

static traverse_cb_ret
traverse_tar_pre(
		void *private,
		const char *name,
		const struct stat *st,
		void *ctx )
{
	int rc;
	traverse_cb_ret cbrc;
	tar_private *tp = private;
	ustar_header tar;
	FILE *source = NULL;

	assert( private != NULL );
	assert( name != NULL );
	assert( st != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_tar_pre: %s entering\n", name );

	switch ( st->st_mode & S_IFMT ) {
		case S_IFREG:
			if ( sizeof(st->st_size) > 4 && ( st->st_size >> 33 ) >= 1 ) {
				Debug( LDAP_DEBUG_TRACE, "homedir: "
						"traverse_tar_pre: %s is larger than 8GiB POSIX UStar "
						"file size limit\n",
						name );
				goto fail;
			}
			/* fallthrough */
		case S_IFDIR:
		case S_IFLNK:
		case S_IFIFO:
		case S_IFCHR:
		case S_IFBLK:
			rc = tar_set_header( &tar, st, name );
			if ( rc ) goto fail;
			break;
		default:
			Debug( LDAP_DEBUG_TRACE, "homedir: "
					"traverse_tar_pre: skipping \"%s\" mode %o\n",
					name, st->st_mode );
			goto done;
	}

	rc = fwrite( &tar, 1, 512, tp->file );
	if ( rc != 512 ) {
		Debug( LDAP_DEBUG_TRACE, "homedir: "
				"traverse_tar_pre: write error in tar header\n" );
		goto fail;
	}

	if ( (st->st_mode & S_IFMT) == S_IFREG ) {
		source = fopen( name, "rb" );
		if ( source == NULL ) {
			report_errno( "traverse_tar_pre", "fopen", name );
			goto fail;
		}
		rc = copy_blocks( source, tp->file, name, tp->name );
		if ( rc != 0 ) goto fail;
		fclose( source );
		source = NULL;
	}

	{ /* advance to end of record */
		off_t pos = ftello( tp->file );
		if ( pos == -1 ) {
			report_errno( "traverse_tar_pre", "ftello", tp->name );
			goto fail;
		}
		pos += ( 512 - ( pos % 512 ) ) % 512;
		rc = fseeko( tp->file, pos, SEEK_SET );
		if ( rc != 0 ) {
			report_errno( "traverse_tar_pre", "fseeko", tp->name );
			goto fail;
		}
	}

done:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_tar_pre: %s exit continue\n",
			name );
	cbrc = TRAVERSE_CB_CONTINUE;
	goto out;
fail:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"traverse_tar_pre: %s exit failure\n",
			name );
	cbrc = TRAVERSE_CB_FAIL;

out:
	if ( source != NULL ) fclose( source );
	return cbrc;
}

static int
tar_tree( const char *path, const char *tar_name, void *ctx )
{
	traverse_cb cb;
	tar_private tp;
	int rc;

	assert( path != NULL );
	assert( tar_name != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"tar_tree: %s into %s entering\n", path,
			tar_name );

	cb.pre_func = traverse_tar_pre;
	cb.post_func = NULL;
	cb.pre_private = &tp;
	cb.post_private = NULL;

	tp.name = tar_name;
	tp.file = fopen( tar_name, "wb" );
	if ( tp.file == NULL ) {
		report_errno( "tar_tree", "fopen", tar_name );
		goto fail;
	}

	rc = traverse( path, &cb, ctx );
	if ( rc != 0 ) goto fail;

	{
		off_t pos = ftello( tp.file );
		if ( pos == -1 ) {
			report_errno( "tar_tree", "ftello", tp.name );
			goto fail;
		}
		pos += 1024; /* two zero records */
		pos += ( 10240 - ( pos % 10240 ) ) % 10240;
		rc = ftruncate( fileno( tp.file ), pos );
		if ( rc != 0 ) {
			report_errno( "tar_tree", "ftrunctate", tp.name );
			goto fail;
		}
	}

	rc = fclose( tp.file );
	tp.file = NULL;
	if ( rc != 0 ) {
		report_errno( "tar_tree", "fclose", tp.name );
		goto fail;
	}
	goto out;

fail:
	rc = 1;
out:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"tar_tree: %s exit %d\n", path, rc );
	if ( tp.file != NULL ) fclose( tp.file );
	return rc;
}

static int
homedir_deprovision( const homedir_data *data, const char *path, void *ctx )
{
	int rc = 0;
	char tar_name[1024];

	assert( data != NULL );
	assert( path != NULL );

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_deprovision: %s entering\n",
			path );

	switch ( data->style ) {
		case DEL_IGNORE:
			Debug( LDAP_DEBUG_TRACE, "homedir: "
					"homedir_deprovision: style is ignore\n" );
			break;
		case DEL_ARCHIVE:
			if ( data->archive_path == NULL ) {
				Debug( LDAP_DEBUG_ANY, "homedir: "
						"homedir_deprovision: archive path not set\n" );
				goto fail;
			}
			rc = get_tar_name( path, data->archive_path, tar_name, 1024 );
			if ( rc != 0 ) goto fail;
			rc = tar_tree( path, tar_name, ctx );
			if ( rc != 0 ) {
				Debug( LDAP_DEBUG_ANY, "homedir: "
						"homedir_deprovision: archive failed, not deleting\n" );
				goto fail;
			}
			/* fall-through */
		case DEL_DELETE:
			rc = delete_tree( path, ctx );
			break;
		default:
			abort();
	}

	rc = 0;
	goto out;

fail:
	rc = 1;
out:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_deprovision: %s leaving\n",
			path );

	return rc;
}

/* FIXME: This assumes ASCII; needs fixing for z/OS */
/* FIXME: This should also be in a slapd library function somewhere */
#define MAX_MATCHES ( 10 )
static int
homedir_match(
		const homedir_regexp *r,
		const char *homedir,
		char *result,
		size_t result_size )
{
	int rc;
	int n;
	regmatch_t matches[MAX_MATCHES];
	char *resc, *repc;

	assert( r != NULL );
	assert( homedir != NULL );
	assert( result_size > 1 );

	memset( matches, 0, sizeof(matches) );
	rc = regexec( &r->compiled, homedir, MAX_MATCHES, matches, 0 );
	if ( rc ) {
		if ( rc != REG_NOMATCH ) {
			char msg[256];
			regerror( rc, &r->compiled, msg, sizeof(msg) );
			Debug( LDAP_DEBUG_ANY, "homedir_match: "
					"%s\n", msg );
		}
		return rc;
	}

	for ( resc = result, repc = r->replace;
			result_size > 1 && *repc != '\0';
			++repc, ++resc, --result_size ) {
		switch ( *repc ) {
			case '$':
				++repc;
				n = ( *repc ) - '0';
				if ( n < 0 || n > ( MAX_MATCHES - 1 ) ||
						matches[n].rm_so < 0 ) {
					Debug( LDAP_DEBUG_ANY, "homedir: "
							"invalid regex term expansion in \"%s\" "
							"at char %ld, n is %d\n",
							r->replace, (long)( repc - r->replace ), n );
					return 1;
				}
				{
					size_t match_len = matches[n].rm_eo - matches[n].rm_so;
					const char *match_start = homedir + matches[n].rm_so;
					if ( match_len >= result_size ) goto too_long;

					memcpy( resc, match_start, match_len );
					result_size -= match_len;
					resc += match_len - 1;
				}
				break;

			case '\\':
				++repc;
				/* fallthrough */

			default:
				*resc = *repc;
		}
	}
	*resc = '\0';
	if ( *repc != '\0' ) goto too_long;

	return 0;

too_long:
	Debug( LDAP_DEBUG_ANY, "homedir: "
			"regex expansion of %s too long\n",
			r->replace );
	*result = '\0';
	return 1;
}

/* Sift through an entry for interesting values
 * return 0 on success and set vars
 * return 1 if homedir is not present or not valid
 * sets presence if any homedir attributes are noticed
 */
static int
harvest_values(
		const homedir_data *data,
		const Entry *e,
		char *home_buf,
		int home_buf_size,
		uid_t *uidn,
		gid_t *gidn,
		int *presence )
{
	Attribute *a;
	char *homedir = NULL;

	assert( data != NULL );
	assert( e != NULL );
	assert( home_buf != NULL );
	assert( home_buf_size > 1 );
	assert( uidn != NULL );
	assert( gidn != NULL );
	assert( presence != NULL );

	*presence = 0;
	if ( e == NULL ) return 1;
	*uidn = 0;
	*gidn = 0;

	for ( a = e->e_attrs; a->a_next != NULL; a = a->a_next ) {
		if ( a->a_desc == data->home_ad ) {
			homedir = a->a_vals[0].bv_val;
			*presence = 1;
		} else if ( a->a_desc == data->uidn_ad ) {
			*uidn = (uid_t)strtol( a->a_vals[0].bv_val, NULL, 10 );
			*presence = 1;
		} else if ( a->a_desc == data->gidn_ad ) {
			*gidn = (gid_t)strtol( a->a_vals[0].bv_val, NULL, 10 );
			*presence = 1;
		}
	}
	if ( homedir != NULL ) {
		homedir_regexp *r;

		for ( r = data->regexps; r != NULL; r = r->next ) {
			int rc = homedir_match( r, homedir, home_buf, home_buf_size );
			if ( rc == 0 ) return 0;
		}
	}

	return 1;
}

static int
homedir_mod_cleanup( Operation *op, SlapReply *rs )
{
	slap_callback *cb = NULL;
	slap_callback **cbp = NULL;
	homedir_cb_data *cb_data = NULL;
	Entry *e = NULL;

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_mod_cleanup: entering\n" );

	for ( cbp = &op->o_callback;
			*cbp != NULL && (*cbp)->sc_cleanup != homedir_mod_cleanup;
			cbp = &(*cbp)->sc_next )
		;

	if ( *cbp == NULL ) goto out;
	cb = *cbp;

	cb_data = (homedir_cb_data *)cb->sc_private;
	e = cb_data->entry;

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_mod_cleanup: found <%s>\n",
			e->e_nname.bv_val );
	entry_free( e );
	op->o_tmpfree( cb_data, op->o_tmpmemctx );
	*cbp = cb->sc_next;
	op->o_tmpfree( cb, op->o_tmpmemctx );

out:

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_mod_cleanup: leaving\n" );
	return SLAP_CB_CONTINUE;
}

static int
homedir_mod_response( Operation *op, SlapReply *rs )
{
	slap_overinst *on = NULL;
	homedir_data *data = NULL;
	slap_callback *cb = NULL;
	homedir_cb_data *cb_data = NULL;
	Entry *e = NULL;
	int rc = SLAP_CB_CONTINUE;

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_mod_response: entering\n" );

	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "homedir: "
				"homedir_mod_response: op was not successful\n" );
		goto out;
	}

	/* Retrieve stashed entry */
	for ( cb = op->o_callback;
			cb != NULL && cb->sc_cleanup != homedir_mod_cleanup;
			cb = cb->sc_next )
		;
	if ( cb == NULL ) goto out;
	cb_data = (homedir_cb_data *)cb->sc_private;
	e = cb_data->entry;
	on = cb_data->on;
	data = on->on_bi.bi_private;
	assert( e != NULL );
	assert( data != NULL );
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_mod_response: found <%s>\n",
			e->e_nname.bv_val );

	switch ( op->o_tag ) {
		case LDAP_REQ_DELETE: {
			char home_buf[1024];
			uid_t uidn = 0;
			gid_t gidn = 0;
			int presence;

			Debug( LDAP_DEBUG_TRACE, "homedir: "
					"homedir_mod_response: successful delete found\n" );
			rc = harvest_values( data, e, home_buf, sizeof(home_buf), &uidn,
					&gidn, &presence );
			if ( rc == 0 && uidn >= data->min_uid ) {
				homedir_deprovision( data, home_buf, op->o_tmpmemctx );
			} else {
				Debug( LDAP_DEBUG_TRACE, "homedir: "
						"homedir_mod_response: skipping\n" );
			}
			rc = SLAP_CB_CONTINUE;
			break;
		}

		case LDAP_REQ_MODIFY:
		case LDAP_REQ_MODRDN: {
			Operation nop = *op;
			Entry *old_entry = e;
			Entry *new_entry = NULL;
			Entry *etmp;
			char old_home[1024];
			char new_home[1024];
			uid_t old_uidn, new_uidn;
			uid_t old_gidn, new_gidn;
			int old_valid = 0;
			int new_valid = 0;
			int old_presence, new_presence;

			Debug( LDAP_DEBUG_TRACE, "homedir: "
					"homedir_mod_response: successful modify/modrdn found\n" );

			/* retrieve the revised entry */
			nop.o_bd = on->on_info->oi_origdb;
			rc = overlay_entry_get_ov(
					&nop, &op->o_req_ndn, NULL, NULL, 0, &etmp, on );
			if ( etmp != NULL ) {
				new_entry = entry_dup( etmp );
				overlay_entry_release_ov( &nop, etmp, 0, on );
			}
			if ( rc || new_entry == NULL ) {
				Debug( LDAP_DEBUG_ANY, "homedir: "
						"homedir_mod_response: unable to get revised <%s>\n",
						op->o_req_ndn.bv_val );
				if ( new_entry != NULL ) {
					entry_free( new_entry );
					new_entry = NULL;
				}
			}

			/* analyze old and new */
			rc = harvest_values( data, old_entry, old_home, 1024, &old_uidn,
					&old_gidn, &old_presence );
			if ( rc == 0 && old_uidn >= data->min_uid ) old_valid = 1;
			if ( new_entry != NULL ) {
				rc = harvest_values( data, new_entry, new_home, 1024, &new_uidn,
						&new_gidn, &new_presence );
				if ( rc == 0 && new_uidn >= data->min_uid ) new_valid = 1;
				entry_free( new_entry );
				new_entry = NULL;
			}

			if ( new_valid && !old_valid ) { /* like an add */
				if ( old_presence )
					Debug( LDAP_DEBUG_TRACE, "homedir: "
							"homedir_mod_response: old entry is now valid\n" );
				Debug( LDAP_DEBUG_TRACE, "homedir: "
						"homedir_mod_response: treating like an add\n" );
				homedir_provision( new_home, data->skeleton_path, new_uidn,
						new_gidn, op->o_tmpmemctx );

			} else if ( old_valid && !new_valid &&
					!new_presence ) { /* like a del */
				Debug( LDAP_DEBUG_TRACE, "homedir: "
						"homedir_mod_response: treating like a del\n" );
				homedir_deprovision( data, old_home, op->o_tmpmemctx );

			} else if ( new_valid && old_valid ) { /* change */
				int did_something = 0;

				if ( strcmp( old_home, new_home ) != 0 ) {
					Debug( LDAP_DEBUG_TRACE, "homedir: "
							"homedir_mod_response: treating like a rename\n" );
					homedir_rename( old_home, new_home );
					did_something = 1;
				}
				if ( old_uidn != new_uidn || old_gidn != new_gidn ) {
					Debug( LDAP_DEBUG_ANY, "homedir: "
							"homedir_mod_response: rechowning\n" );
					chown_tree( new_home, old_uidn, new_uidn, old_gidn,
							new_gidn, op->o_tmpmemctx );
					did_something = 1;
				}
				if ( !did_something ) {
					Debug( LDAP_DEBUG_TRACE, "homedir: "
							"homedir_mod_response: nothing to do\n" );
				}
			} else if ( old_presence || new_presence ) {
				Debug( LDAP_DEBUG_ANY, "homedir: "
						"homedir_mod_response: <%s> values present "
						"but invalid; ignoring\n",
						op->o_req_ndn.bv_val );
			}
			rc = SLAP_CB_CONTINUE;
			break;
		}

		default:
			rc = SLAP_CB_CONTINUE;
	}

out:
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_mod_response: leaving\n" );
	return rc;
}

static int
homedir_op_mod( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	slap_callback *cb = NULL;
	homedir_cb_data *cb_data = NULL;
	Entry *e = NULL;
	Entry *se = NULL;
	Operation nop = *op;
	int rc;

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_op_mod: entering\n" );

	/* retrieve the entry */
	nop.o_bd = on->on_info->oi_origdb;
	rc = overlay_entry_get_ov( &nop, &op->o_req_ndn, NULL, NULL, 0, &e, on );
	if ( e != NULL ) {
		se = entry_dup( e );
		overlay_entry_release_ov( &nop, e, 0, on );
		e = se;
	}
	if ( rc || e == NULL ) {
		Debug( LDAP_DEBUG_ANY, "homedir: "
				"homedir_op_mod: unable to get <%s>\n",
				op->o_req_ndn.bv_val );
		goto out;
	}

	/* Allocate the callback to hold the entry */
	cb = op->o_tmpalloc( sizeof(slap_callback), op->o_tmpmemctx );
	cb_data = op->o_tmpalloc( sizeof(homedir_cb_data), op->o_tmpmemctx );
	cb->sc_cleanup = homedir_mod_cleanup;
	cb->sc_response = homedir_mod_response;
	cb->sc_private = cb_data;
	cb_data->entry = e;
	e = NULL;
	cb_data->on = on;
	cb->sc_next = op->o_callback;
	op->o_callback = cb;

out:
	if ( e != NULL ) entry_free( e );
	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_op_mod: leaving\n" );
	return SLAP_CB_CONTINUE;
}

static int
homedir_response( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	homedir_data *data = on->on_bi.bi_private;

	Debug( LDAP_DEBUG_TRACE, "homedir: "
			"homedir_response: entering\n" );
	if ( rs->sr_err != LDAP_SUCCESS || data == NULL ) return SLAP_CB_CONTINUE;

	switch ( op->o_tag ) {
		case LDAP_REQ_ADD: { /* Check for new homedir */
			char home_buf[1024];
			uid_t uidn = 0;
			gid_t gidn = 0;
			int rc, presence;

			rc = harvest_values( data, op->ora_e, home_buf, sizeof(home_buf),
					&uidn, &gidn, &presence );
			if ( rc == 0 && uidn >= data->min_uid ) {
				homedir_provision( home_buf, data->skeleton_path, uidn, gidn,
						op->o_tmpmemctx );
			}
			return SLAP_CB_CONTINUE;
		}

		default:
			return SLAP_CB_CONTINUE;
	}

	return SLAP_CB_CONTINUE;
}

static int
homedir_db_init( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	homedir_data *data = ch_calloc( 1, sizeof(homedir_data) );
	const char *text;

	if ( slap_str2ad( "homeDirectory", &data->home_ad, &text ) ||
			slap_str2ad( "uidNumber", &data->uidn_ad, &text ) ||
			slap_str2ad( "gidNumber", &data->gidn_ad, &text ) ) {
		Debug( LDAP_DEBUG_ANY, "homedir: "
				"nis schema not available\n" );
		return 1;
	}

	data->skeleton_path = strdup( DEFAULT_SKEL );
	data->min_uid = DEFAULT_MIN_UID;
	data->archive_path = NULL;

	on->on_bi.bi_private = data;
	return 0;
}

static int
homedir_db_destroy( BackendDB *be, ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	homedir_data *data = on->on_bi.bi_private;
	homedir_regexp *r, *rnext;

	if ( data != NULL ) {
		for ( r = data->regexps; r != NULL; r = rnext ) {
			rnext = r->next;
			ch_free( r->match );
			ch_free( r->replace );
			regfree( &r->compiled );
			ch_free( r );
		}
		data->regexps = NULL;
		if ( data->skeleton_path != NULL ) ch_free( data->skeleton_path );
		if ( data->archive_path != NULL ) ch_free( data->archive_path );
		ch_free( data );
	}

	return 0;
}

int
homedir_initialize()
{
	int rc;

	assert( ' ' == 32 ); /* Lots of ASCII requirements for now */

	memset( &homedir, 0, sizeof(homedir) );

	homedir.on_bi.bi_type = "homedir";
	homedir.on_bi.bi_db_init = homedir_db_init;
	homedir.on_bi.bi_db_destroy = homedir_db_destroy;
	homedir.on_bi.bi_op_delete = homedir_op_mod;
	homedir.on_bi.bi_op_modify = homedir_op_mod;
	homedir.on_response = homedir_response;

	homedir.on_bi.bi_cf_ocs = homedirocs;
	rc = config_register_schema( homedircfg, homedirocs );
	if ( rc ) return rc;

	ldap_pvt_thread_mutex_init( &readdir_mutex );

	return overlay_register( &homedir );
}

int
homedir_terminate()
{
	ldap_pvt_thread_mutex_destroy( &readdir_mutex );
	return 0;
}

#if SLAPD_OVER_HOMEDIR == SLAPD_MOD_DYNAMIC && defined(PIC)
int
init_module( int argc, char *argv[] )
{
	return homedir_initialize();
}

int
term_module()
{
	return homedir_terminate();
}
#endif

#endif /* SLAPD_OVER_HOMEDIR */
