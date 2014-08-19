//===--- SerializedDiagnosticPrinter.cpp - Serializer for diagnostics -----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "clang/Frontend/SerializedDiagnosticPrinter.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/DiagnosticOptions.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/Version.h"
#include "clang/Frontend/DiagnosticRenderer.h"
#include "clang/Lex/Lexer.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/raw_ostream.h"
#include <vector>

using namespace clang;
using namespace clang::serialized_diags;

namespace {
  
class AbbreviationMap {
  llvm::DenseMap<unsigned, unsigned> Abbrevs;
public:
  AbbreviationMap() {}
  
  void set(unsigned recordID, unsigned abbrevID) {
    assert(Abbrevs.find(recordID) == Abbrevs.end() 
           && "Abbreviation already set.");
    Abbrevs[recordID] = abbrevID;
  }
  
  unsigned get(unsigned recordID) {
    assert(Abbrevs.find(recordID) != Abbrevs.end() &&
           "Abbreviation not set.");
    return Abbrevs[recordID];
  }
};
 
typedef SmallVector<uint64_t, 64> RecordData;
typedef SmallVectorImpl<uint64_t> RecordDataImpl;

class SDiagsWriter;
  
class SDiagsRenderer : public DiagnosticNoteRenderer {
  SDiagsWriter &Writer;
public:
  SDiagsRenderer(SDiagsWriter &Writer, const LangOptions &LangOpts,
                 DiagnosticOptions *DiagOpts)
    : DiagnosticNoteRenderer(LangOpts, DiagOpts), Writer(Writer) {}

  virtual ~SDiagsRenderer() {}
  
protected:
  void emitDiagnosticMessage(SourceLocation Loc,
                             PresumedLoc PLoc,
                             DiagnosticsEngine::Level Level,
                             StringRef Message,
                             ArrayRef<CharSourceRange> Ranges,
                             const SourceManager *SM,
                             DiagOrStoredDiag D) override;

  void emitDiagnosticLoc(SourceLocation Loc, PresumedLoc PLoc,
                         DiagnosticsEngine::Level Level,
                         ArrayRef<CharSourceRange> Ranges,
                         const SourceManager &SM) override {}

  void emitNote(SourceLocation Loc, StringRef Message,
                const SourceManager *SM) override;

  void emitCodeContext(SourceLocation Loc,
                       DiagnosticsEngine::Level Level,
                       SmallVectorImpl<CharSourceRange>& Ranges,
                       ArrayRef<FixItHint> Hints,
                       const SourceManager &SM) override;

  void beginDiagnostic(DiagOrStoredDiag D,
                       DiagnosticsEngine::Level Level) override;
  void endDiagnostic(DiagOrStoredDiag D,
                     DiagnosticsEngine::Level Level) override;
};
  
class SDiagsWriter : public DiagnosticConsumer {
  friend class SDiagsRenderer;

  struct SharedState;

  explicit SDiagsWriter(IntrusiveRefCntPtr<SharedState> State)
    : LangOpts(nullptr), OriginalInstance(false), State(State) {}

public:
  SDiagsWriter(raw_ostream *os, DiagnosticOptions *diags)
    : LangOpts(nullptr), OriginalInstance(true),
      State(new SharedState(os, diags))
  {
    EmitPreamble();
  }

  ~SDiagsWriter() {}

  void HandleDiagnostic(DiagnosticsEngine::Level DiagLevel,
                        const Diagnostic &Info) override;

  void BeginSourceFile(const LangOptions &LO, const Preprocessor *PP) override {
    LangOpts = &LO;
  }

  void finish() override;

private:
  /// \brief Emit the preamble for the serialized diagnostics.
  void EmitPreamble();
  
  /// \brief Emit the BLOCKINFO block.
  void EmitBlockInfoBlock();

  /// \brief Emit the META data block.
  void EmitMetaBlock();

  /// \brief Start a DIAG block.
  void EnterDiagBlock();

  /// \brief End a DIAG block.
  void ExitDiagBlock();

  /// \brief Emit a DIAG record.
  void EmitDiagnosticMessage(SourceLocation Loc,
                             PresumedLoc PLoc,
                             DiagnosticsEngine::Level Level,
                             StringRef Message,
                             const SourceManager *SM,
                             DiagOrStoredDiag D);

  /// \brief Emit FIXIT and SOURCE_RANGE records for a diagnostic.
  void EmitCodeContext(SmallVectorImpl<CharSourceRange> &Ranges,
                       ArrayRef<FixItHint> Hints,
                       const SourceManager &SM);

