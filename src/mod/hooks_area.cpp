// GJBaseGameLayer's area effects: where GD's own random numbers can put an object an Area Move
// pushes (solver/areaenv.hpp). Every hook here only watches -- each calls the original with the
// arguments it was given, before or after looking.
#include "mod/playlayer_helpers.hpp"

using namespace p1;

namespace {

// Only while a session records moving geometry: the box goes into that recording and nowhere else.
bool envActive() {
    return g_cfg.areaEnv && grouptrace::g_on && g_started && !g_sessionOver;
}

// Area Rotate / Area Scale draw from the same table (their variances), and nothing here bounds
// them -- so they are counted, and the session line says how many ran.
bool rotScaleVaries(EnterEffectInstance* inst) {
    using namespace areaenv;
    return fld<float>(inst, kLenV) != 0.f || fld<float>(inst, kOffV) != 0.f
           || fld<float>(inst, kOffYV) != 0.f || fld<float>(inst, kRotV) != 0.f
           || fld<float>(inst, kScaleXV) != 0.f || fld<float>(inst, kScaleYV) != 0.f;
}

#ifndef GEODE_IS_WINDOWS
// moveAreaObject is inlined into processAreaMoveGroupAction outside Windows, so it cannot be
// hooked. Its body (bindings/inline/GJBaseGameLayer.cpp) opens with
// resetAreaObjectValues(object, true) and, unless nothing moved, closes with
// updateObjectSection(object) -- both real functions here. The move is seen between the two:
// the reset right after an object was processed is the move's own, and the offset it adds is
// the displacement GD applied. A move that returns early (nothing to apply) has no closing call
// and is not reported, where the Windows hook reported it as (0, 0).
GameObject* g_lastProcessed = nullptr;
GameObject* g_moveObj = nullptr;
float g_moveX0 = 0.f, g_moveY0 = 0.f;
#endif

}  // namespace

class $modify(AreaEnvLayer, GJBaseGameLayer) {
    void processAreaMoveGroupAction(cocos2d::CCArray* objects, EnterEffectInstance* instance,
                                    cocos2d::CCPoint position, int outerMin, int outerMax,
                                    int middleMin, int middleMax, int startIndex,
                                    bool targetGroups, bool reset) {
        const bool on = envActive() && instance
                        && areaenv::fld<void*>(instance, areaenv::kTrigger) != nullptr;
#ifndef GEODE_IS_WINDOWS
        g_lastProcessed = g_moveObj = nullptr;
#endif
        if (on) {
            areaenv::g_act = areaenv::Action{true, this, instance,
                                             areaenv::fld<void*>(instance, areaenv::kTrigger),
                                             position, targetGroups};
        }
        GJBaseGameLayer::processAreaMoveGroupAction(objects, instance, position, outerMin,
                                                    outerMax, middleMin, middleMax, startIndex,
                                                    targetGroups, reset);
        if (on) {
            areaenv::settle();
            areaenv::g_act = areaenv::Action{};
        }
#ifndef GEODE_IS_WINDOWS
        g_lastProcessed = g_moveObj = nullptr;
#endif
    }

    // processAreaMoveGroupAction resets each object it is about to process (GD's filter already
    // passed) and reads it straight after -- this is that moment, in that order.
    bool resetAreaObjectValues(GameObject* object, bool update) {
        const bool r = GJBaseGameLayer::resetAreaObjectValues(object, update);
#ifdef GEODE_IS_WINDOWS
        if (areaenv::g_act.on && areaenv::g_act.l == this && areaenv::g_inMove == 0 && object)
            areaenv::onProcessed(object);
#else
        if (areaenv::g_act.on && areaenv::g_act.l == this && object) {
            if (object == g_lastProcessed) {
                // the reset the inlined moveAreaObject opens with: not a new object
                g_lastProcessed = nullptr;
                g_moveObj = object;
                g_moveX0 = object->m_positionXOffset;
                g_moveY0 = object->m_positionYOffset;
            } else {
                areaenv::onProcessed(object);
                g_lastProcessed = object;
            }
        }
#endif
        return r;
    }

#ifdef GEODE_IS_WINDOWS
    void moveAreaObject(GameObject* object, float dx, float dy) {
        ++areaenv::g_inMove;   // it resets its target itself; that is not a new object
        GJBaseGameLayer::moveAreaObject(object, dx, dy);
        --areaenv::g_inMove;
        if (areaenv::g_act.on && areaenv::g_act.l == this && object)
            areaenv::onMove(object, dx, dy);
    }
#else
    void updateObjectSection(GameObject* object) {
        GJBaseGameLayer::updateObjectSection(object);
        if (object && object == g_moveObj && areaenv::g_act.on && areaenv::g_act.l == this) {
            g_moveObj = nullptr;
            // What the move added to the offsets its reset had just settled. An object with its x
            // locked (m_tempOffsetXRelated) takes no x, so its x displacement is not observable
            // here: it reads 0, and the instrument's displacement check can count a miss on that
            // axis where the Windows hook saw GD's argument. The envelope does not use it.
            areaenv::onMove(object, object->m_positionXOffset - g_moveX0,
                            object->m_positionYOffset - g_moveY0);
        }
    }
#endif

    void processAreaRotateGroupAction(cocos2d::CCArray* objects, EnterEffectInstance* instance,
                                      cocos2d::CCPoint position, int outerMin, int outerMax,
                                      int middleMin, int middleMax, int startIndex,
                                      bool targetGroups, bool reset) {
        if (envActive() && instance && objects && objects->count() > 0
            && rotScaleVaries(instance))
            ++areaenv::g_unenveloped;
        GJBaseGameLayer::processAreaRotateGroupAction(objects, instance, position, outerMin,
                                                      outerMax, middleMin, middleMax,
                                                      startIndex, targetGroups, reset);
    }

    void processAreaTransformGroupAction(cocos2d::CCArray* objects, EnterEffectInstance* instance,
                                         cocos2d::CCPoint position, int outerMin, int outerMax,
                                         int middleMin, int middleMax, int startIndex,
                                         bool targetGroups, bool reset) {
        if (envActive() && instance && objects && objects->count() > 0
            && rotScaleVaries(instance))
            ++areaenv::g_unenveloped;
        GJBaseGameLayer::processAreaTransformGroupAction(objects, instance, position, outerMin,
                                                         outerMax, middleMin, middleMax,
                                                         startIndex, targetGroups, reset);
    }
};
