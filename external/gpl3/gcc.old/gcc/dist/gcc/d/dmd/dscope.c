
/* Compiler implementation of the D programming language
 * Copyright (C) 1999-2019 by The D Language Foundation, All Rights Reserved
 * written by Walter Bright
 * http://www.digitalmars.com
 * Distributed under the Boost Software License, Version 1.0.
 * http://www.boost.org/LICENSE_1_0.txt
 * https://github.com/D-Programming-Language/dmd/blob/master/src/scope.c
 */

#include "root/dsystem.h"               // strlen()
#include "root/root.h"
#include "root/rmem.h"
#include "root/speller.h"

#include "mars.h"
#include "init.h"
#include "identifier.h"
#include "scope.h"
#include "attrib.h"
#include "dsymbol.h"
#include "declaration.h"
#include "statement.h"
#include "aggregate.h"
#include "module.h"
#include "id.h"
#include "template.h"

Scope *Scope::freelist = NULL;

void allocFieldinit(Scope *sc, size_t dim)
{
    sc->fieldinit = (unsigned *)mem.xcalloc(sizeof(unsigned), dim);
    sc->fieldinit_dim = dim;
}

void freeFieldinit(Scope *sc)
{
    if (sc->fieldinit)
        mem.xfree(sc->fieldinit);
    sc->fieldinit = NULL;
    sc->fieldinit_dim = 0;
}

Scope *Scope::alloc()
{
    if (freelist)
    {
        Scope *s = freelist;
        freelist = s->enclosing;
        //printf("freelist %p\n", s);
        assert(s->flags & SCOPEfree);
        s->flags &= ~SCOPEfree;
        return s;
    }

    return new Scope();
}

Scope::Scope()
{
    // Create root scope

    //printf("Scope::Scope() %p\n", this);
    this->_module = NULL;
    this->scopesym = NULL;
    this->sds = NULL;
    this->enclosing = NULL;
    this->parent = NULL;
    this->sw = NULL;
    this->tf = NULL;
    this->os = NULL;
    this->tinst = NULL;
    this->minst = NULL;
    this->sbreak = NULL;
    this->scontinue = NULL;
    this->fes = NULL;
    this->callsc = NULL;
    this->aligndecl = NULL;
    this->func = NULL;
    this->slabel = NULL;
    this->linkage = LINKd;
    this->cppmangle = CPPMANGLEdefault;
    this->inlining = PINLINEdefault;
    this->protection = Prot(PROTpublic);
    this->explicitProtection = 0;
    this->stc = 0;
    this->depdecl = NULL;
    this->inunion = 0;
    this->nofree = 0;
    this->noctor = 0;
    this->intypeof = 0;
    this->lastVar = NULL;
    this->callSuper = 0;
    this->fieldinit = NULL;
    this->fieldinit_dim = 0;
    this->flags = 0;
    this->lastdc = NULL;
    this->anchorCounts = NULL;
    this->prevAnchor = NULL;
    this->userAttribDecl = NULL;
}

Scope *Scope::copy()
{
    Scope *sc = Scope::alloc();
    *sc = *this;    // memcpy

    /* Bugzilla 11777: The copied scope should not inherit fieldinit.
     */
    sc->fieldinit = NULL;

    return sc;
}

Scope *Scope::createGlobal(Module *_module)
{
    Scope *sc = Scope::alloc();
    *sc = Scope();  // memset

    sc->aligndecl = NULL;
    sc->linkage = LINKd;
    sc->inlining = PINLINEdefault;
    sc->protection = Prot(PROTpublic);

    sc->_module = _module;

    sc->tinst = NULL;
    sc->minst = _module;

    sc->scopesym = new ScopeDsymbol();
    sc->scopesym->symtab = new DsymbolTable();

    // Add top level package as member of this global scope
    Dsymbol *m = _module;
    while (m->parent)
        m = m->parent;
    m->addMember(NULL, sc->scopesym);
    m->parent = NULL;                   // got changed by addMember()

    // Create the module scope underneath the global scope
    sc = sc->push(_module);
    sc->parent = _module;
    return sc;
}

