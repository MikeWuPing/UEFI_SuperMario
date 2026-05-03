/*
 * uefi_types.h — Minimal UEFI type definitions for the SMB UEFI port.
 * MSVC-compatible: all types defined before use (no forward-reference errors).
 */
#ifndef UEFI_TYPES_H
#define UEFI_TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* ── Basic UEFI types ──────────────────────────────────────────────────── */

typedef uint64_t  UINT64;
typedef uint32_t  UINT32;
typedef uint16_t  UINT16;
typedef uint8_t   UINT8;
typedef size_t    UINTN;
typedef int64_t   INTN;
typedef int32_t   INT32;
typedef uint8_t   BOOLEAN;
typedef void      VOID;
typedef uint16_t  CHAR16;
typedef UINTN     EFI_STATUS;
typedef VOID     *EFI_HANDLE;
typedef UINT64    EFI_PHYSICAL_ADDRESS;
typedef UINT64    EFI_VIRTUAL_ADDRESS;

/* EFIAPI calling convention */
#if defined(__GNUC__) || defined(__clang__)
#define EFIAPI __attribute__((ms_abi))
#elif defined(_MSC_VER)
#define EFIAPI __cdecl
#else
#define EFIAPI
#endif

#define IN
#define OUT
#define OPTIONAL

#define EFI_SUCCESS              0
#define EFI_ERROR(a)             ((INTN)(a) < 0)
#define EFI_LOAD_ERROR           ((EFI_STATUS)1 | 0x8000000000000000ULL)
#define EFI_INVALID_PARAMETER    ((EFI_STATUS)2 | 0x8000000000000000ULL)
#define EFI_UNSUPPORTED          ((EFI_STATUS)3 | 0x8000000000000000ULL)
#define EFI_NOT_FOUND            ((EFI_STATUS)14 | 0x8000000000000000ULL)
#define EFI_OUT_OF_RESOURCES     ((EFI_STATUS)9 | 0x8000000000000000ULL)
#define EFI_DEVICE_ERROR         ((EFI_STATUS)7 | 0x8000000000000000ULL)
#define EFI_BUFFER_TOO_SMALL     ((EFI_STATUS)5 | 0x8000000000000000ULL)
#define EFI_NOT_READY            ((EFI_STATUS)6 | 0x8000000000000000ULL)

/* ── EFI_GUID ──────────────────────────────────────────────────────────── */

typedef struct {
  UINT32  Data1;
  UINT16  Data2;
  UINT16  Data3;
  UINT8   Data4[8];
} EFI_GUID;

/* ── EFI_TABLE_HEADER ──────────────────────────────────────────────────── */

typedef struct {
  UINT64  Signature;
  UINT32  Revision;
  UINT32  HeaderSize;
  UINT32  CRC32;
  UINT32  Reserved;
} EFI_TABLE_HEADER;

/* ── EFI_EVENT ─────────────────────────────────────────────────────────── */

typedef VOID *EFI_EVENT;

/* ── TPL ───────────────────────────────────────────────────────────────── */

typedef UINTN EFI_TPL;

#define TPL_APPLICATION 4
#define TPL_CALLBACK    8
#define TPL_NOTIFY      16
#define TPL_HIGH_LEVEL  31

/* ── Locate search type ────────────────────────────────────────────────── */

typedef enum {
  AllHandles,
  ByRegisterNotify,
  ByProtocol
} EFI_LOCATE_SEARCH_TYPE;

/* ── Timer ─────────────────────────────────────────────────────────────── */

typedef enum {
  TimerCancel,
  TimerPeriodic,
  TimerRelative
} EFI_TIMER_DELAY;

#define EFI_EVENT_TIMER    0x80000000U
#define EVT_TIMER          0x80000000U

/* ── Memory types ──────────────────────────────────────────────────────── */

typedef enum {
  AllocateAnyPages,
  AllocateMaxAddress,
  AllocateAddress,
  MaxAllocateType
} EFI_ALLOCATE_TYPE;

typedef enum {
  EfiReservedMemoryType,
  EfiLoaderCode,
  EfiLoaderData,
  EfiBootServicesCode,
  EfiBootServicesData,
  EfiRuntimeServicesCode,
  EfiRuntimeServicesData,
  EfiConventionalMemory,
  EfiUnusableMemory,
  EfiACPIReclaimMemory,
  EfiACPIMemoryNVS,
  EfiMemoryMappedIO,
  EfiMemoryMappedIOPortSpace,
  EfiPalCode,
  EfiPersistentMemory,
  EfiMaxMemoryType
} EFI_MEMORY_TYPE;

