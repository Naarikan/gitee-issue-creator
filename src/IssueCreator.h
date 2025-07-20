#ifndef ISSUECREATOR_H
#define ISSUECREATOR_H

#include <string>

class IssueCreator {
public:
    IssueCreator(const std::string& owner, const std::string& repo, const std::string& token);

    bool createIssue(const std::string& title, const std::string& body, const std::string& labels = "");

private:
    std::string owner;
    std::string repo;
    std::string token;

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
};

#endif // ISSUECREATOR_H
