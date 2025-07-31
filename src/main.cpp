#include <iostream>
#include <string>
#include <limits>
#include <cstdlib>
#include "cxxopts.hpp"
#include "ConfigSetup.h"
#include "Hasher.h"
#include "IssueCreator.h"

void createIssueInteractive(const RepoConfig& config, ConfigSetup& configSetup) {
    std::string saltContent = ConfigSetup::getKey();
    Hasher hasher(saltContent);
    std::string token = hasher.decrypt(config.encrypted_token);
    IssueCreator issueCreator(config.owner, config.repo, token);

    std::string title, body, labels;

    std::cout << "Enter issue title: ";
    std::getline(std::cin, title);

    std::cout << "Enter issue body (optional): ";
    std::getline(std::cin, body);

    std::cout << "Enter labels (optional, comma-separated): ";
    std::getline(std::cin, labels);

    if (issueCreator.createIssue(title, body, labels)) {
        int issueId = issueCreator.getLastIssueId();
        if (issueId != -1) {
            std::cout << "✅ Issue created successfully! Issue ID: #" << issueId << std::endl;
        } else {
            std::cout << "✅ Issue created successfully! (Issue ID could not be extracted)" << std::endl;
        }
    } else {
        std::cerr << "❌ Failed to create issue." << std::endl;
    }
}

void addRepo(ConfigSetup& configSetup) {
    std::string owner, repo, token;

    std::cout << "Enter repository owner: ";
    std::getline(std::cin, owner);

    std::cout << "Enter repository name: ";
    std::getline(std::cin, repo);

    std::string defaultToken = configSetup.getDecryptedDefaultToken();

    if (!defaultToken.empty()) {
        std::cout << "Default token found. Use it for this repo? (y/n): ";
        char choice;
        std::cin >> choice;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (choice == 'y' || choice == 'Y') {
            token = defaultToken;
        } else {
            std::cout << "Enter access token: ";
            std::getline(std::cin, token);
        }
    } else {
        std::cout << "Enter access token: ";
        std::getline(std::cin, token);
    }

    if (configSetup.saveConfig(repo, owner, token)) {
        std::cout << "✅ Repository added successfully!" << std::endl;
    } else {
        std::cerr << "❌ Failed to add repository." << std::endl;
    }
}

void deleteRepo(ConfigSetup& configSetup) {
    auto configs = configSetup.getConfigs();
    if (configs.empty()) {
        std::cerr << "❗ No repositories found to delete." << std::endl;
        return;
    }

    std::cout << "Available repositories:\n";
    for (size_t i = 0; i < configs.size(); ++i) {
        std::cout << (i + 1) << ") " << configs[i].owner << "/" << configs[i].repo << std::endl;
    }

    std::cout << "Select repository to delete (enter number): ";
    int choice;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice < 1 || choice > (int)configs.size()) {
        std::cerr << "❌ Invalid selection." << std::endl;
        return;
    }

    const RepoConfig& repoToDelete = configs[choice - 1];
    if (configSetup.deleteRepo(repoToDelete.owner, repoToDelete.repo)) {
        std::cout << "✅ Repository deleted successfully: " << repoToDelete.owner << "/" << repoToDelete.repo << std::endl;
    } else {
        std::cerr << "❌ Failed to delete repository." << std::endl;
    }
}

void createIssueWithArgs(const std::string& owner, const std::string& repo,
                         const std::string& title, const std::string& body,
                         const std::string& token, const std::string& labels = "") {
    IssueCreator creator(owner, repo, token);
    if (creator.createIssue(title, body, labels)) {
        int issueId = creator.getLastIssueId();
        if (issueId != -1) {
            std::cout << "✅ Issue created successfully! Issue ID: #" << issueId << std::endl;
        } else {
            std::cout << "✅ Issue created successfully! (Issue ID could not be extracted)" << std::endl;
        }
    } else {
        std::cerr << "❌ Failed to create issue." << std::endl;
    }
}

