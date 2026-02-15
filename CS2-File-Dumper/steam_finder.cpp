#include "steam_finder.h"

#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>
#include <chrono>
#include <iomanip>
#include <ctime>

#ifdef _WIN32
#include <Windows.h>
#endif

std::optional<fs::path> SteamFinder::find_steam_path() {
#ifdef _WIN32
    HKEY hKey;

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\WOW6432Node\\Valve\\Steam", 0,
        KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "InstallPath", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            fs::path p(buffer);
            if (fs::exists(p)) return p;
        }
        RegCloseKey(hKey);
    }

    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE,
        "SOFTWARE\\Valve\\Steam", 0,
        KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "InstallPath", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            fs::path p(buffer);
            if (fs::exists(p)) return p;
        }
        RegCloseKey(hKey);
    }

    if (RegOpenKeyExA(HKEY_CURRENT_USER,
        "SOFTWARE\\Valve\\Steam", 0,
        KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[MAX_PATH];
        DWORD bufferSize = sizeof(buffer);
        if (RegQueryValueExA(hKey, "SteamPath", nullptr, nullptr,
            reinterpret_cast<LPBYTE>(buffer), &bufferSize) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            fs::path p(buffer);
            if (fs::exists(p)) return p;
        }
        RegCloseKey(hKey);
    }
#endif

    std::vector<std::string> common_paths = {
        "C:\\Program Files (x86)\\Steam",
        "C:\\Program Files\\Steam",
        "D:\\Steam",
        "D:\\SteamLibrary",
        "E:\\Steam",
        "E:\\SteamLibrary",
        "F:\\Steam",
        "F:\\SteamLibrary"
    };

    for (auto& p : common_paths) {
        if (fs::exists(p)) return fs::path(p);
    }

    return std::nullopt;
}

