#include "driver_control.hpp"
#include "asrdrv.hpp"
#include "iqvw.hpp"
#include "atzip.hpp"

DriverControl::DriverControl() 
    : current_mode_(sys::GetCurrentMode()) {

    Load();
}

DriverControl::~DriverControl() {
    Unload();
}

bool DriverControl::Load() {

	return (bool) rs.mapDrv();

}

void DriverControl::Unload() {
	rs.unmapDrv();
}

HANDLE DriverControl::GetDeviceHandle(const LPCTSTR device_name) {
    return CreateFile(device_name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
}

bool DriverControl::Ioctl(
    DriverIndex driver_index,
    DWORD ioctl,
    LPVOID in_buf,
    DWORD in_buf_size,
    LPVOID out_buf,
    DWORD out_buf_size,
    LPDWORD b_ret) {

    HANDLE handle = nullptr;
    switch (driver_index) {
    case DriverIndex::Iqvw:
        handle = GetDeviceHandle((const LPCTSTR) iqvw::kDeviceName);
        break;
    case DriverIndex::AsrDrv:
        handle = GetDeviceHandle((const LPCTSTR) asrdrv::kDeviceName);
        break;
    case DriverIndex::Atzio:
        handle = GetDeviceHandle((const LPCTSTR)atszio::kDeviceName);
        break;
    }

    if (handle == nullptr || handle == INVALID_HANDLE_VALUE) {
        return false;
    }

    bool status = DeviceIoControl(handle, ioctl, in_buf, in_buf_size, out_buf, out_buf_size, b_ret, nullptr);

    CloseHandle(handle);

    return status;
}

bool DriverControl::MemCopy(UINT64 dest, UINT64 source, UINT32 size) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::CopyMemoryBufferInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::MemCopy);
        req.source = source;
        req.destination = dest;
        req.length = size;

        return Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);
    }
    case sys::Mode::X86: {
        iqvw::CopyMemoryBufferInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::MemCopy);
        req.source = static_cast<UINT32>(source);
        req.destination = static_cast<UINT32>(dest);
        req.length = static_cast<UINT32>(size);

        return Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);
    }
    default:
        return false;
    }
}

UINT64 DriverControl::MapIoSpace(UINT64 pa, UINT32 size) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::MapIoSpaceBufferInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::MapIoSpace);
        req.physical_address_to_map = pa;
        req.size = size;

        if (!Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret)) {
            return 0;
        }

        return req.return_virtual_address;
    }
    case sys::Mode::X86: {
        iqvw::MapIoSpaceBufferInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::MapIoSpace);
        req.physical_address_to_map = static_cast<UINT32>(pa);
        req.size = size;

        if (!Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret)) {
            return 0;
        }

        return req.return_value;
    }
    default:
        return 0;
    }
}

bool DriverControl::UnmapIoSpace(UINT64 va, UINT32 size) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::UnmapIoSpaceBufferInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::UnmapIoSpace);
        req.virt_address = va;
        req.number_of_bytes = size;

        return Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);
    }
    case sys::Mode::X86: {
        iqvw::UnmapIoSpaceBufferInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::UnmapIoSpace);
        req.virt_address = static_cast<UINT32>(va);
        req.number_of_bytes = size;

        return Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);
    }
    default:
        return false;
    }
}

UINT64 DriverControl::Virt2Phys(UINT64 va) {
    iqvw::GetPhysAddrBufferInfo req{ 0 };
    DWORD b_ret{ 0 };

    req.case_number = static_cast<UINT64>(iqvw::Subcommand::Virt2Phys);
    req.address_to_translate = va;

    if (!Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret)) {
        return 0;
    }

    return req.return_physical_address;
}

bool DriverControl::ReadPhysMem(UINT64 pa, UINT32 size, UINT8* buffer) {
    bool res = false;

    UINT64 va = MapIoSpace(pa, size);
    if (va != NULL) {
        res = MemCopy(reinterpret_cast<UINT64>(buffer), va, size);
        UnmapIoSpace(va, size);
    }

    return res;
}

UINT8 DriverControl::ReadPhysMem8(UINT64 pa) {
    UINT8 res{ 0 };

    UINT64 va = MapIoSpace(pa, sizeof(UINT8));
    if (va != NULL) {
        res = ReadMem8(va);
        UnmapIoSpace(va, sizeof(UINT8));
    }

    return res;
}

