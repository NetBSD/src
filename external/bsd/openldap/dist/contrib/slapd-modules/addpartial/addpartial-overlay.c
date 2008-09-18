/**
 * $Id: addpartial-overlay.c,v 1.1.1.1.6.2 2008/09/18 05:14:35 wrstuden Exp $
 *
 * Copyright (C) 2004 Virginia Tech, David Hawes.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * http://www.OpenLDAP.org/license.html.
 *
 * SEE LICENSE FOR MORE INFORMATION
 *
 * Author:  David H. Hawes, Jr.
 * Email:   dhawes@vt.edu
 * Version: $Revision: 1.1.1.1.6.2 $
 * Updated: $Date: 2008/09/18 05:14:35 $
 * 
 * addpartial-overlay
 *
 * This is an OpenLDAP overlay that intercepts ADD requests, determines if a
 * change has actually taken place for that record, and then performs a modify
 * request for those values that have changed (modified, added, deleted).  If
 * the record has not changed in any way, it is ignored.  If the record does not
 * exist, the record falls through to the normal add mechanism.  This overlay is
 * useful for replicating from sources that are not LDAPs where it is easier to
 * build entire records than to determine the changes (i.e. a database). 
 */

#include "portable.h" 
#include "slap.h"

static int addpartial_search_cb( Operation *op, SlapReply *rs);
static int collect_error_msg_cb( Operation *op, SlapReply *rs);

static slap_overinst addpartial;

/**
 *  The meat of the overlay.  Search for the record, determine changes, take
 *  action or fall through.
 */
