#include "terminal.h"
#include <random>
#include <chrono>

Terminal::Terminal(const std::string id, const User* u, uint16_t keepAlive, const Will* w) : isConnecting(false), cleanSession(false), ID(id), user(u), will(w), keepAlive(keepAlive) {
    std::random_device rnd;
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    mt(); // TODO: apply seed
}

Terminal::~Terminal() {
    delete user;
    delete will;
}
