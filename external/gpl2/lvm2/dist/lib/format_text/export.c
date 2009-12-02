/*	$NetBSD: export.c,v 1.1.1.3 2009/12/02 00:26:29 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2009 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "lib.h"
#include "import-export.h"
#include "metadata.h"
#include "display.h"
#include "lvm-string.h"
#include "segtype.h"
#include "text_export.h"
#include "lvm-version.h"

#include <stdarg.h>
#include <time.h>
#include <sys/utsname.h>

struct formatter;
typedef int (*out_with_comment_fn) (struct formatter * f, const char *comment,
				    const char *fmt, va_list ap);
typedef int (*nl_fn) (struct formatter * f);

/*
 * Macro for formatted output.
 * out_with_comment_fn returns -1 if data didn't fit and buffer was expanded.
 * Then argument list is reset and out_with_comment_fn is called again.
 */
#define _out_with_comment(f, buffer, fmt, ap) \
	do { \
		va_start(ap, fmt); \
		r = f->out_with_comment(f, buffer, fmt, ap); \
		va_end(ap); \
	} while (r == -1)

/*
 * The first half of this file deals with
 * exporting the vg, ie. writing it to a file.
 */
struct formatter {
	struct dm_pool *mem;	/* pv names allocated from here */
	struct dm_hash_table *pv_names;	/* dev_name -> pv_name (eg, pv1) */

	union {
		FILE *fp;	/* where we're writing to */
		struct {
			char *start;
			uint32_t size;
			uint32_t used;
		} buf;
	} data;

	out_with_comment_fn out_with_comment;
	nl_fn nl;

	int indent;		/* current level of indentation */
	int error;
	int header;		/* 1 => comments at start; 0 => end */
};

static struct utsname _utsname;

static void _init(void)
{
	static int _initialised = 0;

	if (_initialised)
		return;

	if (uname(&_utsname)) {
		log_error("uname failed: %s", strerror(errno));
		memset(&_utsname, 0, sizeof(_utsname));
	}

	_initialised = 1;
}

/*
 * Formatting functions.
 */

#define MAX_INDENT 5
static void _inc_indent(struct formatter *f)
{
	if (++f->indent > MAX_INDENT)
		f->indent = MAX_INDENT;
}

static void _dec_indent(struct formatter *f)
{
	if (!f->indent--) {
		log_error("Internal error tracking indentation");
		f->indent = 0;
	}
}

/*
 * Newline function for prettier layout.
 */
static int _nl_file(struct formatter *f)
{
	fprintf(f->data.fp, "\n");

	return 1;
}

static int _extend_buffer(struct formatter *f)
{
	char *newbuf;

	log_debug("Doubling metadata output buffer to %" PRIu32,
		  f->data.buf.size * 2);
	if (!(newbuf = dm_realloc(f->data.buf.start,
				   f->data.buf.size * 2))) {
		log_error("Buffer reallocation failed.");
		return 0;
	}
	f->data.buf.start = newbuf;
	f->data.buf.size *= 2;

	return 1;
}

static int _nl_raw(struct formatter *f)
{
	/* If metadata doesn't fit, extend buffer */
	if ((f->data.buf.used + 2 > f->data.buf.size) &&
	    (!_extend_buffer(f)))
		return_0;

	*(f->data.buf.start + f->data.buf.used) = '\n';
	f->data.buf.used += 1;

	*(f->data.buf.start + f->data.buf.used) = '\0';

	return 1;
}

#define COMMENT_TAB 6
static int _out_with_comment_file(struct formatter *f, const char *comment,
				  const char *fmt, va_list ap)
{
	int i;
	char white_space[MAX_INDENT + 1];

	if (ferror(f->data.fp))
		return 0;

	for (i = 0; i < f->indent; i++)
		white_space[i] = '\t';
	white_space[i] = '\0';
	fputs(white_space, f->data.fp);
	i = vfprintf(f->data.fp, fmt, ap);

	if (comment) {
		/*
		 * line comments up if possible.
		 */
		i += 8 * f->indent;
		i /= 8;
		i++;

		do
			fputc('\t', f->data.fp);

		while (++i < COMMENT_TAB);

		fputs(comment, f->data.fp);
	}
	fputc('\n', f->data.fp);

	return 1;
}