typedef struct {
  UINT32                 Type;
  EFI_PHYSICAL_ADDRESS   PhysicalStart;
  EFI_VIRTUAL_ADDRESS    VirtualStart;
  UINT64                 NumberOfPages;
  UINT64                 Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef VOID (EFIAPI *EFI_EVENT_NOTIFY)(
    IN EFI_EVENT  Event,
    IN VOID      *Context);

/* ── Simple Text Input Protocol ────────────────────────────────────────── */

typedef struct {
  UINT16  ScanCode;
  CHAR16  UnicodeChar;
} EFI_INPUT_KEY;

typedef struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL EFI_SIMPLE_TEXT_INPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_INPUT_READ_KEY)(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    OUT EFI_INPUT_KEY                 *Key);

typedef EFI_STATUS (EFIAPI *EFI_INPUT_RESET)(
    IN EFI_SIMPLE_TEXT_INPUT_PROTOCOL *This,
    IN BOOLEAN                         ExtendedVerification);

struct _EFI_SIMPLE_TEXT_INPUT_PROTOCOL {
  EFI_INPUT_RESET     Reset;
  EFI_INPUT_READ_KEY  ReadKeyStroke;
  EFI_EVENT           WaitForKey;
};

/* ── Simple Text Output Protocol (minimal) ─────────────────────────────── */

typedef struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_TEXT_OUTPUT_STRING)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This,
    IN CHAR16                          *String);

typedef EFI_STATUS (EFIAPI *EFI_TEXT_CLEAR_SCREEN)(
    IN EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *This);

struct _EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL {
  VOID                    *Reset;
  EFI_TEXT_OUTPUT_STRING   OutputString;
  VOID                    *TestString;
  VOID                    *QueryMode;
  VOID                    *SetMode;
  VOID                    *SetAttribute;
  EFI_TEXT_CLEAR_SCREEN    ClearScreen;
  VOID                    *SetCursorPosition;
  VOID                    *EnableCursor;
  VOID                    *Mode;
};

/* ── Graphics Output Protocol (GOP) ────────────────────────────────────── */

typedef struct {
  UINT32  RedMask;
  UINT32  GreenMask;
  UINT32  BlueMask;
  UINT32  ReservedMask;
} EFI_PIXEL_BITMASK;

typedef enum {
  PixelRedGreenBlueReserved8BitPerColor,
  PixelBlueGreenRedReserved8BitPerColor,
  PixelBitMask,
  PixelBltOnly,
  PixelFormatMax
} EFI_GRAPHICS_PIXEL_FORMAT;

typedef struct {
  UINT32                     Version;
  UINT32                     HorizontalResolution;
  UINT32                     VerticalResolution;
  EFI_GRAPHICS_PIXEL_FORMAT  PixelFormat;
  EFI_PIXEL_BITMASK          PixelInformation;
  UINT32                     PixelsPerScanLine;
} EFI_GRAPHICS_OUTPUT_MODE_INFORMATION;

typedef struct {
  UINT32                                 MaxMode;
  UINT32                                 Mode;
  EFI_GRAPHICS_OUTPUT_MODE_INFORMATION   *Info;
  UINTN                                  SizeOfInfo;
  EFI_PHYSICAL_ADDRESS                   FrameBufferBase;
  UINTN                                  FrameBufferSize;
} EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE;

typedef struct _EFI_GRAPHICS_OUTPUT_PROTOCOL EFI_GRAPHICS_OUTPUT_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE)(
    IN  EFI_GRAPHICS_OUTPUT_PROTOCOL          *This,
    IN  UINT32                                 ModeNumber,
    OUT UINTN                                 *SizeOfInfo,
    OUT EFI_GRAPHICS_OUTPUT_MODE_INFORMATION  **Info);

typedef EFI_STATUS (EFIAPI *EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE)(
    IN EFI_GRAPHICS_OUTPUT_PROTOCOL  *This,
    IN UINT32                         ModeNumber);

struct _EFI_GRAPHICS_OUTPUT_PROTOCOL {
  EFI_GRAPHICS_OUTPUT_PROTOCOL_QUERY_MODE  QueryMode;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_SET_MODE    SetMode;
  VOID                                     *Blt;
  EFI_GRAPHICS_OUTPUT_PROTOCOL_MODE        *Mode;
};

/* ── File Protocol ─────────────────────────────────────────────────────── */

