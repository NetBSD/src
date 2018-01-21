#include "citrus_trie.h"
#ifdef DEBUG_TRIE
# include "modules/citrus_euc_data.h"
# include "citrus_trie_test.h"
#endif

static void citrus_trie_dump_table_recursive(citrus_trie_node_t, size_t, FILE *, int);
static void citrus_trie_load_table_recursive(citrus_trie_node_t, size_t, FILE *);
static void citrus_trie_init_recursive(citrus_trie_node_t *, VALUE_TYPE *, int, int *, int *);

citrus_trie_header_t
citrus_trie_create(unsigned int flags, size_t bitwidth,
		   size_t nlevels, size_t off, size_t len)
{
	citrus_trie_header_t h;

	h = (citrus_trie_header_t)malloc(sizeof(*h));
	h->th_flags = flags;
	h->th_bitwidth = bitwidth;
	h->th_bitmask = (1 << bitwidth) - 1;
	h->th_off = off;
	h->th_level = nlevels - 1;
#ifdef DEBUG_TRIE
	h->th_size = sizeof(*h);
#endif
	h->th_root = citrus_trie_node_create(h, h->th_level, len);

	return h;
}

citrus_trie_node_t
citrus_trie_node_create(citrus_trie_header_t h, size_t level, size_t len)
{
	int i;
	citrus_trie_node_t t;

	t = (citrus_trie_node_t)citrus_trie_malloc(h, sizeof(*t));
	t->tr_len = len;
	if (len > 0) {
		t->tr_u = (union citrus_trie_node_union *)citrus_trie_malloc(h, len * sizeof(*t->tr_u));
		if (level == 0) {
			for (i = 0; i < len; i++)
				t->tr_u[i].u_value = INVALID_VALUE;
		} else {
			for (i = 0; i < len; i++)
				t->tr_u[i].u_child = NULL;
		}
	} else
		t->tr_u = NULL;

	return t;
}

int
citrus_trie_node_insert(citrus_trie_header_t h, citrus_trie_node_t t, size_t level, citrus_trie_key_t key, VALUE_TYPE value)
{
	size_t idx, i, olen;

	idx = (key >> (level * h->th_bitwidth)) & h->th_bitmask;
	if (idx < h->th_off)
		return EINVAL;

	idx -= h->th_off;
	if (t->tr_len <= idx) {
		olen = t->tr_len;
		t->tr_len = idx + 1;
#ifdef DEBUG_TRIE
		h->th_size += (t->tr_len - olen) * sizeof(*t->tr_u);
#endif
		t->tr_u = (union citrus_trie_node_union *)realloc(t->tr_u, t->tr_len * sizeof(*t->tr_u));
		for (i = olen; i < t->tr_len; i++) {
			if (level == 0)
				t->tr_u[i].u_value = INVALID_VALUE;
			else
				t->tr_u[i].u_child = NULL;
		}
	}

	if (level == 0) {
		t->tr_u[idx].u_value = value;
		return 0;
	} else {
		if (t->tr_u[idx].u_child == NULL)
			t->tr_u[idx].u_child = citrus_trie_node_create(h, level - 1, 0);
		return citrus_trie_node_insert(h, t->tr_u[idx].u_child, level - 1,
					key, value);
	}
}

int
citrus_trie_insert(citrus_trie_header_t h, citrus_trie_key_t key, VALUE_TYPE value)
{
	int r = citrus_trie_node_insert(h, h->th_root, h->th_level, key, value);
#ifdef DEBUG_TRIE
	int c = citrus_trie_lookup(h, key);
	if (c != value)
		fprintf(stderr, "Error on insert: key %x expected %x got %x\n", (unsigned)key, value, c);
#endif
	return r;
}

