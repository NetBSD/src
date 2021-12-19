/*	$NetBSD: intel_engine_user.c,v 1.4 2021/12/19 11:51:59 riastradh Exp $	*/

/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright © 2019 Intel Corporation
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intel_engine_user.c,v 1.4 2021/12/19 11:51:59 riastradh Exp $");

#include <linux/list.h>
#include <linux/list_sort.h>
#include <linux/llist.h>

#include "i915_drv.h"
#include "intel_engine.h"
#include "intel_engine_user.h"
#include "intel_gt.h"

#include <linux/nbsd-namespace.h>

#ifdef __NetBSD__

static int
compare_engines(void *cookie, const void *va, const void *vb)
{
	const struct intel_engine_cs *csa = va;
	const struct intel_engine_cs *csb = vb;

	if (csa->class < csb->class)
		return -1;
	if (csa->class > csb->class)
		return +1;
	if (csa->instance < csb->instance)
		return -1;
	if (csa->instance > csb->instance)
		return +1;
	return 0;
}

static int
compare_engine_key(void *cookie, const void *vn, const void *vk)
{
	const struct intel_engine_cs *cs = vn;
	const u8 *k = vk;

	if (cs->class < k[0])
		return -1;
	if (cs->class > k[0])
		return +1;
	if (cs->instance < k[1])
		return -1;
	if (cs->instance > k[1])
		return +1;
	return 0;
}

static const rb_tree_ops_t engine_ops = {
	.rbto_compare_nodes = compare_engines,
	.rbto_compare_key = compare_engine_key,
	.rbto_node_offset = offsetof(struct intel_engine_cs, uabi_node.rbtree),
};

#endif

struct intel_engine_cs *
intel_engine_lookup_user(struct drm_i915_private *i915, u8 class, u8 instance)
{
#ifdef __NetBSD__
	const u8 key[2] = {class, instance};
	return rb_tree_find_node(&i915->uabi_engines.rbr_tree, key);
#else
	struct rb_node *p = i915->uabi_engines.rb_node;

	while (p) {
		struct intel_engine_cs *it =
			rb_entry(p, typeof(*it), uabi_node);

		if (class < it->uabi_class)
			p = p->rb_left;
		else if (class > it->uabi_class ||
			 instance > it->uabi_instance)
			p = p->rb_right;
		else if (instance < it->uabi_instance)
			p = p->rb_left;
		else
			return it;
	}

	return NULL;
#endif
}

void intel_engine_add_user(struct intel_engine_cs *engine)
{
	llist_add(&engine->uabi_node.llist, &engine->i915->uabi_engines_llist);
}

static const u8 uabi_classes[] = {
	[RENDER_CLASS] = I915_ENGINE_CLASS_RENDER,
	[COPY_ENGINE_CLASS] = I915_ENGINE_CLASS_COPY,
	[VIDEO_DECODE_CLASS] = I915_ENGINE_CLASS_VIDEO,
	[VIDEO_ENHANCEMENT_CLASS] = I915_ENGINE_CLASS_VIDEO_ENHANCE,
};

static int engine_cmp(void *priv, struct list_head *A, struct list_head *B)
{
	const struct intel_engine_cs *a =
		container_of(A, typeof(*a), uabi_node.list);
	const struct intel_engine_cs *b =
		container_of(B, typeof(*b), uabi_node.list);

	if (uabi_classes[a->class] < uabi_classes[b->class])
		return -1;
	if (uabi_classes[a->class] > uabi_classes[b->class])
		return 1;

	if (a->instance < b->instance)
		return -1;
	if (a->instance > b->instance)
		return 1;

	return 0;
}

static struct llist_node *get_engines(struct drm_i915_private *i915)
{
	return llist_del_all(&i915->uabi_engines_llist);
}

static void sort_engines(struct drm_i915_private *i915,
			 struct list_head *engines)
{
	struct llist_node *pos, *next;