static int _out_with_comment_raw(struct formatter *f,
				 const char *comment __attribute((unused)),
				 const char *fmt, va_list ap)
{
	int n;

	n = vsnprintf(f->data.buf.start + f->data.buf.used,
		      f->data.buf.size - f->data.buf.used, fmt, ap);

	/* If metadata doesn't fit, extend buffer */
	if (n < 0 || (n + f->data.buf.used + 2 > f->data.buf.size)) {
		if (!_extend_buffer(f))
			return_0;
		return -1; /* Retry */
	}

	f->data.buf.used += n;

	outnl(f);

	return 1;
}

/*
 * Formats a string, converting a size specified
 * in 512-byte sectors to a more human readable
 * form (eg, megabytes).  We may want to lift this
 * for other code to use.
 */
static int _sectors_to_units(uint64_t sectors, char *buffer, size_t s)
{
	static const char *_units[] = {
		"Kilobytes",
		"Megabytes",
		"Gigabytes",
		"Terabytes",
		"Petabytes",
		"Exabytes",
		NULL
	};

	int i;
	double d = (double) sectors;

	/* to convert to K */
	d /= 2.0;

	for (i = 0; (d > 1024.0) && _units[i]; i++)
		d /= 1024.0;

	return dm_snprintf(buffer, s, "# %g %s", d, _units[i]) > 0;
}

/* increment indention level */
void out_inc_indent(struct formatter *f)
{
	_inc_indent(f);
}

/* decrement indention level */
void out_dec_indent(struct formatter *f)
{
	_dec_indent(f);
}

/* insert new line */
int out_newline(struct formatter *f)
{
	return f->nl(f);
}

/*
 * Appends a comment giving a size in more easily
 * readable form (eg, 4M instead of 8096).
 */
int out_size(struct formatter *f, uint64_t size, const char *fmt, ...)
{
	char buffer[64];
	va_list ap;
	int r;

	if (!_sectors_to_units(size, buffer, sizeof(buffer)))
		return 0;

	_out_with_comment(f, buffer, fmt, ap);

	return r;
}

/*
 * Appends a comment indicating that the line is
 * only a hint.
 */
int out_hint(struct formatter *f, const char *fmt, ...)
{
	va_list ap;
	int r;

	_out_with_comment(f, "# Hint only", fmt, ap);

	return r;
}

/*
 * Appends a comment
 */
static int _out_comment(struct formatter *f, const char *comment, const char *fmt, ...)
{
	va_list ap;
	int r;

	_out_with_comment(f, comment, fmt, ap);

	return r;
}

/*
 * The normal output function.
 */
int out_text(struct formatter *f, const char *fmt, ...)
{
	va_list ap;
	int r;

	_out_with_comment(f, NULL, fmt, ap);

	return r;
}

static int _out_line(const char *line, void *_f) {
	struct formatter *f = (struct formatter *) _f;
	return out_text(f, "%s", line);
}

int out_config_node(struct formatter *f, const struct config_node *cn)
{
	return write_config_node(cn, _out_line, f);
}

static int _print_header(struct formatter *f,
			 const char *desc)
{
	char *buf;
	time_t t;

	t = time(NULL);

	outf(f, "# Generated by LVM2 version %s: %s", LVM_VERSION, ctime(&t));
	outf(f, CONTENTS_FIELD " = \"" CONTENTS_VALUE "\"");
	outf(f, FORMAT_VERSION_FIELD " = %d", FORMAT_VERSION_VALUE);
	outnl(f);

	if (!(buf = alloca(escaped_len(desc)))) {
		log_error("temporary stack allocation for description"
			  "string failed");
		return 0;
	}
	outf(f, "description = \"%s\"", escape_double_quotes(buf, desc));
	outnl(f);
	outf(f, "creation_host = \"%s\"\t# %s %s %s %s %s", _utsname.nodename,
	     _utsname.sysname, _utsname.nodename, _utsname.release,
	     _utsname.version, _utsname.machine);
	outf(f, "creation_time = %lu\t# %s", t, ctime(&t));

	return 1;
}