#define EFI_FILE_MODE_READ       0x0000000000000001ULL
#define EFI_FILE_MODE_WRITE      0x0000000000000002ULL
#define EFI_FILE_MODE_CREATE     0x8000000000000000ULL
#define EFI_FILE_READ_ONLY       0x0000000000000001ULL
#define EFI_FILE_HIDDEN          0x0000000000000002ULL
#define EFI_FILE_SYSTEM          0x0000000000000004ULL
#define EFI_FILE_DIRECTORY       0x0000000000000010ULL
#define EFI_FILE_ARCHIVE         0x0000000000000020ULL

typedef struct _EFI_FILE_PROTOCOL EFI_FILE_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_FILE_OPEN)(
    IN  EFI_FILE_PROTOCOL *This,
    OUT EFI_FILE_PROTOCOL **NewHandle,
    IN  CHAR16            *FileName,
    IN  UINT64             OpenMode,
    IN  UINT64             Attributes);

typedef EFI_STATUS (EFIAPI *EFI_FILE_CLOSE)(IN EFI_FILE_PROTOCOL *This);

typedef EFI_STATUS (EFIAPI *EFI_FILE_READ)(
    IN     EFI_FILE_PROTOCOL *This,
    IN OUT UINTN             *BufferSize,
    OUT    VOID              *Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FILE_GET_POSITION)(
    IN  EFI_FILE_PROTOCOL *This,
    OUT UINT64            *Position);

typedef EFI_STATUS (EFIAPI *EFI_FILE_SET_POSITION)(
    IN EFI_FILE_PROTOCOL *This,
    IN UINT64             Position);

struct _EFI_FILE_PROTOCOL {
  UINT64                Revision;
  EFI_FILE_OPEN         Open;
  EFI_FILE_CLOSE        Close;
  VOID                  *Delete;
  EFI_FILE_READ         Read;
  VOID                  *Write;
  EFI_FILE_GET_POSITION GetPosition;
  EFI_FILE_SET_POSITION SetPosition;
  VOID                  *GetInfo;
  VOID                  *SetInfo;
  VOID                  *Flush;
};

/* ── Simple File System Protocol ───────────────────────────────────────── */

typedef struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL EFI_SIMPLE_FILE_SYSTEM_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME)(
    IN  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *This,
    OUT EFI_FILE_PROTOCOL              **Root);

struct _EFI_SIMPLE_FILE_SYSTEM_PROTOCOL {
  UINT64                                      Revision;
  EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME  OpenVolume;
};

/* ── Configuration Table ───────────────────────────────────────────────── */

typedef struct {
  EFI_GUID  VendorGuid;
  VOID     *VendorTable;
} EFI_CONFIGURATION_TABLE;

/* ── Boot Services function pointer typedefs ───────────────────────────── */

typedef EFI_STATUS (EFIAPI *EFI_ALLOCATE_POOL)(
    IN  EFI_MEMORY_TYPE  PoolType,
    IN  UINTN             Size,
    OUT VOID             **Buffer);

typedef EFI_STATUS (EFIAPI *EFI_FREE_POOL)(IN VOID *Buffer);

typedef EFI_STATUS (EFIAPI *EFI_LOCATE_HANDLE_BUFFER)(
    IN     EFI_LOCATE_SEARCH_TYPE  SearchType,
    IN     EFI_GUID               *Protocol,
    IN     VOID                   *SearchKey,
    IN OUT UINTN                  *NoHandles,
    OUT    EFI_HANDLE             **Buffer);

typedef EFI_STATUS (EFIAPI *EFI_HANDLE_PROTOCOL)(
    IN  EFI_HANDLE  Handle,
    IN  EFI_GUID   *Protocol,
    OUT VOID       **Interface);

typedef EFI_STATUS (EFIAPI *EFI_CREATE_EVENT)(
    IN  UINT32             Type,
    IN  EFI_TPL            NotifyTpl,
    IN  EFI_EVENT_NOTIFY   NotifyFunction,
    IN  VOID              *NotifyContext,
    OUT EFI_EVENT         *Event);

typedef EFI_STATUS (EFIAPI *EFI_STALL)(IN UINTN Microseconds);
typedef EFI_STATUS (EFIAPI *EFI_CHECK_EVENT)(IN EFI_EVENT Event);
typedef EFI_STATUS (EFIAPI *EFI_WAIT_FOR_EVENT)(
    IN UINTN NumberOfEvents, IN EFI_EVENT *Event, OUT UINTN *Index);
