/*	$NetBSD: lr0.c,v 1.6.8.1 2013/06/23 06:26:26 tls Exp $	*/
/* Id: lr0.c,v 1.13 2012/05/26 00:40:47 tom Exp  */

#include "defs.h"

#include <sys/cdefs.h>
__RCSID("$NetBSD: lr0.c,v 1.6.8.1 2013/06/23 06:26:26 tls Exp $");

static core *new_state(int symbol);
static Value_t get_state(int symbol);
static void allocate_itemsets(void);
static void allocate_storage(void);
static void append_states(void);
static void free_storage(void);
static void generate_states(void);
static void initialize_states(void);
static void new_itemsets(void);
static void save_reductions(void);
static void save_shifts(void);
static void set_derives(void);
static void set_nullable(void);

int nstates;
core *first_state;
shifts *first_shift;
reductions *first_reduction;

static core **state_set;
static core *this_state;
static core *last_state;
static shifts *last_shift;
static reductions *last_reduction;

static int nshifts;
static short *shift_symbol;

static Value_t *redset;
static Value_t *shiftset;

static Value_t **kernel_base;
static Value_t **kernel_end;
static Value_t *kernel_items;

static void
allocate_itemsets(void)
{
    short *itemp;
    short *item_end;
    int symbol;
    int i;
    int count;
    int max;
    short *symbol_count;

    count = 0;
    symbol_count = NEW2(nsyms, short);

    item_end = ritem + nitems;
    for (itemp = ritem; itemp < item_end; itemp++)
    {
	symbol = *itemp;
	if (symbol >= 0)
	{
	    count++;
	    symbol_count[symbol]++;
	}
    }

    kernel_base = NEW2(nsyms, short *);
    kernel_items = NEW2(count, short);

    count = 0;
    max = 0;
    for (i = 0; i < nsyms; i++)
    {
	kernel_base[i] = kernel_items + count;
	count += symbol_count[i];
	if (max < symbol_count[i])
	    max = symbol_count[i];
    }

    shift_symbol = symbol_count;
    kernel_end = NEW2(nsyms, short *);
}

static void
allocate_storage(void)
{
    allocate_itemsets();
    shiftset = NEW2(nsyms, short);
    redset = NEW2(nrules + 1, short);
    state_set = NEW2(nitems, core *);
}

static void
append_states(void)
{
    int i;
    int j;
    Value_t symbol;

#ifdef	TRACE
    fprintf(stderr, "Entering append_states()\n");
#endif
    for (i = 1; i < nshifts; i++)
    {
	symbol = shift_symbol[i];
	j = i;
	while (j > 0 && shift_symbol[j - 1] > symbol)
	{
	    shift_symbol[j] = shift_symbol[j - 1];
	    j--;
	}
	shift_symbol[j] = symbol;
    }

    for (i = 0; i < nshifts; i++)
    {
	symbol = shift_symbol[i];
	shiftset[i] = get_state(symbol);
    }
}

static void
free_storage(void)
{
    FREE(shift_symbol);
    FREE(redset);
    FREE(shiftset);
    FREE(kernel_base);
    FREE(kernel_end);
    FREE(kernel_items);
    FREE(state_set);
}

static void
generate_states(void)
{
    allocate_storage();
    itemset = NEW2(nitems, short);
    ruleset = NEW2(WORDSIZE(nrules), unsigned);
    set_first_derives();
    initialize_states();

    while (this_state)
    {
	closure(this_state->items, this_state->nitems);
	save_reductions();
	new_itemsets();
	append_states();

	if (nshifts > 0)
	    save_shifts();

	this_state = this_state->next;
    }

    free_storage();
}

static Value_t
get_state(int symbol)
{
    int key;
    short *isp1;
    short *isp2;
    short *iend;
    core *sp;
    int found;
    int n;

#ifdef	TRACE
    fprintf(stderr, "Entering get_state(%d)\n", symbol);
#endif

    isp1 = kernel_base[symbol];
    iend = kernel_end[symbol];
    n = (int)(iend - isp1);

    key = *isp1;
    assert(0 <= key && key < nitems);
    sp = state_set[key];
    if (sp)
    {
	found = 0;
	while (!found)
	{
	    if (sp->nitems == n)
	    {
		found = 1;
		isp1 = kernel_base[symbol];
		isp2 = sp->items;

		while (found && isp1 < iend)
		{
		    if (*isp1++ != *isp2++)
			found = 0;
		}
	    }

	    if (!found)
	    {
		if (sp->link)
		{
		    sp = sp->link;
		}
		else
		{
		    sp = sp->link = new_state(symbol);
		    found = 1;
		}
	    }
	}
    }
    else
    {
	state_set[key] = sp = new_state(symbol);
    }

    return (sp->number);
}