static int addpartial_add( Operation *op, SlapReply *rs)
{
    Operation nop = *op;
    SlapReply nrs = { REP_RESULT };
    Filter *filter = NULL;
    Entry *toAdd = NULL;
    struct berval fstr = BER_BVNULL;
    slap_callback cb = { NULL, addpartial_search_cb, NULL, NULL };
    slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
    int rc;

    toAdd = op->oq_add.rs_e;

    Debug(LDAP_DEBUG_TRACE, "%s: toAdd->e_nname.bv_val: %s\n",
          addpartial.on_bi.bi_type, toAdd->e_nname.bv_val,0);

    /* if the user doesn't have access, fall through to the normal ADD */
    if(!access_allowed(op, toAdd, slap_schema.si_ad_entry,
                       NULL, ACL_WRITE, NULL))
    {
        return SLAP_CB_CONTINUE;
    }
    
    rs->sr_text = NULL;

    nop.o_callback = &cb;
    op->o_bd->bd_info = (BackendInfo *) on->on_info;
    nop.o_tag = LDAP_REQ_SEARCH;
    nop.o_ctrls = NULL;
    
    filter = str2filter("(objectclass=*)");
    filter2bv(filter, &fstr);

    nop.ors_scope = LDAP_SCOPE_BASE;
    nop.ors_deref = LDAP_DEREF_NEVER;
    nop.ors_slimit = -1;//SLAP_NO_LIMIT;
    nop.ors_tlimit = -1;//SLAP_NO_LIMIT;
    nop.ors_attrsonly = 0;
    nop.ors_attrs = slap_anlist_no_attrs;
    nop.ors_filter = filter;
    nop.ors_filterstr = fstr;

    memset(&nrs, 0, sizeof(nrs));
    nrs.sr_type = REP_RESULT;
    nrs.sr_err = LDAP_SUCCESS;
    nrs.sr_entry = NULL;
    nrs.sr_flags |= REP_ENTRY_MUSTBEFREED;
    nrs.sr_text = NULL;

    Debug(LDAP_DEBUG_TRACE, "%s: performing search\n", addpartial.on_bi.bi_type,
          0,0);

    if(nop.o_bd->be_search)
    {
        rc = nop.o_bd->be_search(&nop, &nrs);
        Debug(LDAP_DEBUG_TRACE, "%s: search performed\n",
              addpartial.on_bi.bi_type,0,0);
    }
    else
    {
        Debug(LDAP_DEBUG_TRACE, "%s: backend missing search function\n",
              addpartial.on_bi.bi_type,0,0);
    }

    if(filter)
        filter_free(filter);
    if(fstr.bv_val)
        ch_free(fstr.bv_val);

    if(rc != LDAP_SUCCESS)
        return SLAP_CB_CONTINUE;
    else
    { 
        Entry *found = NULL;
        Debug(LDAP_DEBUG_TRACE, "%s: found the dn\n", addpartial.on_bi.bi_type,
              0,0);
        found = (Entry *) cb.sc_private;

        if(found)
        {
            Attribute *attr = NULL;
            Attribute *at = NULL;
            int ret;
            Modifications *mods = NULL;
            Modifications **modtail = &mods;
            Modifications *mod = NULL;

            Debug(LDAP_DEBUG_TRACE, "%s: have an entry!\n",
                  addpartial.on_bi.bi_type,0,0);

           /* determine if the changes are in the found entry */ 
            for(attr = toAdd->e_attrs; attr; attr = attr->a_next)
            {
                if(attr->a_desc->ad_type->sat_atype.at_usage != 0) continue;

                at = attr_find(found->e_attrs, attr->a_desc);
                if(!at)
                {
                    Debug(LDAP_DEBUG_TRACE, "%s: Attribute %s not found!\n",
                          addpartial.on_bi.bi_type,
                          attr->a_desc->ad_cname.bv_val,0);
                    mod = (Modifications *) ch_malloc(sizeof(
                                                            Modifications));
                    mod->sml_flags = 0;
                    mod->sml_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
                    mod->sml_op &= LDAP_MOD_OP;
                    mod->sml_next = NULL;
                    mod->sml_desc = attr->a_desc;
                    mod->sml_type.bv_val = attr->a_desc->ad_cname.bv_val;
                    mod->sml_type.bv_len = strlen(mod->sml_type.bv_val);
                    mod->sml_values = attr->a_vals;
                    mod->sml_nvalues = attr->a_nvals;
                    mod->sml_numvals = attr->a_numvals;
                    *modtail = mod;
                    modtail = &mod->sml_next;
                }
                else
                {
                    MatchingRule *mr = attr->a_desc->ad_type->sat_equality;
                    struct berval *bv;
                    const char *text;
                    int acount , bcount;
                    Debug(LDAP_DEBUG_TRACE, "%s: Attribute %s found\n",
                          addpartial.on_bi.bi_type,
                          attr->a_desc->ad_cname.bv_val,0);

                    for(bv = attr->a_vals, acount = 0; bv->bv_val != NULL; 
                        bv++, acount++)
                    {
                        /* count num values for attr */
                    }
                    for(bv = at->a_vals, bcount = 0; bv->bv_val != NULL; 
                        bv++, bcount++)
                    {
                        /* count num values for attr */
                    }
                    if(acount != bcount)
                    {
                        Debug(LDAP_DEBUG_TRACE, "%s: acount != bcount, %s\n",
                              addpartial.on_bi.bi_type,
                              "replace all",0);
                        mod = (Modifications *) ch_malloc(sizeof(
                                                                Modifications));
                        mod->sml_flags = 0;
                        mod->sml_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
                        mod->sml_op &= LDAP_MOD_OP;
                        mod->sml_next = NULL;
                        mod->sml_desc = attr->a_desc;
                        mod->sml_type.bv_val = attr->a_desc->ad_cname.bv_val;
                        mod->sml_type.bv_len = strlen(mod->sml_type.bv_val);
                        mod->sml_values = attr->a_vals;
                        mod->sml_nvalues = attr->a_nvals;
                        mod->sml_numvals = attr->a_numvals;
                        *modtail = mod;
                        modtail = &mod->sml_next;
                        continue;
                    }
                    
                    for(bv = attr->a_vals; bv->bv_val != NULL; bv++)
                    {
                        struct berval *v;
                        ret = -1;
                        
                        for(v = at->a_vals; v->bv_val != NULL; v++)
                        {
                            int r;
                            if(mr && ((r = value_match(&ret, attr->a_desc, mr,
                                           SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
                                           bv, v, &text)) == 0))
                            {
                                if(ret == 0)
                                    break;
                            }
                            else
                            {
                                Debug(LDAP_DEBUG_TRACE,
                                      "%s: \tvalue DNE, r: %d \n",
                                      addpartial.on_bi.bi_type,
                                      r,0);
                                ret = strcmp(bv->bv_val, v->bv_val);
                                if(ret == 0)
                                    break;
                            }
                        }

                        if(ret == 0)
                        {
                            Debug(LDAP_DEBUG_TRACE,
                                  "%s: \tvalue %s exists, ret: %d\n",
                                  addpartial.on_bi.bi_type, bv->bv_val, ret);
                        }
                        else
                        {
                            Debug(LDAP_DEBUG_TRACE,
                                  "%s: \tvalue %s DNE, ret: %d\n",
                                  addpartial.on_bi.bi_type, bv->bv_val, ret);
                            mod = (Modifications *) ch_malloc(sizeof(
                                                                Modifications));
                            mod->sml_flags = 0;
                            mod->sml_op = LDAP_MOD_REPLACE | LDAP_MOD_BVALUES;
                            mod->sml_op &= LDAP_MOD_OP;
                            mod->sml_next = NULL;
                            mod->sml_desc = attr->a_desc;
                            mod->sml_type.bv_val = 
                                                  attr->a_desc->ad_cname.bv_val;
                            mod->sml_type.bv_len = strlen(mod->sml_type.bv_val);
                            mod->sml_values = attr->a_vals;
                            mod->sml_nvalues = attr->a_nvals;
                            mod->sml_numvals = attr->a_numvals;
                            *modtail = mod;
                            modtail = &mod->sml_next;
                            break;
                        }
                    }
                }
            }

            /* determine if any attributes were deleted */
            for(attr = found->e_attrs; attr; attr = attr->a_next)
            {
                if(attr->a_desc->ad_type->sat_atype.at_usage != 0) continue;

                at = NULL;
                at = attr_find(toAdd->e_attrs, attr->a_desc);
                if(!at)
                {
                    Debug(LDAP_DEBUG_TRACE,
                          "%s: Attribute %s not found in new entry!!!\n",
                          addpartial.on_bi.bi_type,
                          attr->a_desc->ad_cname.bv_val, 0);
                    mod = (Modifications *) ch_malloc(sizeof(
                                                        Modifications));
                    mod->sml_flags = 0;
                    mod->sml_op = LDAP_MOD_REPLACE;
                    mod->sml_next = NULL;
                    mod->sml_desc = attr->a_desc;
                    mod->sml_type.bv_val = 
                                          attr->a_desc->ad_cname.bv_val;
                    mod->sml_type.bv_len = strlen(mod->sml_type.bv_val);
                    mod->sml_values = NULL;
                    mod->sml_nvalues = NULL;
                    mod->sml_numvals = 0;
                    *modtail = mod;
                    modtail = &mod->sml_next;
                }
                else
                {
                    Debug(LDAP_DEBUG_TRACE,
                          "%s: Attribute %s found in new entry\n",
                          addpartial.on_bi.bi_type,
                          at->a_desc->ad_cname.bv_val, 0);
                }
            }

            if(mods)
            {
                Debug(LDAP_DEBUG_TRACE, "%s: mods to do...\n",
                      addpartial.on_bi.bi_type, 0, 0);
                if(nop.o_bd->be_modify)
                {
                    Modifications *m = NULL;
                    int modcount;
                    slap_callback nullcb = { NULL, collect_error_msg_cb, 
                                             NULL, NULL };
                    char textbuf[SLAP_TEXT_BUFLEN];
                    size_t textlen = sizeof textbuf;

                    memset(&nrs, 0, sizeof(nrs));
                    nrs.sr_type = REP_RESULT;
                    nrs.sr_err = LDAP_SUCCESS;
                    nrs.sr_entry = NULL;
                    nrs.sr_text = NULL;

                    nop.o_tag = LDAP_REQ_MODIFY;
                    nop.orm_modlist = mods;
                    nop.o_callback = &nullcb;
                    nop.o_bd->bd_info = (BackendInfo *) on->on_info;

                    for(m = mods, modcount = 0; m; m = m->sml_next, 
                        modcount++)
                    {
                        /* count number of mods */
                    }

                    Debug(LDAP_DEBUG_TRACE, "%s: number of mods: %d\n",
                          addpartial.on_bi.bi_type, modcount, 0);

                    rc = (nop.o_bd->be_modify)(&nop, &nrs);

                    if(rc == LDAP_SUCCESS)
                    {
                        Debug(LDAP_DEBUG_TRACE,
                              "%s: modify successful\n",
                              addpartial.on_bi.bi_type, 0, 0);
                    }
                    else
                    {
                        Debug(LDAP_DEBUG_TRACE, "%s: modify unsuccessful: %d\n",
                              addpartial.on_bi.bi_type, rc, 0);
                        rs->sr_err = rc;
                        if(nrs.sr_text)
                        {
                            rs->sr_text = nullcb.sc_private;
                        }
                    }

                    Debug(LDAP_DEBUG_TRACE, "%s: freeing mods...\n",
                          addpartial.on_bi.bi_type, 0, 0);

                    if(mods != NULL)
                    {
                        Modifications *toDel;

                        for(toDel = mods; toDel; toDel = mods)
                        {
                            mods = mods->sml_next;
                            ch_free(toDel);
                        }
                    }
                }
            }
            else
            {
                Debug(LDAP_DEBUG_TRACE, "%s: no mods to process\n",
                      addpartial.on_bi.bi_type, 0, 0);
            }

            if(found != NULL)
                entry_free(found);
        }
        else
        {
            Debug(LDAP_DEBUG_TRACE, "%s: no entry!\n",
                  addpartial.on_bi.bi_type, 0, 0);
        }

        op->o_callback = NULL;
        send_ldap_result( op, rs );
        ch_free((void *)rs->sr_text);
        rs->sr_text = NULL;

        return LDAP_SUCCESS;
    }
}

static int addpartial_search_cb( Operation *op, SlapReply *rs)
{
    Entry *entry = NULL;

    if(rs->sr_type != REP_SEARCH) return 0;
        
    Debug(LDAP_DEBUG_TRACE, "%s: addpartial_search_cb\n",
          addpartial.on_bi.bi_type, 0, 0);

    if(rs->sr_entry)
    {
        Debug(LDAP_DEBUG_TRACE, "%s: dn found: %s\n",
              addpartial.on_bi.bi_type, rs->sr_entry->e_nname.bv_val, 0);
        entry = rs->sr_entry;
        op->o_callback->sc_private = (void *) entry_dup(entry);
    }

    return 0;
}

static int collect_error_msg_cb( Operation *op, SlapReply *rs)
{
    if(rs->sr_text)
    {
        op->o_callback->sc_private = (void *) ch_strdup(rs->sr_text);
    }

    return LDAP_SUCCESS;
}

int addpartial_init() 
{
    addpartial.on_bi.bi_type = "addpartial";
    addpartial.on_bi.bi_op_add = addpartial_add;

    return (overlay_register(&addpartial));
}

int init_module(int argc, char *argv[]) 
{
        return addpartial_init();
}
