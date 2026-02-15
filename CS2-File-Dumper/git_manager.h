#pragma once

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class GitManager {
public:
    explicit GitManager(const fs::path& repo_path);

    bool init();
    bool set_remote(const std::string& url);
    bool commit_and_push(const std::string& message);
    static bool is_git_available();
    void setup_gitignore();

private:
    fs::path m_repo_path;
    int run_git(const std::string& command);
};