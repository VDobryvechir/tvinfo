#include "http-server.hpp"
#include "show-screen.hpp"
#include <thread>

int main() {
    std::thread video_thread(showScreen);
    HttpServerInstance::runWithSelfTest();
    video_thread.join();
}
