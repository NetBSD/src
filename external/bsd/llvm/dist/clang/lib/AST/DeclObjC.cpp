//===--- DeclObjC.cpp - ObjC Declaration AST Node Implementation ----------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Objective-C related Decl classes.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/DeclObjC.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTMutationListener.h"
#include "clang/AST/Attr.h"
#include "clang/AST/Stmt.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallString.h"
using namespace clang;

//===----------------------------------------------------------------------===//
// ObjCListBase
//===----------------------------------------------------------------------===//

void ObjCListBase::set(void *const* InList, unsigned Elts, ASTContext &Ctx) {
  List = 0;
  if (Elts == 0) return;  // Setting to an empty list is a noop.


  List = new (Ctx) void*[Elts];
  NumElts = Elts;
  memcpy(List, InList, sizeof(void*)*Elts);
}

void ObjCProtocolList::set(ObjCProtocolDecl* const* InList, unsigned Elts, 
                           const SourceLocation *Locs, ASTContext &Ctx) {
  if (Elts == 0)
    return;

  Locations = new (Ctx) SourceLocation[Elts];
  memcpy(Locations, Locs, sizeof(SourceLocation) * Elts);
  set(InList, Elts, Ctx);
}

//===----------------------------------------------------------------------===//
// ObjCInterfaceDecl
//===----------------------------------------------------------------------===//

void ObjCContainerDecl::anchor() { }

/// getIvarDecl - This method looks up an ivar in this ContextDecl.
///
ObjCIvarDecl *
ObjCContainerDecl::getIvarDecl(IdentifierInfo *Id) const {
  lookup_const_result R = lookup(Id);
  for (lookup_const_iterator Ivar = R.begin(), IvarEnd = R.end();
       Ivar != IvarEnd; ++Ivar) {
    if (ObjCIvarDecl *ivar = dyn_cast<ObjCIvarDecl>(*Ivar))
      return ivar;
  }
  return 0;
}

// Get the local instance/class method declared in this interface.
ObjCMethodDecl *
ObjCContainerDecl::getMethod(Selector Sel, bool isInstance,
                             bool AllowHidden) const {
  // If this context is a hidden protocol definition, don't find any
  // methods there.
  if (const ObjCProtocolDecl *Proto = dyn_cast<ObjCProtocolDecl>(this)) {
    if (const ObjCProtocolDecl *Def = Proto->getDefinition())
      if (Def->isHidden() && !AllowHidden)
        return 0;
  }

  // Since instance & class methods can have the same name, the loop below
  // ensures we get the correct method.
  //
  // @interface Whatever
  // - (int) class_method;
  // + (float) class_method;
  // @end
  //
  lookup_const_result R = lookup(Sel);
  for (lookup_const_iterator Meth = R.begin(), MethEnd = R.end();
       Meth != MethEnd; ++Meth) {
    ObjCMethodDecl *MD = dyn_cast<ObjCMethodDecl>(*Meth);
    if (MD && MD->isInstanceMethod() == isInstance)
      return MD;
  }
  return 0;
}

/// HasUserDeclaredSetterMethod - This routine returns 'true' if a user declared setter
/// method was found in the class, its protocols, its super classes or categories.
/// It also returns 'true' if one of its categories has declared a 'readwrite' property.
/// This is because, user must provide a setter method for the category's 'readwrite'
/// property.
bool
ObjCContainerDecl::HasUserDeclaredSetterMethod(const ObjCPropertyDecl *Property) const {
  Selector Sel = Property->getSetterName();
  lookup_const_result R = lookup(Sel);
  for (lookup_const_iterator Meth = R.begin(), MethEnd = R.end();
       Meth != MethEnd; ++Meth) {
    ObjCMethodDecl *MD = dyn_cast<ObjCMethodDecl>(*Meth);
    if (MD && MD->isInstanceMethod() && !MD->isImplicit())
      return true;
  }

  if (const ObjCInterfaceDecl *ID = dyn_cast<ObjCInterfaceDecl>(this)) {
    // Also look into categories, including class extensions, looking
    // for a user declared instance method.
    for (ObjCInterfaceDecl::visible_categories_iterator
         Cat = ID->visible_categories_begin(),
         CatEnd = ID->visible_categories_end();
         Cat != CatEnd;
         ++Cat) {
      if (ObjCMethodDecl *MD = Cat->getInstanceMethod(Sel))
        if (!MD->isImplicit())
          return true;
      if (Cat->IsClassExtension())
        continue;
      // Also search through the categories looking for a 'readwrite' declaration
      // of this property. If one found, presumably a setter will be provided
      // (properties declared in categories will not get auto-synthesized).
      for (ObjCContainerDecl::prop_iterator P = Cat->prop_begin(),
           E = Cat->prop_end(); P != E; ++P)
        if (P->getIdentifier() == Property->getIdentifier()) {
          if (P->getPropertyAttributes() & ObjCPropertyDecl::OBJC_PR_readwrite)
            return true;
          break;
        }
    }
    
    // Also look into protocols, for a user declared instance method.
    for (ObjCInterfaceDecl::all_protocol_iterator P =
         ID->all_referenced_protocol_begin(),
         PE = ID->all_referenced_protocol_end(); P != PE; ++P) {
      ObjCProtocolDecl *Proto = (*P);
      if (Proto->HasUserDeclaredSetterMethod(Property))
        return true;
    }
    // And in its super class.
    ObjCInterfaceDecl *OSC = ID->getSuperClass();
    while (OSC) {
      if (OSC->HasUserDeclaredSetterMethod(Property))
        return true;
      OSC = OSC->getSuperClass();
    }
  }
  if (const ObjCProtocolDecl *PD = dyn_cast<ObjCProtocolDecl>(this))
    for (ObjCProtocolDecl::protocol_iterator PI = PD->protocol_begin(),
         E = PD->protocol_end(); PI != E; ++PI) {
      if ((*PI)->HasUserDeclaredSetterMethod(Property))
        return true;
    }
  return false;
}

ObjCPropertyDecl *
ObjCPropertyDecl::findPropertyDecl(const DeclContext *DC,
                                   IdentifierInfo *propertyID) {
  // If this context is a hidden protocol definition, don't find any
  // property.
  if (const ObjCProtocolDecl *Proto = dyn_cast<ObjCProtocolDecl>(DC)) {
    if (const ObjCProtocolDecl *Def = Proto->getDefinition())
      if (Def->isHidden())
        return 0;
  }

  DeclContext::lookup_const_result R = DC->lookup(propertyID);
  for (DeclContext::lookup_const_iterator I = R.begin(), E = R.end(); I != E;
       ++I)
    if (ObjCPropertyDecl *PD = dyn_cast<ObjCPropertyDecl>(*I))
      return PD;

  return 0;
}

IdentifierInfo *
ObjCPropertyDecl::getDefaultSynthIvarName(ASTContext &Ctx) const {
  SmallString<128> ivarName;
  {
    llvm::raw_svector_ostream os(ivarName);
    os << '_' << getIdentifier()->getName();
  }
  return &Ctx.Idents.get(ivarName.str());
}

/// FindPropertyDeclaration - Finds declaration of the property given its name
/// in 'PropertyId' and returns it. It returns 0, if not found.
ObjCPropertyDecl *
ObjCContainerDecl::FindPropertyDeclaration(IdentifierInfo *PropertyId) const {
  // Don't find properties within hidden protocol definitions.
  if (const ObjCProtocolDecl *Proto = dyn_cast<ObjCProtocolDecl>(this)) {
    if (const ObjCProtocolDecl *Def = Proto->getDefinition())
      if (Def->isHidden())
        return 0;
  }

  if (ObjCPropertyDecl *PD =
        ObjCPropertyDecl::findPropertyDecl(cast<DeclContext>(this), PropertyId))
    return PD;

  switch (getKind()) {
    default:
      break;
    case Decl::ObjCProtocol: {
      const ObjCProtocolDecl *PID = cast<ObjCProtocolDecl>(this);
      for (ObjCProtocolDecl::protocol_iterator I = PID->protocol_begin(),
           E = PID->protocol_end(); I != E; ++I)
        if (ObjCPropertyDecl *P = (*I)->FindPropertyDeclaration(PropertyId))
          return P;
      break;
    }
    case Decl::ObjCInterface: {
      const ObjCInterfaceDecl *OID = cast<ObjCInterfaceDecl>(this);
      // Look through categories (but not extensions).
      for (ObjCInterfaceDecl::visible_categories_iterator
             Cat = OID->visible_categories_begin(),
             CatEnd = OID->visible_categories_end();
           Cat != CatEnd; ++Cat) {
        if (!Cat->IsClassExtension())
          if (ObjCPropertyDecl *P = Cat->FindPropertyDeclaration(PropertyId))
            return P;
      }

      // Look through protocols.
      for (ObjCInterfaceDecl::all_protocol_iterator
            I = OID->all_referenced_protocol_begin(),
            E = OID->all_referenced_protocol_end(); I != E; ++I)
        if (ObjCPropertyDecl *P = (*I)->FindPropertyDeclaration(PropertyId))
          return P;

      // Finally, check the super class.
      if (const ObjCInterfaceDecl *superClass = OID->getSuperClass())
        return superClass->FindPropertyDeclaration(PropertyId);
      break;
    }
    case Decl::ObjCCategory: {
      const ObjCCategoryDecl *OCD = cast<ObjCCategoryDecl>(this);
      // Look through protocols.
      if (!OCD->IsClassExtension())
        for (ObjCCategoryDecl::protocol_iterator
              I = OCD->protocol_begin(), E = OCD->protocol_end(); I != E; ++I)
        if (ObjCPropertyDecl *P = (*I)->FindPropertyDeclaration(PropertyId))
          return P;

      break;
    }
  }
  return 0;
}

