//===--- ToolChains.cpp - ToolChain Implementations -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "ToolChains.h"
#include "clang/Basic/CharInfo.h"
#include "clang/Basic/Version.h"
#include "clang/Driver/Compilation.h"
#include "clang/Driver/Driver.h"
#include "clang/Driver/DriverDiagnostic.h"
#include "clang/Driver/Options.h"
#include "llvm/Option/Arg.h"
#include "llvm/Option/ArgList.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Path.h"

// Include the necessary headers to interface with the Windows registry and
// environment.
#ifdef _MSC_VER
  #define WIN32_LEAN_AND_MEAN
  #define NOGDI
  #define NOMINMAX
  #include <Windows.h>
#endif

using namespace clang::driver;
using namespace clang::driver::toolchains;
using namespace clang;
using namespace llvm::opt;

Windows::Windows(const Driver &D, const llvm::Triple& Triple,
                 const ArgList &Args)
  : ToolChain(D, Triple, Args) {
}

Tool *Windows::buildLinker() const {
  return new tools::visualstudio::Link(*this);
}

Tool *Windows::buildAssembler() const {
  if (getTriple().isOSBinFormatMachO())
    return new tools::darwin::Assemble(*this);
  getDriver().Diag(clang::diag::err_no_external_assembler);
  return nullptr;
}

bool Windows::IsIntegratedAssemblerDefault() const {
  return true;
}

bool Windows::IsUnwindTablesDefault() const {
  return getArch() == llvm::Triple::x86_64;
}

bool Windows::isPICDefault() const {
  return getArch() == llvm::Triple::x86_64;
}

bool Windows::isPIEDefault() const {
  return false;
}

bool Windows::isPICDefaultForced() const {
  return getArch() == llvm::Triple::x86_64;
}

// FIXME: This probably should goto to some platform utils place.
#ifdef _MSC_VER

/// \brief Read registry string.
/// This also supports a means to look for high-versioned keys by use
/// of a $VERSION placeholder in the key path.
/// $VERSION in the key path is a placeholder for the version number,
/// causing the highest value path to be searched for and used.
/// I.e. "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\$VERSION".
/// There can be additional characters in the component.  Only the numberic
/// characters are compared.
static bool getSystemRegistryString(const char *keyPath, const char *valueName,
                                    char *value, size_t maxLength) {
  HKEY hRootKey = NULL;
  HKEY hKey = NULL;
  const char* subKey = NULL;
  DWORD valueType;
  DWORD valueSize = maxLength - 1;
  long lResult;
  bool returnValue = false;

  if (strncmp(keyPath, "HKEY_CLASSES_ROOT\\", 18) == 0) {
    hRootKey = HKEY_CLASSES_ROOT;
    subKey = keyPath + 18;
  } else if (strncmp(keyPath, "HKEY_USERS\\", 11) == 0) {
    hRootKey = HKEY_USERS;
    subKey = keyPath + 11;
  } else if (strncmp(keyPath, "HKEY_LOCAL_MACHINE\\", 19) == 0) {
    hRootKey = HKEY_LOCAL_MACHINE;
    subKey = keyPath + 19;
  } else if (strncmp(keyPath, "HKEY_CURRENT_USER\\", 18) == 0) {
    hRootKey = HKEY_CURRENT_USER;
    subKey = keyPath + 18;
  } else {
    return false;
  }

  const char *placeHolder = strstr(subKey, "$VERSION");
  char bestName[256];
  bestName[0] = '\0';
  // If we have a $VERSION placeholder, do the highest-version search.
  if (placeHolder) {
    const char *keyEnd = placeHolder - 1;
    const char *nextKey = placeHolder;
    // Find end of previous key.
    while ((keyEnd > subKey) && (*keyEnd != '\\'))
      keyEnd--;
    // Find end of key containing $VERSION.
    while (*nextKey && (*nextKey != '\\'))
      nextKey++;
    size_t partialKeyLength = keyEnd - subKey;
    char partialKey[256];
    if (partialKeyLength > sizeof(partialKey))
      partialKeyLength = sizeof(partialKey);
    strncpy(partialKey, subKey, partialKeyLength);
    partialKey[partialKeyLength] = '\0';
    HKEY hTopKey = NULL;
    lResult = RegOpenKeyEx(hRootKey, partialKey, 0, KEY_READ | KEY_WOW64_32KEY,
                           &hTopKey);
    if (lResult == ERROR_SUCCESS) {
      char keyName[256];
      int bestIndex = -1;
      double bestValue = 0.0;
      DWORD index, size = sizeof(keyName) - 1;
      for (index = 0; RegEnumKeyEx(hTopKey, index, keyName, &size, NULL,
          NULL, NULL, NULL) == ERROR_SUCCESS; index++) {
        const char *sp = keyName;
        while (*sp && !isDigit(*sp))
          sp++;
        if (!*sp)
          continue;
        const char *ep = sp + 1;
        while (*ep && (isDigit(*ep) || (*ep == '.')))
          ep++;
        char numBuf[32];
        strncpy(numBuf, sp, sizeof(numBuf) - 1);
        numBuf[sizeof(numBuf) - 1] = '\0';
        double dvalue = strtod(numBuf, NULL);
        if (dvalue > bestValue) {
          // Test that InstallDir is indeed there before keeping this index.
          // Open the chosen key path remainder.
          strcpy(bestName, keyName);
          // Append rest of key.
          strncat(bestName, nextKey, sizeof(bestName) - 1);
          bestName[sizeof(bestName) - 1] = '\0';
          lResult = RegOpenKeyEx(hTopKey, bestName, 0,
                                 KEY_READ | KEY_WOW64_32KEY, &hKey);
          if (lResult == ERROR_SUCCESS) {
            lResult = RegQueryValueEx(hKey, valueName, NULL, &valueType,
              (LPBYTE)value, &valueSize);
            if (lResult == ERROR_SUCCESS) {
              bestIndex = (int)index;
              bestValue = dvalue;
              returnValue = true;
            }
            RegCloseKey(hKey);
          }
        }
        size = sizeof(keyName) - 1;
      }
      RegCloseKey(hTopKey);
    }
  } else {
    lResult = RegOpenKeyEx(hRootKey, subKey, 0, KEY_READ | KEY_WOW64_32KEY,
                           &hKey);
    if (lResult == ERROR_SUCCESS) {
      lResult = RegQueryValueEx(hKey, valueName, NULL, &valueType,
        (LPBYTE)value, &valueSize);
      if (lResult == ERROR_SUCCESS)
        returnValue = true;
      RegCloseKey(hKey);
    }
  }
  return returnValue;
}

