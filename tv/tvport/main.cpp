#include "http-server.hpp"
#include "show-screen.hpp"
#include <thread>

int main() {
    std::thread video_thread(showScreen);
    httpServerTest();
    video_thread.join();
}
