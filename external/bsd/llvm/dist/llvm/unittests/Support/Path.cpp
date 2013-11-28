//===- llvm/unittest/Support/Path.cpp - Path tests ------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Path.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/raw_ostream.h"
#include "gtest/gtest.h"

using namespace llvm;
using namespace llvm::sys;

#define ASSERT_NO_ERROR(x) \
  if (error_code ASSERT_NO_ERROR_ec = x) { \
    SmallString<128> MessageStorage; \
    raw_svector_ostream Message(MessageStorage); \
    Message << #x ": did not return errc::success.\n" \
            << "error number: " << ASSERT_NO_ERROR_ec.value() << "\n" \
            << "error message: " << ASSERT_NO_ERROR_ec.message() << "\n"; \
    GTEST_FATAL_FAILURE_(MessageStorage.c_str()); \
  } else {}

namespace {

TEST(is_separator, Works) {
  EXPECT_TRUE(path::is_separator('/'));
  EXPECT_FALSE(path::is_separator('\0'));
  EXPECT_FALSE(path::is_separator('-'));
  EXPECT_FALSE(path::is_separator(' '));

#ifdef LLVM_ON_WIN32
  EXPECT_TRUE(path::is_separator('\\'));
#else
  EXPECT_FALSE(path::is_separator('\\'));
#endif
}

TEST(Support, Path) {
  SmallVector<StringRef, 40> paths;
  paths.push_back("");
  paths.push_back(".");
  paths.push_back("..");
  paths.push_back("foo");
  paths.push_back("/");
  paths.push_back("/foo");
  paths.push_back("foo/");
  paths.push_back("/foo/");
  paths.push_back("foo/bar");
  paths.push_back("/foo/bar");
  paths.push_back("//net");
  paths.push_back("//net/foo");
  paths.push_back("///foo///");
  paths.push_back("///foo///bar");
  paths.push_back("/.");
  paths.push_back("./");
  paths.push_back("/..");
  paths.push_back("../");
  paths.push_back("foo/.");
  paths.push_back("foo/..");
  paths.push_back("foo/./");
  paths.push_back("foo/./bar");
  paths.push_back("foo/..");
  paths.push_back("foo/../");
  paths.push_back("foo/../bar");
  paths.push_back("c:");
  paths.push_back("c:/");
  paths.push_back("c:foo");
  paths.push_back("c:/foo");
  paths.push_back("c:foo/");
  paths.push_back("c:/foo/");
  paths.push_back("c:/foo/bar");
  paths.push_back("prn:");
  paths.push_back("c:\\");
  paths.push_back("c:foo");
  paths.push_back("c:\\foo");
  paths.push_back("c:foo\\");
  paths.push_back("c:\\foo\\");
  paths.push_back("c:\\foo/");
  paths.push_back("c:/foo\\bar");

  for (SmallVector<StringRef, 40>::const_iterator i = paths.begin(),
                                                  e = paths.end();
                                                  i != e;
                                                  ++i) {
    for (sys::path::const_iterator ci = sys::path::begin(*i),
                                   ce = sys::path::end(*i);
                                   ci != ce;
                                   ++ci) {
      ASSERT_FALSE(ci->empty());
    }

#if 0 // Valgrind is whining about this.
    outs() << "    Reverse Iteration: [";
    for (sys::path::reverse_iterator ci = sys::path::rbegin(*i),
                                     ce = sys::path::rend(*i);
                                     ci != ce;
                                     ++ci) {
      outs() << *ci << ',';
    }
    outs() << "]\n";
#endif

    path::has_root_path(*i);
    path::root_path(*i);
    path::has_root_name(*i);
    path::root_name(*i);
    path::has_root_directory(*i);
    path::root_directory(*i);
    path::has_parent_path(*i);
    path::parent_path(*i);
    path::has_filename(*i);
    path::filename(*i);
    path::has_stem(*i);
    path::stem(*i);
    path::has_extension(*i);
    path::extension(*i);
    path::is_absolute(*i);
    path::is_relative(*i);

    SmallString<128> temp_store;
    temp_store = *i;
    ASSERT_NO_ERROR(fs::make_absolute(temp_store));
    temp_store = *i;
    path::remove_filename(temp_store);

    temp_store = *i;
    path::replace_extension(temp_store, "ext");
    StringRef filename(temp_store.begin(), temp_store.size()), stem, ext;
    stem = path::stem(filename);
    ext  = path::extension(filename);
    EXPECT_EQ(*(--sys::path::end(filename)), (stem + ext).str());

    path::native(*i, temp_store);
  }
}

TEST(Support, RelativePathIterator) {
  SmallString<64> Path(StringRef("c/d/e/foo.txt"));
  typedef SmallVector<StringRef, 4> PathComponents;
  PathComponents ExpectedPathComponents;
  PathComponents ActualPathComponents;

  StringRef(Path).split(ExpectedPathComponents, "/");

  for (path::const_iterator I = path::begin(Path), E = path::end(Path); I != E;
       ++I) {
    ActualPathComponents.push_back(*I);
  }

  ASSERT_EQ(ExpectedPathComponents.size(), ActualPathComponents.size());

  for (size_t i = 0; i <ExpectedPathComponents.size(); ++i) {
    EXPECT_EQ(ExpectedPathComponents[i].str(), ActualPathComponents[i].str());
  }
}

TEST(Support, AbsolutePathIterator) {
  SmallString<64> Path(StringRef("/c/d/e/foo.txt"));
  typedef SmallVector<StringRef, 4> PathComponents;
  PathComponents ExpectedPathComponents;
  PathComponents ActualPathComponents;

  StringRef(Path).split(ExpectedPathComponents, "/");

  // The root path will also be a component when iterating
  ExpectedPathComponents[0] = "/";

  for (path::const_iterator I = path::begin(Path), E = path::end(Path); I != E;
       ++I) {
    ActualPathComponents.push_back(*I);
  }

  ASSERT_EQ(ExpectedPathComponents.size(), ActualPathComponents.size());

  for (size_t i = 0; i <ExpectedPathComponents.size(); ++i) {
    EXPECT_EQ(ExpectedPathComponents[i].str(), ActualPathComponents[i].str());
  }
}

#ifdef LLVM_ON_WIN32
TEST(Support, AbsolutePathIteratorWin32) {
  SmallString<64> Path(StringRef("c:\\c\\e\\foo.txt"));
  typedef SmallVector<StringRef, 4> PathComponents;
  PathComponents ExpectedPathComponents;
  PathComponents ActualPathComponents;

  StringRef(Path).split(ExpectedPathComponents, "\\");

  // The root path (which comes after the drive name) will also be a component
  // when iterating.
  ExpectedPathComponents.insert(ExpectedPathComponents.begin()+1, "\\");

  for (path::const_iterator I = path::begin(Path), E = path::end(Path); I != E;
       ++I) {
    ActualPathComponents.push_back(*I);
  }

  ASSERT_EQ(ExpectedPathComponents.size(), ActualPathComponents.size());

  for (size_t i = 0; i <ExpectedPathComponents.size(); ++i) {
    EXPECT_EQ(ExpectedPathComponents[i].str(), ActualPathComponents[i].str());
  }
}
#endif // LLVM_ON_WIN32

class FileSystemTest : public testing::Test {
protected:
  /// Unique temporary directory in which all created filesystem entities must
  /// be placed. It is recursively removed at the end of each test.
  SmallString<128> TestDirectory;

