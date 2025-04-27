#pragma once

/**
 * SolidDB - A Simple Relational Database in C++
 */

// Core components
#include "core/Database.h"
#include "core/Table.h"

// Parser components
#include "parser/CommandParser.h"

// Utility components
#include "util/StringUtils.h"

namespace soliddb {

/**
 * SolidDB version information
 */
constexpr const char* VERSION = "0.1.0";

} // namespace soliddb 