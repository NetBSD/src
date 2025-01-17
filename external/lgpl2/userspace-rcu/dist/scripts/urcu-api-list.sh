#!/bin/sh

# SPDX-FileCopyrightText: 2013 IBM Corporation
#
# SPDX-License-Identifier: GPL-2.0-or-later

# Run in userspace-rcu git archive.  Prints out a list of API members
# and the version in which they were introduced.  You need to list all
# the API members immediately below. Depends on "cscope".

# Authors: Paul E. McKenney <paulmck@linux.vnet.ibm.com>

api="caa_container_of \
	caa_likely \
	caa_max \
	caa_unlikely \
	call_rcu \
	call_rcu_after_fork_child \
	call_rcu_after_fork_parent \
	call_rcu_before_fork \
	call_rcu_data_free \
	cds_hlist_add_head \
	cds_hlist_add_head_rcu \
	cds_hlist_del \
	cds_hlist_del_rcu \
	cds_hlist_entry \
	cds_hlist_for_each_entry \
	cds_hlist_for_each_entry_rcu \
	cds_hlist_for_each_entry_safe \
	CDS_INIT_HLIST_HEAD \
	CDS_INIT_LIST_HEAD \
	cds_lfht_add \
	cds_lfht_add_replace \
	cds_lfht_add_unique \
	cds_lfht_count_nodes \
	cds_lfht_del \
	cds_lfht_destroy \
	cds_lfht_first \
	cds_lfht_for_each \
	cds_lfht_for_each_duplicate \
	cds_lfht_for_each_entry \
	cds_lfht_for_each_entry_duplicate \
	cds_lfht_is_node_deleted \
	cds_lfht_iter_get_node \
	cds_lfht_lookup \
	cds_lfht_new \
	cds_lfht_next \
	cds_lfht_next_duplicate \
	cds_lfht_replace \
	cds_lfht_resize \
	cds_lfq_dequeue_rcu \
	cds_lfq_destroy_rcu \
	cds_lfq_enqueue_rcu \
	cds_lfq_init_rcu \
	cds_lfq_node_init_rcu \
	cds_lfs_empty \
	cds_lfs_for_each \
	cds_lfs_for_each_safe \
	cds_lfs_init \
	cds_lfs_node_init \
	__cds_lfs_pop \
	__cds_lfs_pop_all \
	cds_lfs_pop_all_blocking \
	cds_lfs_pop_blocking \
	cds_lfs_pop_lock \
	cds_lfs_pop_unlock \
	cds_lfs_push \
	cds_list_add \
	cds_list_add_rcu \
	cds_list_add_tail \
	cds_list_del \
	cds_list_del_init \
	cds_list_del_rcu \
	cds_list_empty \
	cds_list_entry \
	cds_list_first_entry \
	cds_list_for_each \
	cds_list_for_each_entry \
	cds_list_for_each_entry_rcu \
	cds_list_for_each_entry_reverse \
	cds_list_for_each_prev \
	cds_list_for_each_prev_safe \
	cds_list_for_each_rcu \
	cds_list_for_each_safe \
	CDS_LIST_HEAD \
	CDS_LIST_HEAD_INIT \
	cds_list_move \
	cds_list_replace \
	cds_list_replace_init \
	cds_list_replace_rcu \
	cds_list_splice \
	__cds_wfcq_dequeue_blocking \
	cds_wfcq_dequeue_blocking \
	cds_wfcq_dequeue_lock \
	__cds_wfcq_dequeue_nonblocking \
	cds_wfcq_dequeue_unlock \
	cds_wfcq_empty \
	cds_wfcq_enqueue \
	__cds_wfcq_first_blocking \
	__cds_wfcq_first_nonblocking \
	__cds_wfcq_for_each_blocking \
	__cds_wfcq_for_each_blocking_safe \
	cds_wfcq_init \
	__cds_wfcq_next_blocking \
	__cds_wfcq_next_nonblocking \
	cds_wfcq_node_init \
	__cds_wfcq_splice_blocking \
	cds_wfcq_splice_blocking \
	__cds_wfcq_splice_nonblocking \
	cds_wfs_empty \
	cds_wfs_first \
	cds_wfs_for_each_blocking \
	cds_wfs_for_each_blocking_safe \
	cds_wfs_init \
	cds_wfs_next_blocking \
	cds_wfs_next_nonblocking \
	cds_wfs_node_init \
	__cds_wfs_pop_all \
	cds_wfs_pop_all_blocking \
	__cds_wfs_pop_blocking \
	cds_wfs_pop_blocking \
	cds_wfs_pop_lock \
	__cds_wfs_pop_nonblocking \
	cds_wfs_pop_unlock \
	cds_wfs_push \
	CMM_ACCESS_ONCE \
	cmm_barrier \
	CMM_LOAD_SHARED \
	cmm_smp_mb \
	cmm_smp_mb__after_uatomic_add \
	cmm_smp_mb__after_uatomic_and \
	cmm_smp_mb__after_uatomic_dec \
	cmm_smp_mb__after_uatomic_inc \
	cmm_smp_mb__after_uatomic_or \
	cmm_smp_mb__before_uatomic_add \
	cmm_smp_mb__before_uatomic_and \
	cmm_smp_mb__before_uatomic_dec \
	cmm_smp_mb__before_uatomic_inc \
	cmm_smp_mb__before_uatomic_or \
	cmm_smp_rmb \
	cmm_smp_wmb \
	CMM_STORE_SHARED \
	create_all_cpu_call_rcu_data \
	create_call_rcu_data \
	DECLARE_URCU_TLS \
	defer_rcu \
	DEFINE_URCU_TLS \
	free_all_cpu_call_rcu_data \
	get_call_rcu_data \
	get_call_rcu_thread \
	get_cpu_call_rcu_data \
	get_default_call_rcu_data \
	get_thread_call_rcu_data \
	rcu_assign_pointer \
	rcu_cmpxchg_pointer \
	rcu_dereference \
	rcu_exit \
	rcu_init \
	rcu_quiescent_state \
	rcu_read_lock \
	rcu_read_unlock \
	rcu_register_thread \
	rcu_set_pointer \
	rcu_thread_offline \
	rcu_thread_online \
	rcu_unregister_thread \
	rcu_xchg_pointer \
	set_cpu_call_rcu_data \
	set_thread_call_rcu_data \
	synchronize_rcu \
	uatomic_add \
	uatomic_add_return \
	uatomic_and \
	uatomic_cmpxchg \
	uatomic_dec \
	uatomic_inc \
	uatomic_or \
	uatomic_read \
	uatomic_set \
	uatomic_xchg \
	URCU_TLS"

T=/tmp/urcu-api-list.sh.$$
trap 'rm -rf $T' 0
mkdir $T

git remote update 1>&2
git reset --hard origin/master 1>&2
git branch -f master 1>&2
tags=`git tag -l`
lasttag="init"
mkdir $T/init

for tag in $tags master
do
	mkdir $T/$tag
	git reset --hard $tag 1>&2
	rm -rf cscope.* tests
	find . \( -name SCCS -prune \) -o \( -name .git -prune \) -o \( -name '*.[h]' -print \) | grep -v -e '-impl\.h$' | cscope -bkq -i -
	for i in $api
	do
		cscope -d -L -0 $i > $T/$tag/$i
		if test -s $T/$tag/$i
		then
			:
		else
			rm $T/$tag/$i
		fi
	done
	# The call_rcu() preceding v0.6.0 is to be ignored
	if test -d $T/v0.6.0
	then
		:
	else
		rm $T/$tag/call_rcu
	fi
	( ls $T/$tag; ls $T/$lasttag ) | sort | uniq -u |
		awk -v tag=$tag '{ print "\t<tr><td>" $1 "</td><td>" tag "</td></tr>" }'
	lasttag=$tag
done