Scope *Scope::push()
{
    Scope *s = copy();

    //printf("Scope::push(this = %p) new = %p\n", this, s);
    assert(!(flags & SCOPEfree));
    s->scopesym = NULL;
    s->sds = NULL;
    s->enclosing = this;
    s->slabel = NULL;
    s->nofree = 0;
    s->fieldinit = saveFieldInit();
    s->flags = (flags & (SCOPEcontract | SCOPEdebug | SCOPEctfe | SCOPEcompile | SCOPEconstraint |
                         SCOPEnoaccesscheck | SCOPEignoresymbolvisibility));
    s->lastdc = NULL;

    assert(this != s);
    return s;
}

Scope *Scope::push(ScopeDsymbol *ss)
{
    //printf("Scope::push(%s)\n", ss->toChars());
    Scope *s = push();
    s->scopesym = ss;
    return s;
}

Scope *Scope::pop()
{
    //printf("Scope::pop() %p nofree = %d\n", this, nofree);
    Scope *enc = enclosing;

    if (enclosing)
    {
        enclosing->callSuper |= callSuper;
        if (fieldinit)
        {
            if (enclosing->fieldinit)
            {
                assert(fieldinit != enclosing->fieldinit);
                size_t dim = fieldinit_dim;
                for (size_t i = 0; i < dim; i++)
                    enclosing->fieldinit[i] |= fieldinit[i];
            }
            freeFieldinit(this);
        }
    }

    if (!nofree)
    {
        enclosing = freelist;
        freelist = this;
        flags |= SCOPEfree;
    }

    return enc;
}

Scope *Scope::startCTFE()
{
    Scope *sc = this->push();
    sc->flags = this->flags | SCOPEctfe;
    return sc;
}

Scope *Scope::endCTFE()
{
    assert(flags & SCOPEctfe);
    return pop();
}

void Scope::mergeCallSuper(Loc loc, unsigned cs)
{
    // This does a primitive flow analysis to support the restrictions
    // regarding when and how constructors can appear.
    // It merges the results of two paths.
    // The two paths are callSuper and cs; the result is merged into callSuper.

    if (cs != callSuper)
    {
        // Have ALL branches called a constructor?
        int aAll = (cs        & (CSXthis_ctor | CSXsuper_ctor)) != 0;
        int bAll = (callSuper & (CSXthis_ctor | CSXsuper_ctor)) != 0;

        // Have ANY branches called a constructor?
        bool aAny = (cs        & CSXany_ctor) != 0;
        bool bAny = (callSuper & CSXany_ctor) != 0;

        // Have any branches returned?
        bool aRet = (cs        & CSXreturn) != 0;
        bool bRet = (callSuper & CSXreturn) != 0;

        // Have any branches halted?
        bool aHalt = (cs        & CSXhalt) != 0;
        bool bHalt = (callSuper & CSXhalt) != 0;

        bool ok = true;

        if (aHalt && bHalt)
        {
            callSuper = CSXhalt;
        }
        else if ((!aHalt && aRet && !aAny && bAny) ||
                 (!bHalt && bRet && !bAny && aAny))
        {
            // If one has returned without a constructor call, there must be never
            // have been ctor calls in the other.
            ok = false;
        }
        else if (aHalt || (aRet && aAll))
        {
            // If one branch has called a ctor and then exited, anything the
            // other branch has done is OK (except returning without a
            // ctor call, but we already checked that).
            callSuper |= cs & (CSXany_ctor | CSXlabel);
        }
        else if (bHalt || (bRet && bAll))
        {
            callSuper = cs | (callSuper & (CSXany_ctor | CSXlabel));
        }
        else
        {
            // Both branches must have called ctors, or both not.
            ok = (aAll == bAll);
            // If one returned without a ctor, we must remember that
            // (Don't bother if we've already found an error)
            if (ok && aRet && !aAny)
                callSuper |= CSXreturn;
            callSuper |= cs & (CSXany_ctor | CSXlabel);
        }
        if (!ok)
            error(loc, "one path skips constructor");
    }
}

unsigned *Scope::saveFieldInit()
{
    unsigned *fi = NULL;
    if (fieldinit)  // copy
    {
        size_t dim = fieldinit_dim;
        fi = (unsigned *)mem.xmalloc(sizeof(unsigned) * dim);
        for (size_t i = 0; i < dim; i++)
            fi[i] = fieldinit[i];
    }
    return fi;
}

