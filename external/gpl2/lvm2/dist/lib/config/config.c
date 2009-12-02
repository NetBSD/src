/*	$NetBSD: config.c,v 1.1.1.2 2009/12/02 00:26:28 haad Exp $	*/

/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2007 Red Hat, Inc. All rights reserved.
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
#include "config.h"
#include "crc.h"
#include "device.h"
#include "str_list.h"
#include "toolcontext.h"
#include "lvm-string.h"
#include "lvm-file.h"

#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#define SECTION_B_CHAR '{'
#define SECTION_E_CHAR '}'

enum {
	TOK_INT,
	TOK_FLOAT,
	TOK_STRING,		/* Single quotes */
	TOK_STRING_ESCAPED,	/* Double quotes */
	TOK_EQ,
	TOK_SECTION_B,
	TOK_SECTION_E,
	TOK_ARRAY_B,
	TOK_ARRAY_E,
	TOK_IDENTIFIER,
	TOK_COMMA,
	TOK_EOF
};

struct parser {
	const char *fb, *fe;		/* file limits */

	int t;			/* token limits and type */
	const char *tb, *te;

	int fd;			/* descriptor for file being parsed */
	int line;		/* line number we are on */

	struct dm_pool *mem;
};

struct cs {
	struct config_tree cft;
	struct dm_pool *mem;
	time_t timestamp;
	char *filename;
	int exists;
	int keep_open;
	struct device *dev;
};

struct output_line {
	FILE *fp;
	struct dm_pool *mem;
	putline_fn putline;
	void *putline_baton;
};

static void _get_token(struct parser *p, int tok_prev);
static void _eat_space(struct parser *p);
static struct config_node *_file(struct parser *p);
static struct config_node *_section(struct parser *p);
static struct config_value *_value(struct parser *p);
static struct config_value *_type(struct parser *p);
static int _match_aux(struct parser *p, int t);
static struct config_value *_create_value(struct dm_pool *mem);
static struct config_node *_create_node(struct dm_pool *mem);
static char *_dup_tok(struct parser *p);

static const int sep = '/';

#define MAX_INDENT 32

#define match(t) do {\
   if (!_match_aux(p, (t))) {\
	log_error("Parse error at byte %" PRIptrdiff_t " (line %d): unexpected token", \
		  p->tb - p->fb + 1, p->line); \
      return 0;\
   } \
} while(0);

static int _tok_match(const char *str, const char *b, const char *e)
{
	while (*str && (b != e)) {
		if (*str++ != *b++)
			return 0;
	}

	return !(*str || (b != e));
}

/*
 * public interface
 */
struct config_tree *create_config_tree(const char *filename, int keep_open)
{
	struct cs *c;
	struct dm_pool *mem = dm_pool_create("config", 10 * 1024);

	if (!mem) {
		log_error("Failed to allocate config pool.");
		return 0;
	}

	if (!(c = dm_pool_zalloc(mem, sizeof(*c)))) {
		log_error("Failed to allocate config tree.");
		dm_pool_destroy(mem);
		return 0;
	}

	c->mem = mem;
	c->cft.root = (struct config_node *) NULL;
	c->timestamp = 0;
	c->exists = 0;
	c->keep_open = keep_open;
	c->dev = 0;
	if (filename)
		c->filename = dm_pool_strdup(c->mem, filename);
	return &c->cft;
}

void destroy_config_tree(struct config_tree *cft)
{
	struct cs *c = (struct cs *) cft;

	if (c->dev)
		dev_close(c->dev);

	dm_pool_destroy(c->mem);
}

static int _parse_config_file(struct parser *p, struct config_tree *cft)
{
	p->tb = p->te = p->fb;
	p->line = 1;
	_get_token(p, TOK_SECTION_E);
	if (!(cft->root = _file(p)))
		return_0;

	return 1;
}

struct config_tree *create_config_tree_from_string(struct cmd_context *cmd __attribute((unused)),
						   const char *config_settings)
{
	struct cs *c;
	struct config_tree *cft;
	struct parser *p;

	if (!(cft = create_config_tree(NULL, 0)))
		return_NULL;

	c = (struct cs *) cft;
	if (!(p = dm_pool_alloc(c->mem, sizeof(*p)))) {
		log_error("Failed to allocate config tree parser.");
		destroy_config_tree(cft);
		return NULL;
	}

	p->mem = c->mem;
	p->fb = config_settings;
	p->fe = config_settings + strlen(config_settings);

	if (!_parse_config_file(p, cft)) {
		destroy_config_tree(cft);
		return_NULL;
	}

	return cft;
}

