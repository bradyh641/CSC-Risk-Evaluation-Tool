#pragma once

#include <vector>
#include <string>

#include "../matching/CombineResults.hpp"
#include "../scanner/SheetScanner.hpp"

class GenerateSpreadsheet
{
public:

    GenerateSpreadsheet(
        const std::vector<FundingOrganization>& funding,
        const std::vector<std::pair<std::string, RevenueResult>>& audits,
        const std::vector<OrganizationMatch>& matches,
        const std::string& y
    );

private:

    void createWorkbook();
    void openWorkbook();

    std::vector<FundingOrganization> funding;
    std::vector<std::pair<std::string, RevenueResult>> audits;
    std::vector<OrganizationMatch> matches;
    std::string year;

    std::string fileName = "CSC_Funding_Report.xlsx";
};