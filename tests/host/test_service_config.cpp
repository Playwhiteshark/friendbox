#include "ServiceConfig.h"

#include <cassert>
#include <iostream>

int main() {
    using friendbox::service::shouldSeedBootstrap;

    // Only a never-initialized device with no meaningful stored service
    // settings may be seeded from local defaults.
    assert(shouldSeedBootstrap(false, false, true));
    assert(!shouldSeedBootstrap(true, false, true));
    assert(!shouldSeedBootstrap(false, true, true));
    assert(!shouldSeedBootstrap(false, false, false));
    assert(!shouldSeedBootstrap(true, true, true));

    std::cout << "service bootstrap tests passed\n";
    return 0;
}
