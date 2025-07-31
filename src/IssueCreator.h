#ifndef ISSUECREATOR_H
#define ISSUECREATOR_H

#include <string>

class IssueCreator {
public:
    IssueCreator(const std::string& owner, const std::string& repo, const std::string& token);

    // Returns true if successful, false otherwise. Issue ID is stored internally.
    bool createIssue(const std::string& title, const std::string& body, const std::string& labels = "");
    
    // Returns the ID of the last created issue, or -1 if no issue was created
    int getLastIssueId() const;

private:
    std::string owner;
    std::string repo;
    std::string token;
    int lastIssueId; // Store the ID of the last created issue

    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp);
    
    // Helper method to extract issue ID from JSON response
    int extractIssueIdFromResponse(const std::string& response);
};

#endif // ISSUECREATOR_H