/// \brief Get Windows SDK installation directory.
static bool getWindowsSDKDir(std::string &path) {
  char windowsSDKInstallDir[256];
  // Try the Windows registry.
  bool hasSDKDir = getSystemRegistryString(
   "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows\\$VERSION",
                                           "InstallationFolder",
                                           windowsSDKInstallDir,
                                           sizeof(windowsSDKInstallDir) - 1);
    // If we have both vc80 and vc90, pick version we were compiled with.
  if (hasSDKDir && windowsSDKInstallDir[0]) {
    path = windowsSDKInstallDir;
    return true;
  }
  return false;
}

  // Get Visual Studio installation directory.
static bool getVisualStudioDir(std::string &path) {
  // First check the environment variables that vsvars32.bat sets.
  const char* vcinstalldir = getenv("VCINSTALLDIR");
  if (vcinstalldir) {
    char *p = const_cast<char *>(strstr(vcinstalldir, "\\VC"));
    if (p)
      *p = '\0';
    path = vcinstalldir;
    return true;
  }

  char vsIDEInstallDir[256];
  char vsExpressIDEInstallDir[256];
  // Then try the windows registry.
  bool hasVCDir = getSystemRegistryString(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VisualStudio\\$VERSION",
    "InstallDir", vsIDEInstallDir, sizeof(vsIDEInstallDir) - 1);
  bool hasVCExpressDir = getSystemRegistryString(
    "HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\VCExpress\\$VERSION",
    "InstallDir", vsExpressIDEInstallDir, sizeof(vsExpressIDEInstallDir) - 1);
    // If we have both vc80 and vc90, pick version we were compiled with.
  if (hasVCDir && vsIDEInstallDir[0]) {
    char *p = (char*)strstr(vsIDEInstallDir, "\\Common7\\IDE");
    if (p)
      *p = '\0';
    path = vsIDEInstallDir;
    return true;
  }

  if (hasVCExpressDir && vsExpressIDEInstallDir[0]) {
    char *p = (char*)strstr(vsExpressIDEInstallDir, "\\Common7\\IDE");
    if (p)
      *p = '\0';
    path = vsExpressIDEInstallDir;
    return true;
  }

  // Try the environment.
  const char *vs100comntools = getenv("VS100COMNTOOLS");
  const char *vs90comntools = getenv("VS90COMNTOOLS");
  const char *vs80comntools = getenv("VS80COMNTOOLS");
  const char *vscomntools = NULL;

  // Try to find the version that we were compiled with
  if(false) {}
  #if (_MSC_VER >= 1600)  // VC100
  else if(vs100comntools) {
    vscomntools = vs100comntools;
  }
  #elif (_MSC_VER == 1500) // VC80
  else if(vs90comntools) {
    vscomntools = vs90comntools;
  }
  #elif (_MSC_VER == 1400) // VC80
  else if(vs80comntools) {
    vscomntools = vs80comntools;
  }
  #endif
  // Otherwise find any version we can
  else if (vs100comntools)
    vscomntools = vs100comntools;
  else if (vs90comntools)
    vscomntools = vs90comntools;
  else if (vs80comntools)
    vscomntools = vs80comntools;

  if (vscomntools && *vscomntools) {
    const char *p = strstr(vscomntools, "\\Common7\\Tools");
    path = p ? std::string(vscomntools, p) : vscomntools;
    return true;
  }
  return false;
}