static bool mergeFieldInit(unsigned &fieldInit, unsigned fi, bool mustInit)
{
    if (fi != fieldInit)
    {
        // Have any branches returned?
        bool aRet = (fi        & CSXreturn) != 0;
        bool bRet = (fieldInit & CSXreturn) != 0;

        // Have any branches halted?
        bool aHalt = (fi        & CSXhalt) != 0;
        bool bHalt = (fieldInit & CSXhalt) != 0;

        bool ok;

        if (aHalt && bHalt)
        {
            ok = true;
            fieldInit = CSXhalt;
        }
        else if (!aHalt && aRet)
        {
            ok = !mustInit || (fi & CSXthis_ctor);
            fieldInit = fieldInit;
        }
        else if (!bHalt && bRet)
        {
            ok = !mustInit || (fieldInit & CSXthis_ctor);
            fieldInit = fi;
        }
        else if (aHalt)
        {
            ok = !mustInit || (fieldInit & CSXthis_ctor);
            fieldInit = fieldInit;
        }
        else if (bHalt)
        {
            ok = !mustInit || (fi & CSXthis_ctor);
            fieldInit = fi;
        }
        else
        {
            ok = !mustInit || !((fieldInit ^ fi) & CSXthis_ctor);
            fieldInit |= fi;
        }

        return ok;
    }
    return true;
}

void Scope::mergeFieldInit(Loc loc, unsigned *fies)
{
    if (fieldinit && fies)
    {
        FuncDeclaration *f = func;
        if (fes) f = fes->func;
        AggregateDeclaration *ad = f->isMember2();
        assert(ad);

        for (size_t i = 0; i < ad->fields.dim; i++)
        {
            VarDeclaration *v = ad->fields[i];
            bool mustInit = (v->storage_class & STCnodefaultctor ||
                             v->type->needsNested());

            if (!::mergeFieldInit(fieldinit[i], fies[i], mustInit))
            {
                ::error(loc, "one path skips field %s", ad->fields[i]->toChars());
            }
        }
    }
}

Module *Scope::instantiatingModule()
{
    // TODO: in speculative context, returning 'module' is correct?
    return minst ? minst : _module;
}

static Dsymbol *searchScopes(Scope *scope, Loc loc, Identifier *ident, Dsymbol **pscopesym, int flags)
{
    for (Scope *sc = scope; sc; sc = sc->enclosing)
    {
        assert(sc != sc->enclosing);
        if (!sc->scopesym)
            continue;
        //printf("\tlooking in scopesym '%s', kind = '%s', flags = x%x\n", sc->scopesym->toChars(), sc->scopesym->kind(), flags);

        if (sc->scopesym->isModule())
            flags |= SearchUnqualifiedModule;        // tell Module.search() that SearchLocalsOnly is to be obeyed

        if (Dsymbol *s = sc->scopesym->search(loc, ident, flags))
        {
            if (!(flags & (SearchImportsOnly | IgnoreErrors)) &&
                ident == Id::length && sc->scopesym->isArrayScopeSymbol() &&
                sc->enclosing && sc->enclosing->search(loc, ident, NULL, flags))
            {
                warning(s->loc, "array 'length' hides other 'length' name in outer scope");
            }
            if (pscopesym)
                *pscopesym = sc->scopesym;
            return s;
        }
        // Stop when we hit a module, but keep going if that is not just under the global scope
        if (sc->scopesym->isModule() && !(sc->enclosing && !sc->enclosing->enclosing))
            break;
    }
    return NULL;
}

/************************************
 * Perform unqualified name lookup by following the chain of scopes up
 * until found.
 *
 * Params:
 *  loc = location to use for error messages
 *  ident = name to look up
 *  pscopesym = if supplied and name is found, set to scope that ident was found in
 *  flags = modify search based on flags
 *
 * Returns:
 *  symbol if found, null if not
 */