VALUE_TYPE
citrus_trie_node_lookup(citrus_trie_header_t h, citrus_trie_node_t t, size_t level, citrus_trie_key_t key)
{
	size_t idx;

	idx = (key >> (level * h->th_bitwidth)) & h->th_bitmask;
	if (idx < h->th_off)
		return INVALID_VALUE;

	idx -= h->th_off;
	if (idx >= t->tr_len)
		return INVALID_VALUE;

	if (level == 0) {
		return t->tr_u[idx].u_value;
	}
	return citrus_trie_node_lookup(h, t->tr_u[idx].u_child, level - 1, key);
}

VALUE_TYPE
citrus_trie_lookup(citrus_trie_header_t h, citrus_trie_key_t key)
{
	return citrus_trie_node_lookup(h, h->th_root, h->th_level, key);
}

/*
 * Assume VALUE_TYPE flat[N][2].
 */
citrus_trie_header_t
citrus_trie_create_from_flat(VALUE_TYPE *flat, size_t bitwidth, int count) {
	VALUE_TYPE ne_key;
	int i, j;
	unsigned val;
	citrus_trie_header_t h;
	size_t bitmask = (1 << bitwidth) - 1;
	size_t maxshift = (8 * sizeof(VALUE_TYPE)) / bitwidth;
	size_t min = bitmask, max = 0, level = 0;

	/* Loop through every element to see what
	   level, off and len should be */
	for (i = 0; i < count; i++) {
		ne_key = flat[i * 2];
		for (j = 0; j < maxshift; j++) {
			val = (ne_key >> (j * bitwidth)) & bitmask;
			if (level < j + 1 && val)
				level = j + 1;
			if (min > val)
				min = val;
			if (max < val)
				max = val;
		}
	}

	h = citrus_trie_create(0x0, bitwidth, level, min, max - min + 1);

	/* Now add every value */
	for (i = 0; i < count; i++) {
		ne_key = flat[i * 2];
		
		citrus_trie_insert(h, ne_key, flat[i * 2 + 1]);
	}
	return h;
}

void
citrus_trie_node_destroy(citrus_trie_node_t t, size_t level)
{
	size_t i;

	if (level > 0)
		for (i = 0; i < t->tr_len; i++)
			if (t->tr_u[i].u_child != NULL)
				citrus_trie_node_destroy(t->tr_u[i].u_child, level - 1);
	free(t);
}

void
citrus_trie_destroy(citrus_trie_header_t h)
{
	citrus_trie_node_destroy(h->th_root, h->th_level);
	free(h);
}

static void
citrus_trie_dump_table_recursive(citrus_trie_node_t t, size_t level, FILE *fp, int mode)
{
	size_t i;

	if (mode == 0) {
		/* Binary */
		fwrite(t, sizeof(*t), 1, fp);
		fwrite(t->tr_u, sizeof(*t->tr_u), t->tr_len, fp);
	} else if (mode == 1) {
		/* Header */
		fprintf(fp, " { %zu, NULL },\n", t->tr_len);
	} else {
		/* Data */
		if (level == 0) {
			for (i = 0; i < t->tr_len; i++) {
				fprintf(fp, " %d,", t->tr_u[i].u_value);
			}
		}
	}
	if (level)
		for (i = 0; i < t->tr_len; i++)
			if (t->tr_u[i].u_child != NULL)
				citrus_trie_dump_table_recursive(t->tr_u[i].u_child, level - 1, fp, mode);
}

/* Load binary data only */
static void
citrus_trie_load_table_recursive(citrus_trie_node_t t, size_t level, FILE *fp)
{
	int i;

	fread(t, sizeof(*t), 1, fp);
	t->tr_u = (union citrus_trie_node_union *)malloc(t->tr_len * sizeof(*t->tr_u));
	fread(t->tr_u, sizeof(*t->tr_u), t->tr_len, fp);
	if (level) {
		for (i = 0; i < t->tr_len; i++) {
			if (t->tr_u[i].u_child != NULL) {
				t->tr_u[i].u_child = (citrus_trie_node_t)malloc(sizeof(*t));
				citrus_trie_load_table_recursive(t->tr_u[i].u_child, level - 1, fp);
			}
		}
	}
}