  /// \brief Emit a record for a CharSourceRange.
  void EmitCharSourceRange(CharSourceRange R, const SourceManager &SM);
  
  /// \brief Emit the string information for the category.
  unsigned getEmitCategory(unsigned category = 0);
  
  /// \brief Emit the string information for diagnostic flags.
  unsigned getEmitDiagnosticFlag(DiagnosticsEngine::Level DiagLevel,
                                 unsigned DiagID = 0);
  
  /// \brief Emit (lazily) the file string and retrieved the file identifier.
  unsigned getEmitFile(const char *Filename);

  /// \brief Add SourceLocation information the specified record.  
  void AddLocToRecord(SourceLocation Loc, const SourceManager *SM,
                      PresumedLoc PLoc, RecordDataImpl &Record,
                      unsigned TokSize = 0);

  /// \brief Add SourceLocation information the specified record.
  void AddLocToRecord(SourceLocation Loc, RecordDataImpl &Record,
                      const SourceManager *SM,
                      unsigned TokSize = 0) {
    AddLocToRecord(Loc, SM, SM ? SM->getPresumedLoc(Loc) : PresumedLoc(),
                   Record, TokSize);
  }

  /// \brief Add CharSourceRange information the specified record.
  void AddCharSourceRangeToRecord(CharSourceRange R, RecordDataImpl &Record,
                                  const SourceManager &SM);

  /// \brief The version of the diagnostics file.
  enum { Version = 2 };

  /// \brief Language options, which can differ from one clone of this client
  /// to another.
  const LangOptions *LangOpts;

  /// \brief Whether this is the original instance (rather than one of its
  /// clones), responsible for writing the file at the end.
  bool OriginalInstance;

  /// \brief State that is shared among the various clones of this diagnostic
  /// consumer.
  struct SharedState : RefCountedBase<SharedState> {
    SharedState(raw_ostream *os, DiagnosticOptions *diags)
      : DiagOpts(diags), Stream(Buffer), OS(os), EmittedAnyDiagBlocks(false) { }

    /// \brief Diagnostic options.
    IntrusiveRefCntPtr<DiagnosticOptions> DiagOpts;

    /// \brief The byte buffer for the serialized content.
    SmallString<1024> Buffer;

    /// \brief The BitStreamWriter for the serialized diagnostics.
    llvm::BitstreamWriter Stream;

    /// \brief The name of the diagnostics file.
    std::unique_ptr<raw_ostream> OS;

    /// \brief The set of constructed record abbreviations.
    AbbreviationMap Abbrevs;

    /// \brief A utility buffer for constructing record content.
    RecordData Record;

    /// \brief A text buffer for rendering diagnostic text.
    SmallString<256> diagBuf;

    /// \brief The collection of diagnostic categories used.
    llvm::DenseSet<unsigned> Categories;

    /// \brief The collection of files used.
    llvm::DenseMap<const char *, unsigned> Files;

    typedef llvm::DenseMap<const void *, std::pair<unsigned, StringRef> >
    DiagFlagsTy;

    /// \brief Map for uniquing strings.
    DiagFlagsTy DiagFlags;

    /// \brief Whether we have already started emission of any DIAG blocks. Once
    /// this becomes \c true, we never close a DIAG block until we know that we're
    /// starting another one or we're done.
    bool EmittedAnyDiagBlocks;
  };

  /// \brief State shared among the various clones of this diagnostic consumer.
  IntrusiveRefCntPtr<SharedState> State;
};
} // end anonymous namespace

namespace clang {
namespace serialized_diags {
DiagnosticConsumer *create(raw_ostream *OS, DiagnosticOptions *diags) {
  return new SDiagsWriter(OS, diags);
}
} // end namespace serialized_diags
} // end namespace clang

//===----------------------------------------------------------------------===//
// Serialization methods.
//===----------------------------------------------------------------------===//

/// \brief Emits a block ID in the BLOCKINFO block.
static void EmitBlockID(unsigned ID, const char *Name,
                        llvm::BitstreamWriter &Stream,
                        RecordDataImpl &Record) {
  Record.clear();
  Record.push_back(ID);
  Stream.EmitRecord(llvm::bitc::BLOCKINFO_CODE_SETBID, Record);
  
  // Emit the block name if present.
  if (!Name || Name[0] == 0)
    return;

  Record.clear();

  while (*Name)
    Record.push_back(*Name++);

  Stream.EmitRecord(llvm::bitc::BLOCKINFO_CODE_BLOCKNAME, Record);
}