void ObjCInterfaceDecl::anchor() { }

/// FindPropertyVisibleInPrimaryClass - Finds declaration of the property
/// with name 'PropertyId' in the primary class; including those in protocols
/// (direct or indirect) used by the primary class.
///
ObjCPropertyDecl *
ObjCInterfaceDecl::FindPropertyVisibleInPrimaryClass(
                                            IdentifierInfo *PropertyId) const {
  // FIXME: Should make sure no callers ever do this.
  if (!hasDefinition())
    return 0;
  
  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  if (ObjCPropertyDecl *PD =
      ObjCPropertyDecl::findPropertyDecl(cast<DeclContext>(this), PropertyId))
    return PD;

  // Look through protocols.
  for (ObjCInterfaceDecl::all_protocol_iterator
        I = all_referenced_protocol_begin(),
        E = all_referenced_protocol_end(); I != E; ++I)
    if (ObjCPropertyDecl *P = (*I)->FindPropertyDeclaration(PropertyId))
      return P;

  return 0;
}

void ObjCInterfaceDecl::collectPropertiesToImplement(PropertyMap &PM,
                                                     PropertyDeclOrder &PO) const {
  for (ObjCContainerDecl::prop_iterator P = prop_begin(),
      E = prop_end(); P != E; ++P) {
    ObjCPropertyDecl *Prop = *P;
    PM[Prop->getIdentifier()] = Prop;
    PO.push_back(Prop);
  }
  for (ObjCInterfaceDecl::all_protocol_iterator
      PI = all_referenced_protocol_begin(),
      E = all_referenced_protocol_end(); PI != E; ++PI)
    (*PI)->collectPropertiesToImplement(PM, PO);
  // Note, the properties declared only in class extensions are still copied
  // into the main @interface's property list, and therefore we don't
  // explicitly, have to search class extension properties.
}

bool ObjCInterfaceDecl::isArcWeakrefUnavailable() const {
  const ObjCInterfaceDecl *Class = this;
  while (Class) {
    if (Class->hasAttr<ArcWeakrefUnavailableAttr>())
      return true;
    Class = Class->getSuperClass();
  }
  return false;
}

const ObjCInterfaceDecl *ObjCInterfaceDecl::isObjCRequiresPropertyDefs() const {
  const ObjCInterfaceDecl *Class = this;
  while (Class) {
    if (Class->hasAttr<ObjCRequiresPropertyDefsAttr>())
      return Class;
    Class = Class->getSuperClass();
  }
  return 0;
}

void ObjCInterfaceDecl::mergeClassExtensionProtocolList(
                              ObjCProtocolDecl *const* ExtList, unsigned ExtNum,
                              ASTContext &C)
{
  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  if (data().AllReferencedProtocols.empty() && 
      data().ReferencedProtocols.empty()) {
    data().AllReferencedProtocols.set(ExtList, ExtNum, C);
    return;
  }
  
  // Check for duplicate protocol in class's protocol list.
  // This is O(n*m). But it is extremely rare and number of protocols in
  // class or its extension are very few.
  SmallVector<ObjCProtocolDecl*, 8> ProtocolRefs;
  for (unsigned i = 0; i < ExtNum; i++) {
    bool protocolExists = false;
    ObjCProtocolDecl *ProtoInExtension = ExtList[i];
    for (all_protocol_iterator
          p = all_referenced_protocol_begin(),
          e = all_referenced_protocol_end(); p != e; ++p) {
      ObjCProtocolDecl *Proto = (*p);
      if (C.ProtocolCompatibleWithProtocol(ProtoInExtension, Proto)) {
        protocolExists = true;
        break;
      }      
    }
    // Do we want to warn on a protocol in extension class which
    // already exist in the class? Probably not.
    if (!protocolExists)
      ProtocolRefs.push_back(ProtoInExtension);
  }

  if (ProtocolRefs.empty())
    return;

  // Merge ProtocolRefs into class's protocol list;
  for (all_protocol_iterator p = all_referenced_protocol_begin(), 
        e = all_referenced_protocol_end(); p != e; ++p) {
    ProtocolRefs.push_back(*p);
  }

  data().AllReferencedProtocols.set(ProtocolRefs.data(), ProtocolRefs.size(),C);
}

const ObjCInterfaceDecl *
ObjCInterfaceDecl::findInterfaceWithDesignatedInitializers() const {
  const ObjCInterfaceDecl *IFace = this;
  while (IFace) {
    if (IFace->hasDesignatedInitializers())
      return IFace;
    if (!IFace->inheritsDesignatedInitializers())
      break;
    IFace = IFace->getSuperClass();
  }
  return 0;
}

bool ObjCInterfaceDecl::inheritsDesignatedInitializers() const {
  switch (data().InheritedDesignatedInitializers) {
  case DefinitionData::IDI_Inherited:
    return true;
  case DefinitionData::IDI_NotInherited:
    return false;
  case DefinitionData::IDI_Unknown: {
    bool isIntroducingInitializers = false;
    for (instmeth_iterator I = instmeth_begin(),
                           E = instmeth_end(); I != E; ++I) {
      const ObjCMethodDecl *MD = *I;
      if (MD->getMethodFamily() == OMF_init && !MD->isOverriding()) {
        isIntroducingInitializers = true;
        break;
      }
    }
    // If the class introduced initializers we conservatively assume that we
    // don't know if any of them is a designated initializer to avoid possible
    // misleading warnings.
    if (isIntroducingInitializers) {
      data().InheritedDesignatedInitializers = DefinitionData::IDI_NotInherited;
      return false;
    } else {
      data().InheritedDesignatedInitializers = DefinitionData::IDI_Inherited;
      return true;
    }
  }
  }

  llvm_unreachable("unexpected InheritedDesignatedInitializers value");
}

void ObjCInterfaceDecl::getDesignatedInitializers(
    llvm::SmallVectorImpl<const ObjCMethodDecl *> &Methods) const {
  assert(hasDefinition());
  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  const ObjCInterfaceDecl *IFace= findInterfaceWithDesignatedInitializers();
  if (!IFace)
    return;

  for (instmeth_iterator I = IFace->instmeth_begin(),
                         E = IFace->instmeth_end(); I != E; ++I) {
    const ObjCMethodDecl *MD = *I;
    if (MD->isThisDeclarationADesignatedInitializer())
      Methods.push_back(MD);
  }
}

bool ObjCInterfaceDecl::isDesignatedInitializer(Selector Sel,
                                      const ObjCMethodDecl **InitMethod) const {
  assert(hasDefinition());
  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  const ObjCInterfaceDecl *IFace= findInterfaceWithDesignatedInitializers();
  if (!IFace)
    return false;

  if (const ObjCMethodDecl *MD = IFace->getMethod(Sel, /*isInstance=*/true)) {
    if (MD->isThisDeclarationADesignatedInitializer()) {
      if (InitMethod)
        *InitMethod = MD;
      return true;
    }
  }
  return false;
}

void ObjCInterfaceDecl::allocateDefinitionData() {
  assert(!hasDefinition() && "ObjC class already has a definition");
  Data.setPointer(new (getASTContext()) DefinitionData());
  Data.getPointer()->Definition = this;

  // Make the type point at the definition, now that we have one.
  if (TypeForDecl)
    cast<ObjCInterfaceType>(TypeForDecl)->Decl = this;
}

void ObjCInterfaceDecl::startDefinition() {
  allocateDefinitionData();

  // Update all of the declarations with a pointer to the definition.
  for (redecl_iterator RD = redecls_begin(), RDEnd = redecls_end();
       RD != RDEnd; ++RD) {
    if (*RD != this)
      RD->Data = Data;
  }
}

ObjCIvarDecl *ObjCInterfaceDecl::lookupInstanceVariable(IdentifierInfo *ID,
                                              ObjCInterfaceDecl *&clsDeclared) {
  // FIXME: Should make sure no callers ever do this.
  if (!hasDefinition())
    return 0;  

  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  ObjCInterfaceDecl* ClassDecl = this;
  while (ClassDecl != NULL) {
    if (ObjCIvarDecl *I = ClassDecl->getIvarDecl(ID)) {
      clsDeclared = ClassDecl;
      return I;
    }

    for (ObjCInterfaceDecl::visible_extensions_iterator
           Ext = ClassDecl->visible_extensions_begin(),
           ExtEnd = ClassDecl->visible_extensions_end();
         Ext != ExtEnd; ++Ext) {
      if (ObjCIvarDecl *I = Ext->getIvarDecl(ID)) {
        clsDeclared = ClassDecl;
        return I;
      }
    }
      
    ClassDecl = ClassDecl->getSuperClass();
  }
  return NULL;
}

