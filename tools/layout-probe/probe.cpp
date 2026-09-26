// Every class gdsolver reads by raw offset. sizeof() forces the layout to be computed in this
// translation unit, which is what makes -fdump-record-layouts print it.
#include <Geode/Geode.hpp>

using namespace geode::prelude;

size_t gdsolverLayoutProbe() {
    return sizeof(PlayerObject) + sizeof(GameObject) + sizeof(EnhancedGameObject)
         + sizeof(EffectGameObject) + sizeof(EnterEffectObject) + sizeof(GJBaseGameLayer)
         + sizeof(PlayLayer) + sizeof(GJEffectManager) + sizeof(EnterEffectInstance)
         + sizeof(GJGameLevel) + sizeof(TeleportPortalObject) + sizeof(RingObject);
}

// A virtual call makes the compiler lay out GameObject's vtable here, which is what makes
// -fdump-vtable-layouts print it (areaenv calls a GameObject virtual by Windows slot).
void gdsolverVtableProbe(GameObject* o) {
    (void)o->getRealPosition();
    (void)o->getStartPos();
}