	llist_for_each_safe(pos, next, get_engines(i915)) {
		struct intel_engine_cs *engine =
			llist_entry(pos, typeof(*engine), uabi_node.llist);
		list_add(&engine->uabi_node.list, engines);
	}
	list_sort(NULL, engines, engine_cmp);
}

static void set_scheduler_caps(struct drm_i915_private *i915)
{
	static const struct {
		u8 engine;
		u8 sched;
	} map[] = {
#define MAP(x, y) { ilog2(I915_ENGINE_##x), ilog2(I915_SCHEDULER_CAP_##y) }
		MAP(HAS_PREEMPTION, PREEMPTION),
		MAP(HAS_SEMAPHORES, SEMAPHORES),
		MAP(SUPPORTS_STATS, ENGINE_BUSY_STATS),
#undef MAP
	};
	struct intel_engine_cs *engine;
	u32 enabled, disabled;

	enabled = 0;
	disabled = 0;
	for_each_uabi_engine(engine, i915) { /* all engines must agree! */
		int i;

		if (engine->schedule)
			enabled |= (I915_SCHEDULER_CAP_ENABLED |
				    I915_SCHEDULER_CAP_PRIORITY);
		else
			disabled |= (I915_SCHEDULER_CAP_ENABLED |
				     I915_SCHEDULER_CAP_PRIORITY);

		for (i = 0; i < ARRAY_SIZE(map); i++) {
			if (engine->flags & BIT(map[i].engine))
				enabled |= BIT(map[i].sched);
			else
				disabled |= BIT(map[i].sched);
		}
	}

	i915->caps.scheduler = enabled & ~disabled;
	if (!(i915->caps.scheduler & I915_SCHEDULER_CAP_ENABLED))
		i915->caps.scheduler = 0;
}

const char *intel_engine_class_repr(u8 class)
{
	static const char * const uabi_names[] = {
		[RENDER_CLASS] = "rcs",
		[COPY_ENGINE_CLASS] = "bcs",
		[VIDEO_DECODE_CLASS] = "vcs",
		[VIDEO_ENHANCEMENT_CLASS] = "vecs",
	};

	if (class >= ARRAY_SIZE(uabi_names) || !uabi_names[class])
		return "xxx";

	return uabi_names[class];
}

struct legacy_ring {
	struct intel_gt *gt;
	u8 class;
	u8 instance;
};

static int legacy_ring_idx(const struct legacy_ring *ring)
{
	static const struct {
		u8 base, max;
	} map[] = {
		[RENDER_CLASS] = { RCS0, 1 },
		[COPY_ENGINE_CLASS] = { BCS0, 1 },
		[VIDEO_DECODE_CLASS] = { VCS0, I915_MAX_VCS },
		[VIDEO_ENHANCEMENT_CLASS] = { VECS0, I915_MAX_VECS },
	};

	if (GEM_DEBUG_WARN_ON(ring->class >= ARRAY_SIZE(map)))
		return INVALID_ENGINE;

	if (GEM_DEBUG_WARN_ON(ring->instance >= map[ring->class].max))
		return INVALID_ENGINE;

	return map[ring->class].base + ring->instance;
}

static void add_legacy_ring(struct legacy_ring *ring,
			    struct intel_engine_cs *engine)
{
	if (engine->gt != ring->gt || engine->class != ring->class) {
		ring->gt = engine->gt;
		ring->class = engine->class;
		ring->instance = 0;
	}

	engine->legacy_idx = legacy_ring_idx(ring);
	if (engine->legacy_idx != INVALID_ENGINE)
		ring->instance++;
}