int override_config_tree_from_string(struct cmd_context *cmd,
				     const char *config_settings)
{
	if (!(cmd->cft_override = create_config_tree_from_string(cmd,config_settings))) {
		log_error("Failed to set overridden configuration entries.");
		return 1;
	}

	return 0;
}

int read_config_fd(struct config_tree *cft, struct device *dev,
		   off_t offset, size_t size, off_t offset2, size_t size2,
		   checksum_fn_t checksum_fn, uint32_t checksum)
{
	struct cs *c = (struct cs *) cft;
	struct parser *p;
	int r = 0;
	int use_mmap = 1;
	off_t mmap_offset = 0;
	char *buf = NULL;

	if (!(p = dm_pool_alloc(c->mem, sizeof(*p))))
		return_0;
	p->mem = c->mem;

	/* Only use mmap with regular files */
	if (!(dev->flags & DEV_REGULAR) || size2)
		use_mmap = 0;

	if (use_mmap) {
		mmap_offset = offset % lvm_getpagesize();
		/* memory map the file */
		p->fb = mmap((caddr_t) 0, size + mmap_offset, PROT_READ,
			     MAP_PRIVATE, dev_fd(dev), offset - mmap_offset);
		if (p->fb == (caddr_t) (-1)) {
			log_sys_error("mmap", dev_name(dev));
			goto out;
		}
		p->fb = p->fb + mmap_offset;
	} else {
		if (!(buf = dm_malloc(size + size2)))
			return_0;
		if (!dev_read_circular(dev, (uint64_t) offset, size,
				       (uint64_t) offset2, size2, buf)) {
			goto out;
		}
		p->fb = buf;
	}

	if (checksum_fn && checksum !=
	    (checksum_fn(checksum_fn(INITIAL_CRC, p->fb, size),
			 p->fb + size, size2))) {
		log_error("%s: Checksum error", dev_name(dev));
		goto out;
	}

	p->fe = p->fb + size + size2;

	if (!_parse_config_file(p, cft))
		goto_out;

	r = 1;

      out:
	if (!use_mmap)
		dm_free(buf);
	else {
		/* unmap the file */
		if (munmap((char *) (p->fb - mmap_offset), size + mmap_offset)) {
			log_sys_error("munmap", dev_name(dev));
			r = 0;
		}
	}

	return r;
}

int read_config_file(struct config_tree *cft)
{
	struct cs *c = (struct cs *) cft;
	struct stat info;
	int r = 1;

	if (stat(c->filename, &info)) {
		log_sys_error("stat", c->filename);
		c->exists = 0;
		return 0;
	}

	if (!S_ISREG(info.st_mode)) {
		log_error("%s is not a regular file", c->filename);
		c->exists = 0;
		return 0;
	}

	c->exists = 1;

	if (info.st_size == 0) {
		log_verbose("%s is empty", c->filename);
		return 1;
	}

	if (!c->dev) {
		if (!(c->dev = dev_create_file(c->filename, NULL, NULL, 1)))
			return_0;

		if (!dev_open_flags(c->dev, O_RDONLY, 0, 0))
			return_0;
	}

	r = read_config_fd(cft, c->dev, 0, (size_t) info.st_size, 0, 0,
			   (checksum_fn_t) NULL, 0);

	if (!c->keep_open) {
		dev_close(c->dev);
		c->dev = 0;
	}

	c->timestamp = info.st_ctime;

	return r;
}

time_t config_file_timestamp(struct config_tree *cft)
{
	struct cs *c = (struct cs *) cft;

	return c->timestamp;
}

/*
 * Return 1 if config files ought to be reloaded
 */
