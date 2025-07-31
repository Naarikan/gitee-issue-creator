#pragma once
#include <string>
#include <vector>


struct RepoConfig {
    int id;
    std::string repo;
    std::string owner;
    std::string encrypted_token;
};

class ConfigSetup {
public:
    ConfigSetup(const std::string& dbPath);
    ~ConfigSetup();

    bool openDB();
    void closeDB();

    bool saveConfig(const std::string& repo, const std::string& owner, const std::string& token);

    bool setDefaultRepo(const std::string& repoName);

    bool deleteRepo(const std::string& owner, const std::string& repo);

    bool getDefaultRepoConfig(RepoConfig& outConfig);
    std::vector<RepoConfig> getConfigs();
    
    std::string getDecryptedDefaultToken();

    
    static std::string getKey();
     

private:
    std::string dbPath;
    void* db;
};