/// lookupInheritedClass - This method returns ObjCInterfaceDecl * of the super
/// class whose name is passed as argument. If it is not one of the super classes
/// the it returns NULL.
ObjCInterfaceDecl *ObjCInterfaceDecl::lookupInheritedClass(
                                        const IdentifierInfo*ICName) {
  // FIXME: Should make sure no callers ever do this.
  if (!hasDefinition())
    return 0;

  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  ObjCInterfaceDecl* ClassDecl = this;
  while (ClassDecl != NULL) {
    if (ClassDecl->getIdentifier() == ICName)
      return ClassDecl;
    ClassDecl = ClassDecl->getSuperClass();
  }
  return NULL;
}

ObjCProtocolDecl *
ObjCInterfaceDecl::lookupNestedProtocol(IdentifierInfo *Name) {
  for (ObjCInterfaceDecl::all_protocol_iterator P =
       all_referenced_protocol_begin(), PE = all_referenced_protocol_end();
       P != PE; ++P)
    if ((*P)->lookupProtocolNamed(Name))
      return (*P);
  ObjCInterfaceDecl *SuperClass = getSuperClass();
  return SuperClass ? SuperClass->lookupNestedProtocol(Name) : NULL;
}

/// lookupMethod - This method returns an instance/class method by looking in
/// the class, its categories, and its super classes (using a linear search).
/// When argument category "C" is specified, any implicit method found
/// in this category is ignored.
ObjCMethodDecl *ObjCInterfaceDecl::lookupMethod(Selector Sel, 
                                                bool isInstance,
                                                bool shallowCategoryLookup,
                                                bool followSuper,
                                                const ObjCCategoryDecl *C) const
{
  // FIXME: Should make sure no callers ever do this.
  if (!hasDefinition())
    return 0;

  const ObjCInterfaceDecl* ClassDecl = this;
  ObjCMethodDecl *MethodDecl = 0;

  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  while (ClassDecl) {
    if ((MethodDecl = ClassDecl->getMethod(Sel, isInstance)))
      return MethodDecl;

    // Didn't find one yet - look through protocols.
    for (ObjCInterfaceDecl::protocol_iterator I = ClassDecl->protocol_begin(),
                                              E = ClassDecl->protocol_end();
           I != E; ++I)
      if ((MethodDecl = (*I)->lookupMethod(Sel, isInstance)))
        return MethodDecl;
    
    // Didn't find one yet - now look through categories.
    for (ObjCInterfaceDecl::visible_categories_iterator
         Cat = ClassDecl->visible_categories_begin(),
         CatEnd = ClassDecl->visible_categories_end();
         Cat != CatEnd; ++Cat) {
      if ((MethodDecl = Cat->getMethod(Sel, isInstance)))
        if (C != (*Cat) || !MethodDecl->isImplicit())
          return MethodDecl;

      if (!shallowCategoryLookup) {
        // Didn't find one yet - look through protocols.
        const ObjCList<ObjCProtocolDecl> &Protocols =
        Cat->getReferencedProtocols();
        for (ObjCList<ObjCProtocolDecl>::iterator I = Protocols.begin(),
             E = Protocols.end(); I != E; ++I)
          if ((MethodDecl = (*I)->lookupMethod(Sel, isInstance)))
            if (C != (*Cat) || !MethodDecl->isImplicit())
              return MethodDecl;
      }
    }

    if (!followSuper)
      return NULL;

    // Get the super class (if any).
    ClassDecl = ClassDecl->getSuperClass();
  }
  return NULL;
}

// Will search "local" class/category implementations for a method decl.
// If failed, then we search in class's root for an instance method.
// Returns 0 if no method is found.
ObjCMethodDecl *ObjCInterfaceDecl::lookupPrivateMethod(
                                   const Selector &Sel,
                                   bool Instance) const {
  // FIXME: Should make sure no callers ever do this.
  if (!hasDefinition())
    return 0;

  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  ObjCMethodDecl *Method = 0;
  if (ObjCImplementationDecl *ImpDecl = getImplementation())
    Method = Instance ? ImpDecl->getInstanceMethod(Sel) 
                      : ImpDecl->getClassMethod(Sel);

  // Look through local category implementations associated with the class.
  if (!Method)
    Method = Instance ? getCategoryInstanceMethod(Sel)
                      : getCategoryClassMethod(Sel);

  // Before we give up, check if the selector is an instance method.
  // But only in the root. This matches gcc's behavior and what the
  // runtime expects.
  if (!Instance && !Method && !getSuperClass()) {
    Method = lookupInstanceMethod(Sel);
    // Look through local category implementations associated
    // with the root class.
    if (!Method)
      Method = lookupPrivateMethod(Sel, true);
  }

  if (!Method && getSuperClass())
    return getSuperClass()->lookupPrivateMethod(Sel, Instance);
  return Method;
}

//===----------------------------------------------------------------------===//
// ObjCMethodDecl
//===----------------------------------------------------------------------===//

ObjCMethodDecl *ObjCMethodDecl::Create(
    ASTContext &C, SourceLocation beginLoc, SourceLocation endLoc,
    Selector SelInfo, QualType T, TypeSourceInfo *ReturnTInfo,
    DeclContext *contextDecl, bool isInstance, bool isVariadic,
    bool isPropertyAccessor, bool isImplicitlyDeclared, bool isDefined,
    ImplementationControl impControl, bool HasRelatedResultType) {
  return new (C, contextDecl) ObjCMethodDecl(
      beginLoc, endLoc, SelInfo, T, ReturnTInfo, contextDecl, isInstance,
      isVariadic, isPropertyAccessor, isImplicitlyDeclared, isDefined,
      impControl, HasRelatedResultType);
}

ObjCMethodDecl *ObjCMethodDecl::CreateDeserialized(ASTContext &C, unsigned ID) {
  return new (C, ID) ObjCMethodDecl(SourceLocation(), SourceLocation(),
                                    Selector(), QualType(), 0, 0);
}

bool ObjCMethodDecl::isThisDeclarationADesignatedInitializer() const {
  return getMethodFamily() == OMF_init &&
      hasAttr<ObjCDesignatedInitializerAttr>();
}

bool ObjCMethodDecl::isDesignatedInitializerForTheInterface(
    const ObjCMethodDecl **InitMethod) const {
  if (getMethodFamily() != OMF_init)
    return false;
  const DeclContext *DC = getDeclContext();
  if (isa<ObjCProtocolDecl>(DC))
    return false;
  if (const ObjCInterfaceDecl *ID = getClassInterface())
    return ID->isDesignatedInitializer(getSelector(), InitMethod);
  return false;
}

Stmt *ObjCMethodDecl::getBody() const {
  return Body.get(getASTContext().getExternalSource());
}

void ObjCMethodDecl::setAsRedeclaration(const ObjCMethodDecl *PrevMethod) {
  assert(PrevMethod);
  getASTContext().setObjCMethodRedeclaration(PrevMethod, this);
  IsRedeclaration = true;
  PrevMethod->HasRedeclaration = true;
}

void ObjCMethodDecl::setParamsAndSelLocs(ASTContext &C,
                                         ArrayRef<ParmVarDecl*> Params,
                                         ArrayRef<SourceLocation> SelLocs) {
  ParamsAndSelLocs = 0;
  NumParams = Params.size();
  if (Params.empty() && SelLocs.empty())
    return;

  unsigned Size = sizeof(ParmVarDecl *) * NumParams +
                  sizeof(SourceLocation) * SelLocs.size();
  ParamsAndSelLocs = C.Allocate(Size);
  std::copy(Params.begin(), Params.end(), getParams());
  std::copy(SelLocs.begin(), SelLocs.end(), getStoredSelLocs());
}

void ObjCMethodDecl::getSelectorLocs(
                               SmallVectorImpl<SourceLocation> &SelLocs) const {
  for (unsigned i = 0, e = getNumSelectorLocs(); i != e; ++i)
    SelLocs.push_back(getSelectorLoc(i));
}

void ObjCMethodDecl::setMethodParams(ASTContext &C,
                                     ArrayRef<ParmVarDecl*> Params,
                                     ArrayRef<SourceLocation> SelLocs) {
  assert((!SelLocs.empty() || isImplicit()) &&
         "No selector locs for non-implicit method");
  if (isImplicit())
    return setParamsAndSelLocs(C, Params, llvm::None);

  SelLocsKind = hasStandardSelectorLocs(getSelector(), SelLocs, Params,
                                        DeclEndLoc);
  if (SelLocsKind != SelLoc_NonStandard)
    return setParamsAndSelLocs(C, Params, llvm::None);

  setParamsAndSelLocs(C, Params, SelLocs);
}

