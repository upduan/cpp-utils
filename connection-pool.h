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
} // namespace util
