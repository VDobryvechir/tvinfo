#ifndef HTTP_SERVER_INSTANSE_HPP
#define HTTP_SERVER_INSTANSE_HPP 
class HttpServerInstance {
	static void httpClientTest();
public:
	static void run();
	static void runWithSelfTest();
	static bool port_in_use(unsigned short port);
};

#endif
