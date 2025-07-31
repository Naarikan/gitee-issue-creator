#include "ConfigSetup.h"
#include "Hasher.h"
#include <sqlite3.h>
#include <iostream>
#include <sys/stat.h>
#include <filesystem>
#include <cstdlib>

ConfigSetup::ConfigSetup(const std::string& dbPath) : dbPath(dbPath), db(nullptr) {}

ConfigSetup::~ConfigSetup() {
    closeDB();
}

bool ConfigSetup::openDB() {
    // Create config directory if it doesn't exist
    std::filesystem::path configDir = std::filesystem::path(dbPath).parent_path();
    if (!std::filesystem::exists(configDir)) {
        try {
            std::filesystem::create_directories(configDir);
        } catch (const std::exception& e) {
            std::cerr << "Failed to create config directory: " << e.what() << std::endl;
            return false;
        }
    }

    if (sqlite3_open(dbPath.c_str(), (sqlite3**)&db) != SQLITE_OK) {
        std::cerr << "Failed to open DB: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return false;
    }

    const char* sql_create = R"(
        CREATE TABLE IF NOT EXISTS tokens (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            repo TEXT NOT NULL,
            owner TEXT NOT NULL,
            encrypted_token TEXT NOT NULL,
            isDefault INTEGER DEFAULT 0
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec((sqlite3*)db, sql_create, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Failed to create table: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

void ConfigSetup::closeDB() {
    if (db) {
        sqlite3_close((sqlite3*)db);
        db = nullptr;
    }
}

bool ConfigSetup::saveConfig(const std::string& repo, const std::string& owner, const std::string& token) {
    std::string key = getKey();

    Hasher hasher(key);
    std::string encrypted_token;
    try {
        encrypted_token = hasher.encrypt(token);
    } catch (const std::exception& e) {
        std::cerr << "Encryption failed: " << e.what() << std::endl;
        return false;
    }

    const char* check_sql = "SELECT EXISTS(SELECT 1 FROM tokens LIMIT 1);";
    sqlite3_stmt* check_stmt;
    int isDefault = 0;

    if (sqlite3_prepare_v2((sqlite3*)db, check_sql, -1, &check_stmt, nullptr) == SQLITE_OK) {
        if (sqlite3_step(check_stmt) == SQLITE_ROW) {
            int exists = sqlite3_column_int(check_stmt, 0);
            isDefault = (exists == 0) ? 1 : 0; 
        }
    } else {
        std::cerr << "SQL check prepare error: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return false;
    }
    sqlite3_finalize(check_stmt);

    const char* sql_insert = "INSERT INTO tokens (repo, owner, encrypted_token, isDefault) VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2((sqlite3*)db, sql_insert, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, repo.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, encrypted_token.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, isDefault); 

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "SQL step error: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}

std::vector<RepoConfig> ConfigSetup::getConfigs() {
    std::vector<RepoConfig> configs;
    const char* sql_select = "SELECT id, repo, owner, encrypted_token FROM tokens;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2((sqlite3*)db, sql_select, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return configs;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        RepoConfig cfg;
        cfg.id = sqlite3_column_int(stmt, 0);
        cfg.repo = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        cfg.owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        cfg.encrypted_token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        configs.push_back(cfg);
    }

    sqlite3_finalize(stmt);
    return configs;
}

std::string ConfigSetup::getDecryptedDefaultToken() {
    if (!db) {
        std::cerr << "DB not opened!" << std::endl;
        return "";
    }

    const char* sql = "SELECT encrypted_token FROM tokens WHERE isDefault = 1 LIMIT 1;";
    sqlite3_stmt* stmt;
    std::string decryptedToken;

    if (sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return "";
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* encToken = sqlite3_column_text(stmt, 0);
        if (encToken) {
            std::string encryptedTokenStr = reinterpret_cast<const char*>(encToken);
            Hasher hasher(getKey());
            try {
                decryptedToken = hasher.decrypt(encryptedTokenStr);
            } catch (const std::exception& e) {
                std::cerr << "Decryption failed: " << e.what() << std::endl;
            }
        }
    }

    sqlite3_finalize(stmt);
    return decryptedToken;
}

bool ConfigSetup::setDefaultRepo(const std::string& repoName) {
    sqlite3_stmt* stmt1;
    sqlite3_stmt* stmt2;

    
    const char* resetSQL = "UPDATE tokens SET isDefault = 0;";
    if (sqlite3_prepare_v2((sqlite3*)db, resetSQL, -1, &stmt1, nullptr) != SQLITE_OK) {
        return false;
    }

    if (sqlite3_step(stmt1) != SQLITE_DONE) {
        sqlite3_finalize(stmt1);
        return false;
    }
    sqlite3_finalize(stmt1);

    
    const char* setSQL = "UPDATE tokens SET isDefault = 1 WHERE repo = ?;";
    if (sqlite3_prepare_v2((sqlite3*)db, setSQL, -1, &stmt2, nullptr) != SQLITE_OK) {
        return false;
    }

    sqlite3_bind_text(stmt2, 1, repoName.c_str(), -1, SQLITE_STATIC);

    bool success = sqlite3_step(stmt2) == SQLITE_DONE;
    sqlite3_finalize(stmt2);
    return success;
}

bool ConfigSetup::getDefaultRepoConfig(RepoConfig& outConfig) {
    if (!db) return false;

    const char* sql = "SELECT id, repo, owner, encrypted_token FROM tokens WHERE isDefault = 1 LIMIT 1;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return false;
    }

    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        outConfig.id = sqlite3_column_int(stmt, 0);
        outConfig.repo = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        outConfig.owner = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        outConfig.encrypted_token = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

bool ConfigSetup::deleteRepo(const std::string& owner, const std::string& repo) {
    if (!db) return false;

    const char* sql = "DELETE FROM tokens WHERE owner = ? AND repo = ?;";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2((sqlite3*)db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "SQL prepare error: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, owner.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, repo.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::cerr << "SQL step error: " << sqlite3_errmsg((sqlite3*)db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }

    sqlite3_finalize(stmt);
    return true;
}



std::string ConfigSetup::getKey() {
    return std::string{
        "\x12\x34\x56\x78\x9A\xBC\xDE\xF0"
        "\x11\x22\x33\x44\x55\x66\x77\x88"
        "\x99\xAA\xBB\xCC\xDD\xEE\xFF\x00"
        "\x10\x20\x30\x40\x50\x60\x70\x80", 32};
}
