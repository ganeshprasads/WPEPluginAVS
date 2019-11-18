/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
#pragma once

#include <mutex>
#include <string>

#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LogStringFormatter.h>

#include <WPEFramework/core/core.h>

namespace alexaClientSDK {
namespace sampleApp {

/**
 * A trace AVSRedirected category for Thunder tracing
 */
class AVSRedirected {
public:
    AVSRedirected() = delete;
    AVSRedirected(const AVSRedirected& a_Copy) = delete;
    AVSRedirected& operator=(const AVSRedirected& a_RHS) = delete;

    explicit AVSRedirected(const string& text);
    ~AVSRedirected() = default;

    inline const char* Data() const;
    inline uint16_t Length() const;

private:
    std::string _text;
};

/**
 * A simple class that logs/traces using the Thunder framework.
 */
class ThunderLogger : public avsCommon::utils::logger::Logger {
public:

    ThunderLogger(const ThunderLogger& a_Copy) = delete;
    ThunderLogger& operator=(const ThunderLogger& a_RHS) = delete;

    /**
     * Return the one and only @c ThunderLogger instance.
     *
     * @return The one and only @c ThunderLogger instance.
     */
    static std::shared_ptr<avsCommon::utils::logger::Logger> instance();

    /**
     * Trace a messag through Thunder.
     *
     * @param stringToPrint The string to print.
     */
    static void trace(const std::string& stringToPrint);

    /**
     * Trace a message through Thunder with additional formatting.
     *
     * @param stringToPrint The string to print.
     */
    static void prettyTrace(const std::string& stringToPrint);

    /**
     * Trace a multilie message through Thunder with additional formatting.
     *
     * @param stringToPrint The string to print.
     */
    static void prettyTrace(std::initializer_list<std::string> lines);

    void emit(
        avsCommon::utils::logger::Level level,
        std::chrono::system_clock::time_point time,
        const char* threadMoniker,
        const char* text) override;

private:
    /**
     * Constructor.
     */
    ThunderLogger();

    /**
     * Object used to format strings for log messages.
     */
    avsCommon::utils::logger::LogStringFormatter m_logFormatter;
};

/**
 * Return the singleton instance of @c ThunderLogger.
 *
 * @return The singleton instance of @c ThunderLogger.
 */
std::shared_ptr<avsCommon::utils::logger::Logger> getThunderLogger();

}  // namespace sampleApp
}  // namespace alexaClientSDK