/// \brief A definition will return its interface declaration.
/// An interface declaration will return its definition.
/// Otherwise it will return itself.
ObjCMethodDecl *ObjCMethodDecl::getNextRedeclaration() {
  ASTContext &Ctx = getASTContext();
  ObjCMethodDecl *Redecl = 0;
  if (HasRedeclaration)
    Redecl = const_cast<ObjCMethodDecl*>(Ctx.getObjCMethodRedeclaration(this));
  if (Redecl)
    return Redecl;

  Decl *CtxD = cast<Decl>(getDeclContext());

  if (!CtxD->isInvalidDecl()) {
    if (ObjCInterfaceDecl *IFD = dyn_cast<ObjCInterfaceDecl>(CtxD)) {
      if (ObjCImplementationDecl *ImplD = Ctx.getObjCImplementation(IFD))
        if (!ImplD->isInvalidDecl())
          Redecl = ImplD->getMethod(getSelector(), isInstanceMethod());

    } else if (ObjCCategoryDecl *CD = dyn_cast<ObjCCategoryDecl>(CtxD)) {
      if (ObjCCategoryImplDecl *ImplD = Ctx.getObjCImplementation(CD))
        if (!ImplD->isInvalidDecl())
          Redecl = ImplD->getMethod(getSelector(), isInstanceMethod());

    } else if (ObjCImplementationDecl *ImplD =
                 dyn_cast<ObjCImplementationDecl>(CtxD)) {
      if (ObjCInterfaceDecl *IFD = ImplD->getClassInterface())
        if (!IFD->isInvalidDecl())
          Redecl = IFD->getMethod(getSelector(), isInstanceMethod());

    } else if (ObjCCategoryImplDecl *CImplD =
                 dyn_cast<ObjCCategoryImplDecl>(CtxD)) {
      if (ObjCCategoryDecl *CatD = CImplD->getCategoryDecl())
        if (!CatD->isInvalidDecl())
          Redecl = CatD->getMethod(getSelector(), isInstanceMethod());
    }
  }

  if (!Redecl && isRedeclaration()) {
    // This is the last redeclaration, go back to the first method.
    return cast<ObjCContainerDecl>(CtxD)->getMethod(getSelector(),
                                                    isInstanceMethod());
  }

  return Redecl ? Redecl : this;
}

ObjCMethodDecl *ObjCMethodDecl::getCanonicalDecl() {
  Decl *CtxD = cast<Decl>(getDeclContext());

  if (ObjCImplementationDecl *ImplD = dyn_cast<ObjCImplementationDecl>(CtxD)) {
    if (ObjCInterfaceDecl *IFD = ImplD->getClassInterface())
      if (ObjCMethodDecl *MD = IFD->getMethod(getSelector(),
                                              isInstanceMethod()))
        return MD;

  } else if (ObjCCategoryImplDecl *CImplD =
               dyn_cast<ObjCCategoryImplDecl>(CtxD)) {
    if (ObjCCategoryDecl *CatD = CImplD->getCategoryDecl())
      if (ObjCMethodDecl *MD = CatD->getMethod(getSelector(),
                                               isInstanceMethod()))
        return MD;
  }

  if (isRedeclaration())
    return cast<ObjCContainerDecl>(CtxD)->getMethod(getSelector(),
                                                    isInstanceMethod());

  return this;
}

SourceLocation ObjCMethodDecl::getLocEnd() const {
  if (Stmt *Body = getBody())
    return Body->getLocEnd();
  return DeclEndLoc;
}

ObjCMethodFamily ObjCMethodDecl::getMethodFamily() const {
  ObjCMethodFamily family = static_cast<ObjCMethodFamily>(Family);
  if (family != static_cast<unsigned>(InvalidObjCMethodFamily))
    return family;

  // Check for an explicit attribute.
  if (const ObjCMethodFamilyAttr *attr = getAttr<ObjCMethodFamilyAttr>()) {
    // The unfortunate necessity of mapping between enums here is due
    // to the attributes framework.
    switch (attr->getFamily()) {
    case ObjCMethodFamilyAttr::OMF_None: family = OMF_None; break;
    case ObjCMethodFamilyAttr::OMF_alloc: family = OMF_alloc; break;
    case ObjCMethodFamilyAttr::OMF_copy: family = OMF_copy; break;
    case ObjCMethodFamilyAttr::OMF_init: family = OMF_init; break;
    case ObjCMethodFamilyAttr::OMF_mutableCopy: family = OMF_mutableCopy; break;
    case ObjCMethodFamilyAttr::OMF_new: family = OMF_new; break;
    }
    Family = static_cast<unsigned>(family);
    return family;
  }

  family = getSelector().getMethodFamily();
  switch (family) {
  case OMF_None: break;

  // init only has a conventional meaning for an instance method, and
  // it has to return an object.
  case OMF_init:
    if (!isInstanceMethod() || !getReturnType()->isObjCObjectPointerType())
      family = OMF_None;
    break;

  // alloc/copy/new have a conventional meaning for both class and
  // instance methods, but they require an object return.
  case OMF_alloc:
  case OMF_copy:
  case OMF_mutableCopy:
  case OMF_new:
    if (!getReturnType()->isObjCObjectPointerType())
      family = OMF_None;
    break;

  // These selectors have a conventional meaning only for instance methods.
  case OMF_dealloc:
  case OMF_finalize:
  case OMF_retain:
  case OMF_release:
  case OMF_autorelease:
  case OMF_retainCount:
  case OMF_self:
    if (!isInstanceMethod())
      family = OMF_None;
    break;
      
  case OMF_performSelector:
    if (!isInstanceMethod() || !getReturnType()->isObjCIdType())
      family = OMF_None;
    else {
      unsigned noParams = param_size();
      if (noParams < 1 || noParams > 3)
        family = OMF_None;
      else {
        ObjCMethodDecl::param_type_iterator it = param_type_begin();
        QualType ArgT = (*it);
        if (!ArgT->isObjCSelType()) {
          family = OMF_None;
          break;
        }
        while (--noParams) {
          it++;
          ArgT = (*it);
          if (!ArgT->isObjCIdType()) {
            family = OMF_None;
            break;
          }
        }
      }
    }
    break;
      
  }

  // Cache the result.
  Family = static_cast<unsigned>(family);
  return family;
}

void ObjCMethodDecl::createImplicitParams(ASTContext &Context,
                                          const ObjCInterfaceDecl *OID) {
  QualType selfTy;
  if (isInstanceMethod()) {
    // There may be no interface context due to error in declaration
    // of the interface (which has been reported). Recover gracefully.
    if (OID) {
      selfTy = Context.getObjCInterfaceType(OID);
      selfTy = Context.getObjCObjectPointerType(selfTy);
    } else {
      selfTy = Context.getObjCIdType();
    }
  } else // we have a factory method.
    selfTy = Context.getObjCClassType();

  bool selfIsPseudoStrong = false;
  bool selfIsConsumed = false;
  
  if (Context.getLangOpts().ObjCAutoRefCount) {
    if (isInstanceMethod()) {
      selfIsConsumed = hasAttr<NSConsumesSelfAttr>();

      // 'self' is always __strong.  It's actually pseudo-strong except
      // in init methods (or methods labeled ns_consumes_self), though.
      Qualifiers qs;
      qs.setObjCLifetime(Qualifiers::OCL_Strong);
      selfTy = Context.getQualifiedType(selfTy, qs);

      // In addition, 'self' is const unless this is an init method.
      if (getMethodFamily() != OMF_init && !selfIsConsumed) {
        selfTy = selfTy.withConst();
        selfIsPseudoStrong = true;
      }
    }
    else {
      assert(isClassMethod());
      // 'self' is always const in class methods.
      selfTy = selfTy.withConst();
      selfIsPseudoStrong = true;
    }
  }

  ImplicitParamDecl *self
    = ImplicitParamDecl::Create(Context, this, SourceLocation(),
                                &Context.Idents.get("self"), selfTy);
  setSelfDecl(self);

  if (selfIsConsumed)
    self->addAttr(NSConsumedAttr::CreateImplicit(Context));

  if (selfIsPseudoStrong)
    self->setARCPseudoStrong(true);

  setCmdDecl(ImplicitParamDecl::Create(Context, this, SourceLocation(),
                                       &Context.Idents.get("_cmd"),
                                       Context.getObjCSelType()));
}

ObjCInterfaceDecl *ObjCMethodDecl::getClassInterface() {
  if (ObjCInterfaceDecl *ID = dyn_cast<ObjCInterfaceDecl>(getDeclContext()))
    return ID;
  if (ObjCCategoryDecl *CD = dyn_cast<ObjCCategoryDecl>(getDeclContext()))
    return CD->getClassInterface();
  if (ObjCImplDecl *IMD = dyn_cast<ObjCImplDecl>(getDeclContext()))
    return IMD->getClassInterface();

  assert(!isa<ObjCProtocolDecl>(getDeclContext()) && "It's a protocol method");
  llvm_unreachable("unknown method context");
}