int config_file_changed(struct config_tree *cft)
{
	struct cs *c = (struct cs *) cft;
	struct stat info;

	if (!c->filename)
		return 0;

	if (stat(c->filename, &info) == -1) {
		/* Ignore a deleted config file: still use original data */
		if (errno == ENOENT) {
			if (!c->exists)
				return 0;
			log_very_verbose("Config file %s has disappeared!",
					 c->filename);
			goto reload;
		}
		log_sys_error("stat", c->filename);
		log_error("Failed to reload configuration files");
		return 0;
	}

	if (!S_ISREG(info.st_mode)) {
		log_error("Configuration file %s is not a regular file",
			  c->filename);
		goto reload;
	}

	/* Unchanged? */
	if (c->timestamp == info.st_ctime)
		return 0;

      reload:
	log_verbose("Detected config file change to %s", c->filename);
	return 1;
}

static int _line_start(struct output_line *outline)
{
	if (!dm_pool_begin_object(outline->mem, 128)) {
		log_error("dm_pool_begin_object failed for config line");
		return 0;
	}

	return 1;
}

static int _line_append(struct output_line *outline, const char *fmt, ...)
  __attribute__ ((format(printf, 2, 3)));
static int _line_append(struct output_line *outline, const char *fmt, ...)
{
	char buf[4096];
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vsnprintf(&buf[0], sizeof buf - 1, fmt, ap);
	if (n < 0 || n > (int) sizeof buf - 1) {
		log_error("vsnprintf failed for config line");
		return 0;
	}
	va_end(ap);

	if (!dm_pool_grow_object(outline->mem, &buf[0], strlen(buf))) {
		log_error("dm_pool_grow_object failed for config line");
		return 0;
	}

	return 1;
}

#define line_append(args...) do {if (!_line_append(outline, args)) {return_0;}} while (0)

static int _line_end(struct output_line *outline)
{
	const char *line;

	if (!dm_pool_grow_object(outline->mem, "\0", 1)) {
		log_error("dm_pool_grow_object failed for config line");
		return 0;
	}

	line = dm_pool_end_object(outline->mem);
	if (outline->putline)
		outline->putline(line, outline->putline_baton);
	else {
		if (!outline->fp)
			log_print("%s", line);
		else
			fprintf(outline->fp, "%s\n", line);
	}

	return 1;
}

static int _write_value(struct output_line *outline, struct config_value *v)
{
	char *buf;

	switch (v->type) {
	case CFG_STRING:
		if (!(buf = alloca(escaped_len(v->v.str)))) {
			log_error("temporary stack allocation for a config "
				  "string failed");
			return 0;
		}
		line_append("\"%s\"", escape_double_quotes(buf, v->v.str));
		break;

	case CFG_FLOAT:
		line_append("%f", v->v.r);
		break;

	case CFG_INT:
		line_append("%" PRId64, v->v.i);
		break;

	case CFG_EMPTY_ARRAY:
		line_append("[]");
		break;

	default:
		log_error("_write_value: Unknown value type: %d", v->type);

	}

	return 1;
}

static int _write_config(const struct config_node *n, int only_one,
			 struct output_line *outline, int level)
{
	char space[MAX_INDENT + 1];
	int l = (level < MAX_INDENT) ? level : MAX_INDENT;
	int i;

	if (!n)
		return 1;

	for (i = 0; i < l; i++)
		space[i] = '\t';
	space[i] = '\0';

	do {
		if (!_line_start(outline))
			return_0;
		line_append("%s%s", space, n->key);
		if (!n->v) {
			/* it's a sub section */
			line_append(" {");
			if (!_line_end(outline))
				return_0;
			_write_config(n->child, 0, outline, level + 1);
			if (!_line_start(outline))
				return_0;
			line_append("%s}", space);
		} else {
			/* it's a value */
			struct config_value *v = n->v;
			line_append("=");
			if (v->next) {
				line_append("[");
				while (v) {
					if (!_write_value(outline, v))
						return_0;
					v = v->next;
					if (v)
						line_append(", ");
				}
				line_append("]");
			} else
				if (!_write_value(outline, v))
					return_0;
		}
		if (!_line_end(outline))
			return_0;
		n = n->sib;
	} while (n && !only_one);
	/* FIXME: add error checking */
	return 1;
}

int write_config_node(const struct config_node *cn, putline_fn putline, void *baton)
{
	struct output_line outline;
	outline.fp = NULL;
	outline.mem = dm_pool_create("config_line", 1024);
	outline.putline = putline;
	outline.putline_baton = baton;
	if (!_write_config(cn, 0, &outline, 0)) {
		dm_pool_destroy(outline.mem);
		return_0;
	}
	dm_pool_destroy(outline.mem);
	return 1;
}

