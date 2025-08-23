#pragma once

#include <string>
#include <string_view>

namespace Paths
{
    inline constexpr std::string_view ROOT_DIR = PROJECT_ROOT_DIR;
    inline const std::string ASSET_DIR = std::string(ROOT_DIR) + "/assets/";
}