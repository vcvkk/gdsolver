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
