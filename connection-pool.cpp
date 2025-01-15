#include "connection-pool.h"

// #include <atomic>
// #include <functional>
// #include <iostream>
// #include <memory>
// #include <thread>

void connection_pool_test() {
    util::ConnectionPool pool("127.0.0.1", 2222, 10, 100);
    auto sockfd = pool.GetObject(); // get the socket, value wrap by std::optional
    if (sockfd.has_value()) {
        log_debug << "get connection from pool";
        pool.RecycleObject(sockfd.value()); // release the socket
        log_debug << "recycled connection to pool";
        // Do something with the socket
    }
}
