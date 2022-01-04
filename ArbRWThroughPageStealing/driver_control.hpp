#pragma once
#include <Windows.h>
#include <string>
#include "ResourceLoader.h"

#include "sys.hpp"

class DriverControl {
public:
    ~DriverControl();

    static inline DriverControl& GetInstance() {
        static DriverControl instance;
        return instance;
    }

    UINT64  Virt2Phys(UINT64 va);

    UINT64  ReadMsr(UINT32 msr);

    UINT8   ReadMem8(UINT64 va);
    UINT16  ReadMem16(UINT64 va);
    UINT32  ReadMem32(UINT64 va);
    void    WriteMem8(UINT64 va, UINT8 value);
    void    WriteMem16(UINT64 va, UINT16 value);
    void    WriteMem32(UINT64 va, UINT32 value);
    
    bool    ReadPhysMem(UINT64 pa, UINT32 size, UINT8* buffer);
    UINT8   ReadPhysMem8(UINT64 pa);
    UINT16  ReadPhysMem16(UINT64 pa);
    UINT32  ReadPhysMem32(UINT64 pa);
    bool    WritePhysMem(UINT64 pa, UINT32 size, UINT8* buffer);
    void    WritePhysMem8(UINT64 pa, UINT8 value);
    void    WritePhysMem16(UINT64 pa, UINT16 value);
    void    WritePhysMem32(UINT64 pa, UINT32 value);
    
    UINT8   ReadIo8(UINT16 port);
    UINT16  ReadIo16(UINT16 port);
    UINT32  ReadIo32(UINT16 port);
    void    WriteIo8(UINT16 port, UINT8 value);
    void    WriteIo16(UINT16 port, UINT16 value);
    void    WriteIo32(UINT16 port, UINT32 value);

    bool    ReadKernelVA(UINT64 va, UINT64 size, UINT8* outbuffer);
	
	UINT32	ReadPci(UINT8 bus, UINT8 device, UINT8 function, UINT8 offset);
	void	WritePci(UINT8 bus, UINT8 device, UINT8 function, UINT8 offset, DWORD value);
    bool    MapViewOfSection(UINT64 pa, UINT32 size, UINT64* va);
    bool    UnmapViewOfSection(UINT64 va);
    bool    ReadOverMapViewOfSection(UINT64 pa, UINT32 size, UINT8* buffer);
    bool    WriteOverMapViewOfSection(UINT64 pa, UINT32 size, UINT8* buffer);
private:
    enum class DriverIndex {
        AsrDrv,
        Iqvw,
        Atzio,
    };

    bool Load();
    void Unload();

    DriverControl();
    DriverControl(DriverControl&) = delete;
    void operator=(const DriverControl&) = delete;

    static HANDLE GetDeviceHandle(const LPCTSTR device_name);
    bool Ioctl(DriverIndex driver_index, DWORD ioctl, LPVOID in_buf, DWORD in_buf_size, LPVOID out_buf, DWORD out_buf_size, LPDWORD b_ret);
    bool MemCopy(UINT64 dest, UINT64 source, UINT32 size);
    UINT64 MapIoSpace(UINT64 pa, UINT32 size);
    bool UnmapIoSpace(UINT64 va, UINT32 size);
	ResourceLoader rs;

    sys::Mode current_mode_  { sys::Mode::Unkn };
    std::string iqvw_path_      { };
    std::string asrdrv_path_    { };
    std::string atzio_path_     { };
};