#include <any> // for std::any
#include <map> // for std::map<>
#include <string> // for std::string

#include <boost/json.hpp> // for boost::json::*

namespace util::JsonRpc {
    std::string generateUUID() noexcept;
    enum class Type { Query, Result, Error };

    struct Message {
        Type type;
        int id;
        std::string method;
        boost::json::object content;
        std::string toJson() const noexcept;
        boost::json::object to() const noexcept;
    };

    Message parse(std::string const& s) noexcept;
} // namespace util::JsonRpc