UINT16 DriverControl::ReadPhysMem16(UINT64 pa) {
    UINT16 res{ 0 };

    UINT64 va = MapIoSpace(pa, sizeof(UINT16));
    if (va != NULL) {
        res = ReadMem16(va);
        UnmapIoSpace(va, sizeof(UINT16));
    }

    return res;
}

UINT32 DriverControl::ReadPhysMem32(UINT64 pa) {
    UINT32 res{ 0 };

    UINT64 va = MapIoSpace(pa, sizeof(UINT32));
    if (va != NULL) {
        res = ReadMem32(va);
        UnmapIoSpace(va, sizeof(UINT32));
    }

    return res;
}

bool DriverControl::WritePhysMem(UINT64 pa, UINT32 size, UINT8* buffer) {
    bool res = false;

    UINT64 va = MapIoSpace(pa, size);
    if (va != NULL) {
        res = MemCopy(va, reinterpret_cast<UINT64>(buffer), size);
        UnmapIoSpace(va, size);
    }

    return res;
}

void DriverControl::WritePhysMem8(UINT64 pa, UINT8 value) {
    UINT64 va = MapIoSpace(pa, sizeof(UINT8));
    if (va != NULL) {
        WriteMem8(va, value);
        UnmapIoSpace(va, sizeof(UINT8));
    }
}

void DriverControl::WritePhysMem16(UINT64 pa, UINT16 value) {
    UINT64 va = MapIoSpace(pa, sizeof(UINT16));
    if (va != NULL) {
        WriteMem16(va, value);
        UnmapIoSpace(va, sizeof(UINT16));
    }
}

void DriverControl::WritePhysMem32(UINT64 pa, UINT32 value) {
    UINT64 va = MapIoSpace(pa, sizeof(UINT32));
    if (va != NULL) {
        WriteMem32(va, value);
        UnmapIoSpace(va, sizeof(UINT32));
    }
}

UINT8 DriverControl::ReadMem8(UINT64 va) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::MemRead8);
        req.address = va;

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT8>(req.return_value);
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::MemRead8);
        req.address = static_cast<UINT32>(va);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT8>(req.return_value);
    }
    default:
        return 0;
    }
}

UINT16 DriverControl::ReadMem16(UINT64 va) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::MemRead16);
        req.address = va;

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT16>(req.return_value);
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::MemRead16);
        req.address = static_cast<UINT32>(va);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT16>(req.return_value);
    }
    default:
        return 0;
    }
}

UINT32 DriverControl::ReadMem32(UINT64 va) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::MemRead32);
        req.address = va;

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT32>(req.return_value);
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::MemRead32);
        req.address = static_cast<UINT32>(va);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT32>(req.return_value);
    }
    default:
        return 0;
    }
}

void DriverControl::WriteMem8(UINT64 va, UINT8 value) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::MemWrite8);
        req.address = va;
        req.value = static_cast<UINT64>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::MemWrite8);
        req.address = static_cast<UINT32>(va);
        req.value = static_cast<UINT32>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    default:
        return;
    }
}

void DriverControl::WriteMem16(UINT64 va, UINT16 value) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::MemWrite16);
        req.address = va;
        req.value = static_cast<UINT64>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::MemWrite16);
        req.address = static_cast<UINT32>(va);
        req.value = static_cast<UINT32>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    default:
        return;
    }
}

void DriverControl::WriteMem32(UINT64 va, UINT32 value) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::MemWrite32);
        req.address = va;
        req.value = static_cast<UINT64>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::MemWrite32);
        req.address = static_cast<UINT32>(va);
        req.value = static_cast<UINT32>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    default:
        return;
    }
}

UINT8 DriverControl::ReadIo8(UINT16 port) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::IoRead8);
        req.address = static_cast<UINT64>(port);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT8>(req.return_value);
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::IoRead8);
        req.address = static_cast<UINT32>(port);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT8>(req.return_value);
    }
    default:
        return 0;
    }
}

UINT16 DriverControl::ReadIo16(UINT16 port) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::IoRead16);
        req.address = static_cast<UINT64>(port);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT16>(req.return_value);
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::IoRead16);
        req.address = static_cast<UINT32>(port);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT16>(req.return_value);
    }
    default:
        return 0;
    }
}