typedef EFI_STATUS (EFIAPI *EFI_SET_TIMER)(
    IN EFI_EVENT Event, IN EFI_TIMER_DELAY Type, IN UINT64 TriggerTime);
typedef EFI_STATUS (EFIAPI *EFI_CLOSE_EVENT)(IN EFI_EVENT Event);
typedef EFI_TPL (EFIAPI *EFI_RAISE_TPL)(IN EFI_TPL NewTpl);
typedef VOID    (EFIAPI *EFI_RESTORE_TPL)(IN EFI_TPL OldTpl);
typedef VOID   *(EFIAPI *EFI_COPY_MEM)(IN VOID *Destination, IN const VOID *Source, IN UINTN Length);
typedef VOID   *(EFIAPI *EFI_SET_MEM)(IN VOID *Buffer, IN UINTN Size, IN UINT8 Value);

typedef EFI_STATUS (EFIAPI *EFI_OPEN_PROTOCOL)(
    IN  EFI_HANDLE  Handle,
    IN  EFI_GUID   *Protocol,
    OUT VOID       **Interface,
    IN  EFI_HANDLE  AgentHandle,
    IN  EFI_HANDLE  ControllerHandle,
    IN  UINT32      Attributes);

#define EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL  0x00000001U
#define EFI_OPEN_PROTOCOL_GET_PROTOCOL        0x00000002U
#define EFI_OPEN_PROTOCOL_TEST_PROTOCOL       0x00000004U

/* ── System Table ──────────────────────────────────────────────────────── */

typedef struct {
  EFI_TABLE_HEADER                Hdr;
  CHAR16                         *FirmwareVendor;
  UINT32                          FirmwareRevision;
  EFI_HANDLE                      ConsoleInHandle;
  EFI_SIMPLE_TEXT_INPUT_PROTOCOL *ConIn;
  EFI_HANDLE                      ConsoleOutHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *ConOut;
  EFI_HANDLE                      StandardErrorHandle;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL *StdErr;
  VOID                           *RuntimeServices;
  VOID                           *BootServices;
  UINTN                           NumberOfTableEntries;
  EFI_CONFIGURATION_TABLE        *ConfigurationTable;
} EFI_SYSTEM_TABLE;

/* ── Boot Services table (must be after all used types are defined) ────── */

typedef struct {
  EFI_TABLE_HEADER               Hdr;
  EFI_RAISE_TPL                   RaiseTPL;
  EFI_RESTORE_TPL                 RestoreTPL;
  VOID                           *AllocatePages;
  VOID                           *FreePages;
  VOID                           *GetMemoryMap;
  EFI_ALLOCATE_POOL               AllocatePool;
  EFI_FREE_POOL                   FreePool;
  EFI_CREATE_EVENT                CreateEvent;
  EFI_SET_TIMER                   SetTimer;
  EFI_WAIT_FOR_EVENT              WaitForEvent;
  VOID                           *SignalEvent;
  EFI_CLOSE_EVENT                 CloseEvent;
  EFI_CHECK_EVENT                 CheckEvent;
  VOID                           *InstallProtocolInterface;
  VOID                           *ReinstallProtocolInterface;
  VOID                           *UninstallProtocolInterface;
  EFI_HANDLE_PROTOCOL             HandleProtocol;
  VOID                           *PCHandleProtocol;
  VOID                           *RegisterProtocolNotify;
  VOID                           *LocateHandle;
  VOID                           *LocateDevicePath;
  VOID                           *InstallConfigurationTable;
  VOID                           *LoadImage;
  VOID                           *StartImage;
  VOID                           *Exit;
  VOID                           *UnloadImage;
  VOID                           *ExitBootServices;
  VOID                           *GetNextMonotonicCount;
  EFI_STALL                       Stall;
  VOID                           *SetWatchdogTimer;
  VOID                           *ConnectController;
  VOID                           *DisconnectController;
  EFI_OPEN_PROTOCOL               OpenProtocol;
  VOID                           *CloseProtocol;
  VOID                           *OpenProtocolInformation;
  VOID                           *ProtocolsPerHandle;
  EFI_LOCATE_HANDLE_BUFFER        LocateHandleBuffer;
  VOID                           *LocateProtocol;
  VOID                           *InstallMultipleProtocolInterfaces;
  VOID                           *UninstallMultipleProtocolInterfaces;
  VOID                           *CalculateCrc32;
  EFI_COPY_MEM                    CopyMem;
  EFI_SET_MEM                     SetMem;
  VOID                           *CreateEventEx;
} EFI_BOOT_SERVICES;

