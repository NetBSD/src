//===--- CommentCommandTraits.cpp - Comment command properties --*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/CommentCommandTraits.h"
#include "llvm/ADT/STLExtras.h"

namespace clang {
namespace comments {

#include "clang/AST/CommentCommandInfo.inc"

CommandTraits::CommandTraits(llvm::BumpPtrAllocator &Allocator,
                             const CommentOptions &CommentOptions) :
    NextID(llvm::array_lengthof(Commands)), Allocator(Allocator) {
  registerCommentOptions(CommentOptions);
}

void CommandTraits::registerCommentOptions(
    const CommentOptions &CommentOptions) {
  for (CommentOptions::BlockCommandNamesTy::const_iterator
           I = CommentOptions.BlockCommandNames.begin(),
           E = CommentOptions.BlockCommandNames.end();
       I != E; I++) {
    registerBlockCommand(*I);
  }
}

const CommandInfo *CommandTraits::getCommandInfoOrNULL(StringRef Name) const {
  if (const CommandInfo *Info = getBuiltinCommandInfo(Name))
    return Info;
  return getRegisteredCommandInfo(Name);
}

const CommandInfo *CommandTraits::getCommandInfo(unsigned CommandID) const {
  if (const CommandInfo *Info = getBuiltinCommandInfo(CommandID))
    return Info;
  return getRegisteredCommandInfo(CommandID);
}

static void
HelperTypoCorrectCommandInfo(SmallVectorImpl<const CommandInfo *> &BestCommand,
                             StringRef Typo, const CommandInfo *Command) {
  const unsigned MaxEditDistance = 1;
  unsigned BestEditDistance = MaxEditDistance + 1;
  StringRef Name = Command->Name;
  
  unsigned MinPossibleEditDistance = abs((int)Name.size() - (int)Typo.size());
  if (MinPossibleEditDistance > 0 &&
      Typo.size() / MinPossibleEditDistance < 1)
    return;
  unsigned EditDistance = Typo.edit_distance(Name, true, MaxEditDistance);
  if (EditDistance > MaxEditDistance)
    return;
  if (EditDistance == BestEditDistance)
    BestCommand.push_back(Command);
  else if (EditDistance < BestEditDistance) {
    BestCommand.clear();
    BestCommand.push_back(Command);
    BestEditDistance = EditDistance;
  }
}

const CommandInfo *
CommandTraits::getTypoCorrectCommandInfo(StringRef Typo) const {
  // single character command impostures, such as \t or \n must not go
  // through the fixit logic.
  if (Typo.size() <= 1)
    return NULL;
  
  SmallVector<const CommandInfo *, 2> BestCommand;
  
  const int NumOfCommands = llvm::array_lengthof(Commands);
  for (int i = 0; i < NumOfCommands; i++)
    HelperTypoCorrectCommandInfo(BestCommand, Typo, &Commands[i]);
  
  for (unsigned i = 0, e = RegisteredCommands.size(); i != e; ++i)
    if (!RegisteredCommands[i]->IsUnknownCommand)
      HelperTypoCorrectCommandInfo(BestCommand, Typo, RegisteredCommands[i]);
  
  return (BestCommand.size() != 1) ? NULL : BestCommand[0];
}

CommandInfo *CommandTraits::createCommandInfoWithName(StringRef CommandName) {
  char *Name = Allocator.Allocate<char>(CommandName.size() + 1);
  memcpy(Name, CommandName.data(), CommandName.size());
  Name[CommandName.size()] = '\0';

  // Value-initialize (=zero-initialize in this case) a new CommandInfo.
  CommandInfo *Info = new (Allocator) CommandInfo();
  Info->Name = Name;
  Info->ID = NextID++;

  RegisteredCommands.push_back(Info);

  return Info;
}

const CommandInfo *CommandTraits::registerUnknownCommand(
                                                  StringRef CommandName) {
  CommandInfo *Info = createCommandInfoWithName(CommandName);
  Info->IsUnknownCommand = true;
  return Info;
}

const CommandInfo *CommandTraits::registerBlockCommand(StringRef CommandName) {
  CommandInfo *Info = createCommandInfoWithName(CommandName);
  Info->IsBlockCommand = true;
  return Info;
}

const CommandInfo *CommandTraits::getBuiltinCommandInfo(
                                                  unsigned CommandID) {
  if (CommandID < llvm::array_lengthof(Commands))
    return &Commands[CommandID];
  return NULL;
}

const CommandInfo *CommandTraits::getRegisteredCommandInfo(
                                                  StringRef Name) const {
  for (unsigned i = 0, e = RegisteredCommands.size(); i != e; ++i) {
    if (RegisteredCommands[i]->Name == Name)
      return RegisteredCommands[i];
  }
  return NULL;
}

const CommandInfo *CommandTraits::getRegisteredCommandInfo(
                                                  unsigned CommandID) const {
  return RegisteredCommands[CommandID - llvm::array_lengthof(Commands)];
}

} // end namespace comments
} // end namespace clang