int write_config_file(struct config_tree *cft, const char *file,
		      int argc, char **argv)
{
	struct config_node *cn;
	int r = 1;
	struct output_line outline;
	outline.fp = NULL;
	outline.putline = NULL;

	if (!file)
		file = "stdout";
	else if (!(outline.fp = fopen(file, "w"))) {
		log_sys_error("open", file);
		return 0;
	}

	outline.mem = dm_pool_create("config_line", 1024);

	log_verbose("Dumping configuration to %s", file);
	if (!argc) {
		if (!_write_config(cft->root, 0, &outline, 0)) {
			log_error("Failure while writing to %s", file);
			r = 0;
		}
	} else while (argc--) {
		if ((cn = find_config_node(cft->root, *argv))) {
			if (!_write_config(cn, 1, &outline, 0)) {
				log_error("Failure while writing to %s", file);
				r = 0;
			}
		} else {
			log_error("Configuration node %s not found", *argv);
			r = 0;
		}
		argv++;
	}

	if (outline.fp && lvm_fclose(outline.fp, file)) {
		stack;
		r = 0;
	}

	dm_pool_destroy(outline.mem);
	return r;
}

/*
 * parser
 */
static struct config_node *_file(struct parser *p)
{
	struct config_node *root = NULL, *n, *l = NULL;
	while (p->t != TOK_EOF) {
		if (!(n = _section(p)))
			return_0;

		if (!root)
			root = n;
		else
			l->sib = n;
		n->parent = root;
		l = n;
	}
	return root;
}

static struct config_node *_section(struct parser *p)
{
	/* IDENTIFIER SECTION_B_CHAR VALUE* SECTION_E_CHAR */
	struct config_node *root, *n, *l = NULL;
	if (!(root = _create_node(p->mem)))
		return_0;

	if (!(root->key = _dup_tok(p)))
		return_0;

	match(TOK_IDENTIFIER);

	if (p->t == TOK_SECTION_B) {
		match(TOK_SECTION_B);
		while (p->t != TOK_SECTION_E) {
			if (!(n = _section(p)))
				return_0;

			if (!root->child)
				root->child = n;
			else
				l->sib = n;
			n->parent = root;
			l = n;
		}
		match(TOK_SECTION_E);
	} else {
		match(TOK_EQ);
		if (!(root->v = _value(p)))
			return_0;
	}

	return root;
}

static struct config_value *_value(struct parser *p)
{
	/* '[' TYPE* ']' | TYPE */
	struct config_value *h = NULL, *l, *ll = NULL;
	if (p->t == TOK_ARRAY_B) {
		match(TOK_ARRAY_B);
		while (p->t != TOK_ARRAY_E) {
			if (!(l = _type(p)))
				return_0;

			if (!h)
				h = l;
			else
				ll->next = l;
			ll = l;

			if (p->t == TOK_COMMA)
				match(TOK_COMMA);
		}
		match(TOK_ARRAY_E);
		/*
		 * Special case for an empty array.
		 */
		if (!h) {
			if (!(h = _create_value(p->mem)))
				return NULL;

			h->type = CFG_EMPTY_ARRAY;
		}

	} else
		h = _type(p);

	return h;
}

static struct config_value *_type(struct parser *p)
{
	/* [+-]{0,1}[0-9]+ | [0-9]*\.[0-9]* | ".*" */
	struct config_value *v = _create_value(p->mem);

	if (!v)
		return NULL;

	switch (p->t) {
	case TOK_INT:
		v->type = CFG_INT;
		v->v.i = strtoll(p->tb, NULL, 0);	/* FIXME: check error */
		match(TOK_INT);
		break;

	case TOK_FLOAT:
		v->type = CFG_FLOAT;
		v->v.r = strtod(p->tb, NULL);	/* FIXME: check error */
		match(TOK_FLOAT);
		break;

	case TOK_STRING:
		v->type = CFG_STRING;

		p->tb++, p->te--;	/* strip "'s */
		if (!(v->v.str = _dup_tok(p)))
			return_0;
		p->te++;
		match(TOK_STRING);
		break;

	case TOK_STRING_ESCAPED:
		v->type = CFG_STRING;

		p->tb++, p->te--;	/* strip "'s */
		if (!(v->v.str = _dup_tok(p)))
			return_0;
		unescape_double_quotes(v->v.str);
		p->te++;
		match(TOK_STRING_ESCAPED);
		break;

	default:
		log_error("Parse error at byte %" PRIptrdiff_t " (line %d): expected a value",
			  p->tb - p->fb + 1, p->line);
		return 0;
	}
	return v;
}