static void CollectOverriddenMethodsRecurse(const ObjCContainerDecl *Container,
                                            const ObjCMethodDecl *Method,
                               SmallVectorImpl<const ObjCMethodDecl *> &Methods,
                                            bool MovedToSuper) {
  if (!Container)
    return;

  // In categories look for overriden methods from protocols. A method from
  // category is not "overriden" since it is considered as the "same" method
  // (same USR) as the one from the interface.
  if (const ObjCCategoryDecl *
        Category = dyn_cast<ObjCCategoryDecl>(Container)) {
    // Check whether we have a matching method at this category but only if we
    // are at the super class level.
    if (MovedToSuper)
      if (ObjCMethodDecl *
            Overridden = Container->getMethod(Method->getSelector(),
                                              Method->isInstanceMethod(),
                                              /*AllowHidden=*/true))
        if (Method != Overridden) {
          // We found an override at this category; there is no need to look
          // into its protocols.
          Methods.push_back(Overridden);
          return;
        }

    for (ObjCCategoryDecl::protocol_iterator P = Category->protocol_begin(),
                                          PEnd = Category->protocol_end();
         P != PEnd; ++P)
      CollectOverriddenMethodsRecurse(*P, Method, Methods, MovedToSuper);
    return;
  }

  // Check whether we have a matching method at this level.
  if (const ObjCMethodDecl *
        Overridden = Container->getMethod(Method->getSelector(),
                                          Method->isInstanceMethod(),
                                          /*AllowHidden=*/true))
    if (Method != Overridden) {
      // We found an override at this level; there is no need to look
      // into other protocols or categories.
      Methods.push_back(Overridden);
      return;
    }

  if (const ObjCProtocolDecl *Protocol = dyn_cast<ObjCProtocolDecl>(Container)){
    for (ObjCProtocolDecl::protocol_iterator P = Protocol->protocol_begin(),
                                          PEnd = Protocol->protocol_end();
         P != PEnd; ++P)
      CollectOverriddenMethodsRecurse(*P, Method, Methods, MovedToSuper);
  }

  if (const ObjCInterfaceDecl *
        Interface = dyn_cast<ObjCInterfaceDecl>(Container)) {
    for (ObjCInterfaceDecl::protocol_iterator P = Interface->protocol_begin(),
                                           PEnd = Interface->protocol_end();
         P != PEnd; ++P)
      CollectOverriddenMethodsRecurse(*P, Method, Methods, MovedToSuper);

    for (ObjCInterfaceDecl::known_categories_iterator
           Cat = Interface->known_categories_begin(),
           CatEnd = Interface->known_categories_end();
         Cat != CatEnd; ++Cat) {
      CollectOverriddenMethodsRecurse(*Cat, Method, Methods,
                                      MovedToSuper);
    }

    if (const ObjCInterfaceDecl *Super = Interface->getSuperClass())
      return CollectOverriddenMethodsRecurse(Super, Method, Methods,
                                             /*MovedToSuper=*/true);
  }
}

static inline void CollectOverriddenMethods(const ObjCContainerDecl *Container,
                                            const ObjCMethodDecl *Method,
                             SmallVectorImpl<const ObjCMethodDecl *> &Methods) {
  CollectOverriddenMethodsRecurse(Container, Method, Methods,
                                  /*MovedToSuper=*/false);
}

static void collectOverriddenMethodsSlow(const ObjCMethodDecl *Method,
                          SmallVectorImpl<const ObjCMethodDecl *> &overridden) {
  assert(Method->isOverriding());

  if (const ObjCProtocolDecl *
        ProtD = dyn_cast<ObjCProtocolDecl>(Method->getDeclContext())) {
    CollectOverriddenMethods(ProtD, Method, overridden);

  } else if (const ObjCImplDecl *
               IMD = dyn_cast<ObjCImplDecl>(Method->getDeclContext())) {
    const ObjCInterfaceDecl *ID = IMD->getClassInterface();
    if (!ID)
      return;
    // Start searching for overridden methods using the method from the
    // interface as starting point.
    if (const ObjCMethodDecl *IFaceMeth = ID->getMethod(Method->getSelector(),
                                                    Method->isInstanceMethod(),
                                                    /*AllowHidden=*/true))
      Method = IFaceMeth;
    CollectOverriddenMethods(ID, Method, overridden);

  } else if (const ObjCCategoryDecl *
               CatD = dyn_cast<ObjCCategoryDecl>(Method->getDeclContext())) {
    const ObjCInterfaceDecl *ID = CatD->getClassInterface();
    if (!ID)
      return;
    // Start searching for overridden methods using the method from the
    // interface as starting point.
    if (const ObjCMethodDecl *IFaceMeth = ID->getMethod(Method->getSelector(),
                                                     Method->isInstanceMethod(),
                                                     /*AllowHidden=*/true))
      Method = IFaceMeth;
    CollectOverriddenMethods(ID, Method, overridden);

  } else {
    CollectOverriddenMethods(
                  dyn_cast_or_null<ObjCContainerDecl>(Method->getDeclContext()),
                  Method, overridden);
  }
}

void ObjCMethodDecl::getOverriddenMethods(
                    SmallVectorImpl<const ObjCMethodDecl *> &Overridden) const {
  const ObjCMethodDecl *Method = this;

  if (Method->isRedeclaration()) {
    Method = cast<ObjCContainerDecl>(Method->getDeclContext())->
                   getMethod(Method->getSelector(), Method->isInstanceMethod());
  }

  if (Method->isOverriding()) {
    collectOverriddenMethodsSlow(Method, Overridden);
    assert(!Overridden.empty() &&
           "ObjCMethodDecl's overriding bit is not as expected");
  }
}

const ObjCPropertyDecl *
ObjCMethodDecl::findPropertyDecl(bool CheckOverrides) const {
  Selector Sel = getSelector();
  unsigned NumArgs = Sel.getNumArgs();
  if (NumArgs > 1)
    return 0;

  if (!isInstanceMethod() || getMethodFamily() != OMF_None)
    return 0;
  
  if (isPropertyAccessor()) {
    const ObjCContainerDecl *Container = cast<ObjCContainerDecl>(getParent());
    // If container is class extension, find its primary class.
    if (const ObjCCategoryDecl *CatDecl = dyn_cast<ObjCCategoryDecl>(Container))
      if (CatDecl->IsClassExtension())
        Container = CatDecl->getClassInterface();
    
    bool IsGetter = (NumArgs == 0);

    for (ObjCContainerDecl::prop_iterator I = Container->prop_begin(),
                                          E = Container->prop_end();
         I != E; ++I) {
      Selector NextSel = IsGetter ? (*I)->getGetterName()
                                  : (*I)->getSetterName();
      if (NextSel == Sel)
        return *I;
    }

    llvm_unreachable("Marked as a property accessor but no property found!");
  }

  if (!CheckOverrides)
    return 0;

  typedef SmallVector<const ObjCMethodDecl *, 8> OverridesTy;
  OverridesTy Overrides;
  getOverriddenMethods(Overrides);
  for (OverridesTy::const_iterator I = Overrides.begin(), E = Overrides.end();
       I != E; ++I) {
    if (const ObjCPropertyDecl *Prop = (*I)->findPropertyDecl(false))
      return Prop;
  }

  return 0;

}

//===----------------------------------------------------------------------===//
// ObjCInterfaceDecl
//===----------------------------------------------------------------------===//

ObjCInterfaceDecl *ObjCInterfaceDecl::Create(const ASTContext &C,
                                             DeclContext *DC,
                                             SourceLocation atLoc,
                                             IdentifierInfo *Id,
                                             ObjCInterfaceDecl *PrevDecl,
                                             SourceLocation ClassLoc,
                                             bool isInternal){
  ObjCInterfaceDecl *Result = new (C, DC)
      ObjCInterfaceDecl(DC, atLoc, Id, ClassLoc, PrevDecl, isInternal);
  Result->Data.setInt(!C.getLangOpts().Modules);
  C.getObjCInterfaceType(Result, PrevDecl);
  return Result;
}

ObjCInterfaceDecl *ObjCInterfaceDecl::CreateDeserialized(ASTContext &C,
                                                         unsigned ID) {
  ObjCInterfaceDecl *Result = new (C, ID) ObjCInterfaceDecl(0, SourceLocation(),
                                                            0, SourceLocation(),
                                                            0, false);
  Result->Data.setInt(!C.getLangOpts().Modules);
  return Result;
}

ObjCInterfaceDecl::
ObjCInterfaceDecl(DeclContext *DC, SourceLocation atLoc, IdentifierInfo *Id,
                  SourceLocation CLoc, ObjCInterfaceDecl *PrevDecl,
                  bool isInternal)
  : ObjCContainerDecl(ObjCInterface, DC, Id, CLoc, atLoc),
    TypeForDecl(0), Data()
{
  setPreviousDecl(PrevDecl);
  
  // Copy the 'data' pointer over.
  if (PrevDecl)
    Data = PrevDecl->Data;
  
  setImplicit(isInternal);
}

void ObjCInterfaceDecl::LoadExternalDefinition() const {
  assert(data().ExternallyCompleted && "Class is not externally completed");
  data().ExternallyCompleted = false;
  getASTContext().getExternalSource()->CompleteType(
                                        const_cast<ObjCInterfaceDecl *>(this));
}

void ObjCInterfaceDecl::setExternallyCompleted() {
  assert(getASTContext().getExternalSource() && 
         "Class can't be externally completed without an external source");
  assert(hasDefinition() && 
         "Forward declarations can't be externally completed");
  data().ExternallyCompleted = true;
}

void ObjCInterfaceDecl::setHasDesignatedInitializers() {
  assert(hasDefinition() && "Forward declarations can't contain methods");
  data().HasDesignatedInitializers = true;
}

