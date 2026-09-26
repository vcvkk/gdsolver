#pragma once

// Phase 1: measured trace of the physics tick structure + state dump + autorun + input injection
//
// Output (the session data dir, resolved at startup by resolveDataDir):
//   trace.csv  — hook-call event stream (frame, attempt, step, event, a, b, c)
//   dump.csv   — player state after every PlayerObject::update
//   result.txt — session result
// Control (autorun.cfg, in that same dir):
//   enabled=1 / level=1 / attempts=2 / delay=2.0 / quitwhendone=1
//   input=<step>,<1|0>   (1=press, 0=release; multiple lines allowed; the same sequence is
//                         injected in every attempt)
#include <Geode/Geode.hpp>
#include <Geode/modify/GJBaseGameLayer.hpp>
#include <Geode/modify/EnhancedGameObject.hpp>
#include <Geode/modify/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/modify/GameStatsManager.hpp>
#include <Geode/modify/GJGameLevel.hpp>
#include <Geode/modify/FMODAudioEngine.hpp>
#include <Geode/modify/AppDelegate.hpp>
#ifdef GEODE_IS_WINDOWS
#include <Windows.h>
#endif
#include <algorithm>
#include <chrono>
#include <coroutine>
#include <thread>
#include <atomic>
#include <cmath>
#include <exception>
#include <new>
#include <climits>
#include <deque>
#include <cstring>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <vector>

using namespace geode::prelude;

#include "mod/platform.hpp"

// The byte a member occupies, as memory holds it. The diagnostics used to read game fields by raw
// Windows 2.2081 offset and print the byte they found; every one of those offsets turned out to be
// the start of a member the bindings name (tools/layout-probe), so they now read the member -- and
// this keeps them printing the raw byte, not a value the compiler has normalised.
template <class T>
inline int rawByte(T const& member) {
    return (int)*reinterpret_cast<unsigned char const*>(&member);
}

// Where the canary and the ring-claim byte sit, per layout. 0x9bf / 0x740 are Windows 2.2081; the
// iOS values are the ones tools/layout-probe measured for the same members.
#ifdef GEODE_IS_IOS
inline constexpr unsigned kUpsideDownOff = 0x977;
#else
inline constexpr unsigned kUpsideDownOff = 0x9bf;
#endif
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winvalid-offsetof"
inline constexpr size_t ringClaimOff() { return offsetof(RingObject, m_claimTouch); }
#pragma clang diagnostic pop
