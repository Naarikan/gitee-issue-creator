#include "IssueCreator.h"
#include <curl/curl.h>
#include <iostream>

IssueCreator::IssueCreator(const std::string& owner, const std::string& repo, const std::string& token)
    : owner(owner), repo(repo), token(token) {}

size_t IssueCreator::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool IssueCreator::createIssue(const std::string& title, const std::string& body, const std::string& labels) {
    std::string url = "https://gitee.com/api/v5/repos/" + owner + "/issues";

    std::string jsonStr = "{";
    jsonStr += "\"access_token\":\"" + token + "\",";
    jsonStr += "\"repo\":\"" + repo + "\",";
    jsonStr += "\"title\":\"" + title + "\"";

    if (!body.empty()) {
        jsonStr += ",\"body\":\"" + body + "\"";
    }
    if (!labels.empty()) {
        jsonStr += ",\"labels\":\"" + labels + "\"";
    }
    jsonStr += "}";

    CURL* curl = curl_easy_init();
    if (!curl) {
        std::cerr << "❌ Failed to initialize curl!" << std::endl;
        return false;
    }

    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonStr.c_str());

    std::string response_string;
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_string);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK) {
        std::cerr << "❌ Curl error: " << curl_easy_strerror(res) << std::endl;
        return false;
    }
    if (response_code == 201) {
        return true;
    } else {
        std::cerr << "❌ Failed: HTTP " << response_code << "\n" << response_string << std::endl;
        return false;
    }
}