void setupDefaultRepo(ConfigSetup& configSetup) {
    auto configs = configSetup.getConfigs();
    if (configs.empty()) {
        std::cerr << "❗ No repositories found. Please add one first." << std::endl;
        return;
    }

    std::cout << "Available repositories:\n";
    for (size_t i = 0; i < configs.size(); ++i) {
        std::cout << i + 1 << ") " << configs[i].owner << "/" << configs[i].repo << std::endl;
    }

    std::cout << "Select repository to set as default (enter number): ";
    int choice;
    std::cin >> choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice < 1 || choice > (int)configs.size()) {
        std::cerr << "❌ Invalid selection." << std::endl;
        return;
    }

    const std::string& selectedRepo = configs[choice - 1].repo;

    if (configSetup.setDefaultRepo(selectedRepo)) {
        std::cout << "✅ Default repository set successfully: " << selectedRepo << std::endl;
    } else {
        std::cerr << "❌ Failed to set default repository." << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // Get home directory and create config path
    const char* homeDir = std::getenv("HOME");
    if (!homeDir) {
        std::cerr << "❌ HOME environment variable not set!" << std::endl;
        return 1;
    }
    
    std::string configPath = std::string(homeDir) + "/.gitee-issue/config.db";
    ConfigSetup configSetup(configPath);
    if (!configSetup.openDB()) {
        std::cerr << "❌ Failed to open database!" << std::endl;
        return 1;
    }

    try {
        cxxopts::Options options("gitee-issue", "Gitee Issue Manager CLI");
        options.add_options()
            ("m,menu", "Start in interactive menu mode")
            ("a,add", "Add a new repository")
            ("c,create", "Create an issue with arguments")
            ("s,setup", "Set default repository")
            ("d,delete", "Delete a repository")
            ("owner", "Repository owner (optional; if omitted, default repo owner will be used)", cxxopts::value<std::string>())
            ("repo", "Repository name (optional; if omitted, default repo will be used)", cxxopts::value<std::string>())
            ("title", "Issue title (required)", cxxopts::value<std::string>())
            ("body", "Issue body", cxxopts::value<std::string>()->default_value(""))
            ("labels", "Comma-separated labels", cxxopts::value<std::string>()->default_value(""))
            ("token", "Gitee access token (optional; if omitted, default token will be used if set)", cxxopts::value<std::string>())
            ("h,help", "Print help");

        auto result = options.parse(argc, argv);

        if (result.count("help")) {
            std::cout << options.help() << std::endl;
            return 0;
        }

        if (result.count("menu")) {
            std::cout << "=== MENU ===\n";
            std::cout << "1) Create an issue\n";
            std::cout << "2) Add new repository\n";
            std::cout << "3) Delete repository\n";
            std::cout << "Select option: ";

            std::string input;
            std::getline(std::cin, input);
            int choice = std::stoi(input);

            if (choice == 2) {
                addRepo(configSetup);
            } else if (choice == 3) {
                deleteRepo(configSetup);
            } else {
                auto configs = configSetup.getConfigs();
                if (configs.empty()) {
                    std::cerr << "❗ No repositories found. Please add one first." << std::endl;
                    configSetup.closeDB();
                    return 1;
                }

                std::cout << "Available repositories:\n";
                for (size_t i = 0; i < configs.size(); ++i) {
                    std::cout << (i + 1) << ") " << configs[i].owner << "/" << configs[i].repo << std::endl;
                }

                std::cout << "Select repository (enter number): ";
                std::getline(std::cin, input);
                int repoChoice = std::stoi(input);

                if (repoChoice < 1 || repoChoice > (int)configs.size()) {
                    std::cerr << "❌ Invalid selection." << std::endl;
                    configSetup.closeDB();
                    return 1;
                }

                createIssueInteractive(configs[repoChoice - 1], configSetup);
            }

        } else if (result.count("add")) {
            addRepo(configSetup);

        } else if (result.count("setup")) {
            setupDefaultRepo(configSetup);

        } else if (result.count("delete")) {
            deleteRepo(configSetup);

        } else if (result.count("create")) {
            std::string owner, repo, title, body, labels, token;

            if (!result.count("title")) {
                std::cerr << "❗ Missing required option: --title" << std::endl;
                configSetup.closeDB();
                return 1;
            }
            title = result["title"].as<std::string>();
            body = result.count("body") ? result["body"].as<std::string>() : "";
            labels = result.count("labels") ? result["labels"].as<std::string>() : "";

            owner = result.count("owner") ? result["owner"].as<std::string>() : "";
            repo = result.count("repo") ? result["repo"].as<std::string>() : "";
            token = result.count("token") ? result["token"].as<std::string>() : "";

            if (owner.empty() || repo.empty() || token.empty()) {
                RepoConfig defConfig;
                if (!configSetup.getDefaultRepoConfig(defConfig)) {
                    std::cerr << "❌ No default repository config found, and some required fields are missing." << std::endl;
                    configSetup.closeDB();
                    return 1;
                }

                if (owner.empty()) owner = defConfig.owner;
                if (repo.empty()) repo = defConfig.repo;

                if (token.empty()) {
                    Hasher hasher(configSetup.getKey());
                    try {
                        token = hasher.decrypt(defConfig.encrypted_token);
                    } catch (const std::exception& e) {
                        std::cerr << "❌ Failed to decrypt token: " << e.what() << std::endl;
                        configSetup.closeDB();
                        return 1;
                    }
                }
            }

            createIssueWithArgs(owner, repo, title, body, token, labels);
        } else {
            std::cout << options.help() << std::endl;
        }

        configSetup.closeDB();

    } catch (const std::exception& e) {
        std::cerr << "Error parsing options: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