/// \brief Emits a record ID in the BLOCKINFO block.
static void EmitRecordID(unsigned ID, const char *Name,
                         llvm::BitstreamWriter &Stream,
                         RecordDataImpl &Record){
  Record.clear();
  Record.push_back(ID);

  while (*Name)
    Record.push_back(*Name++);

  Stream.EmitRecord(llvm::bitc::BLOCKINFO_CODE_SETRECORDNAME, Record);
}

void SDiagsWriter::AddLocToRecord(SourceLocation Loc,
                                  const SourceManager *SM,
                                  PresumedLoc PLoc,
                                  RecordDataImpl &Record,
                                  unsigned TokSize) {
  if (PLoc.isInvalid()) {
    // Emit a "sentinel" location.
    Record.push_back((unsigned)0); // File.
    Record.push_back((unsigned)0); // Line.
    Record.push_back((unsigned)0); // Column.
    Record.push_back((unsigned)0); // Offset.
    return;
  }

  Record.push_back(getEmitFile(PLoc.getFilename()));
  Record.push_back(PLoc.getLine());
  Record.push_back(PLoc.getColumn()+TokSize);
  Record.push_back(SM->getFileOffset(Loc));
}

void SDiagsWriter::AddCharSourceRangeToRecord(CharSourceRange Range,
                                              RecordDataImpl &Record,
                                              const SourceManager &SM) {
  AddLocToRecord(Range.getBegin(), Record, &SM);
  unsigned TokSize = 0;
  if (Range.isTokenRange())
    TokSize = Lexer::MeasureTokenLength(Range.getEnd(),
                                        SM, *LangOpts);
  
  AddLocToRecord(Range.getEnd(), Record, &SM, TokSize);
}

unsigned SDiagsWriter::getEmitFile(const char *FileName){
  if (!FileName)
    return 0;
  
  unsigned &entry = State->Files[FileName];
  if (entry)
    return entry;
  
  // Lazily generate the record for the file.
  entry = State->Files.size();
  RecordData Record;
  Record.push_back(RECORD_FILENAME);
  Record.push_back(entry);
  Record.push_back(0); // For legacy.
  Record.push_back(0); // For legacy.
  StringRef Name(FileName);
  Record.push_back(Name.size());
  State->Stream.EmitRecordWithBlob(State->Abbrevs.get(RECORD_FILENAME), Record,
                                   Name);

  return entry;
}

void SDiagsWriter::EmitCharSourceRange(CharSourceRange R,
                                       const SourceManager &SM) {
  State->Record.clear();
  State->Record.push_back(RECORD_SOURCE_RANGE);
  AddCharSourceRangeToRecord(R, State->Record, SM);
  State->Stream.EmitRecordWithAbbrev(State->Abbrevs.get(RECORD_SOURCE_RANGE),
                                     State->Record);
}

/// \brief Emits the preamble of the diagnostics file.
void SDiagsWriter::EmitPreamble() {
  // Emit the file header.
  State->Stream.Emit((unsigned)'D', 8);
  State->Stream.Emit((unsigned)'I', 8);
  State->Stream.Emit((unsigned)'A', 8);
  State->Stream.Emit((unsigned)'G', 8);

  EmitBlockInfoBlock();
  EmitMetaBlock();
}

static void AddSourceLocationAbbrev(llvm::BitCodeAbbrev *Abbrev) {
  using namespace llvm;
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 10)); // File ID.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 32)); // Line.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 32)); // Column.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 32)); // Offset;
}

static void AddRangeLocationAbbrev(llvm::BitCodeAbbrev *Abbrev) {
  AddSourceLocationAbbrev(Abbrev);
  AddSourceLocationAbbrev(Abbrev);  
}

