#include "steam_finder.h"
#include "file_collector.h"
#include "git_manager.h"
#include <iostream>
#include <fstream>

#ifdef _WIN32
#include <Windows.h>
#endif

struct Config {
    std::string github_url;
    bool use_git = true;
};

Config load_config(const fs::path& root) {
    Config cfg;
    fs::path path = root / "cs2_dumper.cfg";
    if (fs::exists(path)) {
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string k = line.substr(0, eq), v = line.substr(eq + 1);
            while (!k.empty() && k.back() == ' ') k.pop_back();
            while (!v.empty() && v.front() == ' ') v.erase(v.begin());
            if (k == "github_url") cfg.github_url = v;
            else if (k == "use_git") cfg.use_git = (v == "true" || v == "1");
        }
    }
    else {
        std::ofstream f(path);
        f << "# CS2 Dumper Config\n";
        f << "github_url=\n";
        f << "use_git=true\n";
    }
    return cfg;
}

void show_dumps(const fs::path& dir) {
    if (!fs::exists(dir)) return;
    int count = 0;
    for (auto& e : fs::directory_iterator(dir)) {
        if (!e.is_directory()) continue;
        std::string name = e.path().filename().string();
        if (name.rfind("cs2_", 0) != 0) continue;
        size_t files = 0; uintmax_t size = 0;
        for (auto& f : fs::recursive_directory_iterator(e.path()))
            if (f.is_regular_file()) { files++; size += f.file_size(); }
        std::cout << "    " << name << "  |  " << files << " files  |  " << (size / 1024 / 1024) << " MB\n";
        count++;
    }
    if (count == 0) std::cout << "    (no dumps yet)\n";
}

bool dump_already_exists(const fs::path& dumps_dir, const std::string& timestamp) {
    std::string target = "cs2_" + timestamp;
    if (!fs::exists(dumps_dir)) return false;
    for (auto& e : fs::directory_iterator(dumps_dir)) {
        if (!e.is_directory()) continue;
        if (e.path().filename().string() == target) return true;
    }
    return false;
}

int main(int argc, char* argv[]) {
#ifdef _WIN32
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
#endif

    fs::path root = fs::path(PROJECT_ROOT_DIR);
    fs::path dumps = root / "dumps";

    std::cout << "\n  CS2 DLL Dumper v1.0\n";
    std::cout << "  ===================\n\n";

    Config cfg = load_config(root);

    std::cout << "[*] Project: " << root.string() << "\n";
    std::cout << "[*] Dumps:   " << dumps.string() << "\n\n";

    std::cout << "[*] Previous dumps:\n";
    show_dumps(dumps);

    std::cout << "\n[*] Searching for CS2...\n";
    auto cs2 = SteamFinder::find_cs2();
    if (!cs2) {
        std::cerr << "[!] CS2 not found. Is it installed?\n";
        std::cout << "\nPress Enter...";
        std::cin.get();
        return 1;
    }

    std::cout << "[+] Found: " << cs2->root_path.string() << "\n";
    std::cout << "[+] Update: " << cs2->last_update << "\n\n";

    if (dump_already_exists(dumps, cs2->last_update)) {
        std::cout << "[=] Dump for this update already exists, skipping.\n";
        std::cout << "\n[*] All dumps:\n";
        show_dumps(dumps);
        std::cout << "\nPress Enter...";
        std::cin.get();
        return 0;
    }

    std::cout << "[*] Copying files...\n";
    FileCollector collector(*cs2);
    DumpResult result = collector.collect(dumps);

    std::cout << "\n[+] Done! " << result.files_copied << " files, "
        << (result.total_size / 1024 / 1024) << " MB\n";
    std::cout << "[+] Saved: " << result.dump_folder.string() << "\n";

    if (cfg.use_git && result.files_copied > 0) {
        std::cout << "\n[*] Git commit & push...\n";
        GitManager git(root);
        if (git.init()) {
            if (!cfg.github_url.empty())
                git.set_remote(cfg.github_url);
            std::string msg = "dump: " + result.timestamp + " (" + std::to_string(result.files_copied) + " files)";
            git.commit_and_push(msg);
        }
    }

    std::cout << "\n[*] All dumps:\n";
    show_dumps(dumps);

    std::cout << "\nPress Enter...";
    std::cin.get();
    return 0;
}