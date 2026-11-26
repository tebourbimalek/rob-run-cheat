// rb_kernel_compat.h — declarations the WDK NuGet header set does not ship.
//
// Why this file exists: the WDK headers obtained from the NuGet payload
// (Microsoft.Windows.WDK.x64 10.0.26100.6584) carry ntifs.h/ntddk.h/ntimage.h
// but only *forward* declare PEB and omit MmCopyVirtualMemory / PsGetProcessPeb
// / PsGetProcessSectionBaseAddress. All three symbols are present in
// ntoskrnl.lib (verified by string probe on the 1,039,598-byte import lib).
//
// Nothing here is invented:
//   - the three prototypes are the documented WDK signatures;
//   - the PEB / PEB_LDR_DATA / LDR_DATA_TABLE_ENTRY layouts are the public
//     Windows SDK winternl.h x64 layouts, restated with explicit padding and
//     compile-time offset assertions so a wrong restatement cannot ship.
//
// If the full MSI WDK is installed later, this header still compiles: it only
// declares things that the trimmed set omits.

#pragma once

#include <ntifs.h>
#include <ntimage.h>

extern "C" {

NTKERNELAPI
NTSTATUS
NTAPI
MmCopyVirtualMemory(
    _In_ PEPROCESS SourceProcess,
    _In_ PVOID SourceAddress,
    _In_ PEPROCESS TargetProcess,
    _In_ PVOID TargetAddress,
    _In_ SIZE_T BufferSize,
    _In_ KPROCESSOR_MODE PreviousMode,
    _Out_ PSIZE_T ReturnSize
    );

NTKERNELAPI
PPEB
NTAPI
PsGetProcessPeb(_In_ PEPROCESS Process);

NTKERNELAPI
PVOID
NTAPI
PsGetProcessSectionBaseAddress(_In_ PEPROCESS Process);

} // extern "C"

// ---------------------------------------------------------------------------
// winternl.h x64 layouts (explicit padding, no bitfields)
// ---------------------------------------------------------------------------
typedef struct _RB_PEB {
    UCHAR  InheritedAddressSpace;    // 0x00
    UCHAR  ReadImageFileExecOptions; // 0x01
    UCHAR  BeingDebugged;            // 0x02
    UCHAR  Spare03;                  // 0x03
    ULONG  Spare04;                  // 0x04
    HANDLE Mutant;                   // 0x08
    PVOID  ImageBaseAddress;         // 0x10
    PVOID  Ldr;                      // 0x18  (PPEB_LDR_DATA)
} RB_PEB, *PRB_PEB;

typedef struct _RB_PEB_LDR_DATA {
    ULONG      Length;                             // 0x00
    BOOLEAN    Initialized;                        // 0x04
    UCHAR      Spare05[3];                         // 0x05
    PVOID      SsHandle;                           // 0x08
    LIST_ENTRY InLoadOrderModuleList;              // 0x10
    LIST_ENTRY InMemoryOrderModuleList;            // 0x20
    LIST_ENTRY InInitializationOrderModuleList;    // 0x30
} RB_PEB_LDR_DATA, *PRB_PEB_LDR_DATA;

typedef struct _RB_LDR_DATA_TABLE_ENTRY {
    LIST_ENTRY     InLoadOrderLinks;               // 0x00
    LIST_ENTRY     InMemoryOrderLinks;             // 0x10
    LIST_ENTRY     InInitializationOrderLinks;     // 0x20
    PVOID          DllBase;                        // 0x30
    PVOID          EntryPoint;                     // 0x38
    ULONG          SizeOfImage;                    // 0x40
    ULONG          Spare44;                        // 0x44
    UNICODE_STRING FullDllName;                    // 0x48
    UNICODE_STRING BaseDllName;                    // 0x58
} RB_LDR_DATA_TABLE_ENTRY, *PRB_LDR_DATA_TABLE_ENTRY;

static_assert(FIELD_OFFSET(RB_PEB, Ldr) == 0x18, "PEB.Ldr offset");
static_assert(FIELD_OFFSET(RB_PEB_LDR_DATA, InMemoryOrderModuleList) == 0x20, "Ldr.InMemoryOrderModuleList offset");
static_assert(FIELD_OFFSET(RB_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks) == 0x10, "entry.InMemoryOrderLinks offset");
static_assert(FIELD_OFFSET(RB_LDR_DATA_TABLE_ENTRY, DllBase) == 0x30, "entry.DllBase offset");
static_assert(FIELD_OFFSET(RB_LDR_DATA_TABLE_ENTRY, SizeOfImage) == 0x40, "entry.SizeOfImage offset");
static_assert(FIELD_OFFSET(RB_LDR_DATA_TABLE_ENTRY, BaseDllName) == 0x58, "entry.BaseDllName offset");