void SDiagsWriter::EmitBlockInfoBlock() {
  State->Stream.EnterBlockInfoBlock(3);

  using namespace llvm;
  llvm::BitstreamWriter &Stream = State->Stream;
  RecordData &Record = State->Record;
  AbbreviationMap &Abbrevs = State->Abbrevs;

  // ==---------------------------------------------------------------------==//
  // The subsequent records and Abbrevs are for the "Meta" block.
  // ==---------------------------------------------------------------------==//

  EmitBlockID(BLOCK_META, "Meta", Stream, Record);
  EmitRecordID(RECORD_VERSION, "Version", Stream, Record);
  BitCodeAbbrev *Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(RECORD_VERSION));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 32));
  Abbrevs.set(RECORD_VERSION, Stream.EmitBlockInfoAbbrev(BLOCK_META, Abbrev));

  // ==---------------------------------------------------------------------==//
  // The subsequent records and Abbrevs are for the "Diagnostic" block.
  // ==---------------------------------------------------------------------==//

  EmitBlockID(BLOCK_DIAG, "Diag", Stream, Record);
  EmitRecordID(RECORD_DIAG, "DiagInfo", Stream, Record);
  EmitRecordID(RECORD_SOURCE_RANGE, "SrcRange", Stream, Record);
  EmitRecordID(RECORD_CATEGORY, "CatName", Stream, Record);
  EmitRecordID(RECORD_DIAG_FLAG, "DiagFlag", Stream, Record);
  EmitRecordID(RECORD_FILENAME, "FileName", Stream, Record);
  EmitRecordID(RECORD_FIXIT, "FixIt", Stream, Record);

  // Emit abbreviation for RECORD_DIAG.
  Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(RECORD_DIAG));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 3));  // Diag level.
  AddSourceLocationAbbrev(Abbrev);
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 10)); // Category.  
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 10)); // Mapped Diag ID.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 16)); // Text size.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob)); // Diagnostc text.
  Abbrevs.set(RECORD_DIAG, Stream.EmitBlockInfoAbbrev(BLOCK_DIAG, Abbrev));
  
  // Emit abbrevation for RECORD_CATEGORY.
  Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(RECORD_CATEGORY));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 16)); // Category ID.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 8));  // Text size.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob));      // Category text.
  Abbrevs.set(RECORD_CATEGORY, Stream.EmitBlockInfoAbbrev(BLOCK_DIAG, Abbrev));

  // Emit abbrevation for RECORD_SOURCE_RANGE.
  Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(RECORD_SOURCE_RANGE));
  AddRangeLocationAbbrev(Abbrev);
  Abbrevs.set(RECORD_SOURCE_RANGE,
              Stream.EmitBlockInfoAbbrev(BLOCK_DIAG, Abbrev));
  
  // Emit the abbreviation for RECORD_DIAG_FLAG.
  Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(RECORD_DIAG_FLAG));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 10)); // Mapped Diag ID.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 16)); // Text size.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob)); // Flag name text.
  Abbrevs.set(RECORD_DIAG_FLAG, Stream.EmitBlockInfoAbbrev(BLOCK_DIAG,
                                                           Abbrev));
  
  // Emit the abbreviation for RECORD_FILENAME.
  Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(RECORD_FILENAME));
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 10)); // Mapped file ID.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 32)); // Size.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 32)); // Modifcation time.  
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 16)); // Text size.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob)); // File name text.
  Abbrevs.set(RECORD_FILENAME, Stream.EmitBlockInfoAbbrev(BLOCK_DIAG,
                                                          Abbrev));
  
  // Emit the abbreviation for RECORD_FIXIT.
  Abbrev = new BitCodeAbbrev();
  Abbrev->Add(BitCodeAbbrevOp(RECORD_FIXIT));
  AddRangeLocationAbbrev(Abbrev);
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Fixed, 16)); // Text size.
  Abbrev->Add(BitCodeAbbrevOp(BitCodeAbbrevOp::Blob));      // FixIt text.
  Abbrevs.set(RECORD_FIXIT, Stream.EmitBlockInfoAbbrev(BLOCK_DIAG,
                                                       Abbrev));

  Stream.ExitBlock();
}

void SDiagsWriter::EmitMetaBlock() {
  llvm::BitstreamWriter &Stream = State->Stream;
  RecordData &Record = State->Record;
  AbbreviationMap &Abbrevs = State->Abbrevs;

  Stream.EnterSubblock(BLOCK_META, 3);
  Record.clear();
  Record.push_back(RECORD_VERSION);
  Record.push_back(Version);
  Stream.EmitRecordWithAbbrev(Abbrevs.get(RECORD_VERSION), Record);  
  Stream.ExitBlock();
}

