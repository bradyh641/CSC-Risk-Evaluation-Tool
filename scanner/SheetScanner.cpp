#include "SheetScanner.hpp"

#include <iostream>
#include "vincentlaucsb-csv-parser/csv.hpp"
#include <unordered_map>
#include <regex>

double SheetScanner::parseCurrency(std::string val) {
    val.erase(std::remove(val.begin(), val.end(), '$'),
              val.end());
    val.erase(std::remove(val.begin(), val.end(), ','),
              val.end());

    return std::stod(val);
}

SheetScanner::SheetScanner(const std::string &path)
    : filePath(path) {
}

std::vector<SAMISOrganization> SheetScanner::scan() {
    std::vector<SAMISOrganization> organizations;

    std::regex programException = std::regex("^.*[0-9]+-[0-9]+.*$");

    try {
        csv::CSVFormat format;
        format.header_row(3);

        csv::CSVReader reader(filePath, format);

        std::unordered_map<std::string, SAMISOrganization> organizationMap;

        for (csv::CSVRow &row: reader) {
            std::string organizationName =
                    row["grp_organization"].get<std::string>();

            if (organizationName.empty()) {
                continue;
            }

            std::string programName =
                    row["det_program"].get<std::string>();

            if (std::regex_match(programName, programException))
                continue;

            double funding =
                    parseCurrency(
                        row["det_tot_csc"].get<std::string>());

            auto &organization =
                    organizationMap[organizationName];

            // First time we've seen this organization
            if (organization.organizationName.empty()) {
                organization.organizationName =
                        organizationName;
            }

            organization.programNames.push_back(programName);
            organization.totalFunding += funding;
            organization.fundingList.push_back(funding);
        }

        organizations.reserve(
            organizationMap.size());

        for (auto &pair: organizationMap) {
            organizations.push_back(
                std::move(pair.second));
        }
    } catch (const std::exception &e) {
        std::cerr
                << "Failed to parse SAMIS file: "
                << e.what()
                << "\n";
    }

    return organizations;
}