bool ObjCInterfaceDecl::hasDesignatedInitializers() const {
  assert(hasDefinition() && "Forward declarations can't contain methods");
  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  return data().HasDesignatedInitializers;
}

ObjCImplementationDecl *ObjCInterfaceDecl::getImplementation() const {
  if (const ObjCInterfaceDecl *Def = getDefinition()) {
    if (data().ExternallyCompleted)
      LoadExternalDefinition();
    
    return getASTContext().getObjCImplementation(
             const_cast<ObjCInterfaceDecl*>(Def));
  }
  
  // FIXME: Should make sure no callers ever do this.
  return 0;
}

void ObjCInterfaceDecl::setImplementation(ObjCImplementationDecl *ImplD) {
  getASTContext().setObjCImplementation(getDefinition(), ImplD);
}

namespace {
  struct SynthesizeIvarChunk {
    uint64_t Size;
    ObjCIvarDecl *Ivar;
    SynthesizeIvarChunk(uint64_t size, ObjCIvarDecl *ivar)
      : Size(size), Ivar(ivar) {}
  };

  bool operator<(const SynthesizeIvarChunk & LHS,
                 const SynthesizeIvarChunk &RHS) {
      return LHS.Size < RHS.Size;
  }
}

/// all_declared_ivar_begin - return first ivar declared in this class,
/// its extensions and its implementation. Lazily build the list on first
/// access.
///
/// Caveat: The list returned by this method reflects the current
/// state of the parser. The cache will be updated for every ivar
/// added by an extension or the implementation when they are
/// encountered.
/// See also ObjCIvarDecl::Create().
ObjCIvarDecl *ObjCInterfaceDecl::all_declared_ivar_begin() {
  // FIXME: Should make sure no callers ever do this.
  if (!hasDefinition())
    return 0;
  
  ObjCIvarDecl *curIvar = 0;
  if (!data().IvarList) {
    if (!ivar_empty()) {
      ObjCInterfaceDecl::ivar_iterator I = ivar_begin(), E = ivar_end();
      data().IvarList = *I; ++I;
      for (curIvar = data().IvarList; I != E; curIvar = *I, ++I)
        curIvar->setNextIvar(*I);
    }

    for (ObjCInterfaceDecl::known_extensions_iterator
           Ext = known_extensions_begin(),
           ExtEnd = known_extensions_end();
         Ext != ExtEnd; ++Ext) {
      if (!Ext->ivar_empty()) {
        ObjCCategoryDecl::ivar_iterator
          I = Ext->ivar_begin(),
          E = Ext->ivar_end();
        if (!data().IvarList) {
          data().IvarList = *I; ++I;
          curIvar = data().IvarList;
        }
        for ( ;I != E; curIvar = *I, ++I)
          curIvar->setNextIvar(*I);
      }
    }
    data().IvarListMissingImplementation = true;
  }

  // cached and complete!
  if (!data().IvarListMissingImplementation)
      return data().IvarList;
  
  if (ObjCImplementationDecl *ImplDecl = getImplementation()) {
    data().IvarListMissingImplementation = false;
    if (!ImplDecl->ivar_empty()) {
      SmallVector<SynthesizeIvarChunk, 16> layout;
      for (ObjCImplementationDecl::ivar_iterator I = ImplDecl->ivar_begin(),
           E = ImplDecl->ivar_end(); I != E; ++I) {
        ObjCIvarDecl *IV = *I;
        if (IV->getSynthesize() && !IV->isInvalidDecl()) {
          layout.push_back(SynthesizeIvarChunk(
                             IV->getASTContext().getTypeSize(IV->getType()), IV));
          continue;
        }
        if (!data().IvarList)
          data().IvarList = *I;
        else
          curIvar->setNextIvar(*I);
        curIvar = *I;
      }
      
      if (!layout.empty()) {
        // Order synthesized ivars by their size.
        std::stable_sort(layout.begin(), layout.end());
        unsigned Ix = 0, EIx = layout.size();
        if (!data().IvarList) {
          data().IvarList = layout[0].Ivar; Ix++;
          curIvar = data().IvarList;
        }
        for ( ; Ix != EIx; curIvar = layout[Ix].Ivar, Ix++)
          curIvar->setNextIvar(layout[Ix].Ivar);
      }
    }
  }
  return data().IvarList;
}

/// FindCategoryDeclaration - Finds category declaration in the list of
/// categories for this class and returns it. Name of the category is passed
/// in 'CategoryId'. If category not found, return 0;
///
ObjCCategoryDecl *
ObjCInterfaceDecl::FindCategoryDeclaration(IdentifierInfo *CategoryId) const {
  // FIXME: Should make sure no callers ever do this.
  if (!hasDefinition())
    return 0;

  if (data().ExternallyCompleted)
    LoadExternalDefinition();

  for (visible_categories_iterator Cat = visible_categories_begin(),
                                   CatEnd = visible_categories_end();
       Cat != CatEnd;
       ++Cat) {
    if (Cat->getIdentifier() == CategoryId)
      return *Cat;
  }
  
  return 0;
}

ObjCMethodDecl *
ObjCInterfaceDecl::getCategoryInstanceMethod(Selector Sel) const {
  for (visible_categories_iterator Cat = visible_categories_begin(),
                                   CatEnd = visible_categories_end();
       Cat != CatEnd;
       ++Cat) {
    if (ObjCCategoryImplDecl *Impl = Cat->getImplementation())
      if (ObjCMethodDecl *MD = Impl->getInstanceMethod(Sel))
        return MD;
  }

  return 0;
}

ObjCMethodDecl *ObjCInterfaceDecl::getCategoryClassMethod(Selector Sel) const {
  for (visible_categories_iterator Cat = visible_categories_begin(),
                                   CatEnd = visible_categories_end();
       Cat != CatEnd;
       ++Cat) {
    if (ObjCCategoryImplDecl *Impl = Cat->getImplementation())
      if (ObjCMethodDecl *MD = Impl->getClassMethod(Sel))
        return MD;
  }
  
  return 0;
}

/// ClassImplementsProtocol - Checks that 'lProto' protocol
/// has been implemented in IDecl class, its super class or categories (if
/// lookupCategory is true).
bool ObjCInterfaceDecl::ClassImplementsProtocol(ObjCProtocolDecl *lProto,
                                    bool lookupCategory,
                                    bool RHSIsQualifiedID) {
  if (!hasDefinition())
    return false;
  
  ObjCInterfaceDecl *IDecl = this;
  // 1st, look up the class.
  for (ObjCInterfaceDecl::protocol_iterator
        PI = IDecl->protocol_begin(), E = IDecl->protocol_end(); PI != E; ++PI){
    if (getASTContext().ProtocolCompatibleWithProtocol(lProto, *PI))
      return true;
    // This is dubious and is added to be compatible with gcc.  In gcc, it is
    // also allowed assigning a protocol-qualified 'id' type to a LHS object
    // when protocol in qualified LHS is in list of protocols in the rhs 'id'
    // object. This IMO, should be a bug.
    // FIXME: Treat this as an extension, and flag this as an error when GCC
    // extensions are not enabled.
    if (RHSIsQualifiedID &&
        getASTContext().ProtocolCompatibleWithProtocol(*PI, lProto))
      return true;
  }

  // 2nd, look up the category.
  if (lookupCategory)
    for (visible_categories_iterator Cat = visible_categories_begin(),
                                     CatEnd = visible_categories_end();
         Cat != CatEnd;
         ++Cat) {
      for (ObjCCategoryDecl::protocol_iterator PI = Cat->protocol_begin(),
                                               E = Cat->protocol_end();
           PI != E; ++PI)
        if (getASTContext().ProtocolCompatibleWithProtocol(lProto, *PI))
          return true;
    }

  // 3rd, look up the super class(s)
  if (IDecl->getSuperClass())
    return
  IDecl->getSuperClass()->ClassImplementsProtocol(lProto, lookupCategory,
                                                  RHSIsQualifiedID);

  return false;
}

//===----------------------------------------------------------------------===//
// ObjCIvarDecl
//===----------------------------------------------------------------------===//

void ObjCIvarDecl::anchor() { }

ObjCIvarDecl *ObjCIvarDecl::Create(ASTContext &C, ObjCContainerDecl *DC,
                                   SourceLocation StartLoc,
                                   SourceLocation IdLoc, IdentifierInfo *Id,
                                   QualType T, TypeSourceInfo *TInfo,
                                   AccessControl ac, Expr *BW,
                                   bool synthesized) {
  if (DC) {
    // Ivar's can only appear in interfaces, implementations (via synthesized
    // properties), and class extensions (via direct declaration, or synthesized
    // properties).
    //
    // FIXME: This should really be asserting this:
    //   (isa<ObjCCategoryDecl>(DC) &&
    //    cast<ObjCCategoryDecl>(DC)->IsClassExtension()))
    // but unfortunately we sometimes place ivars into non-class extension
    // categories on error. This breaks an AST invariant, and should not be
    // fixed.
    assert((isa<ObjCInterfaceDecl>(DC) || isa<ObjCImplementationDecl>(DC) ||
            isa<ObjCCategoryDecl>(DC)) &&
           "Invalid ivar decl context!");
    // Once a new ivar is created in any of class/class-extension/implementation
    // decl contexts, the previously built IvarList must be rebuilt.
    ObjCInterfaceDecl *ID = dyn_cast<ObjCInterfaceDecl>(DC);
    if (!ID) {
      if (ObjCImplementationDecl *IM = dyn_cast<ObjCImplementationDecl>(DC))
        ID = IM->getClassInterface();
      else
        ID = cast<ObjCCategoryDecl>(DC)->getClassInterface();
    }
    ID->setIvarList(0);
  }

  return new (C, DC) ObjCIvarDecl(DC, StartLoc, IdLoc, Id, T, TInfo, ac, BW,
                                  synthesized);
}