unsigned SDiagsWriter::getEmitCategory(unsigned int category) {
  if (State->Categories.count(category))
    return category;
  
  State->Categories.insert(category);
  
  // We use a local version of 'Record' so that we can be generating
  // another record when we lazily generate one for the category entry.
  RecordData Record;
  Record.push_back(RECORD_CATEGORY);
  Record.push_back(category);
  StringRef catName = DiagnosticIDs::getCategoryNameFromID(category);
  Record.push_back(catName.size());
  State->Stream.EmitRecordWithBlob(State->Abbrevs.get(RECORD_CATEGORY), Record,
                                   catName);
  
  return category;
}

unsigned SDiagsWriter::getEmitDiagnosticFlag(DiagnosticsEngine::Level DiagLevel,
                                             unsigned DiagID) {
  if (DiagLevel == DiagnosticsEngine::Note)
    return 0; // No flag for notes.
  
  StringRef FlagName = DiagnosticIDs::getWarningOptionForDiag(DiagID);
  if (FlagName.empty())
    return 0;

  // Here we assume that FlagName points to static data whose pointer
  // value is fixed.  This allows us to unique by diagnostic groups.
  const void *data = FlagName.data();
  std::pair<unsigned, StringRef> &entry = State->DiagFlags[data];
  if (entry.first == 0) {
    entry.first = State->DiagFlags.size();
    entry.second = FlagName;
    
    // Lazily emit the string in a separate record.
    RecordData Record;
    Record.push_back(RECORD_DIAG_FLAG);
    Record.push_back(entry.first);
    Record.push_back(FlagName.size());
    State->Stream.EmitRecordWithBlob(State->Abbrevs.get(RECORD_DIAG_FLAG),
                                     Record, FlagName);
  }

  return entry.first;
}

void SDiagsWriter::HandleDiagnostic(DiagnosticsEngine::Level DiagLevel,
                                    const Diagnostic &Info) {
  // Enter the block for a non-note diagnostic immediately, rather than waiting
  // for beginDiagnostic, in case associated notes are emitted before we get
  // there.
  if (DiagLevel != DiagnosticsEngine::Note) {
    if (State->EmittedAnyDiagBlocks)
      ExitDiagBlock();

    EnterDiagBlock();
    State->EmittedAnyDiagBlocks = true;
  }

  // Compute the diagnostic text.
  State->diagBuf.clear();
  Info.FormatDiagnostic(State->diagBuf);

  if (Info.getLocation().isInvalid()) {
    // Special-case diagnostics with no location. We may not have entered a
    // source file in this case, so we can't use the normal DiagnosticsRenderer
    // machinery.

    // Make sure we bracket all notes as "sub-diagnostics".  This matches
    // the behavior in SDiagsRenderer::emitDiagnostic().
    if (DiagLevel == DiagnosticsEngine::Note)
      EnterDiagBlock();

    EmitDiagnosticMessage(SourceLocation(), PresumedLoc(), DiagLevel,
                          State->diagBuf, nullptr, &Info);

    if (DiagLevel == DiagnosticsEngine::Note)
      ExitDiagBlock();

    return;
  }

  assert(Info.hasSourceManager() && LangOpts &&
         "Unexpected diagnostic with valid location outside of a source file");
  SDiagsRenderer Renderer(*this, *LangOpts, &*State->DiagOpts);
  Renderer.emitDiagnostic(Info.getLocation(), DiagLevel,
                          State->diagBuf.str(),
                          Info.getRanges(),
                          Info.getFixItHints(),
                          &Info.getSourceManager(),
                          &Info);
}

static serialized_diags::Level getStableLevel(DiagnosticsEngine::Level Level) {
  switch (Level) {
#define CASE(X) case DiagnosticsEngine::X: return serialized_diags::X;
  CASE(Ignored)
  CASE(Note)
  CASE(Remark)
  CASE(Warning)
  CASE(Error)
  CASE(Fatal)
#undef CASE
  }

  llvm_unreachable("invalid diagnostic level");
}

void SDiagsWriter::EmitDiagnosticMessage(SourceLocation Loc,
                                         PresumedLoc PLoc,
                                         DiagnosticsEngine::Level Level,
                                         StringRef Message,
                                         const SourceManager *SM,
                                         DiagOrStoredDiag D) {
  llvm::BitstreamWriter &Stream = State->Stream;
  RecordData &Record = State->Record;
  AbbreviationMap &Abbrevs = State->Abbrevs;
  
  // Emit the RECORD_DIAG record.
  Record.clear();
  Record.push_back(RECORD_DIAG);
  Record.push_back(getStableLevel(Level));
  AddLocToRecord(Loc, SM, PLoc, Record);

  if (const Diagnostic *Info = D.dyn_cast<const Diagnostic*>()) {
    // Emit the category string lazily and get the category ID.
    unsigned DiagID = DiagnosticIDs::getCategoryNumberForDiag(Info->getID());
    Record.push_back(getEmitCategory(DiagID));
    // Emit the diagnostic flag string lazily and get the mapped ID.
    Record.push_back(getEmitDiagnosticFlag(Level, Info->getID()));
  } else {
    Record.push_back(getEmitCategory());
    Record.push_back(getEmitDiagnosticFlag(Level));
  }

  Record.push_back(Message.size());
  Stream.EmitRecordWithBlob(Abbrevs.get(RECORD_DIAG), Record, Message);
}