static int _print_flag_config(struct formatter *f, int status, int type)
{
	char buffer[4096];
	if (!print_flags(status, type | STATUS_FLAG, buffer, sizeof(buffer)))
		return_0;
	outf(f, "status = %s", buffer);

	if (!print_flags(status, type, buffer, sizeof(buffer)))
		return_0;
	outf(f, "flags = %s", buffer);

	return 1;
}

static int _print_vg(struct formatter *f, struct volume_group *vg)
{
	char buffer[4096];

	if (!id_write_format(&vg->id, buffer, sizeof(buffer)))
		return_0;

	outf(f, "id = \"%s\"", buffer);

	outf(f, "seqno = %u", vg->seqno);

	if (!_print_flag_config(f, vg->status, VG_FLAGS))
		return_0;

	if (!dm_list_empty(&vg->tags)) {
		if (!print_tags(&vg->tags, buffer, sizeof(buffer)))
			return_0;
		outf(f, "tags = %s", buffer);
	}

	if (vg->system_id && *vg->system_id)
		outf(f, "system_id = \"%s\"", vg->system_id);

	if (!out_size(f, (uint64_t) vg->extent_size, "extent_size = %u",
		      vg->extent_size))
		return_0;
	outf(f, "max_lv = %u", vg->max_lv);
	outf(f, "max_pv = %u", vg->max_pv);

	/* Default policy is NORMAL; INHERIT is meaningless */
	if (vg->alloc != ALLOC_NORMAL && vg->alloc != ALLOC_INHERIT) {
		outnl(f);
		outf(f, "allocation_policy = \"%s\"",
		     get_alloc_string(vg->alloc));
	}

	return 1;
}

/*
 * Get the pv%d name from the formatters hash
 * table.
 */
static const char *_get_pv_name_from_uuid(struct formatter *f, char *uuid)
{
	return dm_hash_lookup(f->pv_names, uuid);
}

static const char *_get_pv_name(struct formatter *f, struct physical_volume *pv)
{
	char uuid[64] __attribute((aligned(8)));

	if (!pv || !id_write_format(&pv->id, uuid, sizeof(uuid)))
		return_NULL;

	return _get_pv_name_from_uuid(f, uuid);
}

static int _print_pvs(struct formatter *f, struct volume_group *vg)
{
	struct pv_list *pvl;
	struct physical_volume *pv;
	char buffer[4096];
	char *buf;
	const char *name;

	outf(f, "physical_volumes {");
	_inc_indent(f);

	dm_list_iterate_items(pvl, &vg->pvs) {
		pv = pvl->pv;

		if (!id_write_format(&pv->id, buffer, sizeof(buffer)))
			return_0;

		if (!(name = _get_pv_name_from_uuid(f, buffer)))
			return_0;

		outnl(f);
		outf(f, "%s {", name);
		_inc_indent(f);

		outf(f, "id = \"%s\"", buffer);

		if (!(buf = alloca(escaped_len(pv_dev_name(pv))))) {
			log_error("temporary stack allocation for device name"
				  "string failed");
			return 0;
		}

		if (!out_hint(f, "device = \"%s\"",
			      escape_double_quotes(buf, pv_dev_name(pv))))
			return_0;
		outnl(f);

		if (!_print_flag_config(f, pv->status, PV_FLAGS))
			return_0;

		if (!dm_list_empty(&pv->tags)) {
			if (!print_tags(&pv->tags, buffer, sizeof(buffer)))
				return_0;
			outf(f, "tags = %s", buffer);
		}

		if (!out_size(f, pv->size, "dev_size = %" PRIu64, pv->size))
			return_0;

		outf(f, "pe_start = %" PRIu64, pv->pe_start);
		if (!out_size(f, vg->extent_size * (uint64_t) pv->pe_count,
			      "pe_count = %u", pv->pe_count))
			return_0;

		_dec_indent(f);
		outf(f, "}");
	}

	_dec_indent(f);
	outf(f, "}");
	return 1;
}