static int _match_aux(struct parser *p, int t)
{
	if (p->t != t)
		return 0;

	_get_token(p, t);
	return 1;
}

/*
 * tokeniser
 */
static void _get_token(struct parser *p, int tok_prev)
{
	int values_allowed = 0;

	p->tb = p->te;
	_eat_space(p);
	if (p->tb == p->fe || !*p->tb) {
		p->t = TOK_EOF;
		return;
	}

	/* Should next token be interpreted as value instead of identifier? */
	if (tok_prev == TOK_EQ || tok_prev == TOK_ARRAY_B ||
	    tok_prev == TOK_COMMA)
		values_allowed = 1;

	p->t = TOK_INT;		/* fudge so the fall through for
				   floats works */
	switch (*p->te) {
	case SECTION_B_CHAR:
		p->t = TOK_SECTION_B;
		p->te++;
		break;

	case SECTION_E_CHAR:
		p->t = TOK_SECTION_E;
		p->te++;
		break;

	case '[':
		p->t = TOK_ARRAY_B;
		p->te++;
		break;

	case ']':
		p->t = TOK_ARRAY_E;
		p->te++;
		break;

	case ',':
		p->t = TOK_COMMA;
		p->te++;
		break;

	case '=':
		p->t = TOK_EQ;
		p->te++;
		break;

	case '"':
		p->t = TOK_STRING_ESCAPED;
		p->te++;
		while ((p->te != p->fe) && (*p->te) && (*p->te != '"')) {
			if ((*p->te == '\\') && (p->te + 1 != p->fe) &&
			    *(p->te + 1))
				p->te++;
			p->te++;
		}

		if ((p->te != p->fe) && (*p->te))
			p->te++;
		break;

	case '\'':
		p->t = TOK_STRING;
		p->te++;
		while ((p->te != p->fe) && (*p->te) && (*p->te != '\''))
			p->te++;

		if ((p->te != p->fe) && (*p->te))
			p->te++;
		break;

	case '.':
		p->t = TOK_FLOAT;
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case '+':
	case '-':
		if (values_allowed) {
			p->te++;
			while ((p->te != p->fe) && (*p->te)) {
				if (*p->te == '.') {
					if (p->t == TOK_FLOAT)
						break;
					p->t = TOK_FLOAT;
				} else if (!isdigit((int) *p->te))
					break;
				p->te++;
			}
			break;
		}

	default:
		p->t = TOK_IDENTIFIER;
		while ((p->te != p->fe) && (*p->te) && !isspace(*p->te) &&
		       (*p->te != '#') && (*p->te != '=') &&
		       (*p->te != SECTION_B_CHAR) &&
		       (*p->te != SECTION_E_CHAR))
			p->te++;
		break;
	}
}

static void _eat_space(struct parser *p)
{
	while ((p->tb != p->fe) && (*p->tb)) {
		if (*p->te == '#')
			while ((p->te != p->fe) && (*p->te) && (*p->te != '\n'))
				p->te++;

		else if (isspace(*p->te)) {
			while ((p->te != p->fe) && (*p->te) && isspace(*p->te)) {
				if (*p->te == '\n')
					p->line++;
				p->te++;
			}
		}

		else
			return;

		p->tb = p->te;
	}
}

/*
 * memory management
 */
static struct config_value *_create_value(struct dm_pool *mem)
{
	struct config_value *v = dm_pool_alloc(mem, sizeof(*v));

	if (v)
		memset(v, 0, sizeof(*v));

	return v;
}

static struct config_node *_create_node(struct dm_pool *mem)
{
	struct config_node *n = dm_pool_alloc(mem, sizeof(*n));

	if (n)
		memset(n, 0, sizeof(*n));

	return n;
}

