#pragma once

#include <vector>

#include "../scanner/SheetScanner.hpp"
#include "../extraction/LLMCaller.hpp"

struct OrganizationMatch
{
    int samisIndex = -1;
    int auditIndex = -1;

    double similarity = 0.0;

    bool autoMatched = false;
    bool userModified = false;
};

class CombineResults
{
public:

    static std::vector<OrganizationMatch> matchOrganizations(
        const std::vector<SAMISOrganization>& samis,
        const std::vector<std::pair<std::string, RevenueResult>>& audits
    );

private:

    static std::string normalizeName(const std::string& name);

    static void scanMemory(const std::vector<SAMISOrganization>& samis,
        const std::vector<std::pair<std::string, RevenueResult>>& audits, std::vector<OrganizationMatch>& matches,
        std::vector<bool>& samisMatched, std::vector<bool>& auditMatched);

    static double similarityScore(
        const std::string& first,
        const std::string& second
    );
};