Dsymbol *Scope::search(Loc loc, Identifier *ident, Dsymbol **pscopesym, int flags)
{
    // This function is called only for unqualified lookup
    assert(!(flags & (SearchLocalsOnly | SearchImportsOnly)));

    /* If ident is "start at module scope", only look at module scope
     */
    if (ident == Id::empty)
    {
        // Look for module scope
        for (Scope *sc = this; sc; sc = sc->enclosing)
        {
            assert(sc != sc->enclosing);
            if (!sc->scopesym)
                continue;

            if (Dsymbol *s = sc->scopesym->isModule())
            {
                if (pscopesym)
                    *pscopesym = sc->scopesym;
                return s;
            }
        }
        return NULL;
    }

    if (this->flags & SCOPEignoresymbolvisibility)
        flags |= IgnoreSymbolVisibility;

    Dsymbol *sold = NULL;
    if (global.params.bug10378 || global.params.check10378)
    {
        sold = searchScopes(this, loc, ident, pscopesym, flags | IgnoreSymbolVisibility);
        if (!global.params.check10378)
            return sold;

        if (ident == Id::dollar) // Bugzilla 15825
            return sold;

        // Search both ways
    }

    // First look in local scopes
    Dsymbol *s = searchScopes(this, loc, ident, pscopesym, flags | SearchLocalsOnly);
    if (!s)
    {
        // Second look in imported modules
        s = searchScopes(this, loc, ident, pscopesym, flags | SearchImportsOnly);
        /** Still find private symbols, so that symbols that weren't access
         * checked by the compiler remain usable.  Once the deprecation is over,
         * this should be moved to search_correct instead.
         */
        if (!s && !(flags & IgnoreSymbolVisibility))
        {
            s = searchScopes(this, loc, ident, pscopesym, flags | SearchLocalsOnly | IgnoreSymbolVisibility);
            if (!s)
                s = searchScopes(this, loc, ident, pscopesym, flags | SearchImportsOnly | IgnoreSymbolVisibility);

            if (s && !(flags & IgnoreErrors))
                ::deprecation(loc, "%s is not visible from module %s", s->toPrettyChars(), _module->toChars());
        }
    }

    if (global.params.check10378)
    {
        Dsymbol *snew = s;
        if (sold != snew)
            deprecation10378(loc, sold, snew);
        if (global.params.bug10378)
            s = sold;
    }
    return s;
}

Dsymbol *Scope::insert(Dsymbol *s)
{
    if (VarDeclaration *vd = s->isVarDeclaration())
    {
        if (lastVar)
            vd->lastVar = lastVar;
        lastVar = vd;
    }
    else if (WithScopeSymbol *ss = s->isWithScopeSymbol())
    {
        if (VarDeclaration *vd = ss->withstate->wthis)
        {
            if (lastVar)
                vd->lastVar = lastVar;
            lastVar = vd;
        }
        return NULL;
    }
    for (Scope *sc = this; sc; sc = sc->enclosing)
    {
        //printf("\tsc = %p\n", sc);
        if (sc->scopesym)
        {
            //printf("\t\tsc->scopesym = %p\n", sc->scopesym);
            if (!sc->scopesym->symtab)
                sc->scopesym->symtab = new DsymbolTable();
            return sc->scopesym->symtabInsert(s);
        }
    }
    assert(0);
    return NULL;
}

/********************************************
 * Search enclosing scopes for ClassDeclaration.
 */

ClassDeclaration *Scope::getClassScope()
{
    for (Scope *sc = this; sc; sc = sc->enclosing)
    {
        if (!sc->scopesym)
            continue;

        ClassDeclaration *cd = sc->scopesym->isClassDeclaration();
        if (cd)
            return cd;
    }
    return NULL;
}

/********************************************
 * Search enclosing scopes for ClassDeclaration.
 */

AggregateDeclaration *Scope::getStructClassScope()
{
    for (Scope *sc = this; sc; sc = sc->enclosing)
    {
        if (!sc->scopesym)
            continue;

        AggregateDeclaration *ad = sc->scopesym->isClassDeclaration();
        if (ad)
            return ad;
        ad = sc->scopesym->isStructDeclaration();
        if (ad)
            return ad;
    }
    return NULL;
}

/*******************************************
 * For TemplateDeclarations, we need to remember the Scope
 * where it was declared. So mark the Scope as not
 * to be free'd.
 */

