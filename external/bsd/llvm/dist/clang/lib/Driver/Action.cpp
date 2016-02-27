//===--- Action.cpp - Abstract compilation steps --------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Driver/Action.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Regex.h"
#include <cassert>
using namespace clang::driver;
using namespace llvm::opt;

Action::~Action() {}

const char *Action::getClassName(ActionClass AC) {
  switch (AC) {
  case InputClass: return "input";
  case BindArchClass: return "bind-arch";
  case CudaDeviceClass: return "cuda-device";
  case CudaHostClass: return "cuda-host";
  case PreprocessJobClass: return "preprocessor";
  case PrecompileJobClass: return "precompiler";
  case AnalyzeJobClass: return "analyzer";
  case MigrateJobClass: return "migrator";
  case CompileJobClass: return "compiler";
  case BackendJobClass: return "backend";
  case AssembleJobClass: return "assembler";
  case LinkJobClass: return "linker";
  case LipoJobClass: return "lipo";
  case DsymutilJobClass: return "dsymutil";
  case VerifyDebugInfoJobClass: return "verify-debug-info";
  case VerifyPCHJobClass: return "verify-pch";
  }

  llvm_unreachable("invalid class");
}

void InputAction::anchor() {}

InputAction::InputAction(const Arg &_Input, types::ID _Type)
  : Action(InputClass, _Type), Input(_Input) {
}

void BindArchAction::anchor() {}

BindArchAction::BindArchAction(Action *Input, const char *_ArchName)
    : Action(BindArchClass, Input), ArchName(_ArchName) {}

// Converts CUDA GPU architecture, e.g. "sm_21", to its corresponding virtual
// compute arch, e.g. "compute_20".  Returns null if the input arch is null or
// doesn't match an existing arch.
static const char* GpuArchToComputeName(const char *ArchName) {
  if (!ArchName)
    return nullptr;
  return llvm::StringSwitch<const char *>(ArchName)
      .Cases("sm_20", "sm_21", "compute_20")
      .Case("sm_30", "compute_30")
      .Case("sm_32", "compute_32")
      .Case("sm_35", "compute_35")
      .Case("sm_37", "compute_37")
      .Case("sm_50", "compute_50")
      .Case("sm_52", "compute_52")
      .Case("sm_53", "compute_53")
      .Default(nullptr);
}

void CudaDeviceAction::anchor() {}

CudaDeviceAction::CudaDeviceAction(Action *Input, const char *ArchName,
                                   bool AtTopLevel)
    : Action(CudaDeviceClass, Input), GpuArchName(ArchName),
      AtTopLevel(AtTopLevel) {
  assert(IsValidGpuArchName(GpuArchName));
}

const char *CudaDeviceAction::getComputeArchName() const {
  return GpuArchToComputeName(GpuArchName);
}

bool CudaDeviceAction::IsValidGpuArchName(llvm::StringRef ArchName) {
  return GpuArchToComputeName(ArchName.data()) != nullptr;
}

void CudaHostAction::anchor() {}

CudaHostAction::CudaHostAction(Action *Input, const ActionList &DeviceActions)
    : Action(CudaHostClass, Input), DeviceActions(DeviceActions) {}

void JobAction::anchor() {}

JobAction::JobAction(ActionClass Kind, Action *Input, types::ID Type)
    : Action(Kind, Input, Type) {}

JobAction::JobAction(ActionClass Kind, const ActionList &Inputs, types::ID Type)
  : Action(Kind, Inputs, Type) {
}

void PreprocessJobAction::anchor() {}

PreprocessJobAction::PreprocessJobAction(Action *Input, types::ID OutputType)
    : JobAction(PreprocessJobClass, Input, OutputType) {}

void PrecompileJobAction::anchor() {}

PrecompileJobAction::PrecompileJobAction(Action *Input, types::ID OutputType)
    : JobAction(PrecompileJobClass, Input, OutputType) {}

void AnalyzeJobAction::anchor() {}

AnalyzeJobAction::AnalyzeJobAction(Action *Input, types::ID OutputType)
    : JobAction(AnalyzeJobClass, Input, OutputType) {}

void MigrateJobAction::anchor() {}

MigrateJobAction::MigrateJobAction(Action *Input, types::ID OutputType)
    : JobAction(MigrateJobClass, Input, OutputType) {}

void CompileJobAction::anchor() {}

CompileJobAction::CompileJobAction(Action *Input, types::ID OutputType)
    : JobAction(CompileJobClass, Input, OutputType) {}

void BackendJobAction::anchor() {}

BackendJobAction::BackendJobAction(Action *Input, types::ID OutputType)
    : JobAction(BackendJobClass, Input, OutputType) {}

void AssembleJobAction::anchor() {}

AssembleJobAction::AssembleJobAction(Action *Input, types::ID OutputType)
    : JobAction(AssembleJobClass, Input, OutputType) {}

void LinkJobAction::anchor() {}

LinkJobAction::LinkJobAction(ActionList &Inputs, types::ID Type)
  : JobAction(LinkJobClass, Inputs, Type) {
}

void LipoJobAction::anchor() {}

LipoJobAction::LipoJobAction(ActionList &Inputs, types::ID Type)
  : JobAction(LipoJobClass, Inputs, Type) {
}

void DsymutilJobAction::anchor() {}

DsymutilJobAction::DsymutilJobAction(ActionList &Inputs, types::ID Type)
  : JobAction(DsymutilJobClass, Inputs, Type) {
}

void VerifyJobAction::anchor() {}

VerifyJobAction::VerifyJobAction(ActionClass Kind, Action *Input,
                                 types::ID Type)
    : JobAction(Kind, Input, Type) {
  assert((Kind == VerifyDebugInfoJobClass || Kind == VerifyPCHJobClass) &&
         "ActionClass is not a valid VerifyJobAction");
}

void VerifyDebugInfoJobAction::anchor() {}

VerifyDebugInfoJobAction::VerifyDebugInfoJobAction(Action *Input,
                                                   types::ID Type)
    : VerifyJobAction(VerifyDebugInfoJobClass, Input, Type) {}

void VerifyPCHJobAction::anchor() {}

VerifyPCHJobAction::VerifyPCHJobAction(Action *Input, types::ID Type)
    : VerifyJobAction(VerifyPCHJobClass, Input, Type) {}
