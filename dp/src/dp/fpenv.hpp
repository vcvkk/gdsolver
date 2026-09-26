#pragma once
// The floating-point control word, read and set the same way on every CPU the solver runs on.
// It is printed at the top of every solve (the `fpenv:` lines) because rounding mode and
// flush-to-zero are properties of the process, not of this code: cocos2d, fmod and the graphics
// driver share the process with the mod and got to it first. On x86 the word is MXCSR; on
// arm64 (iOS) it is FPCR, whose rounding (RMode, bits 22-23) and flush-to-zero (FZ, bit 24)
// fields play the same part. The CLI flag keeps its historical name, --mxcsr.
#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#include <xmmintrin.h>
namespace dp {
inline constexpr const char* kFpenvName = "mxcsr";
inline unsigned fpenvRead() { return (unsigned)_mm_getcsr(); }
inline void fpenvWrite(unsigned v) { _mm_setcsr(v); }
}
#elif defined(__aarch64__) || defined(_M_ARM64)
namespace dp {
inline constexpr const char* kFpenvName = "fpcr";
inline unsigned fpenvRead() {
    uint64_t v;
    __asm__ volatile("mrs %0, fpcr" : "=r"(v));
    return (unsigned)v;
}
inline void fpenvWrite(unsigned v) {
    const uint64_t x = v;
    __asm__ volatile("msr fpcr, %0" : : "r"(x));
}
}
#else
#error "fpenv.hpp: no floating-point control word known for this CPU"
#endif