static void
initialize_states(void)
{
    unsigned i;
    short *start_derives;
    core *p;

    start_derives = derives[start_symbol];
    for (i = 0; start_derives[i] >= 0; ++i)
	continue;

    p = (core *)MALLOC(sizeof(core) + i * sizeof(short));
    NO_SPACE(p);

    p->next = 0;
    p->link = 0;
    p->number = 0;
    p->accessing_symbol = 0;
    p->nitems = (Value_t) i;

    for (i = 0; start_derives[i] >= 0; ++i)
	p->items[i] = rrhs[start_derives[i]];

    first_state = last_state = this_state = p;
    nstates = 1;
}

static void
new_itemsets(void)
{
    Value_t i;
    int shiftcount;
    short *isp;
    short *ksp;
    Value_t symbol;

    for (i = 0; i < nsyms; i++)
	kernel_end[i] = 0;

    shiftcount = 0;
    isp = itemset;
    while (isp < itemsetend)
    {
	i = *isp++;
	symbol = ritem[i];
	if (symbol > 0)
	{
	    ksp = kernel_end[symbol];
	    if (!ksp)
	    {
		shift_symbol[shiftcount++] = symbol;
		ksp = kernel_base[symbol];
	    }

	    *ksp++ = (Value_t) (i + 1);
	    kernel_end[symbol] = ksp;
	}
    }

    nshifts = shiftcount;
}

static core *
new_state(int symbol)
{
    unsigned n;
    core *p;
    short *isp1;
    short *isp2;
    short *iend;

#ifdef	TRACE
    fprintf(stderr, "Entering new_state(%d)\n", symbol);
#endif

    if (nstates >= MAXSHORT)
	fatal("too many states");

    isp1 = kernel_base[symbol];
    iend = kernel_end[symbol];
    n = (unsigned)(iend - isp1);

    p = (core *)allocate((sizeof(core) + (n - 1) * sizeof(short)));
    p->accessing_symbol = (Value_t) symbol;
    p->number = (Value_t) nstates;
    p->nitems = (Value_t) n;

    isp2 = p->items;
    while (isp1 < iend)
	*isp2++ = *isp1++;

    last_state->next = p;
    last_state = p;

    nstates++;

    return (p);
}

/* show_cores is used for debugging */

void
show_cores(void)
{
    core *p;
    int i, j, k, n;
    int itemno;

    k = 0;
    for (p = first_state; p; ++k, p = p->next)
    {
	if (k)
	    printf("\n");
	printf("state %d, number = %d, accessing symbol = %s\n",
	       k, p->number, symbol_name[p->accessing_symbol]);
	n = p->nitems;
	for (i = 0; i < n; ++i)
	{
	    itemno = p->items[i];
	    printf("%4d  ", itemno);
	    j = itemno;
	    while (ritem[j] >= 0)
		++j;
	    printf("%s :", symbol_name[rlhs[-ritem[j]]]);
	    j = rrhs[-ritem[j]];
	    while (j < itemno)
		printf(" %s", symbol_name[ritem[j++]]);
	    printf(" .");
	    while (ritem[j] >= 0)
		printf(" %s", symbol_name[ritem[j++]]);
	    printf("\n");
	    fflush(stdout);
	}
    }
}

/* show_ritems is used for debugging */

void
show_ritems(void)
{
    int i;

    for (i = 0; i < nitems; ++i)
	printf("ritem[%d] = %d\n", i, ritem[i]);
}

/* show_rrhs is used for debugging */
void
show_rrhs(void)
{
    int i;

    for (i = 0; i < nrules; ++i)
	printf("rrhs[%d] = %d\n", i, rrhs[i]);
}

/* show_shifts is used for debugging */

