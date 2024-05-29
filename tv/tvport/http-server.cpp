#include "webserver/client_http.hpp"
#include "webserver/server_http.hpp"
#include "parameters.hpp"
#include "slots.hpp"

#define BOOST_SPIRIT_THREADSAFE
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <algorithm>
#include <boost/filesystem.hpp>
#include <fstream>
#include <vector>
#ifdef HAVE_OPENSSL
#include "webserver/crypto.hpp"
#endif
#include "http-server.hpp"

using namespace boost::property_tree;

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using HttpClient = SimpleWeb::Client<SimpleWeb::HTTP>;


void HttpServerInstance::run() {
  // HTTP-server at port 80 using 1 thread
  // Unless you do more heavy non-threaded processing in the resources,
  // 1 thread is usually faster than several threads
  HttpServer server;
  server.config.port = ParamUtils::readParameterPortNumber();

  // Add resources using path-regex and method-string, and an anonymous function
  // POST-example for the path /string, responds the posted string
  server.resource["^/string$"]["POST"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    // Retrieve string:
    auto content = request->content.string();
    // request->content.string() is a convenience function for:
    // stringstream ss;
    // ss << request->content.rdbuf();
    // auto content=ss.str();

    *response << "HTTP/1.1 200 OK\r\nContent-Length: " << content.length() << "\r\n\r\n"
              << content;


    // Alternatively, use one of the convenience functions, for instance:
    // response->write(content);
  };

  server.resource["^/config$"]["POST"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    try {
        std::string conf = request->content.string();
        std::string resp = tvPortSlots.uploadConfig(conf);

      *response << "HTTP/1.1 200 OK\r\n"
                << "Content-Length: " << resp.length() << "\r\n\r\n"
                << resp;
    }
    catch(const std::exception &e) {
      *response << "HTTP/1.1 400 Bad Request\r\nContent-Length: " << strlen(e.what()) << "\r\n\r\n"
                << e.what();
    }
  };


  // Responds with request-information
  server.resource["^/info$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    std::stringstream stream;
    stream << "<h1>Request from " << request->remote_endpoint_address() << ":" << request->remote_endpoint_port() << "</h1>";

    stream << request->method << " " << request->path << " HTTP/" << request->http_version;

    stream << "<h2>Query Fields</h2>";
    auto query_fields = request->parse_query_string();
    for(auto &field : query_fields)
      stream << field.first << ": " << field.second << "<br>";

    stream << "<h2>Header Fields</h2>";
    for(auto &field : request->header)
      stream << field.first << ": " << field.second << "<br>";

    response->write(stream);
  };

  server.resource["^/status$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
      std::stringstream stream;
      stream << "UP " << request->remote_endpoint_address() << ":" << request->remote_endpoint_port();
      response->write(stream);
      };

  server.resource["^/upload/([0-9,_]+)$"]["POST"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
      std::string nr = request->path_match[1];
      int offset1 = nr.find('_');
      if (offset1 == std::string::npos) {
          response->write("url must contain _");
          return;
      }
      int offset2 = nr.find('_', offset1 + 1);
      if (offset2 == std::string::npos) {
          response->write("url must contain _ and _");
          return;
      }
      std::string nrAmount = nr.substr(offset2 + 1);
      int amount = std::stoi(nrAmount);
      std::string nrUpload = nr.substr(0, offset2);
      std::unique_ptr<char[]> buffer(new char[amount]);
      char* data = buffer.get();
      request->content.read(data, static_cast<std::streamsize>(amount));
      std::string res = tvPortSlots.uploadFile(nrUpload, amount, data);
      response->write(res);
  };

  // GET-example simulating heavy work in a separate thread
  server.resource["^/work$"]["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> /*request*/) {
    std::thread work_thread([response] {
      std::this_thread::sleep_for(std::chrono::seconds(5));
      response->write("Work done");
    });
    work_thread.detach();
  };

  // Default GET-example. If no other matches, this anonymous function will be called.
  // Will respond with content in the web/-directory, and its subdirectories.
  // Default file: index.html
  // Can for instance be used to retrieve an HTML 5 client that uses REST-resources on this server
  server.default_resource["GET"] = [](std::shared_ptr<HttpServer::Response> response, std::shared_ptr<HttpServer::Request> request) {
    try {
      auto web_root_path = boost::filesystem::canonical("0");
      auto path = boost::filesystem::canonical(web_root_path / request->path);
      // Check if path is within web_root_path
      if(std::distance(web_root_path.begin(), web_root_path.end()) > std::distance(path.begin(), path.end()) ||
         !std::equal(web_root_path.begin(), web_root_path.end(), path.begin()))
        throw std::invalid_argument("path must be within root path");
      if(boost::filesystem::is_directory(path))
        path /= "index.html";

      SimpleWeb::CaseInsensitiveMultimap header;

      // Uncomment the following line to enable Cache-Control
      // header.emplace("Cache-Control", "max-age=86400");

#ifdef HAVE_OPENSSL
//    Uncomment the following lines to enable ETag
//    {
//      ifstream ifs(path.string(), ifstream::in | ios::binary);
//      if(ifs) {
//        auto hash = SimpleWeb::Crypto::to_hex_string(SimpleWeb::Crypto::md5(ifs));
//        header.emplace("ETag", "\"" + hash + "\"");
//        auto it = request->header.find("If-None-Match");
//        if(it != request->header.end()) {
//          if(!it->second.empty() && it->second.compare(1, hash.size(), hash) == 0) {
//            response->write(SimpleWeb::StatusCode::redirection_not_modified, header);
//            return;
//          }
//        }
//      }
//      else
//        throw invalid_argument("could not read file");
//    }
#endif

      auto ifs = std::make_shared<std::ifstream>();
      ifs->open(path.string(), std::ifstream::in | std::ios::binary | std::ios::ate);

      if(*ifs) {
        auto length = ifs->tellg();
        ifs->seekg(0, std::ios::beg);

        header.emplace("Content-Length", to_string(length));
        response->write(header);

        // Trick to define a recursive function within this scope (for example purposes)
        class FileServer {
        public:
          static void read_and_send(const std::shared_ptr<HttpServer::Response> &response, const std::shared_ptr<std::ifstream> &ifs) {
            // Read and send 128 KB at a time
            static std::vector<char> buffer(131072); // Safe when server is running on one thread
            std::streamsize read_length;
            if((read_length = ifs->read(&buffer[0], static_cast<std::streamsize>(buffer.size())).gcount()) > 0) {
              response->write(&buffer[0], read_length);
              if(read_length == static_cast<std::streamsize>(buffer.size())) {
                response->send([response, ifs](const SimpleWeb::error_code &ec) {
                  if(!ec)
                    read_and_send(response, ifs);
                  else
                    std::cerr << "Connection interrupted" << std::endl;
                });
              }
            }
          }
        };
        FileServer::read_and_send(response, ifs);
      }
      else
        throw std::invalid_argument("could not read file");
    }
    catch(const std::exception &e) {
      response->write(SimpleWeb::StatusCode::client_error_bad_request, "Could not open path " + request->path + ": " + e.what());
    }
  };

  server.on_error = [](std::shared_ptr<HttpServer::Request> /*request*/, const SimpleWeb::error_code & /*ec*/) {
    // Handle errors here
    // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
  };
  while (port_in_use(server.config.port)) {
      std::cout << "To start http server, please release port " << server.config.port << ". If you cannot release the port, stop this program and change the port number in port-number.txt. " << std::endl;
      std::this_thread::sleep_for(std::chrono::seconds(10));
  }
  std::cout << "Starting http server at " << server.config.port << std::endl;
    // Start server
  server.start();
  std::cout << "Http server either could not start or was stopped" << std::endl;
}

bool HttpServerInstance::port_in_use(unsigned short port) {
    using namespace boost::asio;
    using ip::tcp;

    io_service svc;
    tcp::acceptor a(svc);

    boost::system::error_code ec;
    a.open(tcp::v4(), ec) || a.bind({tcp::v4(), port}, ec);
    std::cout << "Binding at "<< port << " error code="<< ec << std::endl;
    return ec == error::address_in_use;
}

void HttpServerInstance::httpClientTest() {
  std::this_thread::sleep_for(std::chrono::seconds(1));
  char clientUrlPath[40];
  int portNumber = ParamUtils::readParameterPortNumber();
  sprintf_s(clientUrlPath, "localhost:%d", portNumber);
  // Client 
  HttpClient client(clientUrlPath);

  std::string json_string = "{\"Http Self-Test Passed\"}";

  // Synchronous request 
  try {
    auto r2 = client.request("POST", "/string", json_string);
    std::cout << r2->content.rdbuf() << std::endl;
  }
  catch(const SimpleWeb::system_error &e) {
    std::cerr << "Client request error: " << e.what() << std::endl;
  }
 
}

void HttpServerInstance::runWithSelfTest() {
    std::thread server_thread(run);
    httpClientTest();
    server_thread.join(); 
}

