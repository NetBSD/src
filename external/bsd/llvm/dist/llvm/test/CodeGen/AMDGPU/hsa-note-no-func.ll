; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=gfx700 | FileCheck --check-prefix=HSA --check-prefix=HSA-CI700 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=gfx701 | FileCheck --check-prefix=HSA --check-prefix=HSA-CI701 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=gfx702 | FileCheck --check-prefix=HSA --check-prefix=HSA-CI702 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=kaveri | FileCheck --check-prefix=HSA --check-prefix=HSA-CI700 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=carrizo | FileCheck --check-prefix=HSA --check-prefix=HSA-VI801 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=tonga | FileCheck --check-prefix=HSA --check-prefix=HSA-VI802 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=fiji | FileCheck --check-prefix=HSA --check-prefix=HSA-VI803 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=polaris10 | FileCheck --check-prefix=HSA --check-prefix=HSA-VI803 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=polaris11 | FileCheck --check-prefix=HSA --check-prefix=HSA-VI803 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=gfx800 | FileCheck --check-prefix=HSA --check-prefix=HSA-VI800 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=gfx801 | FileCheck --check-prefix=HSA --check-prefix=HSA-VI801 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=gfx802 | FileCheck --check-prefix=HSA --check-prefix=HSA-VI802 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=gfx803 | FileCheck --check-prefix=HSA --check-prefix=HSA-VI803 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=gfx804 | FileCheck --check-prefix=HSA --check-prefix=HSA-VI804 %s
; RUN: llc < %s -mtriple=amdgcn--amdhsa -mcpu=gfx810 | FileCheck --check-prefix=HSA --check-prefix=HSA-VI810 %s

; HSA: .hsa_code_object_version 2,1
; HSA-CI700: .hsa_code_object_isa 7,0,0,"AMD","AMDGPU"
; HSA-CI701: .hsa_code_object_isa 7,0,1,"AMD","AMDGPU"
; HSA-CI702: .hsa_code_object_isa 7,0,2,"AMD","AMDGPU"
; HSA-VI800: .hsa_code_object_isa 8,0,0,"AMD","AMDGPU"
; HSA-VI801: .hsa_code_object_isa 8,0,1,"AMD","AMDGPU"
; HSA-VI802: .hsa_code_object_isa 8,0,2,"AMD","AMDGPU"
; HSA-VI803: .hsa_code_object_isa 8,0,3,"AMD","AMDGPU"
; HSA-VI804: .hsa_code_object_isa 8,0,4,"AMD","AMDGPU"
; HSA-VI810: .hsa_code_object_isa 8,1,0,"AMD","AMDGPU"
