#ifndef HTTP_SERVER_INSTANSE_HPP
#define HTTP_SERVER_INSTANSE_HPP 
#include <string> 

class HttpServerInstance {
	static void httpClientTest();
public:
	static void run();
	static void runWithSelfTest();
	static bool port_in_use(unsigned short port);
	static std::string detectContentType(std::string url);
	static std::string detectWebFolderName(std::string url);
	static std::string getWebRestPath(std::string url);
	static int readIntValueInParams(std::string body, std::string param, int defValue);
};

#endif