static char *_dup_tok(struct parser *p)
{
	size_t len = p->te - p->tb;
	char *str = dm_pool_alloc(p->mem, len + 1);
	if (!str)
		return_0;
	strncpy(str, p->tb, len);
	str[len] = '\0';
	return str;
}

/*
 * utility functions
 */
static struct config_node *_find_config_node(const struct config_node *cn,
					     const char *path)
{
	const char *e;
	const struct config_node *cn_found = NULL;

	while (cn) {
		/* trim any leading slashes */
		while (*path && (*path == sep))
			path++;

		/* find the end of this segment */
		for (e = path; *e && (*e != sep); e++) ;

		/* hunt for the node */
		cn_found = NULL;
		while (cn) {
			if (_tok_match(cn->key, path, e)) {
				/* Inefficient */
				if (!cn_found)
					cn_found = cn;
				else
					log_error("WARNING: Ignoring duplicate"
						  " config node: %s ("
						  "seeking %s)", cn->key, path);
			}

			cn = cn->sib;
		}

		if (cn_found && *e)
			cn = cn_found->child;
		else
			break;	/* don't move into the last node */

		path = e;
	}

	return (struct config_node *) cn_found;
}

static struct config_node *_find_first_config_node(const struct config_node *cn1,
						   const struct config_node *cn2,
						   const char *path)
{
	struct config_node *cn;

	if (cn1 && (cn = _find_config_node(cn1, path)))
		return cn;

	if (cn2 && (cn = _find_config_node(cn2, path)))
		return cn;

	return NULL;
}

struct config_node *find_config_node(const struct config_node *cn,
				     const char *path)
{
	return _find_config_node(cn, path);
}

static const char *_find_config_str(const struct config_node *cn1,
				    const struct config_node *cn2,
				    const char *path, const char *fail)
{
	const struct config_node *n = _find_first_config_node(cn1, cn2, path);

	/* Empty strings are ignored */
	if ((n && n->v && n->v->type == CFG_STRING) && (*n->v->v.str)) {
		log_very_verbose("Setting %s to %s", path, n->v->v.str);
		return n->v->v.str;
	}

	if (fail)
		log_very_verbose("%s not found in config: defaulting to %s",
				 path, fail);
	return fail;
}

const char *find_config_str(const struct config_node *cn,
			    const char *path, const char *fail)
{
	return _find_config_str(cn, NULL, path, fail);
}

static int64_t _find_config_int64(const struct config_node *cn1,
				  const struct config_node *cn2,
				  const char *path, int64_t fail)
{
	const struct config_node *n = _find_first_config_node(cn1, cn2, path);

	if (n && n->v && n->v->type == CFG_INT) {
		log_very_verbose("Setting %s to %" PRId64, path, n->v->v.i);
		return n->v->v.i;
	}

	log_very_verbose("%s not found in config: defaulting to %" PRId64,
			 path, fail);
	return fail;
}

int find_config_int(const struct config_node *cn, const char *path, int fail)
{
	/* FIXME Add log_error message on overflow */
	return (int) _find_config_int64(cn, NULL, path, (int64_t) fail);
}

static float _find_config_float(const struct config_node *cn1,
				const struct config_node *cn2,
				const char *path, float fail)
{
	const struct config_node *n = _find_first_config_node(cn1, cn2, path);

	if (n && n->v && n->v->type == CFG_FLOAT) {
		log_very_verbose("Setting %s to %f", path, n->v->v.r);
		return n->v->v.r;
	}

	log_very_verbose("%s not found in config: defaulting to %f",
			 path, fail);

	return fail;

}

float find_config_float(const struct config_node *cn, const char *path,
			float fail)
{
	return _find_config_float(cn, NULL, path, fail);
}

struct config_node *find_config_tree_node(struct cmd_context *cmd,
					  const char *path)
{
	return _find_first_config_node(cmd->cft_override ? cmd->cft_override->root : NULL, cmd->cft->root, path);
}

const char *find_config_tree_str(struct cmd_context *cmd,
				 const char *path, const char *fail)
{
	return _find_config_str(cmd->cft_override ? cmd->cft_override->root : NULL, cmd->cft->root, path, fail);
}

