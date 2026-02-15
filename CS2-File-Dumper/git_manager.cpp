#include "git_manager.h"

#include <iostream>
#include <fstream>
#include <cstdlib>

GitManager::GitManager(const fs::path& repo_path)
    : m_repo_path(repo_path) {
}

bool GitManager::is_git_available() {
    int ret = std::system("git --version >nul 2>&1");
    return ret == 0;
}

int GitManager::run_git(const std::string& command) {
    std::string full_cmd = "cd /d \"" + m_repo_path.string() + "\" && git " + command;
    std::cout << "  [git] " << command << "\n";
    return std::system(full_cmd.c_str());
}

bool GitManager::init() {
    if (!is_git_available()) {
        std::cerr << "[!] Git is not installed! Skipping git operations.\n";
        return false;
    }

    // Если .git уже есть — ничего не делаем
    if (fs::exists(m_repo_path / ".git")) {
        std::cout << "[*] Git repo already exists\n";
        return true;
    }

    std::cout << "[*] Initializing git repository...\n";

    if (run_git("init") != 0) {
        std::cerr << "[!] git init failed\n";
        return false;
    }

    run_git("config user.name \"CS2 Dumper\"");
    run_git("config user.email \"dumper@local\"");
    run_git("branch -M main");

    return true;
}

bool GitManager::set_remote(const std::string& url) {
    if (url.empty()) return false;
    run_git("remote remove origin 2>nul");
    std::string cmd = "remote add origin " + url;
    return run_git(cmd) == 0;
}

void GitManager::setup_gitignore() {
    // Не нужна — .gitignore уже лежит в корне проекта
}

bool GitManager::commit_and_push(const std::string& message) {
    std::cout << "\n[*] Git: committing changes...\n";

    // git add .
    if (run_git("add .") != 0) {
        std::cerr << "[!] git add failed\n";
        return false;
    }

    // git commit
    std::string commit_cmd = "commit -m \"" + message + "\"";
    if (run_git(commit_cmd) != 0) {
        std::cerr << "[!] git commit failed (nothing to commit?)\n";
        return false;
    }

    // git push
    std::cout << "[*] Pushing...\n";
    if (run_git("push origin main") == 0) {
        std::cout << "[+] Push successful!\n";
        return true;
    }

    // Может первый пуш — нужен -u
    if (run_git("push -u origin main") == 0) {
        std::cout << "[+] Push successful!\n";
        return true;
    }

    std::cout << "[!] Push failed. Check your remote.\n";
    std::cout << "    Commit saved locally!\n";
    return false;
}