std::vector<fs::path> SteamFinder::find_steam_libraries(const fs::path& steam_path) {
    std::vector<fs::path> libraries;
    libraries.push_back(steam_path);

    fs::path vdf_path = steam_path / "steamapps" / "libraryfolders.vdf";
    if (!fs::exists(vdf_path)) {
        vdf_path = steam_path / "config" / "libraryfolders.vdf";
    }

    if (!fs::exists(vdf_path)) {
        std::cerr << "[!] libraryfolders.vdf not found" << std::endl;
        return libraries;
    }

    std::ifstream file(vdf_path);
    std::string content((std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    // Ищем "path"  "какой-то путь"
    std::string search_key = "\"path\"";
    size_t pos = 0;
    while ((pos = content.find(search_key, pos)) != std::string::npos) {
        pos += search_key.length();

        // Ищем первую кавычку значения
        size_t quote1 = content.find('"', pos);
        if (quote1 == std::string::npos) break;

        // Ищем закрывающую кавычку
        size_t quote2 = content.find('"', quote1 + 1);
        if (quote2 == std::string::npos) break;

        std::string path_str = content.substr(quote1 + 1, quote2 - quote1 - 1);

        // Убираем двойные бэкслеши
        std::string clean_path;
        for (size_t i = 0; i < path_str.size(); ++i) {
            if (path_str[i] == '\\' && i + 1 < path_str.size() && path_str[i + 1] == '\\') {
                clean_path += '\\';
                ++i;
            }
            else {
                clean_path += path_str[i];
            }
        }

        fs::path lib_path(clean_path);
        if (fs::exists(lib_path)) {
            bool already = false;
            for (auto& l : libraries) {
                try {
                    if (fs::equivalent(l, lib_path)) { already = true; break; }
                }
                catch (...) {}
            }
            if (!already) libraries.push_back(lib_path);
        }

        pos = quote2 + 1;
    }

    std::cout << "[*] Steam libraries found: " << libraries.size() << std::endl;
    for (auto& l : libraries) {
        std::cout << "    " << l.string() << std::endl;
    }

    return libraries;
}

std::string SteamFinder::get_last_update_time(const fs::path& library_path) {
    fs::path manifest = library_path / "steamapps" / "appmanifest_730.acf";

    if (!fs::exists(manifest)) return "";

    std::ifstream file(manifest);
    std::string line;
    std::string last_updated;

    while (std::getline(file, line)) {
        // Ищем LastUpdated или lastupdated
        if (line.find("LastUpdated") != std::string::npos ||
            line.find("lastupdated") != std::string::npos) {

            // Вытаскиваем число из строки вида:  "LastUpdated"  "1234567890"
            size_t first_digit = std::string::npos;
            size_t last_digit = std::string::npos;

            for (size_t i = 0; i < line.size(); ++i) {
                if (line[i] >= '0' && line[i] <= '9') {
                    if (first_digit == std::string::npos) first_digit = i;
                    last_digit = i;
                }
            }

            // Нам нужно число после ключа, поэтому ищем второе число
            // Сначала пропускаем ключ
            size_t key_end = line.find("\"", line.find("pdated") != std::string::npos ?
                line.find("pdated") : 0);
            if (key_end == std::string::npos) continue;

            // Ищем число после ключа
            std::string after_key = line.substr(key_end);
            std::string digits;
            for (char c : after_key) {
                if (c >= '0' && c <= '9') {
                    digits += c;
                }
                else if (!digits.empty()) {
                    break;
                }
            }

            if (!digits.empty()) {
                last_updated = digits;
            }
        }
    }

    if (!last_updated.empty()) {
        try {
            time_t timestamp = static_cast<time_t>(std::stoll(last_updated));
            std::tm tm_buf;
            localtime_s(&tm_buf, &timestamp);
            std::ostringstream oss;
            oss << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S");
            return oss.str();
        }
        catch (...) {}
    }

    return "";
}

std::optional<CS2Installation> SteamFinder::check_cs2_in_library(const fs::path& library_path) {
    fs::path cs2_root = library_path / "steamapps" / "common" / "Counter-Strike Global Offensive";

    if (!fs::exists(cs2_root)) return std::nullopt;

    fs::path game_path = cs2_root / "game";
    fs::path exe_path = game_path / "bin" / "win64" / "cs2.exe";

    if (!fs::exists(exe_path)) {
        exe_path = cs2_root / "cs2.exe";
        if (!fs::exists(exe_path)) {
            std::cerr << "[!] CS2 folder found but cs2.exe missing: "
                << cs2_root.string() << std::endl;
            return std::nullopt;
        }
    }

    CS2Installation install;
    install.root_path = cs2_root;
    install.game_path = game_path;
    install.exe_path = exe_path;
    install.last_update = get_last_update_time(library_path);

    if (install.last_update.empty()) {
        auto ftime = fs::last_write_time(exe_path);
        auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - fs::file_time_type::clock::now() + std::chrono::system_clock::now());
        time_t tt = std::chrono::system_clock::to_time_t(sctp);
        std::tm tm_buf;
        localtime_s(&tm_buf, &tt);
        std::ostringstream oss;
        oss << std::put_time(&tm_buf, "%Y-%m-%d_%H-%M-%S");
        install.last_update = oss.str();
    }

    return install;
}

std::optional<CS2Installation> SteamFinder::find_cs2() {
    std::cout << "========================================" << std::endl;
    std::cout << "[*] Searching for CS2..." << std::endl;
    std::cout << "========================================" << std::endl;

    auto steam_path = find_steam_path();
    if (steam_path) {
        std::cout << "[+] Steam found: " << steam_path->string() << std::endl;
        auto libraries = find_steam_libraries(*steam_path);

        for (auto& lib : libraries) {
            auto result = check_cs2_in_library(lib);
            if (result) {
                std::cout << "[+] CS2 found: " << result->root_path.string() << std::endl;
                std::cout << "[+] Last update: " << result->last_update << std::endl;
                return result;
            }
        }
    }

    std::cout << "[!] Registry search failed, brute-forcing all drives..." << std::endl;
    return brute_force_search();
}

std::optional<CS2Installation> SteamFinder::brute_force_search() {
#ifdef _WIN32
    for (char drive = 'A'; drive <= 'Z'; ++drive) {
        std::string root = std::string(1, drive) + ":\\";
        if (GetDriveTypeA(root.c_str()) != DRIVE_FIXED) continue;

        std::cout << "[*] Scanning drive " << root << "..." << std::endl;

        std::vector<fs::path> candidates = {
            fs::path(root) / "Steam",
            fs::path(root) / "SteamLibrary",
            fs::path(root) / "Program Files (x86)" / "Steam",
            fs::path(root) / "Program Files" / "Steam",
            fs::path(root) / "Games" / "Steam",
        };

        for (auto& candidate : candidates) {
            if (fs::exists(candidate)) {
                auto result = check_cs2_in_library(candidate);
                if (result) {
                    std::cout << "[+] CS2 found (brute): "
                        << result->root_path.string() << std::endl;
                    return result;
                }
            }
        }
    }
#endif
    return std::nullopt;
}