ObjCIvarDecl *ObjCIvarDecl::CreateDeserialized(ASTContext &C, unsigned ID) {
  return new (C, ID) ObjCIvarDecl(0, SourceLocation(), SourceLocation(), 0,
                                  QualType(), 0, ObjCIvarDecl::None, 0, false);
}

const ObjCInterfaceDecl *ObjCIvarDecl::getContainingInterface() const {
  const ObjCContainerDecl *DC = cast<ObjCContainerDecl>(getDeclContext());

  switch (DC->getKind()) {
  default:
  case ObjCCategoryImpl:
  case ObjCProtocol:
    llvm_unreachable("invalid ivar container!");

    // Ivars can only appear in class extension categories.
  case ObjCCategory: {
    const ObjCCategoryDecl *CD = cast<ObjCCategoryDecl>(DC);
    assert(CD->IsClassExtension() && "invalid container for ivar!");
    return CD->getClassInterface();
  }

  case ObjCImplementation:
    return cast<ObjCImplementationDecl>(DC)->getClassInterface();

  case ObjCInterface:
    return cast<ObjCInterfaceDecl>(DC);
  }
}

//===----------------------------------------------------------------------===//
// ObjCAtDefsFieldDecl
//===----------------------------------------------------------------------===//

void ObjCAtDefsFieldDecl::anchor() { }

ObjCAtDefsFieldDecl
*ObjCAtDefsFieldDecl::Create(ASTContext &C, DeclContext *DC,
                             SourceLocation StartLoc,  SourceLocation IdLoc,
                             IdentifierInfo *Id, QualType T, Expr *BW) {
  return new (C, DC) ObjCAtDefsFieldDecl(DC, StartLoc, IdLoc, Id, T, BW);
}

ObjCAtDefsFieldDecl *ObjCAtDefsFieldDecl::CreateDeserialized(ASTContext &C,
                                                             unsigned ID) {
  return new (C, ID) ObjCAtDefsFieldDecl(0, SourceLocation(), SourceLocation(),
                                         0, QualType(), 0);
}

//===----------------------------------------------------------------------===//
// ObjCProtocolDecl
//===----------------------------------------------------------------------===//

void ObjCProtocolDecl::anchor() { }

ObjCProtocolDecl::ObjCProtocolDecl(DeclContext *DC, IdentifierInfo *Id,
                                   SourceLocation nameLoc, 
                                   SourceLocation atStartLoc,
                                   ObjCProtocolDecl *PrevDecl)
  : ObjCContainerDecl(ObjCProtocol, DC, Id, nameLoc, atStartLoc), Data()
{
  setPreviousDecl(PrevDecl);
  if (PrevDecl)
    Data = PrevDecl->Data;
}

ObjCProtocolDecl *ObjCProtocolDecl::Create(ASTContext &C, DeclContext *DC,
                                           IdentifierInfo *Id,
                                           SourceLocation nameLoc,
                                           SourceLocation atStartLoc,
                                           ObjCProtocolDecl *PrevDecl) {
  ObjCProtocolDecl *Result =
      new (C, DC) ObjCProtocolDecl(DC, Id, nameLoc, atStartLoc, PrevDecl);
  Result->Data.setInt(!C.getLangOpts().Modules);
  return Result;
}

ObjCProtocolDecl *ObjCProtocolDecl::CreateDeserialized(ASTContext &C,
                                                       unsigned ID) {
  ObjCProtocolDecl *Result =
      new (C, ID) ObjCProtocolDecl(0, 0, SourceLocation(), SourceLocation(), 0);
  Result->Data.setInt(!C.getLangOpts().Modules);
  return Result;
}

ObjCProtocolDecl *ObjCProtocolDecl::lookupProtocolNamed(IdentifierInfo *Name) {
  ObjCProtocolDecl *PDecl = this;

  if (Name == getIdentifier())
    return PDecl;

  for (protocol_iterator I = protocol_begin(), E = protocol_end(); I != E; ++I)
    if ((PDecl = (*I)->lookupProtocolNamed(Name)))
      return PDecl;

  return NULL;
}

// lookupMethod - Lookup a instance/class method in the protocol and protocols
// it inherited.
ObjCMethodDecl *ObjCProtocolDecl::lookupMethod(Selector Sel,
                                               bool isInstance) const {
  ObjCMethodDecl *MethodDecl = NULL;

  // If there is no definition or the definition is hidden, we don't find
  // anything.
  const ObjCProtocolDecl *Def = getDefinition();
  if (!Def || Def->isHidden())
    return NULL;

  if ((MethodDecl = getMethod(Sel, isInstance)))
    return MethodDecl;

  for (protocol_iterator I = protocol_begin(), E = protocol_end(); I != E; ++I)
    if ((MethodDecl = (*I)->lookupMethod(Sel, isInstance)))
      return MethodDecl;
  return NULL;
}

void ObjCProtocolDecl::allocateDefinitionData() {
  assert(!Data.getPointer() && "Protocol already has a definition!");
  Data.setPointer(new (getASTContext()) DefinitionData);
  Data.getPointer()->Definition = this;
}

void ObjCProtocolDecl::startDefinition() {
  allocateDefinitionData();
  
  // Update all of the declarations with a pointer to the definition.
  for (redecl_iterator RD = redecls_begin(), RDEnd = redecls_end();
       RD != RDEnd; ++RD)
    RD->Data = this->Data;
}

void ObjCProtocolDecl::collectPropertiesToImplement(PropertyMap &PM,
                                                    PropertyDeclOrder &PO) const {
  
  if (const ObjCProtocolDecl *PDecl = getDefinition()) {
    for (ObjCProtocolDecl::prop_iterator P = PDecl->prop_begin(),
         E = PDecl->prop_end(); P != E; ++P) {
      ObjCPropertyDecl *Prop = *P;
      // Insert into PM if not there already.
      PM.insert(std::make_pair(Prop->getIdentifier(), Prop));
      PO.push_back(Prop);
    }
    // Scan through protocol's protocols.
    for (ObjCProtocolDecl::protocol_iterator PI = PDecl->protocol_begin(),
         E = PDecl->protocol_end(); PI != E; ++PI)
      (*PI)->collectPropertiesToImplement(PM, PO);
  }
}

    
void ObjCProtocolDecl::collectInheritedProtocolProperties(
                                                const ObjCPropertyDecl *Property,
                                                ProtocolPropertyMap &PM) const {
  if (const ObjCProtocolDecl *PDecl = getDefinition()) {
    bool MatchFound = false;
    for (ObjCProtocolDecl::prop_iterator P = PDecl->prop_begin(),
         E = PDecl->prop_end(); P != E; ++P) {
      ObjCPropertyDecl *Prop = *P;
      if (Prop == Property)
        continue;
      if (Prop->getIdentifier() == Property->getIdentifier()) {
        PM[PDecl] = Prop;
        MatchFound = true;
        break;
      }
    }
    // Scan through protocol's protocols which did not have a matching property.
    if (!MatchFound)
      for (ObjCProtocolDecl::protocol_iterator PI = PDecl->protocol_begin(),
           E = PDecl->protocol_end(); PI != E; ++PI)
        (*PI)->collectInheritedProtocolProperties(Property, PM);
  }
}

//===----------------------------------------------------------------------===//
// ObjCCategoryDecl
//===----------------------------------------------------------------------===//

void ObjCCategoryDecl::anchor() { }

ObjCCategoryDecl *ObjCCategoryDecl::Create(ASTContext &C, DeclContext *DC,
                                           SourceLocation AtLoc,
                                           SourceLocation ClassNameLoc,
                                           SourceLocation CategoryNameLoc,
                                           IdentifierInfo *Id,
                                           ObjCInterfaceDecl *IDecl,
                                           SourceLocation IvarLBraceLoc,
                                           SourceLocation IvarRBraceLoc) {
  ObjCCategoryDecl *CatDecl =
      new (C, DC) ObjCCategoryDecl(DC, AtLoc, ClassNameLoc, CategoryNameLoc, Id,
                                   IDecl, IvarLBraceLoc, IvarRBraceLoc);
  if (IDecl) {
    // Link this category into its class's category list.
    CatDecl->NextClassCategory = IDecl->getCategoryListRaw();
    if (IDecl->hasDefinition()) {
      IDecl->setCategoryListRaw(CatDecl);
      if (ASTMutationListener *L = C.getASTMutationListener())
        L->AddedObjCCategoryToInterface(CatDecl, IDecl);
    }
  }

  return CatDecl;
}