#endif // _MSC_VER

void Windows::AddClangSystemIncludeArgs(const ArgList &DriverArgs,
                                        ArgStringList &CC1Args) const {
  if (DriverArgs.hasArg(options::OPT_nostdinc))
    return;

  if (!DriverArgs.hasArg(options::OPT_nobuiltininc)) {
    SmallString<128> P(getDriver().ResourceDir);
    llvm::sys::path::append(P, "include");
    addSystemInclude(DriverArgs, CC1Args, P.str());
  }

  if (DriverArgs.hasArg(options::OPT_nostdlibinc))
    return;

#ifdef _MSC_VER
  // Honor %INCLUDE%. It should know essential search paths with vcvarsall.bat.
  if (const char *cl_include_dir = getenv("INCLUDE")) {
    SmallVector<StringRef, 8> Dirs;
    StringRef(cl_include_dir)
        .split(Dirs, ";", /*MaxSplit=*/-1, /*KeepEmpty=*/false);
    for (StringRef Dir : Dirs)
      addSystemInclude(DriverArgs, CC1Args, Dir);
    if (!Dirs.empty())
      return;
  }

  std::string VSDir;
  std::string WindowsSDKDir;

  // When built with access to the proper Windows APIs, try to actually find
  // the correct include paths first.
  if (getVisualStudioDir(VSDir)) {
    SmallString<128> P;
    P = VSDir;
    llvm::sys::path::append(P, "VC\\include");
    addSystemInclude(DriverArgs, CC1Args, P.str());
    if (getWindowsSDKDir(WindowsSDKDir)) {
      P = WindowsSDKDir;
      llvm::sys::path::append(P, "include");
      addSystemInclude(DriverArgs, CC1Args, P.str());
    } else {
      P = VSDir;
      llvm::sys::path::append(P, "VC\\PlatformSDK\\Include");
      addSystemInclude(DriverArgs, CC1Args, P.str());
    }
    return;
  }

  // As a fallback, select default install paths.
  const StringRef Paths[] = {
    "C:/Program Files/Microsoft Visual Studio 10.0/VC/include",
    "C:/Program Files/Microsoft Visual Studio 9.0/VC/include",
    "C:/Program Files/Microsoft Visual Studio 9.0/VC/PlatformSDK/Include",
    "C:/Program Files/Microsoft Visual Studio 8/VC/include",
    "C:/Program Files/Microsoft Visual Studio 8/VC/PlatformSDK/Include"
  };
  addSystemIncludes(DriverArgs, CC1Args, Paths);
#endif // _MSC_VER
}

void Windows::AddClangCXXStdlibIncludeArgs(const ArgList &DriverArgs,
                                           ArgStringList &CC1Args) const {
  // FIXME: There should probably be logic here to find libc++ on Windows.
}