citrus_trie_header_t
citrus_trie_load(char *filename)
{
	citrus_trie_header_t h = (citrus_trie_header_t)malloc(sizeof(*h));
	FILE *fp = fopen(filename, "rb");
	fread(h, sizeof(*h), 1, fp);
	h->th_root = (citrus_trie_node_t)malloc(sizeof(*h->th_root));
	citrus_trie_load_table_recursive(h->th_root, h->th_level, fp);
	fclose(fp);

	return h;
}

void
citrus_trie_dump(citrus_trie_header_t h, char *filename, char *prefix, int mode)
{
	FILE *fp = fopen(filename, "wb");

	if (mode == 0) {
		fwrite(h, sizeof(*h), 1, fp);
		citrus_trie_dump_table_recursive(h->th_root, h->th_level, fp, 0);
	} else {
		/* Dump header info */
		fprintf(fp, "#include \"citrus_trie.h\"\n\n");
		fprintf(fp, "struct citrus_trie_header %s_header = {"
			"  %u, %zu, %zu, %zu, %zu, 0 };\n",
			prefix,
			h->th_flags, h->th_bitwidth, h->th_bitmask, h->th_off, h->th_level);
		/* Dump tree */
		fprintf(fp, "struct citrus_trie_node %s_nodes[] = {\n", prefix);
		citrus_trie_dump_table_recursive(h->th_root, h->th_level, fp, 1);
		fprintf(fp, "};\n");
		/* Dump data */
		fprintf(fp, VALUE_TYPE_STRING " %s_data[] = {\n", prefix);
		citrus_trie_dump_table_recursive(h->th_root, h->th_level, fp, 2);
		fprintf(fp, "};\n");
	}
	fclose(fp);
}

/* Walk through the list of nodes, assigning tr_u to each. */
static void
citrus_trie_init_recursive(citrus_trie_node_t *np, VALUE_TYPE *vp, int level, int *nidx, int *vidx)
{
	int i;

	for (i = 0; i < (*np)->tr_len; i++) {
		if (level) {
			(*np)->tr_u->u_child = *np + *nidx++;
			citrus_trie_init_recursive(np, vp, --level, nidx, vidx);
		} else {
			(*np)->tr_u->u_value = vp[*vidx];
			*vidx += (*np)->tr_len;
		}
	}
}

void
citrus_trie_init(citrus_trie_header_t h, citrus_trie_node_t *np, VALUE_TYPE *vp)
{
	int nidx = 0;
	int vidx = 0;

	h->th_root = *np;
	citrus_trie_init_recursive(np, vp, h->th_level, &nidx, &vidx);
}

#ifdef DEBUG_TRIE
int main(int argc, char **argv)
{
	int i, j, length;
	int32_t *flat;
	int result0;

	flat = (int32_t *)__euc_jp_jisx0213_table__unicode2kuten_lookup;
	length = _EUC_JP_JISX0213_TABLE__U2K_LIST_LENGTH;

	result0 = flat[0];

	for (i = 1; i < 24; i++) {
		citrus_trie_header_t h = citrus_trie_create_from_flat(flat, i, length);
		for (j = 0; j < length; j++) {
			if (result0 != flat[0]) {
				fprintf(stderr, "i=%d j=%d corruption!\n", i, j);
			}
			int result = citrus_trie_lookup(h, flat[2 * j]);
			if (result != flat[2 * j + 1]) {
				/* fprintf(stderr, "i=%d key=%x expected %x found %x\n", i, flat[2 * j], flat[2 * j + 1], result); */
			}
		}
		printf("i = %d, levels = %d, space = %zu\n", i, (int)h->th_level, h->th_size);

		/* Test save/load as well */
		citrus_trie_dump(h, "foo", NULL, 0);
		citrus_trie_destroy(h);
		h = citrus_trie_load("foo");

		/* Test source code drop */
		if (i == 5) {
			/* Dump for next compilation */
			citrus_trie_dump(h, "citrus_trie_test.h", "test", 1);
			/* Check previous compilation, if present */
#ifdef NODES
			citrus_trie_init(test_header, test_nodes, test_values);
#endif
		}

		for (j = 0; j < length; j++) {
			int result = citrus_trie_lookup(h, flat[2 * j]);
			if (result != flat[2 * j + 1]) {
				/* fprintf(stderr, "from file: i=%d key=%x expected %x found %x\n", i, flat[2 * j], flat[2 * j + 1], result); */
			}
#ifdef NODES
			if (i == 5) {
				int result = citrus_trie_lookup(HEADER, flat[2 * j]);
				if (result != flat[2 * j + 1]) {
					/* fprintf(stderr, "from srcsfile: i=%d key=%x expected %x found %x\n", i, flat[2 * j], flat[2 * j + 1], result); */
				}
			}
#endif
		}
	}
	
}
#endif /* DEBUG_TRIE */

