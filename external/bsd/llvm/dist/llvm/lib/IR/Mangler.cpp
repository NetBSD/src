//===-- Mangler.cpp - Self-contained c/asm llvm name mangler --------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Unified name mangler for assembly backends.
//
//===----------------------------------------------------------------------===//

#include "llvm/IR/Mangler.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

/// getNameWithPrefix - Fill OutName with the name of the appropriate prefix
/// and the specified name as the global variable name.  GVName must not be
/// empty.
void Mangler::getNameWithPrefix(SmallVectorImpl<char> &OutName,
                                const Twine &GVName, ManglerPrefixTy PrefixTy) {
  SmallString<256> TmpData;
  StringRef Name = GVName.toStringRef(TmpData);
  assert(!Name.empty() && "getNameWithPrefix requires non-empty name");

  // If the global name is not led with \1, add the appropriate prefixes.
  if (Name[0] == '\1') {
    Name = Name.substr(1);
  } else {
    if (PrefixTy == Mangler::Private) {
      const char *Prefix = DL->getPrivateGlobalPrefix();
      OutName.append(Prefix, Prefix+strlen(Prefix));
    } else if (PrefixTy == Mangler::LinkerPrivate) {
      const char *Prefix = DL->getLinkerPrivateGlobalPrefix();
      OutName.append(Prefix, Prefix+strlen(Prefix));
    }

    char Prefix = DL->getGlobalPrefix();
    if (Prefix != '\0')
      OutName.push_back(Prefix);
  }

  // If this is a simple string that doesn't need escaping, just append it.
  OutName.append(Name.begin(), Name.end());
}

/// AddFastCallStdCallSuffix - Microsoft fastcall and stdcall functions require
/// a suffix on their name indicating the number of words of arguments they
/// take.
static void AddFastCallStdCallSuffix(SmallVectorImpl<char> &OutName,
                                     const Function *F, const DataLayout &TD) {
  // Calculate arguments size total.
  unsigned ArgWords = 0;
  for (Function::const_arg_iterator AI = F->arg_begin(), AE = F->arg_end();
       AI != AE; ++AI) {
    Type *Ty = AI->getType();
    // 'Dereference' type in case of byval parameter attribute
    if (AI->hasByValAttr())
      Ty = cast<PointerType>(Ty)->getElementType();
    // Size should be aligned to DWORD boundary
    ArgWords += ((TD.getTypeAllocSize(Ty) + 3)/4)*4;
  }
  
  raw_svector_ostream(OutName) << '@' << ArgWords;
}


/// getNameWithPrefix - Fill OutName with the name of the appropriate prefix
/// and the specified global variable's name.  If the global variable doesn't
/// have a name, this fills in a unique name for the global.
void Mangler::getNameWithPrefix(SmallVectorImpl<char> &OutName,
                                const GlobalValue *GV) {
  ManglerPrefixTy PrefixTy = Mangler::Default;
  if (GV->hasPrivateLinkage())
    PrefixTy = Mangler::Private;
  else if (GV->hasLinkerPrivateLinkage() || GV->hasLinkerPrivateWeakLinkage())
    PrefixTy = Mangler::LinkerPrivate;

  size_t NameBegin = OutName.size();

  // If this global has a name, handle it simply.
  if (GV->hasName()) {
    StringRef Name = GV->getName();
    getNameWithPrefix(OutName, Name, PrefixTy);
    // No need to do anything else if the global has the special "do not mangle"
    // flag in the name.
    if (Name[0] == 1)
      return;
  } else {
    // Get the ID for the global, assigning a new one if we haven't got one
    // already.
    unsigned &ID = AnonGlobalIDs[GV];
    if (ID == 0) ID = NextAnonGlobalID++;
  
    // Must mangle the global into a unique ID.
    getNameWithPrefix(OutName, "__unnamed_" + Twine(ID), PrefixTy);
  }

  // If we are supposed to add a microsoft-style suffix for stdcall/fastcall,
  // add it.
  if (DL->hasMicrosoftFastStdCallMangling()) {
    if (const Function *F = dyn_cast<Function>(GV)) {
      CallingConv::ID CC = F->getCallingConv();
    
      // fastcall functions need to start with @ instead of _.
      if (CC == CallingConv::X86_FastCall) {
        assert(OutName[NameBegin] == '_' && DL->getGlobalPrefix() == '_');
        OutName[NameBegin] = '@';
      }
    
      // fastcall and stdcall functions usually need @42 at the end to specify
      // the argument info.
      FunctionType *FT = F->getFunctionType();
      if ((CC == CallingConv::X86_FastCall || CC == CallingConv::X86_StdCall) &&
          // "Pure" variadic functions do not receive @0 suffix.
          (!FT->isVarArg() || FT->getNumParams() == 0 ||
           (FT->getNumParams() == 1 && F->hasStructRetAttr())))
        AddFastCallStdCallSuffix(OutName, F, *DL);
    }
  }
}
