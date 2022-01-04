# PageStealer

Uses vulnerable driver to copy page entries.

### PageStealer namespace exports:

* get PID from process name
* leak KPROCESS by PID
* get physical address using KPROCESS and virtual address from target process address space
* Map target virtual page from source process address space to dest. process address space (different process virtual spaces points to the same physical page)

### Features:
* Dynamically get kernel structure offsets (requires internet to download ntoskrnl.pdb)
* automatically extract and install services (based on ATSZIO, IQWV and ASROCK well-known vulnerabilities) 

### TO DO:
- [x]: Steal signle target page from process
- [ ]: Steal entire address space from process
- [ ]: Map process virtual pages to arbitary allocated pages of dest. process (like malloc(PAGE_COUNT * PAGE_SIZE) will point to target physical pages)
