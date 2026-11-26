// rb_drv.cpp — ROB RUN transport driver (T0 bring-up stub).
//
// Contract, exactly four IOCTLs:
//   IOCTL_ATTACH  in:  ULONG pid            out: RB_ATTACH_OUT
//   IOCTL_READ    in:  RB_OP_IN             out: length bytes
//   IOCTL_WRITE   in:  RB_OP_IN + payload   out: none
//   IOCTL_DETACH  in:  none                 out: none
//
// No feature code, no hooks, no callbacks, no thread, no work item.
// Build: WDM (native), x64, /kernel. Map with kdmapper. See build_driver.ps1.
//
// RANGE VALIDATION (rule 9), stated precisely because a wrong claim here is a BSOD:
//   - address is rejected unless it is a canonical user address: nonzero,
//     below 0x7FFFFFFF0000, and addr+len does not wrap.
//   - length is capped at RB_MAX_XFER (64 KiB) so a single IRP cannot span
//     arbitrary amounts of the address space.
//   - the copy is issued page-by-page through MmCopyVirtualMemory, which does
//     its own probe/lock and fails with STATUS_PARTIAL_COPY on an uncommitted
//     page instead of faulting. The first failing page aborts the whole IOCTL.
//   - a true VAD-avl walk is NOT implemented. The AVL layout under
//     EPROCESS.VadRoot is version-specific; hardcoding its offsets would be an
//     invented offset (rule 12). The page-wise commit probe is the honest
//     equivalent and the escalation path is documented in vuln_driver.hpp.

#include <ntifs.h>
#include <ntddk.h>

#include "rb_kernel_compat.h"

#define RB_DEVICE_NAME  L"\\Device\\RbDrv"
#define RB_SYMLINK_NAME L"\\DosDevices\\RbDrv"

#define RB_IOCTL_ATTACH  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define RB_IOCTL_READ    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define RB_IOCTL_WRITE   CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define RB_IOCTL_DETACH  CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define RB_MAX_XFER     0x10000u        // 64 KiB per IOCTL
#define RB_PAGE         0x1000u
#define RB_USER_LIMIT   0x7FFFFFFF0000ull

#define RB_TAG          'rDbR'

#pragma pack(push, 8)
typedef struct _RB_ATTACH_OUT {
    LONG   status;      // NTSTATUS of the attach attempt
    ULONG  reserved;
    UINT64 mainBase;    // ROB RUN.exe image base
    UINT64 mainSize;    // SizeOfImage
    UINT64 gaBase;      // GameAssembly.dll base (0 if not found)
    UINT64 gaSize;      // GameAssembly.dll SizeOfImage
} RB_ATTACH_OUT;

typedef struct _RB_OP_IN {
    UINT64 address;
    ULONG  length;
    ULONG  reserved;
} RB_OP_IN;
#pragma pack(pop)

static PEPROCESS    g_target   = NULL;
static FAST_MUTEX   g_lock;
static DEVICE_OBJECT* g_device = NULL;

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------
static BOOLEAN rbRangeOk(UINT64 address, ULONG length) {
    UINT64 end;
    if (length == 0 || length > RB_MAX_XFER) return FALSE;
    if (address == 0) return FALSE;
    end = address + (UINT64)length;
    if (end < address) return FALSE;             // wrap
    if (end > RB_USER_LIMIT) return FALSE;       // kernel space
    return TRUE;
}

