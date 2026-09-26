#pragma once
// The few process-level facts the diagnostics ask the OS for, on the platform this port builds
// for. Upstream read them through Win32 (psapi, the PE header, VirtualQuery,
// RtlCaptureStackBackTrace); on iOS they come from Mach and dyld. Nothing here touches physics.
#include <cstddef>
#include <cstdint>

#ifdef GEODE_IS_IOS
#include <dlfcn.h>
#include <execinfo.h>
#include <mach/mach.h>
#include <pthread.h>
#endif

namespace platform {

// {current, peak} physical memory of the process in MB. iOS reports the footprint the jetsam
// limit is measured against (phys_footprint), which is the number that decides whether a long
// solve gets the game killed.
inline void procMemMB(size_t& cur, size_t& peak) {
    cur = peak = 0;
#ifdef GEODE_IS_IOS
    task_vm_info_data_t info{};
    mach_msg_type_number_t count = TASK_VM_INFO_COUNT;
    if (task_info(mach_task_self(), TASK_VM_INFO, reinterpret_cast<task_info_t>(&info), &count)
        == KERN_SUCCESS) {
        cur = (size_t)(info.phys_footprint / (1024 * 1024));
        peak = (size_t)(info.ledger_phys_footprint_peak / (1024 * 1024));
    }
#endif
}

// Offset of `addr` inside the game's own image, or false if it lies elsewhere (Geode, the mod,
// a system library). Used to name call sites the way the Windows build printed RVAs.
inline bool gameRva(uintptr_t addr, uintptr_t& rva) {
#ifdef GEODE_IS_IOS
    const uintptr_t base = (uintptr_t)geode::base::get();
    Dl_info info{};
    if (addr < base || !dladdr(reinterpret_cast<void*>(addr), &info)) return false;
    if ((uintptr_t)info.dli_fbase != base) return false;
    rva = addr - base;
    return true;
#else
    (void)addr; (void)rva;
    return false;
#endif
}

// The calling thread's return addresses, innermost first.
inline int captureStack(void** frames, int max) {
#ifdef GEODE_IS_IOS
    return backtrace(frames, max);
#else
    (void)frames; (void)max;
    return 0;
#endif
}

// The end (highest address) of the calling thread's stack, for scans that walk up from a local.
inline uintptr_t stackTop() {
#ifdef GEODE_IS_IOS
    return (uintptr_t)pthread_get_stackaddr_np(pthread_self());
#else
    return 0;
#endif
}

}  // namespace platform