UINT32 DriverControl::ReadIo32(UINT16 port) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::IoRead32);
        req.address = static_cast<UINT64>(port);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT32>(req.return_value);
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::IoRead32);
        req.address = static_cast<UINT32>(port);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return static_cast<UINT32>(req.return_value);
    }
    default:
        return 0;
    }
}

void DriverControl::WriteIo8(UINT16 port, UINT8 value) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::IoWrite8);
        req.address = static_cast<UINT64>(port);
        req.value = static_cast<UINT64>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::IoWrite8);
        req.address = static_cast<UINT32>(port);
        req.value = static_cast<UINT32>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    default:
        return;
    }
}

void DriverControl::WriteIo16(UINT16 port, UINT16 value) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::IoWrite16);
        req.address = static_cast<UINT64>(port);
        req.value = static_cast<UINT64>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::IoWrite16);
        req.address = static_cast<UINT32>(port);
        req.value = static_cast<UINT32>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    default:
        return;
    }
}

void DriverControl::WriteIo32(UINT16 port, UINT32 value) {
    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        iqvw::ReadWriteInfoX64 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT64>(iqvw::Subcommand::IoWrite32);
        req.address = static_cast<UINT64>(port);
        req.value = static_cast<UINT64>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    case sys::Mode::X86: {
        iqvw::ReadWriteInfoX32 req{ 0 };
        DWORD b_ret{ 0 };

        req.case_number = static_cast<UINT32>(iqvw::Subcommand::IoWrite32);
        req.address = static_cast<UINT32>(port);
        req.value = static_cast<UINT32>(value);

        Ioctl(DriverIndex::Iqvw, iqvw::kIoctlMain, &req, sizeof(req), nullptr, 0, &b_ret);

        return;
    }
    default:
        return;
    }
}

UINT64 DriverControl::ReadMsr(UINT32 msr) {
    DWORD bytes_returned = 0;

    asrdrv::MsrReadBuffer req = { 0 };
    req.msr_addr = msr;

    if (Ioctl(DriverIndex::AsrDrv, asrdrv::kIoctlReadMsr, &req, sizeof(req), &req, sizeof(req), &bytes_returned)) {
        return (static_cast<UINT64>(req.low) + (static_cast<UINT64>(req.high) << 32));
    }

    return 0;
}

bool DriverControl::ReadKernelVA(UINT64 va, UINT64 size, UINT8* outbuffer) {
    constexpr UINT64 kPageSize{ 0x1000 };
    
    UINT64 start_page_addr = va & 0xFFFFFFFFFFFFF000;
    UINT64 end_page_addr = (va + size) | 0xFFF;

    DWORD total_page_count = static_cast<DWORD>((end_page_addr - start_page_addr + 1) / kPageSize);
    UINT8* buffer = (UINT8*)malloc(total_page_count * kPageSize);
    DWORD not_success_iters = 0;
    
    for (unsigned int i = 0; i < total_page_count; ++i) {
        UINT64 pa = Virt2Phys(start_page_addr + (i * kPageSize));
        if (pa != 0) {
            if (ReadPhysMem(pa, kPageSize, &buffer[i * kPageSize])) {
                ;
            }
            else {
                memset(&buffer[i * kPageSize], 0xCC, kPageSize);
                not_success_iters++;
            }
        }
        else {
            memset(&buffer[i * kPageSize], 0xCC, kPageSize);
            not_success_iters++;
        }
    }

    memcpy_s(outbuffer, static_cast<rsize_t>(size), reinterpret_cast<void*>(reinterpret_cast<UINT64>(buffer) + va - start_page_addr), static_cast<rsize_t>(size));

    free(buffer);

    return not_success_iters == 0;
}

UINT32 DriverControl::ReadPci(UINT8 bus, UINT8 device, UINT8 function, UINT8 offset)
{
	asrdrv::ReadPciDwordReq req = { 0 };

	req.bus = bus;
	req.device = device;
	req.func = function;
	req.offset = offset;
	req.ret_val= 0;

	DWORD bytes_returned = 0;

	if (Ioctl(DriverIndex::AsrDrv,
		asrdrv::kIoctlReadPciDword,
		&req,
		sizeof(req),
		&req,
		sizeof(req),
		&bytes_returned)
		) 
	{
		return req.ret_val;
		// return *(DWORD*)(&req);
	}
	return -1;
}

