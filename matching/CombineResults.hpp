#pragma once

#include <vector>

#include "../scanner/SheetScanner.hpp"
#include "../extraction/LLMCaller.hpp"
#include "../scanner/FileScanner.hpp"

struct OrganizationMatch
{
    int fundingIndex = -1;
    int auditIndex = -1;

    double similarity = 0.0;

    bool autoMatched = false;
    bool userModified = false;
};

class CombineResults
{
public:

    static std::vector<OrganizationMatch> matchOrganizations(
        const std::vector<FundingOrganization>& funding,
        const std::vector<OrganizationAudit>& audits
    );

private:

    static std::string normalizeName(const std::string& name);

    static void scanMemory(const std::vector<FundingOrganization>& funding,
        const std::vector<OrganizationAudit>& audits, std::vector<OrganizationMatch>& matches,
        std::vector<bool>& fundingMatched, std::vector<bool>& auditMatched);

    static double similarityScore(
        const std::string& first,
        const std::string& second
    );
};