void
show_shifts(void)
{
    shifts *p;
    int i, j, k;

    k = 0;
    for (p = first_shift; p; ++k, p = p->next)
    {
	if (k)
	    printf("\n");
	printf("shift %d, number = %d, nshifts = %d\n", k, p->number,
	       p->nshifts);
	j = p->nshifts;
	for (i = 0; i < j; ++i)
	    printf("\t%d\n", p->shift[i]);
    }
}

static void
save_shifts(void)
{
    shifts *p;
    short *sp1;
    short *sp2;
    short *send;

    p = (shifts *)allocate((sizeof(shifts) +
			      (unsigned)(nshifts - 1) * sizeof(short)));

    p->number = this_state->number;
    p->nshifts = (Value_t) nshifts;

    sp1 = shiftset;
    sp2 = p->shift;
    send = shiftset + nshifts;

    while (sp1 < send)
	*sp2++ = *sp1++;

    if (last_shift)
    {
	last_shift->next = p;
	last_shift = p;
    }
    else
    {
	first_shift = p;
	last_shift = p;
    }
}

static void
save_reductions(void)
{
    short *isp;
    short *rp1;
    short *rp2;
    int item;
    Value_t count;
    reductions *p;
    short *rend;

    count = 0;
    for (isp = itemset; isp < itemsetend; isp++)
    {
	item = ritem[*isp];
	if (item < 0)
	{
	    redset[count++] = (Value_t) - item;
	}
    }

    if (count)
    {
	p = (reductions *)allocate((sizeof(reductions) +
				      (unsigned)(count - 1) *
				    sizeof(short)));

	p->number = this_state->number;
	p->nreds = count;

	rp1 = redset;
	rp2 = p->rules;
	rend = rp1 + count;

	while (rp1 < rend)
	    *rp2++ = *rp1++;

	if (last_reduction)
	{
	    last_reduction->next = p;
	    last_reduction = p;
	}
	else
	{
	    first_reduction = p;
	    last_reduction = p;
	}
    }
}

static void
set_derives(void)
{
    Value_t i, k;
    int lhs;
    short *rules;

    derives = NEW2(nsyms, short *);
    rules = NEW2(nvars + nrules, short);

    k = 0;
    for (lhs = start_symbol; lhs < nsyms; lhs++)
    {
	derives[lhs] = rules + k;
	for (i = 0; i < nrules; i++)
	{
	    if (rlhs[i] == lhs)
	    {
		rules[k] = i;
		k++;
	    }
	}
	rules[k] = -1;
	k++;
    }

#ifdef	DEBUG
    print_derives();
#endif
}

#ifdef	DEBUG
void
print_derives(void)
{
    int i;
    short *sp;

    printf("\nDERIVES\n\n");

    for (i = start_symbol; i < nsyms; i++)
    {
	printf("%s derives ", symbol_name[i]);
	for (sp = derives[i]; *sp >= 0; sp++)
	{
	    printf("  %d", *sp);
	}
	putchar('\n');
    }

    putchar('\n');
}
#endif

static void
set_nullable(void)
{
    int i, j;
    int empty;
    int done_flag;

    nullable = TMALLOC(char, nsyms);
    NO_SPACE(nullable);

    for (i = 0; i < nsyms; ++i)
	nullable[i] = 0;

    done_flag = 0;
    while (!done_flag)
    {
	done_flag = 1;
	for (i = 1; i < nitems; i++)
	{
	    empty = 1;
	    while ((j = ritem[i]) >= 0)
	    {
		if (!nullable[j])
		    empty = 0;
		++i;
	    }
	    if (empty)
	    {
		j = rlhs[-j];
		if (!nullable[j])
		{
		    nullable[j] = 1;
		    done_flag = 0;
		}
	    }
	}
    }

#ifdef DEBUG
    for (i = 0; i < nsyms; i++)
    {
	if (nullable[i])
	    printf("%s is nullable\n", symbol_name[i]);
	else
	    printf("%s is not nullable\n", symbol_name[i]);
    }
#endif
}

void
lr0(void)
{
    set_derives();
    set_nullable();
    generate_states();
}

#ifdef NO_LEAKS
void
lr0_leaks(void)
{
    DO_FREE(derives[start_symbol]);
    DO_FREE(derives);
    DO_FREE(nullable);
}
#endif