#ifdef BOOTSTRAP
# include "modules/citrus_big5_data.h"
# include "modules/citrus_euc_data.h"
# include "modules/citrus_iso2022_data.h"
# include "modules/citrus_mskanji_data.h"

int main(int argc, char **argv)
{
	int length;
	int32_t *flat;
	citrus_trie_header_t h;

	flat = (int32_t *)__big5_table__kuten2unicode_lookup;
	length = _BIG5_TABLE__K2U_LIST_LENGTH;
	h = citrus_trie_create_from_flat(flat, 5, length);
	citrus_trie_dump(h, "modules/citrus_big5_k2u.h", "__big5_k2u", 1);
	flat = (int32_t *)__big5_table__unicode2kuten_lookup;
	length = _BIG5_TABLE__U2K_LIST_LENGTH;
	h = citrus_trie_create_from_flat(flat, 5, length);
	citrus_trie_dump(h, "modules/citrus_big5_u2k.h", "__big5_u2k", 1);

	flat = (int32_t *)__euc_jp_jisx0213_table__kuten2unicode_lookup;
	length = _EUC_JP_JISX0213_TABLE__K2U_LIST_LENGTH;
	h = citrus_trie_create_from_flat(flat, 5, length);
	citrus_trie_dump(h, "modules/citrus_euc_k2u.h", "__euc_k2u", 1);
	flat = (int32_t *)__euc_jp_jisx0213_table__unicode2kuten_lookup;
	length = _EUC_JP_JISX0213_TABLE__U2K_LIST_LENGTH;
	h = citrus_trie_create_from_flat(flat, 5, length);
	citrus_trie_dump(h, "modules/citrus_euc_u2k.h", "__euc_u2k", 1);

	flat = (int32_t *)__kuten2unicode_lookup;
	length = K2U_LIST_LENGTH;
	h = citrus_trie_create_from_flat(flat, 5, length);
	citrus_trie_dump(h, "modules/citrus_iso2022_k2u.h", "__iso2022_k2u", 1);
	flat = (int32_t *)__unicode2kuten_lookup;
	length = U2K_LIST_LENGTH;
	h = citrus_trie_create_from_flat(flat, 5, length);
	citrus_trie_dump(h, "modules/citrus_iso2022_u2k.h", "__iso2022_u2k", 1);

	flat = (int32_t *)__shiftjis_mskanji_table__kuten2unicode_lookup;
	length = _SHIFTJIS_MSKANJI_TABLE__K2U_LIST_LENGTH;
	h = citrus_trie_create_from_flat(flat, 5, length);
	citrus_trie_dump(h, "modules/citrus_mskanji_k2u.h", "__mskanji_k2u", 1);
	flat = (int32_t *)__shiftjis_mskanji_table__unicode2kuten_lookup;
	length = _SHIFTJIS_MSKANJI_TABLE__U2K_LIST_LENGTH;
	h = citrus_trie_create_from_flat(flat, 5, length);
	citrus_trie_dump(h, "modules/citrus_mskanji_u2k.h", "__mskanji_u2k", 1);
}
#endif
