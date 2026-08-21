#ifndef SHEETSCANNER_HPP
#define SHEETSCANNER_HPP

#include <string>
#include <vector>


struct FundingOrganization
{
    std::string organizationName;

    std::vector<std::string> programNames;

    std::vector<double> fundingList;

    double totalFunding = 0.0;
};


class SheetScanner
{

private:

    std::string filePath;
    double parseCurrency(std::string val);

public:

    SheetScanner(const std::string& path);


    std::vector<FundingOrganization> scan();

};


#endif