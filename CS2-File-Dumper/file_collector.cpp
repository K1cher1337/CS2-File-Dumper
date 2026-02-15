#include "file_collector.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <algorithm>

FileCollector::FileCollector(const CS2Installation& install) : m_install(install) {}

std::vector<fs::path> FileCollector::get_scan_directories() {
    fs::path g = m_install.game_path;
    std::vector<fs::path> dirs = {
        g / "bin" / "win64",
        g / "csgo" / "bin" / "win64",
        g / "core" / "bin" / "win64",
        g / "platform" / "bin" / "win64",
        g / "sdktools" / "bin" / "win64",
    };
    std::vector<fs::path> result;
    for (auto& d : dirs)
        if (fs::exists(d) && fs::is_directory(d))
            result.push_back(d);
    return result;
}

bool FileCollector::is_target_file(const fs::path& file) {
    if (!fs::is_regular_file(file)) return false;
    std::string ext = file.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return ext == ".dll" || ext == ".exe";
}

std::vector<CollectedFile> FileCollector::scan_files() {
    std::vector<CollectedFile> files;
    for (auto& dir : get_scan_directories()) {
        try {
            for (auto& entry : fs::directory_iterator(dir)) {
                if (is_target_file(entry.path())) {
                    CollectedFile cf;
                    cf.source = entry.path();
                    cf.relative_path = fs::relative(entry.path(), m_install.root_path);
                    cf.size = fs::file_size(entry.path());
                    files.push_back(cf);
                }
            }
        }
        catch (...) {}
    }
    std::sort(files.begin(), files.end(),
        [](const CollectedFile& a, const CollectedFile& b) {
            return a.relative_path < b.relative_path;
        });
    return files;
}

DumpResult FileCollector::collect(const fs::path& output_root) {
    DumpResult result;
    result.timestamp = m_install.last_update;
    result.files_copied = 0;
    result.total_size = 0;
    result.dump_folder = output_root / ("cs2_" + result.timestamp);

    fs::create_directories(result.dump_folder);
    auto files = scan_files();

    for (auto& cf : files) {
        fs::path dest = result.dump_folder / cf.relative_path;
        fs::create_directories(dest.parent_path());
        try {
            fs::copy_file(cf.source, dest, fs::copy_options::overwrite_existing);
            result.files_copied++;
            result.total_size += cf.size;
            double mb = static_cast<double>(cf.size) / (1024.0 * 1024.0);
            std::cout << "  [+] " << cf.relative_path.string()
                << " (" << std::fixed << std::setprecision(2) << mb << " MB)\n";
        }
        catch (const std::exception& e) {
            std::cerr << "  [!] " << cf.relative_path.string() << ": " << e.what() << "\n";
        }
    }

    std::ofstream info(result.dump_folder / "dump_info.txt");
    info << "CS2 DLL Dump | " << result.timestamp << "\n";
    info << "Path: " << m_install.root_path.string() << "\n";
    info << "Files: " << result.files_copied << "\n";
    info << "Size: " << (result.total_size / 1024 / 1024) << " MB\n\n";
    for (auto& cf : files)
        info << cf.relative_path.string() << " | " << cf.size << " bytes\n";

    return result;
}