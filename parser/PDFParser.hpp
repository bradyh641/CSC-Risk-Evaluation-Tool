#pragma once

#include <filesystem>
#include <string>
#include <vector>


struct AuditPage
{
    int pageNumber;
    std::string text;
};


class PDFParser
{
public:

    std::vector<AuditPage> extractRelevantPages(const std::filesystem::path& pdfPath, const bool& OCR);
};