// Page-wise copy. toTarget==FALSE: target -> local buffer. TRUE: local -> target.
static NTSTATUS rbCopy(UINT64 address, PVOID local, ULONG length, BOOLEAN toTarget) {
    NTSTATUS status = STATUS_SUCCESS;
    ULONG    offset = 0;

    if (!g_target) return STATUS_DEVICE_NOT_READY;
    if (!local || !rbRangeOk(address, length)) return STATUS_INVALID_PARAMETER;

    __try {
        while (offset < length) {
            const ULONG chunk = (length - offset) > RB_PAGE ? RB_PAGE : (length - offset);
            SIZE_T copied = 0;
            PVOID  remote = (PVOID)(ULONG_PTR)(address + offset);
            PVOID  here   = (PVOID)((PUCHAR)local + offset);

            status = toTarget
                ? MmCopyVirtualMemory(PsGetCurrentProcess(), here, g_target, remote, chunk, KernelMode, &copied)
                : MmCopyVirtualMemory(g_target, remote, PsGetCurrentProcess(), here, chunk, KernelMode, &copied);

            if (!NT_SUCCESS(status)) break;
            if (copied != chunk) { status = STATUS_PARTIAL_COPY; break; }
            offset += chunk;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = STATUS_ACCESS_VIOLATION;
    }
    return status;
}

static UINT64 rbImageSizeInTarget(UINT64 base) {
    UINT64 size = 0;
    __try {
        PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)(ULONG_PTR)base;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE) {
            PIMAGE_NT_HEADERS64 nt =
                (PIMAGE_NT_HEADERS64)((ULONG_PTR)base + dos->e_lfanew);
            if (nt->Signature == IMAGE_NT_SIGNATURE &&
                nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
                size = nt->OptionalHeader.SizeOfImage;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        size = 0;
    }
    return size;
}

// Called with the target address space already attached.
static NTSTATUS rbFindGameAssembly(UINT64* gaBase, UINT64* gaSize) {
    NTSTATUS status = STATUS_NOT_FOUND;
    PRB_PEB peb = (PRB_PEB)PsGetProcessPeb(g_target);

    *gaBase = 0;
    *gaSize = 0;
    if (!peb || !peb->Ldr) return STATUS_NOT_FOUND;

    __try {
        PRB_PEB_LDR_DATA ldr = (PRB_PEB_LDR_DATA)peb->Ldr;
        PLIST_ENTRY head = &ldr->InMemoryOrderModuleList;
        for (PLIST_ENTRY e = head->Flink; e && e != head; e = e->Flink) {
            PRB_LDR_DATA_TABLE_ENTRY mod =
                CONTAINING_RECORD(e, RB_LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
            UNICODE_STRING* name = &mod->BaseDllName;
            if (!name->Buffer || name->Length != 17 * sizeof(WCHAR)) continue;
            if (_wcsnicmp(name->Buffer, L"GameAssembly.dll", 17) != 0) continue;

            // Runtime validator: the base must be page aligned and hold MZ.
            if (!mod->DllBase ||
                (((ULONG_PTR)mod->DllBase) & (PAGE_SIZE - 1)) != 0) {
                status = STATUS_INVALID_ADDRESS;
                break;
            }
            if (*(PUSHORT)mod->DllBase != IMAGE_DOS_SIGNATURE) {
                status = STATUS_INVALID_IMAGE_FORMAT;
                break;
            }
            *gaBase = (UINT64)(ULONG_PTR)mod->DllBase;
            *gaSize = (UINT64)mod->SizeOfImage;
            status = STATUS_SUCCESS;
            break;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        *gaBase = 0;
        *gaSize = 0;
        status = STATUS_ACCESS_VIOLATION;
    }
    return status;
}

static VOID rbDetachLocked(VOID) {
    if (g_target) {
        ObDereferenceObject(g_target);
        g_target = NULL;
    }
}

static NTSTATUS rbAttach(ULONG pid, RB_ATTACH_OUT* out) {
    PEPROCESS proc = NULL;
    KAPC_STATE apc;
    NTSTATUS status;

    RtlZeroMemory(out, sizeof(*out));

    status = PsLookupProcessByProcessId((HANDLE)(ULONG_PTR)pid, &proc);
    if (!NT_SUCCESS(status)) { out->status = status; return status; }

    ExAcquireFastMutex(&g_lock);
    rbDetachLocked();
    g_target = proc;                 // transfer the reference taken above
    ExReleaseFastMutex(&g_lock);

    KeStackAttachProcess(proc, &apc);
    __try {
        out->mainBase = (UINT64)(ULONG_PTR)PsGetProcessSectionBaseAddress(proc);
        out->mainSize = rbImageSizeInTarget(out->mainBase);
        status = rbFindGameAssembly(&out->gaBase, &out->gaSize);
        if (!NT_SUCCESS(status)) status = STATUS_SUCCESS; // GA may be absent; report zeros
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        status = STATUS_ACCESS_VIOLATION;
    }
    KeUnstackDetachProcess(&apc);

    out->status = status;
    return status;
}

// ---------------------------------------------------------------------------
// dispatch
// ---------------------------------------------------------------------------
static NTSTATUS rbCreate(PDEVICE_OBJECT dev, PIRP irp) {
    UNREFERENCED_PARAMETER(dev);
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS rbClose(PDEVICE_OBJECT dev, PIRP irp) {
    UNREFERENCED_PARAMETER(dev);
    ExAcquireFastMutex(&g_lock);
    rbDetachLocked();                // last handle closed -> drop the reference
    ExReleaseFastMutex(&g_lock);
    irp->IoStatus.Status = STATUS_SUCCESS;
    irp->IoStatus.Information = 0;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

static NTSTATUS rbControl(PDEVICE_OBJECT dev, PIRP irp) {
    PIO_STACK_LOCATION sp = IoGetCurrentIrpStackLocation(irp);
    PVOID  inBuf   = irp->AssociatedIrp.SystemBuffer;
    ULONG  inLen   = sp->Parameters.DeviceIoControl.InputBufferLength;
    ULONG  outLen  = sp->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG  code    = sp->Parameters.DeviceIoControl.IoControlCode;
    NTSTATUS status = STATUS_INVALID_DEVICE_REQUEST;
    ULONG_PTR info = 0;

    UNREFERENCED_PARAMETER(dev);

    switch (code) {
    case RB_IOCTL_ATTACH:
        if (inLen < sizeof(ULONG) || outLen < sizeof(RB_ATTACH_OUT)) {
            status = STATUS_BUFFER_TOO_SMALL;
        } else {
            status = rbAttach(*(ULONG*)inBuf, (RB_ATTACH_OUT*)inBuf);
            info = sizeof(RB_ATTACH_OUT);
        }
        break;

    case RB_IOCTL_READ: {
        RB_OP_IN op;
        if (inLen < sizeof(op) || !inBuf) { status = STATUS_BUFFER_TOO_SMALL; break; }
        op = *(RB_OP_IN*)inBuf;
        if (!rbRangeOk(op.address, op.length) || outLen < op.length) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        status = rbCopy(op.address, inBuf, op.length, FALSE);
        if (NT_SUCCESS(status)) info = op.length;
        break;
    }

    case RB_IOCTL_WRITE: {
        RB_OP_IN op;
        if (inLen < sizeof(op) || !inBuf) { status = STATUS_BUFFER_TOO_SMALL; break; }
        op = *(RB_OP_IN*)inBuf;
        if (!rbRangeOk(op.address, op.length) || inLen < sizeof(op) + op.length) {
            status = STATUS_INVALID_PARAMETER;
            break;
        }
        status = rbCopy(op.address, (PUCHAR)inBuf + sizeof(op), op.length, TRUE);
        if (NT_SUCCESS(status)) info = op.length;
        break;
    }

    case RB_IOCTL_DETACH:
        ExAcquireFastMutex(&g_lock);
        rbDetachLocked();
        ExReleaseFastMutex(&g_lock);
        status = STATUS_SUCCESS;
        break;

    default:
        break;
    }

    irp->IoStatus.Status = status;
    irp->IoStatus.Information = info;
    IoCompleteRequest(irp, IO_NO_INCREMENT);
    return status;
}

// ---------------------------------------------------------------------------
// entry
// ---------------------------------------------------------------------------
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT drv, PUNICODE_STRING registryPath) {
    UNICODE_STRING devName, symName;
    PDEVICE_OBJECT device = NULL;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(registryPath);

    ExInitializeFastMutex(&g_lock);

    RtlInitUnicodeString(&devName, RB_DEVICE_NAME);
    status = IoCreateDevice(drv, 0, &devName, FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN, FALSE, &device);
    if (!NT_SUCCESS(status)) return status;

    device->Flags |= DO_BUFFERED_IO;
    device->Flags &= ~DO_DEVICE_INITIALIZING;
    g_device = device;

    RtlInitUnicodeString(&symName, RB_SYMLINK_NAME);
    status = IoCreateSymbolicLink(&symName, &devName);
    if (!NT_SUCCESS(status)) {
        IoDeleteDevice(device);
        g_device = NULL;
        return status;
    }

    drv->MajorFunction[IRP_MJ_CREATE]         = rbCreate;
    drv->MajorFunction[IRP_MJ_CLOSE]          = rbClose;
    drv->MajorFunction[IRP_MJ_DEVICE_CONTROL] = rbControl;

    DbgPrint("[rb_drv] loaded, device %wZ\n", &devName);
    return STATUS_SUCCESS;
}