int find_config_tree_int(struct cmd_context *cmd, const char *path,
			 int fail)
{
	/* FIXME Add log_error message on overflow */
	return (int) _find_config_int64(cmd->cft_override ? cmd->cft_override->root : NULL, cmd->cft->root, path, (int64_t) fail);
}

float find_config_tree_float(struct cmd_context *cmd, const char *path,
			     float fail)
{
	return _find_config_float(cmd->cft_override ? cmd->cft_override->root : NULL, cmd->cft->root, path, fail);
}

static int _str_in_array(const char *str, const char * const values[])
{
	int i;

	for (i = 0; values[i]; i++)
		if (!strcasecmp(str, values[i]))
			return 1;

	return 0;
}

static int _str_to_bool(const char *str, int fail)
{
	const char * const _true_values[]  = { "y", "yes", "on", "true", NULL };
	const char * const _false_values[] = { "n", "no", "off", "false", NULL };

	if (_str_in_array(str, _true_values))
		return 1;

	if (_str_in_array(str, _false_values))
		return 0;

	return fail;
}

static int _find_config_bool(const struct config_node *cn1,
			     const struct config_node *cn2,
			     const char *path, int fail)
{
	const struct config_node *n = _find_first_config_node(cn1, cn2, path);
	struct config_value *v;

	if (!n)
		return fail;

	v = n->v;

	switch (v->type) {
	case CFG_INT:
		return v->v.i ? 1 : 0;

	case CFG_STRING:
		return _str_to_bool(v->v.str, fail);
	}

	return fail;
}

int find_config_bool(const struct config_node *cn, const char *path, int fail)
{
	return _find_config_bool(cn, NULL, path, fail);
}

int find_config_tree_bool(struct cmd_context *cmd, const char *path, int fail)
{
	return _find_config_bool(cmd->cft_override ? cmd->cft_override->root : NULL, cmd->cft->root, path, fail);
}

int get_config_uint32(const struct config_node *cn, const char *path,
		      uint32_t *result)
{
	const struct config_node *n;

	n = find_config_node(cn, path);

	if (!n || !n->v || n->v->type != CFG_INT)
		return 0;

	*result = n->v->v.i;
	return 1;
}

int get_config_uint64(const struct config_node *cn, const char *path,
		      uint64_t *result)
{
	const struct config_node *n;

	n = find_config_node(cn, path);

	if (!n || !n->v || n->v->type != CFG_INT)
		return 0;

	*result = (uint64_t) n->v->v.i;
	return 1;
}

int get_config_str(const struct config_node *cn, const char *path,
		   char **result)
{
	const struct config_node *n;

	n = find_config_node(cn, path);

	if (!n || !n->v || n->v->type != CFG_STRING)
		return 0;

	*result = n->v->v.str;
	return 1;
}

/* Insert cn2 after cn1 */
static void _insert_config_node(struct config_node **cn1,
				struct config_node *cn2)
{
	if (!*cn1) {
		*cn1 = cn2;
		cn2->sib = NULL;
	} else {
		cn2->sib = (*cn1)->sib;
		(*cn1)->sib = cn2;
	}
}

/*
 * Merge section cn2 into section cn1 (which has the same name)
 * overwriting any existing cn1 nodes with matching names.
 */
static void _merge_section(struct config_node *cn1, struct config_node *cn2)
{
	struct config_node *cn, *nextn, *oldn;
	struct config_value *cv;

	for (cn = cn2->child; cn; cn = nextn) {
		nextn = cn->sib;

		/* Skip "tags" */
		if (!strcmp(cn->key, "tags"))
			continue;

		/* Subsection? */
		if (!cn->v)
			/* Ignore - we don't have any of these yet */
			continue;
		/* Not already present? */
		if (!(oldn = find_config_node(cn1->child, cn->key))) {
			_insert_config_node(&cn1->child, cn);
			continue;
		}
		/* Merge certain value lists */
		if ((!strcmp(cn1->key, "activation") &&
		     !strcmp(cn->key, "volume_list")) ||
		    (!strcmp(cn1->key, "devices") &&
		     (!strcmp(cn->key, "filter") || !strcmp(cn->key, "types")))) {
			cv = cn->v;
			while (cv->next)
				cv = cv->next;
			cv->next = oldn->v;
		}

		/* Replace values */
		oldn->v = cn->v;
	}
}

