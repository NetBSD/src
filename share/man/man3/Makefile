#	$NetBSD: Makefile,v 1.34 2010/01/24 21:05:45 jruoho Exp $
#	@(#)Makefile	8.2 (Berkeley) 12/13/93

MAN=	_DIAGASSERT.3 CMSG_DATA.3 assert.3 bits.3 bitstring.3 dlfcn.3 end.3 \
	gcq.3 intro.3 queue.3 stdarg.3 sysexits.3 timeradd.3 tree.3 varargs.3

USETBL=	# used by queue.3

MLINKS+=bitstring.3 bit_alloc.3 \
	bitstring.3 bit_clear.3 \
	bitstring.3 bit_decl.3 \
	bitstring.3 bit_ffc.3 \
	bitstring.3 bit_ffs.3 \
	bitstring.3 bit_nclear.3 \
	bitstring.3 bit_nset.3 \
	bitstring.3 bit_set.3 \
	bitstring.3 bitstr_size.3 \
	bitstring.3 bit_test.3
MLINKS+=bits.3 __BIT.3 \
	bits.3 __BITS.3 \
	bits.3 __SHIFTIN.3 \
	bits.3 __SHIFTOUT.3 \
	bits.3 __SHIFTOUT_MASK.3
MLINKS+=end.3 edata.3 end.3 etext.3
MLINKS+=gcq.3 GCQ_INIT.3 \
	gcq.3 GCQ_INIT_HEAD.3 \
	gcq.3 gcq_init.3 \
	gcq.3 gcq_init_head.3 \
	gcq.3 gcq_q.3 \
	gcq.3 gcq_hq.3 \
	gcq.3 gcq_head.3 \
	gcq.3 gcq_remove.3 \
	gcq.3 gcq_onlist.3 \
	gcq.3 gcq_empty.3 \
	gcq.3 gcq_linked.3 \
	gcq.3 gcq_insert_after.3 \
	gcq.3 gcq_insert_before.3 \
	gcq.3 gcq_insert_head.3 \
	gcq.3 gcq_insert_tail.3 \
	gcq.3 gcq_tie.3 \
	gcq.3 gcq_tie_after.3 \
	gcq.3 gcq_tie_before.3 \
	gcq.3 gcq_merge.3 \
	gcq.3 gcq_merge_head.3 \
	gcq.3 gcq_merge_tail.3 \
	gcq.3 gcq_clear.3 \
	gcq.3 gcq_remove_all.3 \
	gcq.3 GCQ_ITEM.3 \
	gcq.3 GCQ_GOT_FIRST.3 \
	gcq.3 GCQ_GOT_LAST.3 \
	gcq.3 GCQ_GOT_NEXT.3 \
	gcq.3 GCQ_GOT_PREV.3 \
	gcq.3 GCQ_DEQUEUED_FIRST.3 \
	gcq.3 GCQ_DEQUEUED_LAST.3 \
	gcq.3 GCQ_DEQUEUED_NEXT.3 \
	gcq.3 GCQ_DEQUEUED_PREV.3 \
	gcq.3 GCQ_GOT_FIRST_TYPED.3 \
	gcq.3 GCQ_GOT_LAST_TYPED.3 \
	gcq.3 GCQ_GOT_NEXT_TYPED.3 \
	gcq.3 GCQ_GOT_PREV_TYPED.3 \
	gcq.3 GCQ_DEQUEUED_FIRST_TYPED.3 \
	gcq.3 GCQ_DEQUEUED_LAST_TYPED.3 \
	gcq.3 GCQ_DEQUEUED_NEXT_TYPED.3 \
	gcq.3 GCQ_DEQUEUED_PREV_TYPED.3 \
	gcq.3 GCQ_GOT_FIRST_COND.3 \
	gcq.3 GCQ_GOT_LAST_COND.3 \
	gcq.3 GCQ_GOT_NEXT_COND.3 \
	gcq.3 GCQ_GOT_PREV_COND.3 \
	gcq.3 GCQ_DEQUEUED_FIRST_COND.3 \
	gcq.3 GCQ_DEQUEUED_LAST_COND.3 \
	gcq.3 GCQ_DEQUEUED_NEXT_COND.3 \
	gcq.3 GCQ_DEQUEUED_PREV_COND.3 \
	gcq.3 GCQ_GOT_FIRST_COND_TYPED.3 \
	gcq.3 GCQ_GOT_LAST_COND_TYPED.3 \
	gcq.3 GCQ_GOT_NEXT_COND_TYPED.3 \
	gcq.3 GCQ_GOT_PREV_COND_TYPED.3 \
	gcq.3 GCQ_DEQUEUED_FIRST_COND_TYPED.3 \
	gcq.3 GCQ_DEQUEUED_LAST_COND_TYPED.3 \
	gcq.3 GCQ_DEQUEUED_NEXT_COND_TYPED.3 \
	gcq.3 GCQ_DEQUEUED_PREV_COND_TYPED.3 \
	gcq.3 GCQ_FOREACH.3 \
	gcq.3 GCQ_FOREACH_REV.3 \
	gcq.3 GCQ_FOREACH_NVAR.3 \
	gcq.3 GCQ_FOREACH_NVAR_REV.3 \
	gcq.3 GCQ_FOREACH_RO.3 \
	gcq.3 GCQ_FOREACH_RO_REV.3 \
	gcq.3 GCQ_FOREACH_DEQUEUED.3 \
	gcq.3 GCQ_FOREACH_DEQUEUED_REV.3 \
	gcq.3 GCQ_FOREACH_TYPED.3 \
	gcq.3 GCQ_FOREACH_REV_TYPED.3 \
	gcq.3 GCQ_FOREACH_NVAR_TYPED.3 \
	gcq.3 GCQ_FOREACH_NVAR_REV_TYPED.3 \
	gcq.3 GCQ_FOREACH_RO_TYPED.3 \
	gcq.3 GCQ_FOREACH_RO_REV_TYPED.3 \
	gcq.3 GCQ_FOREACH_DEQUEUED_TYPED.3 \
	gcq.3 GCQ_FOREACH_DEQUEUED_REV_TYPED.3 \
	gcq.3 GCQ_FIND.3 \
	gcq.3 GCQ_FIND_REV.3 \
	gcq.3 GCQ_FIND_TYPED.3 \
	gcq.3 GCQ_FIND_REV_TYPED.3