void Scope::setNoFree()
{
    //int i = 0;

    //printf("Scope::setNoFree(this = %p)\n", this);
    for (Scope *sc = this; sc; sc = sc->enclosing)
    {
        //printf("\tsc = %p\n", sc);
        sc->nofree = 1;

        assert(!(flags & SCOPEfree));
        //assert(sc != sc->enclosing);
        //assert(!sc->enclosing || sc != sc->enclosing->enclosing);
        //if (++i == 10)
            //assert(0);
    }
}

structalign_t Scope::alignment()
{
    if (aligndecl)
        return aligndecl->getAlignment(this);
    else
        return STRUCTALIGN_DEFAULT;
}

/************************************************
 * Given the failed search attempt, try to find
 * one with a close spelling.
 */

void *scope_search_fp(void *arg, const char *seed, int* cost)
{
    //printf("scope_search_fp('%s')\n", seed);

    /* If not in the lexer's string table, it certainly isn't in the symbol table.
     * Doing this first is a lot faster.
     */
    size_t len = strlen(seed);
    if (!len)
        return NULL;
    Identifier *id = Identifier::lookup(seed, len);
    if (!id)
        return NULL;

    Scope *sc = (Scope *)arg;
    Module::clearCache();
    Dsymbol *scopesym = NULL;
    Dsymbol *s = sc->search(Loc(), id, &scopesym, IgnoreErrors);
    if (s)
    {
        for (*cost = 0; sc; sc = sc->enclosing, (*cost)++)
            if (sc->scopesym == scopesym)
                break;
        if (scopesym != s->parent)
        {
            (*cost)++; // got to the symbol through an import
            if (s->prot().kind == PROTprivate)
                return NULL;
        }
    }
    return (void*)s;
}

void Scope::deprecation10378(Loc loc, Dsymbol *sold, Dsymbol *snew)
{
    // Bugzilla 15857
    //
    // The overloadset found via the new lookup rules is either
    // equal or a subset of the overloadset found via the old
    // lookup rules, so it suffices to compare the dimension to
    // check for equality.
    OverloadSet *osold = NULL;
    OverloadSet *osnew = NULL;
    if (sold && (osold = sold->isOverloadSet()) != NULL &&
        snew && (osnew = snew->isOverloadSet()) != NULL &&
        osold->a.dim == osnew->a.dim)
        return;

    OutBuffer buf;
    buf.writestring("local import search method found ");
    if (osold)
        buf.printf("%s %s (%d overloads)", sold->kind(), sold->toPrettyChars(), (int)osold->a.dim);
    else if (sold)
        buf.printf("%s %s", sold->kind(), sold->toPrettyChars());
    else
        buf.writestring("nothing");
    buf.writestring(" instead of ");
    if (osnew)
        buf.printf("%s %s (%d overloads)", snew->kind(), snew->toPrettyChars(), (int)osnew->a.dim);
    else if (snew)
        buf.printf("%s %s", snew->kind(), snew->toPrettyChars());
    else
        buf.writestring("nothing");

    deprecation(loc, "%s", buf.peekString());
}

Dsymbol *Scope::search_correct(Identifier *ident)
{
    if (global.gag)
        return NULL;            // don't do it for speculative compiles; too time consuming

    return (Dsymbol *)speller(ident->toChars(), &scope_search_fp, this, idchars);
}

/************************************
 * Maybe `ident` was a C or C++ name. Check for that,
 * and suggest the D equivalent.
 * Params:
 *  ident = unknown identifier
 * Returns:
 *  D identifier string if found, null if not
 */
const char *Scope::search_correct_C(Identifier *ident)
{
    TOK tok;
    if (ident == Id::C_NULL)
        tok = TOKnull;
    else if (ident == Id::C_TRUE)
        tok = TOKtrue;
    else if (ident == Id::C_FALSE)
        tok = TOKfalse;
    else if (ident == Id::C_unsigned)
        tok = TOKuns32;
    else if (ident == Id::C_wchar_t)
        tok = global.params.isWindows ? TOKwchar : TOKdchar;
    else
        return NULL;
    return Token::toChars(tok);
}