ObjCCategoryDecl *ObjCCategoryDecl::CreateDeserialized(ASTContext &C,
                                                       unsigned ID) {
  return new (C, ID) ObjCCategoryDecl(0, SourceLocation(), SourceLocation(),
                                      SourceLocation(), 0, 0);
}

ObjCCategoryImplDecl *ObjCCategoryDecl::getImplementation() const {
  return getASTContext().getObjCImplementation(
                                           const_cast<ObjCCategoryDecl*>(this));
}

void ObjCCategoryDecl::setImplementation(ObjCCategoryImplDecl *ImplD) {
  getASTContext().setObjCImplementation(this, ImplD);
}


//===----------------------------------------------------------------------===//
// ObjCCategoryImplDecl
//===----------------------------------------------------------------------===//

void ObjCCategoryImplDecl::anchor() { }

ObjCCategoryImplDecl *
ObjCCategoryImplDecl::Create(ASTContext &C, DeclContext *DC,
                             IdentifierInfo *Id,
                             ObjCInterfaceDecl *ClassInterface,
                             SourceLocation nameLoc,
                             SourceLocation atStartLoc,
                             SourceLocation CategoryNameLoc) {
  if (ClassInterface && ClassInterface->hasDefinition())
    ClassInterface = ClassInterface->getDefinition();
  return new (C, DC) ObjCCategoryImplDecl(DC, Id, ClassInterface, nameLoc,
                                          atStartLoc, CategoryNameLoc);
}

ObjCCategoryImplDecl *ObjCCategoryImplDecl::CreateDeserialized(ASTContext &C, 
                                                               unsigned ID) {
  return new (C, ID) ObjCCategoryImplDecl(0, 0, 0, SourceLocation(),
                                          SourceLocation(), SourceLocation());
}

ObjCCategoryDecl *ObjCCategoryImplDecl::getCategoryDecl() const {
  // The class interface might be NULL if we are working with invalid code.
  if (const ObjCInterfaceDecl *ID = getClassInterface())
    return ID->FindCategoryDeclaration(getIdentifier());
  return 0;
}


void ObjCImplDecl::anchor() { }

void ObjCImplDecl::addPropertyImplementation(ObjCPropertyImplDecl *property) {
  // FIXME: The context should be correct before we get here.
  property->setLexicalDeclContext(this);
  addDecl(property);
}

void ObjCImplDecl::setClassInterface(ObjCInterfaceDecl *IFace) {
  ASTContext &Ctx = getASTContext();

  if (ObjCImplementationDecl *ImplD
        = dyn_cast_or_null<ObjCImplementationDecl>(this)) {
    if (IFace)
      Ctx.setObjCImplementation(IFace, ImplD);

  } else if (ObjCCategoryImplDecl *ImplD =
             dyn_cast_or_null<ObjCCategoryImplDecl>(this)) {
    if (ObjCCategoryDecl *CD = IFace->FindCategoryDeclaration(getIdentifier()))
      Ctx.setObjCImplementation(CD, ImplD);
  }

  ClassInterface = IFace;
}

/// FindPropertyImplIvarDecl - This method lookup the ivar in the list of
/// properties implemented in this \@implementation block and returns
/// the implemented property that uses it.
///
ObjCPropertyImplDecl *ObjCImplDecl::
FindPropertyImplIvarDecl(IdentifierInfo *ivarId) const {
  for (propimpl_iterator i = propimpl_begin(), e = propimpl_end(); i != e; ++i){
    ObjCPropertyImplDecl *PID = *i;
    if (PID->getPropertyIvarDecl() &&
        PID->getPropertyIvarDecl()->getIdentifier() == ivarId)
      return PID;
  }
  return 0;
}

/// FindPropertyImplDecl - This method looks up a previous ObjCPropertyImplDecl
/// added to the list of those properties \@synthesized/\@dynamic in this
/// category \@implementation block.
///
ObjCPropertyImplDecl *ObjCImplDecl::
FindPropertyImplDecl(IdentifierInfo *Id) const {
  for (propimpl_iterator i = propimpl_begin(), e = propimpl_end(); i != e; ++i){
    ObjCPropertyImplDecl *PID = *i;
    if (PID->getPropertyDecl()->getIdentifier() == Id)
      return PID;
  }
  return 0;
}

raw_ostream &clang::operator<<(raw_ostream &OS,
                               const ObjCCategoryImplDecl &CID) {
  OS << CID.getName();
  return OS;
}

//===----------------------------------------------------------------------===//
// ObjCImplementationDecl
//===----------------------------------------------------------------------===//

void ObjCImplementationDecl::anchor() { }

ObjCImplementationDecl *
ObjCImplementationDecl::Create(ASTContext &C, DeclContext *DC,
                               ObjCInterfaceDecl *ClassInterface,
                               ObjCInterfaceDecl *SuperDecl,
                               SourceLocation nameLoc,
                               SourceLocation atStartLoc,
                               SourceLocation superLoc,
                               SourceLocation IvarLBraceLoc,
                               SourceLocation IvarRBraceLoc) {
  if (ClassInterface && ClassInterface->hasDefinition())
    ClassInterface = ClassInterface->getDefinition();
  return new (C, DC) ObjCImplementationDecl(DC, ClassInterface, SuperDecl,
                                            nameLoc, atStartLoc, superLoc,
                                            IvarLBraceLoc, IvarRBraceLoc);
}

ObjCImplementationDecl *
ObjCImplementationDecl::CreateDeserialized(ASTContext &C, unsigned ID) {
  return new (C, ID) ObjCImplementationDecl(0, 0, 0, SourceLocation(),
                                            SourceLocation());
}

void ObjCImplementationDecl::setIvarInitializers(ASTContext &C,
                                             CXXCtorInitializer ** initializers,
                                                 unsigned numInitializers) {
  if (numInitializers > 0) {
    NumIvarInitializers = numInitializers;
    CXXCtorInitializer **ivarInitializers =
    new (C) CXXCtorInitializer*[NumIvarInitializers];
    memcpy(ivarInitializers, initializers,
           numInitializers * sizeof(CXXCtorInitializer*));
    IvarInitializers = ivarInitializers;
  }
}

raw_ostream &clang::operator<<(raw_ostream &OS,
                               const ObjCImplementationDecl &ID) {
  OS << ID.getName();
  return OS;
}

//===----------------------------------------------------------------------===//
// ObjCCompatibleAliasDecl
//===----------------------------------------------------------------------===//

void ObjCCompatibleAliasDecl::anchor() { }

ObjCCompatibleAliasDecl *
ObjCCompatibleAliasDecl::Create(ASTContext &C, DeclContext *DC,
                                SourceLocation L,
                                IdentifierInfo *Id,
                                ObjCInterfaceDecl* AliasedClass) {
  return new (C, DC) ObjCCompatibleAliasDecl(DC, L, Id, AliasedClass);
}

ObjCCompatibleAliasDecl *
ObjCCompatibleAliasDecl::CreateDeserialized(ASTContext &C, unsigned ID) {
  return new (C, ID) ObjCCompatibleAliasDecl(0, SourceLocation(), 0, 0);
}

//===----------------------------------------------------------------------===//
// ObjCPropertyDecl
//===----------------------------------------------------------------------===//

void ObjCPropertyDecl::anchor() { }

ObjCPropertyDecl *ObjCPropertyDecl::Create(ASTContext &C, DeclContext *DC,
                                           SourceLocation L,
                                           IdentifierInfo *Id,
                                           SourceLocation AtLoc,
                                           SourceLocation LParenLoc,
                                           TypeSourceInfo *T,
                                           PropertyControl propControl) {
  return new (C, DC) ObjCPropertyDecl(DC, L, Id, AtLoc, LParenLoc, T);
}

ObjCPropertyDecl *ObjCPropertyDecl::CreateDeserialized(ASTContext &C,
                                                       unsigned ID) {
  return new (C, ID) ObjCPropertyDecl(0, SourceLocation(), 0, SourceLocation(),
                                      SourceLocation(), 0);
}

//===----------------------------------------------------------------------===//
// ObjCPropertyImplDecl
//===----------------------------------------------------------------------===//

ObjCPropertyImplDecl *ObjCPropertyImplDecl::Create(ASTContext &C,
                                                   DeclContext *DC,
                                                   SourceLocation atLoc,
                                                   SourceLocation L,
                                                   ObjCPropertyDecl *property,
                                                   Kind PK,
                                                   ObjCIvarDecl *ivar,
                                                   SourceLocation ivarLoc) {
  return new (C, DC) ObjCPropertyImplDecl(DC, atLoc, L, property, PK, ivar,
                                          ivarLoc);
}

ObjCPropertyImplDecl *ObjCPropertyImplDecl::CreateDeserialized(ASTContext &C,
                                                               unsigned ID) {
  return new (C, ID) ObjCPropertyImplDecl(0, SourceLocation(), SourceLocation(),
                                          0, Dynamic, 0, SourceLocation());
}

SourceRange ObjCPropertyImplDecl::getSourceRange() const {
  SourceLocation EndLoc = getLocation();
  if (IvarLoc.isValid())
    EndLoc = IvarLoc;

  return SourceRange(AtLoc, EndLoc);
}
