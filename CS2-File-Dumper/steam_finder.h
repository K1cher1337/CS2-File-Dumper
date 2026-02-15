#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

struct CS2Installation {
    fs::path root_path;
    fs::path game_path;
    fs::path exe_path;
    std::string last_update;
};

class SteamFinder {
public:
    static std::optional<fs::path> find_steam_path();
    static std::vector<fs::path> find_steam_libraries(const fs::path& steam_path);
    static std::optional<CS2Installation> find_cs2();
    static std::optional<CS2Installation> brute_force_search();

private:
    static std::string get_last_update_time(const fs::path& library_path);
    static std::optional<CS2Installation> check_cs2_in_library(const fs::path& library_path);
};