void DriverControl::WritePci(UINT8 bus, UINT8 device, UINT8 function, UINT8 offset, DWORD value)
{
	asrdrv::WritePciDwordReq req = { 0 };

	req.bus = bus;
	req.device = device;
	req.func = function;
	req.offset = offset;
	req.val = value;

	DWORD bytes_returned = 0;

	if (Ioctl(DriverIndex::AsrDrv,
		asrdrv::kIoctlWritePciDword,
		&req,
		sizeof(req),
		&req,
		sizeof(req),
		&bytes_returned)
		)
	{
		return;
		// return *(DWORD*)(&req);
	}
	return;
}

bool DriverControl::MapViewOfSection(UINT64 pa, UINT32 size, UINT64* va)
{
    DWORD bytes_returned = 0;

    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        atszio::MapMemIoCtlStructX64 msg = { 0 };

        msg.CountOfBytes = 0;
        msg.Handle = NULL;
        msg.MapLength = size;
        msg.PA = pa;
        msg.VA = NULL;

        if (Ioctl(DriverIndex::Atzio,
            atszio::kIoctlMapMem,
            &msg,
            sizeof(msg),
            &msg,
            sizeof(msg),
            &bytes_returned))
        {
            *va = (ULONGLONG)msg.VA;
            return true;
        }
        break;
    }
    case sys::Mode::X86: {
        atszio::MapMemIoCtlStruct msg = { 0 };
        msg.CountOfBytes = 0;
        msg.Handle = NULL;
        msg.MapLength = size;
        msg.PA = (PVOID)pa;
        msg.VA = NULL;

        if (Ioctl(DriverIndex::Atzio,
            atszio::kIoctlMapMem,
            &msg,
            sizeof(msg),
            &msg,
            sizeof(msg),
            &bytes_returned))
        {
            *va = (ULONGLONG)msg.VA;
            return true;
        }
    }
    }

    return false;
}


bool    DriverControl::UnmapViewOfSection(UINT64 va)
{
    DWORD bytes_returned = 0;

    switch (current_mode_) {
    case sys::Mode::X64:
    case sys::Mode::X86UnderX64: {
        atszio::UnmapMemIoCtlStructX64 msg = { 0 };
        msg.handle = (HANDLE) - 1;
        msg.VA = va;


        if (Ioctl(DriverIndex::Atzio,
            atszio::kIoctlUnmapMem,
            &msg,
            sizeof(msg),
            &msg,
            sizeof(msg),
            &bytes_returned))
        {
            return true;
        }
        break;
    }
    case sys::Mode::X86:
    {
        atszio::UnmapMemIoCtlStruct msg = { 0 };

        msg.VA = static_cast<UINT32> (va);

        if (Ioctl(DriverIndex::Atzio,
            atszio::kIoctlUnmapMem,
            &msg,
            sizeof(msg),
            &msg,
            sizeof(msg),
            &bytes_returned))
        {
            return true;
        }
    }
    }

    return false;
}

bool    DriverControl::ReadOverMapViewOfSection(UINT64 pa, UINT32 size, UINT8* buffer)
{
    UINT64 tmpPtr = 0;
    if (size > 0x1000)
    {
        printf("%S size err");
        return false;
    }

    DriverControl& dc = DriverControl::GetInstance();
    if (dc.MapViewOfSection(pa & 0xFFFFFFFFFFFFF000, 0x1000, &tmpPtr) && tmpPtr != 0)
    {
        UINT64 offset = pa & 0xFFF;
        memcpy_s(buffer, size, (void*)(tmpPtr + offset), size);
        dc.UnmapViewOfSection(tmpPtr);
        return true;
    }

    return false;
}

bool    DriverControl::WriteOverMapViewOfSection(UINT64 pa, UINT32 size, UINT8* buffer)
{
    UINT64 tmpPtr = 0;

    DriverControl& dc = DriverControl::GetInstance();
    if (dc.MapViewOfSection(pa & 0xFFFFFFFFFFFFF000, 0x1000, &tmpPtr) && tmpPtr != 0)
    {
        UINT64 offset = pa & 0xFFF;
        memcpy_s((void*)(tmpPtr + offset), size, buffer, size);
        dc.UnmapViewOfSection(tmpPtr);
        return true;
    }

    return false;
}