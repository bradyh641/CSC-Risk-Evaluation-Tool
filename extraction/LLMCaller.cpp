#include "LLMCaller.hpp"
#include "../parser/PDFParser.hpp"

#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <nlohmann/json.hpp>

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

static std::string loadAPIKEY(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open())
        throw std::runtime_error("Could not open config file: " + filename);

    nlohmann::json j;
    file >> j;

    std::string apikey = j.value("api_key", "");

    return apikey;
}

RevenueResult LLMCaller::extractRevenue(const std::vector<AuditPage> &pages) {
    // get the API key from config.json before we prompt
    apiKey = loadAPIKEY("config.json");
    std::cout << apiKey << std::endl;

    // combines multipage audits into one string for prompt optimization
    std::string prompt = buildPrompt(pages);

    // queries LLM API for JSON
    std::string response = sendRequest(prompt);

    // cleans JSON and extracts values
    return parseResponse(response);
}


std::string LLMCaller::buildPrompt(const std::vector<AuditPage> &pages) {
    std::stringstream prompt;

    for (const auto &page: pages) {
        prompt
                << "\n--- PAGE "
                << page.pageNumber
                << " ---\n";


        prompt
                << page.text;
    }


    return prompt.str();
}


std::string LLMCaller::sendRequest(const std::string &prompt) {
    // Build JSON body
    json body;

    /*
    *"You are extracting data from an independent audit.\n"
        "Return ONLY valid JSON.\n"
        "Find the total revenue for the most recent year listed, typically listed as "
        "'total revenue' or 'total support and revenue'.\n"
        "Rules:\n"
        "- Return the total only.\n"
        "- Ignore subtotals.\n"
        "- Ignore prior-year values.\n"
        "- Ignore notes.\n"
        "- If no value exists, return null.\n\n"
        "Page:\n";
     */

    body["model"] = "openai/gpt-5-mini";
    body["messages"] = json::array({
        {
            {"role", "user"},
            {
                "content",
                "You are extracting data from an independent audit.\n"
                "Find the total revenue for the most recent year listed, typically listed as 'total revenue' or"
                "'total support and revenue'.\n"
                "Return ONLY valid JSON in this format:\n"
                "{\n\"value\": number|null,\n\"confidence\": number\n}\n"
                "Rules:\n"
                "- Use the final total revenue/support and revenue.\n"
                "- Ignore subtotals.\n"
                "- Ignore notes.\n"
                "- Ignore prior-year columns.\n"
                "- If no value exists, return null.\n\n"
                "Document:\n" + prompt
            }
        }
    });


    std::string jsonBody = body.dump();


    // Open WinHTTP session
    HINTERNET hSession = WinHttpOpen(
        L"OpenAI C++ Client",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0
    );

    if (!hSession)
        throw std::runtime_error("WinHttpOpen failed");


    // Connect to OpenAI
    HINTERNET hConnect = WinHttpConnect(
        hSession,
        L"openrouter.ai",
        INTERNET_DEFAULT_HTTPS_PORT,
        0
    );

    if (!hConnect) {
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("WinHttpConnect failed");
    }


    // Create POST request
    HINTERNET hRequest = WinHttpOpenRequest(
        hConnect,
        L"POST",
        L"/api/v1/chat/completions",
        nullptr,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE
    );


    if (!hRequest) {
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("WinHttpOpenRequest failed");
    }


    // Headers
    std::wstring headers =
            L"Content-Type: application/json\r\n"
            L"Authorization: Bearer ";

    headers += std::wstring(apiKey.begin(), apiKey.end());

    headers +=
            L"\r\nHTTP-Referer: http://localhost\r\n"
            L"X-Title: My C++ App";

    // Send request
    BOOL result = WinHttpSendRequest(
        hRequest,
        headers.c_str(),
        -1L,
        (LPVOID) jsonBody.data(),
        jsonBody.size(),
        jsonBody.size(),
        0
    );


    if (!result) {
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);

        throw std::runtime_error("WinHttpSendRequest failed");
    }


    // Receive response
    result = WinHttpReceiveResponse(
        hRequest,
        nullptr
    );

    DWORD statusCode = 0;
    DWORD size = sizeof(statusCode);

    WinHttpQueryHeaders(
        hRequest,
        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
        nullptr,
        &statusCode,
        &size,
        nullptr
    );

    std::cout << "HTTP Status: "
            << statusCode
            << "\n";


    if (!result)
        throw std::runtime_error("WinHttpReceiveResponse failed");


    // Read response body
    std::string response;

    DWORD bytesAvailable = 0;

    while (WinHttpQueryDataAvailable(
               hRequest,
               &bytesAvailable
           ) && bytesAvailable > 0) {
        std::string buffer(bytesAvailable, '\0');

        DWORD bytesRead = 0;

        WinHttpReadData(
            hRequest,
            buffer.data(),
            bytesAvailable,
            &bytesRead
        );

        buffer.resize(bytesRead);

        response += buffer;
    }


    // Cleanup
    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);


    return response;
}


RevenueResult LLMCaller::parseResponse(
    const std::string &response
) {
    RevenueResult result;


    try {
        json outer = json::parse(response);

        std::string content =
                outer["choices"][0]["message"]["content"];

        json data =
                json::parse(content);

        if (data.contains("confidence")) {
            result.confidence =
                    data["confidence"].get<double>();
        }


        if (data.contains("value")
            &&
            !data["value"].is_null()) {
            result.revenue =
                    data["value"].get<float>();


            result.found = true;
        } else {
            result.found = false;
        }
    } catch (const std::exception &e) {
        std::cerr
                << "Failed to parse LLM response: "
                << e.what()
                << "\n";


        result.found = false;
    }


    return result;
}