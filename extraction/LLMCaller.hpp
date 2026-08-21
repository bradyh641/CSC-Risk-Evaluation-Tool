#pragma once

#include <string>
#include <vector>
#include "../parser/PDFParser.hpp"
#include <optional>


// struct TextPage
// {
//     int pageNumber;
//     std::string text;
// };


struct RevenueResult
{
    float revenue = 0.0f;
    double confidence = 0.0;
    std::optional<bool> found = false;
};



class LLMCaller
{

public:

    RevenueResult extractRevenue(
        const std::vector<AuditPage>& pages
    );


private:

    std::string apiKey;

    std::string buildPrompt(
        const std::vector<AuditPage>& pages
    );


    std::string sendRequest(
        const std::string& prompt
    );


    RevenueResult parseResponse(
        const std::string& response
    );


};