#pragma once

#include <string> // for std::string

#include <boost/log/trivial.hpp>

#define log_trace BOOST_LOG_TRIVIAL(trace)
#define log_debug BOOST_LOG_TRIVIAL(debug)
#define log_info BOOST_LOG_TRIVIAL(info)
#define log_warning BOOST_LOG_TRIVIAL(warning)
#define log_error BOOST_LOG_TRIVIAL(error)
#define log_fatal BOOST_LOG_TRIVIAL(fatal)

#define log_trace_with_location auto c_s_l = std::source_location::current(); BOOST_LOG_TRIVIAL(trace) << "file: " << c_s_l.file_name() << ", function: " << c_s_l.function_name() << ", line: " << c_s_l.line() << ", column: " << c_s_l.column() << ", "
#define log_debug_with_location auto c_s_l = std::source_location::current(); BOOST_LOG_TRIVIAL(debug) << "file: " << c_s_l.file_name() << ", function: " << c_s_l.function_name() << ", line: " << c_s_l.line() << ", column: " << c_s_l.column() << ", "
#define log_info_with_location auto c_s_l = std::source_location::current(); BOOST_LOG_TRIVIAL(info) << "file: " << c_s_l.file_name() << ", function: " << c_s_l.function_name() << ", line: " << c_s_l.line() << ", column: " << c_s_l.column() << ", "
#define log_warning_with_location auto c_s_l = std::source_location::current(); BOOST_LOG_TRIVIAL(warning) << "file: " << c_s_l.file_name() << ", function: " << c_s_l.function_name() << ", line: " << c_s_l.line() << ", column: " << c_s_l.column() << ", "
#define log_error_with_location auto c_s_l = std::source_location::current(); BOOST_LOG_TRIVIAL(error) << "file: " << c_s_l.file_name() << ", function: " << c_s_l.function_name() << ", line: " << c_s_l.line() << ", column: " << c_s_l.column() << ", "
#define log_fatal_with_location auto c_s_l = std::source_location::current(); BOOST_LOG_TRIVIAL(fatal) << "file: " << c_s_l.file_name() << ", function: " << c_s_l.function_name() << ", line: " << c_s_l.line() << ", column: " << c_s_l.column() << ", "

namespace util::Log {
    void init(std::string const& folder, std::string const& filename_prefix, boost::log::trivial::severity_level level) noexcept;
    void start_clean_routine(int days) noexcept;
    void stop_clean_routine() noexcept;
} // namespace util::Log
