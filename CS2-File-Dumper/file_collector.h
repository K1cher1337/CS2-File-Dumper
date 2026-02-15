#pragma once

#include "steam_finder.h"
#include <vector>
#include <string>

struct CollectedFile {
    fs::path source;
    fs::path relative_path;
    std::uintmax_t size;
};

struct DumpResult {
    fs::path dump_folder;
    size_t files_copied;
    size_t total_size;
    std::string timestamp;
};

class FileCollector {
public:
    explicit FileCollector(const CS2Installation& install);

    std::vector<CollectedFile> scan_files();
    DumpResult collect(const fs::path& output_root);

private:
    CS2Installation m_install;

    std::vector<fs::path> get_scan_directories();
    bool is_target_file(const fs::path& file);
};