#include "object-pool.h"

#include <string>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace util {
    class ConnectionPool : public ObjectPool<int> {
    public:
        ConnectionPool(std::string const& ip, short port, int min, int max)
            : ObjectPool(
                  min, max,
                  [&ip, port] {
                      std::pair<bool, int> result{false, 0};
                      int the_socket = socket(AF_INET, SOCK_STREAM, 0);
                      if (the_socket == -1) {
                          log_error << "Failed to create socket";
                      } else {
                          struct sockaddr_in server_addr;
                          std::memset(&server_addr, 0, sizeof(server_addr));
                          server_addr.sin_family = AF_INET;
                          server_addr.sin_port = htons(port);
                          if (inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr) <= 0) {
                              log_error << "Invalid address or Address not supported: " << ip;
                          } else {
                              if (connect(the_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
                                  log_error << "Failed to connect to server";
                                  close(the_socket);
                              } else {
                                  result.first = true;
                                  result.second = the_socket;
                                  log_trace << "connect success";
                              }
                          }
                      }
                      return result;
                  },
                  [](int connection) {
                      log_trace << "destroying connection: " << connection;
                      if (connection > 0) {
                          shutdown(connection, SHUT_RDWR);
                      }
                      close(connection);
                  }) {}

        ~ConnectionPool() override {
            log_trace << "deleting connection pool";
        }

        std::optional<int> Reset(int connection) noexcept {
            RemoveObject(connection);
            return GetObject();
        }

        void CloseAll() noexcept {
            destroy_objects_();
        }
    };

    template <typename StreamPtr> class NetworkObjectPool : public ObjectPool<StreamPtr> {
        NetworkObjectPool(std::string const& ip, short port, int min, int max)
            : ObjectPool(
                  min, max,
                  [&ip, port] {
                      std::pair<bool, StreamPtr> result{false, nullptr};
                      // The io_context is required for all I/O
                      boost::asio::io_context ioc;
                      // These objects perform our I/O
                      boost::asio::ip::tcp::resolver resolver(ioc);
                      // Look up the domain name
                      auto const results = resolver.resolve(host, port);
                      // The SSL context is required, and holds certificates
                      boost::asio::ssl::context ctx(boost::asio::ssl::context::tlsv12_client);
                      // This holds the root certificate used for verification
                      load_root_certificates(ctx);
                      // Verify the remote server's certificate
                      // ctx.set_verify_mode(boost::asio::ssl::verify_peer);
                      ctx.set_verify_mode(boost::asio::ssl::verify_none);
                      // boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioc, ctx);
                      ssl_stream = std::make_shared<boost::beast::ssl_stream<boost::beast::tcp_stream>>(ioc, ctx);
                      //  Set SNI Hostname (many hosts need this to handshake successfully)
                      if (!SSL_set_tlsext_host_name((*ssl_stream).native_handle(), host.data())) {
                          boost::beast::error_code ec{static_cast<int>(::ERR_get_error()), boost::asio::error::get_ssl_category()};
                          // FIXME throw boost::beast::system_error{ec};
                          log_error << "SSL_set_tlsext_host_name:" << ec.message();
                          if (post_processor) {
                              post_processor("");
                          }
                          return;
                      }
                      // Make the connection on the IP address we get from a lookup
                      boost::beast::get_lowest_layer(*ssl_stream).connect(results);
                      // Perform the SSL handshake
                      ssl_stream->handshake(boost::asio::ssl::stream_base::client);
                      return result;
                      // boost::beast::tcp_stream stream(ioc);
                      plain_stream = std::make_shared<boost::beast::tcp_stream>(ioc);
                      // Make the connection on the IP address we get from a lookup
                      plain_stream->connect(results);
                  },
                  [](Stream connection) {
                      log_trace << "destroying connection: " << connection;
                      shutdown_ssl(ssl_stream);
                      boost::beast::get_lowest_layer(*ssl_stream).close();
                      ssl_stream = nullptr;
                      shutdown_plain(plain_stream);
                      plain_stream->close();
                      plain_stream = nullptr;
                  }) {}

        ~NetworkObjectPool() override {
            log_trace << "deleting connection pool";
        }

        std::optional<StreamPtr> Reset(StreamPtr connection) noexcept {
            RemoveObject(connection);
            return GetObject();
        }

        void CloseAll() noexcept {
            destroy_objects_();
        }
    };
} // namespace util