  virtual void SetUp() {
    ASSERT_NO_ERROR(
        fs::createUniqueDirectory("file-system-test", TestDirectory));
    // We don't care about this specific file.
    errs() << "Test Directory: " << TestDirectory << '\n';
    errs().flush();
  }

  virtual void TearDown() {
    uint32_t removed;
    ASSERT_NO_ERROR(fs::remove_all(TestDirectory.str(), removed));
  }
};

TEST_F(FileSystemTest, Unique) {
  // Create a temp file.
  int FileDescriptor;
  SmallString<64> TempPath;
  ASSERT_NO_ERROR(
      fs::createTemporaryFile("prefix", "temp", FileDescriptor, TempPath));

  // The same file should return an identical unique id.
  fs::UniqueID F1, F2;
  ASSERT_NO_ERROR(fs::getUniqueID(Twine(TempPath), F1));
  ASSERT_NO_ERROR(fs::getUniqueID(Twine(TempPath), F2));
  ASSERT_EQ(F1, F2);

  // Different files should return different unique ids.
  int FileDescriptor2;
  SmallString<64> TempPath2;
  ASSERT_NO_ERROR(
      fs::createTemporaryFile("prefix", "temp", FileDescriptor2, TempPath2));

  fs::UniqueID D;
  ASSERT_NO_ERROR(fs::getUniqueID(Twine(TempPath2), D));
  ASSERT_NE(D, F1);
  ::close(FileDescriptor2);

  ASSERT_NO_ERROR(fs::remove(Twine(TempPath2)));

  // Two paths representing the same file on disk should still provide the
  // same unique id.  We can test this by making a hard link.
  ASSERT_NO_ERROR(fs::create_hard_link(Twine(TempPath), Twine(TempPath2)));
  fs::UniqueID D2;
  ASSERT_NO_ERROR(fs::getUniqueID(Twine(TempPath2), D2));
  ASSERT_EQ(D2, F1);

  ::close(FileDescriptor);

  SmallString<128> Dir1;
  ASSERT_NO_ERROR(
     fs::createUniqueDirectory("dir1", Dir1));
  ASSERT_NO_ERROR(fs::getUniqueID(Dir1.c_str(), F1));
  ASSERT_NO_ERROR(fs::getUniqueID(Dir1.c_str(), F2));
  ASSERT_EQ(F1, F2);

  SmallString<128> Dir2;
  ASSERT_NO_ERROR(
     fs::createUniqueDirectory("dir2", Dir2));
  ASSERT_NO_ERROR(fs::getUniqueID(Dir2.c_str(), F2));
  ASSERT_NE(F1, F2);
}

TEST_F(FileSystemTest, TempFiles) {
  // Create a temp file.
  int FileDescriptor;
  SmallString<64> TempPath;
  ASSERT_NO_ERROR(
      fs::createTemporaryFile("prefix", "temp", FileDescriptor, TempPath));

  // Make sure it exists.
  bool TempFileExists;
  ASSERT_NO_ERROR(sys::fs::exists(Twine(TempPath), TempFileExists));
  EXPECT_TRUE(TempFileExists);

  // Create another temp tile.
  int FD2;
  SmallString<64> TempPath2;
  ASSERT_NO_ERROR(fs::createTemporaryFile("prefix", "temp", FD2, TempPath2));
  ASSERT_TRUE(TempPath2.endswith(".temp"));
  ASSERT_NE(TempPath.str(), TempPath2.str());

  fs::file_status A, B;
  ASSERT_NO_ERROR(fs::status(Twine(TempPath), A));
  ASSERT_NO_ERROR(fs::status(Twine(TempPath2), B));
  EXPECT_FALSE(fs::equivalent(A, B));

  ::close(FD2);

  // Remove Temp2.
  ASSERT_NO_ERROR(fs::remove(Twine(TempPath2), TempFileExists));
  EXPECT_TRUE(TempFileExists);

  error_code EC = fs::status(TempPath2.c_str(), B);
  EXPECT_EQ(EC, errc::no_such_file_or_directory);
  EXPECT_EQ(B.type(), fs::file_type::file_not_found);

  // Make sure Temp2 doesn't exist.
  ASSERT_NO_ERROR(fs::exists(Twine(TempPath2), TempFileExists));
  EXPECT_FALSE(TempFileExists);

  SmallString<64> TempPath3;
  ASSERT_NO_ERROR(fs::createTemporaryFile("prefix", "", TempPath3));
  ASSERT_FALSE(TempPath3.endswith("."));

  // Create a hard link to Temp1.
  ASSERT_NO_ERROR(fs::create_hard_link(Twine(TempPath), Twine(TempPath2)));
  bool equal;
  ASSERT_NO_ERROR(fs::equivalent(Twine(TempPath), Twine(TempPath2), equal));
  EXPECT_TRUE(equal);
  ASSERT_NO_ERROR(fs::status(Twine(TempPath), A));
  ASSERT_NO_ERROR(fs::status(Twine(TempPath2), B));
  EXPECT_TRUE(fs::equivalent(A, B));

  // Remove Temp1.
  ::close(FileDescriptor);
  ASSERT_NO_ERROR(fs::remove(Twine(TempPath), TempFileExists));
  EXPECT_TRUE(TempFileExists);

  // Remove the hard link.
  ASSERT_NO_ERROR(fs::remove(Twine(TempPath2), TempFileExists));
  EXPECT_TRUE(TempFileExists);

  // Make sure Temp1 doesn't exist.
  ASSERT_NO_ERROR(fs::exists(Twine(TempPath), TempFileExists));
  EXPECT_FALSE(TempFileExists);

#ifdef LLVM_ON_WIN32
  // Path name > 260 chars should get an error.
  const char *Path270 =
    "abcdefghijklmnopqrstuvwxyz9abcdefghijklmnopqrstuvwxyz8"
    "abcdefghijklmnopqrstuvwxyz7abcdefghijklmnopqrstuvwxyz6"
    "abcdefghijklmnopqrstuvwxyz5abcdefghijklmnopqrstuvwxyz4"
    "abcdefghijklmnopqrstuvwxyz3abcdefghijklmnopqrstuvwxyz2"
    "abcdefghijklmnopqrstuvwxyz1abcdefghijklmnopqrstuvwxyz0";
  EXPECT_EQ(fs::createUniqueFile(Twine(Path270), FileDescriptor, TempPath),
            windows_error::path_not_found);
#endif
}

TEST_F(FileSystemTest, DirectoryIteration) {
  error_code ec;
  for (fs::directory_iterator i(".", ec), e; i != e; i.increment(ec))
    ASSERT_NO_ERROR(ec);

  // Create a known hierarchy to recurse over.
  bool existed;
  ASSERT_NO_ERROR(fs::create_directories(Twine(TestDirectory)
                  + "/recursive/a0/aa1", existed));
  ASSERT_NO_ERROR(fs::create_directories(Twine(TestDirectory)
                  + "/recursive/a0/ab1", existed));
  ASSERT_NO_ERROR(fs::create_directories(Twine(TestDirectory)
                  + "/recursive/dontlookhere/da1", existed));
  ASSERT_NO_ERROR(fs::create_directories(Twine(TestDirectory)
                  + "/recursive/z0/za1", existed));
  ASSERT_NO_ERROR(fs::create_directories(Twine(TestDirectory)
                  + "/recursive/pop/p1", existed));
  typedef std::vector<std::string> v_t;
  v_t visited;
  for (fs::recursive_directory_iterator i(Twine(TestDirectory)
         + "/recursive", ec), e; i != e; i.increment(ec)){
    ASSERT_NO_ERROR(ec);
    if (path::filename(i->path()) == "p1") {
      i.pop();
      // FIXME: recursive_directory_iterator should be more robust.
      if (i == e) break;
    }
    if (path::filename(i->path()) == "dontlookhere")
      i.no_push();
    visited.push_back(path::filename(i->path()));
  }
  v_t::const_iterator a0 = std::find(visited.begin(), visited.end(), "a0");
  v_t::const_iterator aa1 = std::find(visited.begin(), visited.end(), "aa1");
  v_t::const_iterator ab1 = std::find(visited.begin(), visited.end(), "ab1");
  v_t::const_iterator dontlookhere = std::find(visited.begin(), visited.end(),
                                               "dontlookhere");
  v_t::const_iterator da1 = std::find(visited.begin(), visited.end(), "da1");
  v_t::const_iterator z0 = std::find(visited.begin(), visited.end(), "z0");
  v_t::const_iterator za1 = std::find(visited.begin(), visited.end(), "za1");
  v_t::const_iterator pop = std::find(visited.begin(), visited.end(), "pop");
  v_t::const_iterator p1 = std::find(visited.begin(), visited.end(), "p1");

  // Make sure that each path was visited correctly.
  ASSERT_NE(a0, visited.end());
  ASSERT_NE(aa1, visited.end());
  ASSERT_NE(ab1, visited.end());
  ASSERT_NE(dontlookhere, visited.end());
  ASSERT_EQ(da1, visited.end()); // Not visited.
  ASSERT_NE(z0, visited.end());
  ASSERT_NE(za1, visited.end());
  ASSERT_NE(pop, visited.end());
  ASSERT_EQ(p1, visited.end()); // Not visited.

  // Make sure that parents were visited before children. No other ordering
  // guarantees can be made across siblings.
  ASSERT_LT(a0, aa1);
  ASSERT_LT(a0, ab1);
  ASSERT_LT(z0, za1);
}

const char archive[] = "!<arch>\x0A";
const char bitcode[] = "\xde\xc0\x17\x0b";
const char coff_object[] = "\x00\x00......";
const char coff_import_library[] = "\x00\x00\xff\xff....";
const char elf_relocatable[] = { 0x7f, 'E', 'L', 'F', 1, 2, 1, 0, 0,
                                 0,    0,   0,   0,   0, 0, 0, 0, 1 };
const char macho_universal_binary[] = "\xca\xfe\xba\xbe...\0x00";
const char macho_object[] = "\xfe\xed\xfa\xce..........\x00\x01";
const char macho_executable[] = "\xfe\xed\xfa\xce..........\x00\x02";
const char macho_fixed_virtual_memory_shared_lib[] =
    "\xfe\xed\xfa\xce..........\x00\x03";
const char macho_core[] = "\xfe\xed\xfa\xce..........\x00\x04";
const char macho_preload_executable[] = "\xfe\xed\xfa\xce..........\x00\x05";
const char macho_dynamically_linked_shared_lib[] =
    "\xfe\xed\xfa\xce..........\x00\x06";
const char macho_dynamic_linker[] = "\xfe\xed\xfa\xce..........\x00\x07";
const char macho_bundle[] = "\xfe\xed\xfa\xce..........\x00\x08";
const char macho_dsym_companion[] = "\xfe\xed\xfa\xce..........\x00\x0a";
const char windows_resource[] = "\x00\x00\x00\x00\x020\x00\x00\x00\xff";

TEST_F(FileSystemTest, Magic) {
  struct type {
    const char *filename;
    const char *magic_str;
    size_t magic_str_len;
    fs::file_magic magic;
  } types[] = {
#define DEFINE(magic)                                           \
    { #magic, magic, sizeof(magic), fs::file_magic::magic }
    DEFINE(archive),
    DEFINE(bitcode),
    DEFINE(coff_object),
    DEFINE(coff_import_library),
    DEFINE(elf_relocatable),
    DEFINE(macho_universal_binary),
    DEFINE(macho_object),
    DEFINE(macho_executable),
    DEFINE(macho_fixed_virtual_memory_shared_lib),
    DEFINE(macho_core),
    DEFINE(macho_preload_executable),
    DEFINE(macho_dynamically_linked_shared_lib),
    DEFINE(macho_dynamic_linker),
    DEFINE(macho_bundle),
    DEFINE(macho_dsym_companion),
    DEFINE(windows_resource)
#undef DEFINE
    };

  // Create some files filled with magic.
  for (type *i = types, *e = types + (sizeof(types) / sizeof(type)); i != e;
                                                                     ++i) {
    SmallString<128> file_pathname(TestDirectory);
    path::append(file_pathname, i->filename);
    std::string ErrMsg;
    raw_fd_ostream file(file_pathname.c_str(), ErrMsg, sys::fs::F_Binary);
    ASSERT_FALSE(file.has_error());
    StringRef magic(i->magic_str, i->magic_str_len);
    file << magic;
    file.close();
    bool res = false;
    ASSERT_NO_ERROR(fs::has_magic(file_pathname.c_str(), magic, res));
    EXPECT_TRUE(res);
    EXPECT_EQ(i->magic, fs::identify_magic(magic));
  }
}

#ifdef LLVM_ON_WIN32
TEST_F(FileSystemTest, CarriageReturn) {
  SmallString<128> FilePathname(TestDirectory);
  std::string ErrMsg;
  path::append(FilePathname, "test");

  {
    raw_fd_ostream File(FilePathname.c_str(), ErrMsg);
    EXPECT_EQ(ErrMsg, "");
    File << '\n';
  }
  {
    OwningPtr<MemoryBuffer> Buf;
    MemoryBuffer::getFile(FilePathname.c_str(), Buf);
    EXPECT_EQ(Buf->getBuffer(), "\r\n");
  }

  {
    raw_fd_ostream File(FilePathname.c_str(), ErrMsg, sys::fs::F_Binary);
    EXPECT_EQ(ErrMsg, "");
    File << '\n';
  }
  {
    OwningPtr<MemoryBuffer> Buf;
    MemoryBuffer::getFile(FilePathname.c_str(), Buf);
    EXPECT_EQ(Buf->getBuffer(), "\n");
  }
}
#endif

TEST_F(FileSystemTest, FileMapping) {
  // Create a temp file.
  int FileDescriptor;
  SmallString<64> TempPath;
  ASSERT_NO_ERROR(
      fs::createTemporaryFile("prefix", "temp", FileDescriptor, TempPath));
  // Map in temp file and add some content
  error_code EC;
  StringRef Val("hello there");
  {
    fs::mapped_file_region mfr(FileDescriptor,
                               true,
                               fs::mapped_file_region::readwrite,
                               4096,
                               0,
                               EC);
    ASSERT_NO_ERROR(EC);
    std::copy(Val.begin(), Val.end(), mfr.data());
    // Explicitly add a 0.
    mfr.data()[Val.size()] = 0;
    // Unmap temp file
  }

  // Map it back in read-only
  fs::mapped_file_region mfr(Twine(TempPath),
                             fs::mapped_file_region::readonly,
                             0,
                             0,
                             EC);
  ASSERT_NO_ERROR(EC);

  // Verify content
  EXPECT_EQ(StringRef(mfr.const_data()), Val);

  // Unmap temp file

#if LLVM_HAS_RVALUE_REFERENCES
  fs::mapped_file_region m(Twine(TempPath),
                             fs::mapped_file_region::readonly,
                             0,
                             0,
                             EC);
  ASSERT_NO_ERROR(EC);
  const char *Data = m.const_data();
  fs::mapped_file_region mfrrv(llvm_move(m));
  EXPECT_EQ(mfrrv.const_data(), Data);
#endif
}
} // anonymous namespace