static int _print_segment(struct formatter *f, struct volume_group *vg,
			  int count, struct lv_segment *seg)
{
	char buffer[4096];

	outf(f, "segment%u {", count);
	_inc_indent(f);

	outf(f, "start_extent = %u", seg->le);
	if (!out_size(f, (uint64_t) seg->len * vg->extent_size,
		      "extent_count = %u", seg->len))
		return_0;

	outnl(f);
	outf(f, "type = \"%s\"", seg->segtype->name);

	if (!dm_list_empty(&seg->tags)) {
		if (!print_tags(&seg->tags, buffer, sizeof(buffer)))
			return_0;
		outf(f, "tags = %s", buffer);
	}

	if (seg->segtype->ops->text_export &&
	    !seg->segtype->ops->text_export(seg, f))
		return_0;

	_dec_indent(f);
	outf(f, "}");

	return 1;
}

int out_areas(struct formatter *f, const struct lv_segment *seg,
	      const char *type)
{
	const char *name;
	unsigned int s;

	outnl(f);

	outf(f, "%ss = [", type);
	_inc_indent(f);

	for (s = 0; s < seg->area_count; s++) {
		switch (seg_type(seg, s)) {
		case AREA_PV:
			if (!(name = _get_pv_name(f, seg_pv(seg, s))))
				return_0;

			outf(f, "\"%s\", %u%s", name,
			     seg_pe(seg, s),
			     (s == seg->area_count - 1) ? "" : ",");
			break;
		case AREA_LV:
			outf(f, "\"%s\", %u%s",
			     seg_lv(seg, s)->name,
			     seg_le(seg, s),
			     (s == seg->area_count - 1) ? "" : ",");
			break;
		case AREA_UNASSIGNED:
			return 0;
		}
	}

	_dec_indent(f);
	outf(f, "]");
	return 1;
}

static int _print_lv(struct formatter *f, struct logical_volume *lv)
{
	struct lv_segment *seg;
	char buffer[4096];
	int seg_count;

	outnl(f);
	outf(f, "%s {", lv->name);
	_inc_indent(f);

	/* FIXME: Write full lvid */
	if (!id_write_format(&lv->lvid.id[1], buffer, sizeof(buffer)))
		return_0;

	outf(f, "id = \"%s\"", buffer);

	if (!_print_flag_config(f, lv->status, LV_FLAGS))
		return_0;

	if (!dm_list_empty(&lv->tags)) {
		if (!print_tags(&lv->tags, buffer, sizeof(buffer)))
			return_0;
		outf(f, "tags = %s", buffer);
	}

	if (lv->alloc != ALLOC_INHERIT)
		outf(f, "allocation_policy = \"%s\"",
		     get_alloc_string(lv->alloc));

	switch (lv->read_ahead) {
	case DM_READ_AHEAD_NONE:
		_out_comment(f, "# None", "read_ahead = -1");
		break;
	case DM_READ_AHEAD_AUTO:
		/* No output - use default */
		break;
	default:
		outf(f, "read_ahead = %u", lv->read_ahead);
	}

	if (lv->major >= 0)
		outf(f, "major = %d", lv->major);
	if (lv->minor >= 0)
		outf(f, "minor = %d", lv->minor);
	outf(f, "segment_count = %u", dm_list_size(&lv->segments));
	outnl(f);

	seg_count = 1;
	dm_list_iterate_items(seg, &lv->segments) {
		if (!_print_segment(f, lv->vg, seg_count++, seg))
			return_0;
	}

	_dec_indent(f);
	outf(f, "}");

	return 1;
}

static int _print_lvs(struct formatter *f, struct volume_group *vg)
{
	struct lv_list *lvl;

	/*
	 * Don't bother with an lv section if there are no lvs.
	 */
	if (dm_list_empty(&vg->lvs))
		return 1;

	outf(f, "logical_volumes {");
	_inc_indent(f);

	/*
	 * Write visible LVs first
	 */
	dm_list_iterate_items(lvl, &vg->lvs) {
		if (!(lv_is_visible(lvl->lv)))
			continue;
		if (!_print_lv(f, lvl->lv))
			return_0;
	}

	dm_list_iterate_items(lvl, &vg->lvs) {
		if ((lv_is_visible(lvl->lv)))
			continue;
		if (!_print_lv(f, lvl->lv))
			return_0;
	}

	_dec_indent(f);
	outf(f, "}");

	return 1;
}