void intel_engines_driver_register(struct drm_i915_private *i915)
{
	struct legacy_ring ring = {};
	u8 uabi_instances[4] = {};
	struct list_head *it, *next;
	struct rb_node **p, *prev;
	LIST_HEAD(engines);

	sort_engines(i915, &engines);

#ifdef __NetBSD__
	__USE(prev);
	__USE(p);
	rb_tree_init(&i915->uabi_engines.rbr_tree, &engine_ops);
#else
	prev = NULL;
	p = &i915->uabi_engines.rb_node;
#endif
	list_for_each_safe(it, next, &engines) {
		struct intel_engine_cs *engine =
			container_of(it, typeof(*engine), uabi_node.list);
		char old[sizeof(engine->name)];

		if (intel_gt_has_init_error(engine->gt))
			continue; /* ignore incomplete engines */

		GEM_BUG_ON(engine->class >= ARRAY_SIZE(uabi_classes));
		engine->uabi_class = uabi_classes[engine->class];

		GEM_BUG_ON(engine->uabi_class >= ARRAY_SIZE(uabi_instances));
		engine->uabi_instance = uabi_instances[engine->uabi_class]++;

		/* Replace the internal name with the final user facing name */
		memcpy(old, engine->name, sizeof(engine->name));
		scnprintf(engine->name, sizeof(engine->name), "%s%u",
			  intel_engine_class_repr(engine->class),
			  engine->uabi_instance);
		DRM_DEBUG_DRIVER("renamed %s to %s\n", old, engine->name);

#ifdef __NetBSD__
		struct intel_engine_cs *collision __diagused;
		collision = rb_tree_insert_node(&i915->uabi_engines.rbr_tree,
		    engine);
		KASSERT(collision == engine);
#else
		rb_link_node(&engine->uabi_node, prev, p);
		rb_insert_color(&engine->uabi_node, &i915->uabi_engines);
#endif

		GEM_BUG_ON(intel_engine_lookup_user(i915,
						    engine->uabi_class,
						    engine->uabi_instance) != engine);

		/* Fix up the mapping to match default execbuf::user_map[] */
		add_legacy_ring(&ring, engine);

#ifndef __NetBSD__
		prev = &engine->uabi_node;
		p = &prev->rb_right;
#endif
	}

	if (IS_ENABLED(CONFIG_DRM_I915_SELFTEST) &&
	    IS_ENABLED(CONFIG_DRM_I915_DEBUG_GEM)) {
		struct intel_engine_cs *engine;
		unsigned int isolation;
		int class, inst;
		int errors = 0;

		for (class = 0; class < ARRAY_SIZE(uabi_instances); class++) {
			for (inst = 0; inst < uabi_instances[class]; inst++) {
				engine = intel_engine_lookup_user(i915,
								  class, inst);
				if (!engine) {
					pr_err("UABI engine not found for { class:%d, instance:%d }\n",
					       class, inst);
					errors++;
					continue;
				}

				if (engine->uabi_class != class ||
				    engine->uabi_instance != inst) {
					pr_err("Wrong UABI engine:%s { class:%d, instance:%d } found for { class:%d, instance:%d }\n",
					       engine->name,
					       engine->uabi_class,
					       engine->uabi_instance,
					       class, inst);
					errors++;
					continue;
				}
			}
		}

		/*
		 * Make sure that classes with multiple engine instances all
		 * share the same basic configuration.
		 */
		isolation = intel_engines_has_context_isolation(i915);
		for_each_uabi_engine(engine, i915) {
			unsigned int bit = BIT(engine->uabi_class);
			unsigned int expected = engine->default_state ? bit : 0;

			if ((isolation & bit) != expected) {
				pr_err("mismatching default context state for class %d on engine %s\n",
				       engine->uabi_class, engine->name);
				errors++;
			}
		}

		if (WARN(errors, "Invalid UABI engine mapping found"))
#ifdef __NetBSD__
			rb_tree_init(&i915->uabi_engines.rbr_tree,
			    &engine_ops);
#else
			i915->uabi_engines = RB_ROOT;
#endif
	}

	set_scheduler_caps(i915);
}

unsigned int intel_engines_has_context_isolation(struct drm_i915_private *i915)
{
	struct intel_engine_cs *engine;
	unsigned int which;

	which = 0;
	for_each_uabi_engine(engine, i915)
		if (engine->default_state)
			which |= BIT(engine->uabi_class);

	return which;
}