/* ── EFI Loaded Image Protocol ─────────────────────────────────────────── */

typedef struct {
  UINT32      Revision;
  EFI_HANDLE  ParentHandle;
  VOID       *SystemTable;
  EFI_HANDLE  DeviceHandle;    /* ← the filesystem device we were loaded from */
  VOID       *FilePath;
  VOID       *Reserved;
  UINT32      LoadOptionsSize;
  VOID       *LoadOptions;
  VOID       *ImageBase;
  UINT64      ImageSize;
  EFI_MEMORY_TYPE  ImageCodeType;
  EFI_MEMORY_TYPE  ImageDataType;
  VOID       *Unload;
} EFI_LOADED_IMAGE_PROTOCOL;

/* ── EFI Shell Protocol (minimal — file I/O only) ──────────────────────── */

typedef struct _EFI_SHELL_PROTOCOL EFI_SHELL_PROTOCOL;

typedef EFI_STATUS (EFIAPI *EFI_SHELL_OPEN_FILE_BY_NAME)(
    IN CHAR16 *FileName,
    OUT EFI_FILE_PROTOCOL **FileHandle,
    IN UINT64 OpenMode);

typedef EFI_STATUS (EFIAPI *EFI_SHELL_CLOSE_FILE)(
    IN EFI_FILE_PROTOCOL *FileHandle);

struct _EFI_SHELL_PROTOCOL {
  VOID                       *Execute;
  VOID                       *GetEnv;
  VOID                       *SetEnv;
  VOID                       *GetAlias;
  VOID                       *SetAlias;
  VOID                       *GetHelpText;
  VOID                       *GetDevicePathFromMap;
  VOID                       *GetMapFromDevicePath;
  VOID                       *GetDevicePathFromFilePath;
  VOID                       *GetFilePathFromDevicePath;
  VOID                       *SetMap;
  VOID                       *GetCurDir;
  VOID                       *SetCurDir;
  VOID                       *OpenFileList;
  EFI_SHELL_OPEN_FILE_BY_NAME OpenFileByName;
  EFI_SHELL_CLOSE_FILE        CloseFile;
  VOID                       *CreateFile;
  VOID                       *ReadFile;
  VOID                       *WriteFile;
  VOID                       *DeleteFile;
  VOID                       *DeleteFileByName;
  VOID                       *GetFileInfo;
  VOID                       *SetFileInfo;
  VOID                       *FlushFile;
  VOID                       *FindFiles;
  VOID                       *FindFilesInDir;
  VOID                       *GetFileSize;
  VOID                       *OpenRoot;
  VOID                       *OpenRootByHandle;
  VOID                       *GetCurrentDir;
  VOID                       *SetCurrentDir;
  VOID                       *CurDir;
  VOID                       *NewShell;
  VOID                       *BatchFile;
  VOID                       *BatchIsActive;
  VOID                       *Execute2;
  VOID                       *GetGlobalDevicePath;
  VOID                       *GetDevicePathFromFilePath2;
  VOID                       *NameToGuid;
  VOID                       *GuidToName;
  VOID                       *GetEnv2;
  VOID                       *SetEnv2;
};

/* ── Protocol GUIDs ────────────────────────────────────────────────────── */

#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID \
  {0x9042a9de, 0x23dc, 0x4a38, {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a}}

#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID \
  {0x0964e5b2, 0x6459, 0x11d2, {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}}

#define EFI_LOADED_IMAGE_PROTOCOL_GUID \
  {0x5B1B31A1, 0x9562, 0x11d2, {0x8E, 0x3F, 0x00, 0xA0, 0xC9, 0x69, 0x72, 0x3B}}

#define EFI_SHELL_PROTOCOL_GUID \
  {0x6302d008, 0x7f9b, 0x4f30, {0x87, 0xac, 0x60, 0xc9, 0xfe, 0xf5, 0xda, 0x4e}}

/* ── Helpers ───────────────────────────────────────────────────────────── */

#define EFI_GUID_EQUAL(a, b) \
  (*(UINT64*)(&(a)) == *(UINT64*)(&(b)) && \
   *(UINT64*)((UINT8*)&(a) + 8) == *(UINT64*)((UINT8*)&(b) + 8))

/* ── Global state ──────────────────────────────────────────────────────── */

extern EFI_BOOT_SERVICES *gBS;
extern EFI_SYSTEM_TABLE  *gST;
extern EFI_HANDLE         gImageHandle;

#endif /* UEFI_TYPES_H */