/*
 * In the text format we refer to pv's as 'pv1',
 * 'pv2' etc.  This function builds a hash table
 * to enable a quick lookup from device -> name.
 */
static int _build_pv_names(struct formatter *f, struct volume_group *vg)
{
	int count = 0;
	struct pv_list *pvl;
	struct physical_volume *pv;
	char buffer[32], *uuid, *name;

	if (!(f->mem = dm_pool_create("text pv_names", 512)))
		return_0;

	if (!(f->pv_names = dm_hash_create(128)))
		return_0;

	dm_list_iterate_items(pvl, &vg->pvs) {
		pv = pvl->pv;

		/* FIXME But skip if there's already an LV called pv%d ! */
		if (dm_snprintf(buffer, sizeof(buffer), "pv%d", count++) < 0)
			return_0;

		if (!(name = dm_pool_strdup(f->mem, buffer)))
			return_0;

		if (!(uuid = dm_pool_zalloc(f->mem, 64)) ||
		   !id_write_format(&pv->id, uuid, 64))
			return_0;

		if (!dm_hash_insert(f->pv_names, uuid, name))
			return_0;
	}

	return 1;
}

static int _text_vg_export(struct formatter *f,
			   struct volume_group *vg, const char *desc)
{
	int r = 0;

	if (!_build_pv_names(f, vg))
		goto_out;

	if (f->header && !_print_header(f, desc))
		goto_out;

	if (!out_text(f, "%s {", vg->name))
		goto_out;

	_inc_indent(f);

	if (!_print_vg(f, vg))
		goto_out;

	outnl(f);
	if (!_print_pvs(f, vg))
		goto_out;

	outnl(f);
	if (!_print_lvs(f, vg))
		goto_out;

	_dec_indent(f);
	if (!out_text(f, "}"))
		goto_out;

	if (!f->header && !_print_header(f, desc))
		goto_out;

	r = 1;

      out:
	if (f->mem)
		dm_pool_destroy(f->mem);

	if (f->pv_names)
		dm_hash_destroy(f->pv_names);

	return r;
}

int text_vg_export_file(struct volume_group *vg, const char *desc, FILE *fp)
{
	struct formatter *f;
	int r;

	_init();

	if (!(f = dm_malloc(sizeof(*f))))
		return_0;

	memset(f, 0, sizeof(*f));
	f->data.fp = fp;
	f->indent = 0;
	f->header = 1;
	f->out_with_comment = &_out_with_comment_file;
	f->nl = &_nl_file;

	r = _text_vg_export(f, vg, desc);
	if (r)
		r = !ferror(f->data.fp);
	dm_free(f);
	return r;
}

/* Returns amount of buffer used incl. terminating NUL */
int text_vg_export_raw(struct volume_group *vg, const char *desc, char **buf)
{
	struct formatter *f;
	int r = 0;

	_init();

	if (!(f = dm_malloc(sizeof(*f))))
		return_0;

	memset(f, 0, sizeof(*f));

	f->data.buf.size = 65536;	/* Initial metadata limit */
	if (!(f->data.buf.start = dm_malloc(f->data.buf.size))) {
		log_error("text_export buffer allocation failed");
		goto out;
	}

	f->indent = 0;
	f->header = 0;
	f->out_with_comment = &_out_with_comment_raw;
	f->nl = &_nl_raw;

	if (!_text_vg_export(f, vg, desc)) {
		dm_free(f->data.buf.start);
		goto_out;
	}

	r = f->data.buf.used + 1;
	*buf = f->data.buf.start;

      out:
	dm_free(f);
	return r;
}

int export_vg_to_buffer(struct volume_group *vg, char **buf)
{
	return text_vg_export_raw(vg, "", buf);
}

#undef outf
#undef outnl