void
SDiagsRenderer::emitDiagnosticMessage(SourceLocation Loc,
                                      PresumedLoc PLoc,
                                      DiagnosticsEngine::Level Level,
                                      StringRef Message,
                                      ArrayRef<clang::CharSourceRange> Ranges,
                                      const SourceManager *SM,
                                      DiagOrStoredDiag D) {
  Writer.EmitDiagnosticMessage(Loc, PLoc, Level, Message, SM, D);
}

void SDiagsWriter::EnterDiagBlock() {
  State->Stream.EnterSubblock(BLOCK_DIAG, 4);
}

void SDiagsWriter::ExitDiagBlock() {
  State->Stream.ExitBlock();
}

void SDiagsRenderer::beginDiagnostic(DiagOrStoredDiag D,
                                     DiagnosticsEngine::Level Level) {
  if (Level == DiagnosticsEngine::Note)
    Writer.EnterDiagBlock();
}

void SDiagsRenderer::endDiagnostic(DiagOrStoredDiag D,
                                   DiagnosticsEngine::Level Level) {
  // Only end note diagnostics here, because we can't be sure when we've seen
  // the last note associated with a non-note diagnostic.
  if (Level == DiagnosticsEngine::Note)
    Writer.ExitDiagBlock();
}

void SDiagsWriter::EmitCodeContext(SmallVectorImpl<CharSourceRange> &Ranges,
                                   ArrayRef<FixItHint> Hints,
                                   const SourceManager &SM) {
  llvm::BitstreamWriter &Stream = State->Stream;
  RecordData &Record = State->Record;
  AbbreviationMap &Abbrevs = State->Abbrevs;

  // Emit Source Ranges.
  for (ArrayRef<CharSourceRange>::iterator I = Ranges.begin(), E = Ranges.end();
       I != E; ++I)
    if (I->isValid())
      EmitCharSourceRange(*I, SM);

  // Emit FixIts.
  for (ArrayRef<FixItHint>::iterator I = Hints.begin(), E = Hints.end();
       I != E; ++I) {
    const FixItHint &Fix = *I;
    if (Fix.isNull())
      continue;
    Record.clear();
    Record.push_back(RECORD_FIXIT);
    AddCharSourceRangeToRecord(Fix.RemoveRange, Record, SM);
    Record.push_back(Fix.CodeToInsert.size());
    Stream.EmitRecordWithBlob(Abbrevs.get(RECORD_FIXIT), Record,
                              Fix.CodeToInsert);
  }
}

void SDiagsRenderer::emitCodeContext(SourceLocation Loc,
                                     DiagnosticsEngine::Level Level,
                                     SmallVectorImpl<CharSourceRange> &Ranges,
                                     ArrayRef<FixItHint> Hints,
                                     const SourceManager &SM) {
  Writer.EmitCodeContext(Ranges, Hints, SM);
}

void SDiagsRenderer::emitNote(SourceLocation Loc, StringRef Message,
                              const SourceManager *SM) {
  Writer.EnterDiagBlock();
  PresumedLoc PLoc = SM ? SM->getPresumedLoc(Loc) : PresumedLoc();
  Writer.EmitDiagnosticMessage(Loc, PLoc, DiagnosticsEngine::Note,
                               Message, SM, DiagOrStoredDiag());
  Writer.ExitDiagBlock();
}

void SDiagsWriter::finish() {
  // The original instance is responsible for writing the file.
  if (!OriginalInstance)
    return;

  // Finish off any diagnostic we were in the process of emitting.
  if (State->EmittedAnyDiagBlocks)
    ExitDiagBlock();

  // Write the generated bitstream to "Out".
  State->OS->write((char *)&State->Buffer.front(), State->Buffer.size());
  State->OS->flush();

  State->OS.reset();
}
