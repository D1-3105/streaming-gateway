//
// Created by oleg on 18.04.24.
//

#ifndef STREAMING_GATEWAY_CROW_LOGGING_H
#define STREAMING_GATEWAY_CROW_LOGGING_H

#include <iostream>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/utility/setup/console.hpp>

namespace logging
{
    void init_logging(boost::log::trivial::severity_level level = boost::log::trivial::info);
}


#endif //STREAMING_GATEWAY_CROW_LOGGING_H
