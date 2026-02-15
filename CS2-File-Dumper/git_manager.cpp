#include "git_manager.h"
#include <iostream>
#include <cstdlib>

GitManager::GitManager(const fs::path& repo_path) : m_repo_path(repo_path) {}

bool GitManager::is_git_available() {
    return std::system("git --version >nul 2>&1") == 0;
}

int GitManager::run_git(const std::string& command) {
    std::string cmd = "cd /d \"" + m_repo_path.string() + "\" && git " + command;
    return std::system(cmd.c_str());
}

bool GitManager::init() {
    if (!is_git_available()) {
        std::cerr << "[!] Git not installed\n";
        return false;
    }
    if (fs::exists(m_repo_path / ".git")) return true;
    run_git("init");
    run_git("branch -M master");
    return true;
}

bool GitManager::set_remote(const std::string& url) {
    if (url.empty()) return false;
    run_git("remote remove origin 2>nul");
    return run_git("remote add origin " + url) == 0;
}

void GitManager::setup_gitignore() {}

bool GitManager::commit_and_push(const std::string& message) {
    run_git("add .");
    std::string commit = "commit -m \"" + message + "\"";
    if (run_git(commit) != 0) {
        std::cout << "[!] Nothing to commit\n";
        return false;
    }
    if (run_git("push origin master") == 0) return true;
    if (run_git("push -u origin master") == 0) return true;
    std::cout << "[!] Push failed, commit saved locally\n";
    return false;
}