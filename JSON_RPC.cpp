#include "JSON_RPC.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

namespace util::JsonRpc {
    std::string generateUUID() noexcept {
        auto uuid = boost::uuids::random_generator()();
        return boost::uuids::to_string(uuid);
    }

    namespace {
        std::string version_ = "2.0";
    }

    std::string Message::toJson() const noexcept {
        return boost::json::serialize(to());
    }

    boost::json::object Message::to() const noexcept {
        boost::json::object result;
        result["jsonrpc"] = JsonRpc::version_;
        if (id) {
            result["id"] = id;
        }
        switch (type) {
        case Type::Query:
            result["method"] = method;
            result["params"] = content;
            break;
        case Type::Result:
            result["result"] = content;
            break;
        case Type::Error:
            result["error"] = content;
            break;
        }
        return result;
    }

    Message parse(std::string const& s) noexcept {
        auto j = boost::json::parse(s);
        auto o = j.as_object();
        auto id = o.contains("id") ? int(o.at("id").is_null() ? 0 : o.at("id").as_int64()) : 0;
        auto t = Type::Result;
        std::string method = "";
        boost::json::object content;
        if (o.contains("result")) {
            t = Type::Result;
            content = o.at("result").as_object();
        } else if (o.contains("error")) {
            t = Type::Error;
            content = o.at("error").as_object();
        } else if (o.contains("method")) {
            t = Type::Query;
            method = o.at("method").as_string().c_str();
            content = o.at("params").as_object();
        }
        return {t, id, method, content};
    }
} // namespace util::JsonRpc