MLINKS+=queue.3 LIST_HEAD.3 \
	queue.3 LIST_HEAD_INITIALIZER.3 \
	queue.3 LIST_ENTRY.3 \
	queue.3 LIST_INIT.3 \
	queue.3 LIST_INSERT_AFTER.3 \
	queue.3 LIST_INSERT_BEFORE.3 \
	queue.3 LIST_INSERT_HEAD.3 \
	queue.3 LIST_REMOVE.3 \
	queue.3 LIST_FOREACH.3 \
	queue.3 LIST_EMPTY.3 \
	queue.3 LIST_FIRST.3 \
	queue.3 LIST_NEXT.3
MLINKS+=queue.3 SLIST_HEAD.3 \
	queue.3 SLIST_HEAD_INITIALIZER.3 \
	queue.3 SLIST_ENTRY.3 \
	queue.3 SLIST_INIT.3 \
	queue.3 SLIST_INSERT_AFTER.3 \
	queue.3 SLIST_INSERT_HEAD.3 \
	queue.3 SLIST_REMOVE_HEAD.3 \
	queue.3 SLIST_REMOVE.3 \
	queue.3 SLIST_FOREACH.3 \
	queue.3 SLIST_EMPTY.3 \
	queue.3 SLIST_FIRST.3 \
	queue.3 SLIST_NEXT.3
MLINKS+=queue.3 STAILQ_HEAD.3 \
	queue.3 STAILQ_HEAD_INITIALIZER.3 \
	queue.3 STAILQ_ENTRY.3 \
	queue.3 STAILQ_INIT.3 \
	queue.3 STAILQ_INSERT_HEAD.3 \
	queue.3 STAILQ_INSERT_TAIL.3 \
	queue.3 STAILQ_INSERT_AFTER.3 \
	queue.3 STAILQ_REMOVE_HEAD.3 \
	queue.3 STAILQ_REMOVE.3 \
	queue.3 STAILQ_FOREACH.3 \
	queue.3 STAILQ_EMPTY.3 \
	queue.3 STAILQ_FIRST.3 \
	queue.3 STAILQ_NEXT.3
MLINKS+=queue.3 SIMPLEQ_HEAD.3 \
	queue.3 SIMPLEQ_HEAD_INITIALIZER.3 \
	queue.3 SIMPLEQ_ENTRY.3 \
	queue.3 SIMPLEQ_INIT.3 \
	queue.3 SIMPLEQ_INSERT_HEAD.3 \
	queue.3 SIMPLEQ_INSERT_TAIL.3 \
	queue.3 SIMPLEQ_INSERT_AFTER.3 \
	queue.3 SIMPLEQ_REMOVE_HEAD.3 \
	queue.3 SIMPLEQ_REMOVE.3 \
	queue.3 SIMPLEQ_FOREACH.3 \
	queue.3 SIMPLEQ_EMPTY.3 \
	queue.3 SIMPLEQ_FIRST.3 \
	queue.3 SIMPLEQ_NEXT.3 