static int _match_host_tags(struct dm_list *tags, struct config_node *tn)
{
	struct config_value *tv;
	const char *str;

	for (tv = tn->v; tv; tv = tv->next) {
		if (tv->type != CFG_STRING)
			continue;
		str = tv->v.str;
		if (*str == '@')
			str++;
		if (!*str)
			continue;
		if (str_list_match_item(tags, str))
			return 1;
	}

	return 0;
}

/* Destructively merge a new config tree into an existing one */
int merge_config_tree(struct cmd_context *cmd, struct config_tree *cft,
		      struct config_tree *newdata)
{
	struct config_node *root = cft->root;
	struct config_node *cn, *nextn, *oldn, *tn, *cn2;

	for (cn = newdata->root; cn; cn = nextn) {
		nextn = cn->sib;
		/* Ignore tags section */
		if (!strcmp(cn->key, "tags"))
			continue;
		/* If there's a tags node, skip if host tags don't match */
		if ((tn = find_config_node(cn->child, "tags"))) {
			if (!_match_host_tags(&cmd->tags, tn))
				continue;
		}
		if (!(oldn = find_config_node(root, cn->key))) {
			_insert_config_node(&cft->root, cn);
			/* Remove any "tags" nodes */
			for (cn2 = cn->child; cn2; cn2 = cn2->sib) {
				if (!strcmp(cn2->key, "tags")) {
					cn->child = cn2->sib;
					continue;
				}
				if (cn2->sib && !strcmp(cn2->sib->key, "tags")) {
					cn2->sib = cn2->sib->sib;
					continue;
				}
			}
			continue;
		}
		_merge_section(oldn, cn);
	}

	return 1;
}

/*
 * Convert a token type to the char it represents.
 */
static char _token_type_to_char(int type)
{
	switch (type) {
		case TOK_SECTION_B:
			return SECTION_B_CHAR;
		case TOK_SECTION_E:
			return SECTION_E_CHAR;
		default:
			return 0;
	}
}

/*
 * Returns:
 *  # of 'type' tokens in 'str'.
 */
static unsigned _count_tokens(const char *str, unsigned len, int type)
{
	char c;

	c = _token_type_to_char(type);

	return count_chars(str, len, c);
}

const char *config_parent_name(const struct config_node *n)
{
	return (n->parent ? n->parent->key : "(root)");
}
/*
 * Heuristic function to make a quick guess as to whether a text
 * region probably contains a valid config "section".  (Useful for
 * scanning areas of the disk for old metadata.)
 * Config sections contain various tokens, may contain other sections
 * and strings, and are delimited by begin (type 'TOK_SECTION_B') and
 * end (type 'TOK_SECTION_E') tokens.  As a quick heuristic, we just
 * count the number of begin and end tokens, and see if they are
 * non-zero and the counts match.
 * Full validation of the section should be done with another function
 * (for example, read_config_fd).
 *
 * Returns:
 *  0 - probably is not a valid config section
 *  1 - probably _is_ a valid config section
 */
unsigned maybe_config_section(const char *str, unsigned len)
{
	int begin_count;
	int end_count;

	begin_count = _count_tokens(str, len, TOK_SECTION_B);
	end_count = _count_tokens(str, len, TOK_SECTION_E);

	if (begin_count && end_count && (begin_count == end_count))
		return 1;
	else
		return 0;
}

static struct config_value *_clone_config_value(struct dm_pool *mem, const struct config_value *v)
{
	if (!v)
		return NULL;
	struct config_value *new = _create_value(mem);
	new->type = v->type;
	if (v->type == CFG_STRING)
		new->v.str = dm_pool_strdup(mem, v->v.str);
	else
		new->v = v->v;
	new->next = _clone_config_value(mem, v->next);
	return new;
}

struct config_node *clone_config_node(struct dm_pool *mem, const struct config_node *cn,
				      int siblings)
{
	if (!cn)
		return NULL;
	struct config_node *new = _create_node(mem);
	new->key = dm_pool_strdup(mem, cn->key);
	new->child = clone_config_node(mem, cn->child, 1);
	new->v = _clone_config_value(mem, cn->v);
	if (siblings)
		new->sib = clone_config_node(mem, cn->sib, siblings);
	else
		new->sib = NULL;
	return new;
}
