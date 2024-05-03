#include "http-server.hpp"
#include "show-screen.hpp"
#include <thread>

using namespace std;

int main() {
    thread video_thread(showScreen);
   // showScreen();
    httpServerTest();
    video_thread.join();
}