MLINKS+=queue.3 TAILQ_HEAD.3 \
	queue.3 TAILQ_HEAD_INITIALIZER.3 \
	queue.3 TAILQ_ENTRY.3 \
	queue.3 TAILQ_INIT.3 \
	queue.3 TAILQ_INSERT_HEAD.3 \
	queue.3 TAILQ_INSERT_TAIL.3 \
	queue.3 TAILQ_INSERT_AFTER.3 \
	queue.3 TAILQ_INSERT_BEFORE.3 \
	queue.3 TAILQ_REMOVE.3 \
	queue.3 TAILQ_FOREACH.3 \
	queue.3 TAILQ_FOREACH_REVERSE.3 \
	queue.3 TAILQ_EMPTY.3 \
	queue.3 TAILQ_FIRST.3 \
	queue.3 TAILQ_NEXT.3 \
	queue.3 TAILQ_LAST.3 \
	queue.3 TAILQ_PREV.3
MLINKS+=queue.3 CIRCLEQ_HEAD.3 \
	queue.3 CIRCLEQ_HEAD_INITIALIZER.3 \
	queue.3 CIRCLEQ_ENTRY.3 \
	queue.3 CIRCLEQ_INIT.3 \
	queue.3 CIRCLEQ_INSERT_AFTER.3 \
	queue.3 CIRCLEQ_INSERT_BEFORE.3 \
	queue.3 CIRCLEQ_INSERT_HEAD.3 \
	queue.3 CIRCLEQ_INSERT_TAIL.3 \
	queue.3 CIRCLEQ_REMOVE.3 \
	queue.3 CIRCLEQ_FOREACH.3 \
	queue.3 CIRCLEQ_FOREACH_REVERSE.3 \
	queue.3 CIRCLEQ_EMPTY.3 \
	queue.3 CIRCLEQ_FIRST.3 \
	queue.3 CIRCLEQ_LAST.3 \
	queue.3 CIRCLEQ_NEXT.3 \
	queue.3 CIRCLEQ_PREV.3 \
	queue.3 CIRCLEQ_LOOP_NEXT.3 \
	queue.3 CIRCLEQ_LOOP_PREV.3
MLINKS+=stdarg.3 va_arg.3 stdarg.3 va_copy.3 \
	stdarg.3 va_end.3 stdarg.3 va_start.3
MLINKS+=dlfcn.3 dlopen.3 dlfcn.3 dlclose.3 dlfcn.3 dlsym.3 dlfcn.3 dlctl.3 \
	dlfcn.3 dlerror.3 dlfcn.3 dladdr.3
MLINKS+=timeradd.3 timeradd.3 \
	timeradd.3 timersub.3 \
	timeradd.3 timerclear.3 \
	timeradd.3 timerisset.3 \
	timeradd.3 timercmp.3
MLINKS+=tree.3 SPLAY_PROTOTYPE.3 tree.3 SPLAY_GENERATE.3 \
	tree.3 SPLAY_ENTRY.3 tree.3 SPLAY_HEAD.3 \
	tree.3 SPLAY_INITIALIZER.3 tree.3 SPLAY_ROOT.3 \
	tree.3 SPLAY_EMPTY.3 tree.3 SPLAY_NEXT.3 \
	tree.3 SPLAY_MIN.3 tree.3 SPLAY_MAX.3 tree.3 SPLAY_FIND.3 \
	tree.3 SPLAY_LEFT.3 tree.3 SPLAY_RIGHT.3 tree.3 SPLAY_INIT.3 \
	tree.3 SPLAY_INSERT.3 tree.3 SPLAY_REMOVE.3 \
	tree.3 SPLAY_FOREACH.3
MLINKS+=tree.3 RB_PROTOTYPE.3 tree.3 RB_GENERATE.3 \
	tree.3 RB_ENTRY.3 tree.3 RB_HEAD.3 \
	tree.3 RB_INITIALIZER.3 tree.3 RB_ROOT.3 \
	tree.3 RB_EMPTY.3 tree.3 RB_NEXT.3 \
	tree.3 RB_MIN.3 tree.3 RB_MAX.3 tree.3 RB_FIND.3 \
	tree.3 RB_LEFT.3 tree.3 RB_RIGHT.3 tree.3 RB_PARENT.3 \
	tree.3 RB_INIT.3 tree.3 RB_INSERT.3 tree.3 RB_REMOVE.3 \
	tree.3 RB_FOREACH.3
MLINKS+=CMSG_DATA.3 CMSG_FIRSTHDR.3 \
	CMSG_DATA.3 CMSG_LEN.3 \
	CMSG_DATA.3 CMSG_NXTHDR.3 \
	CMSG_DATA.3 CMSG_SPACE.3

.include <bsd.man.mk>
.include <bsd.subdir.mk>
