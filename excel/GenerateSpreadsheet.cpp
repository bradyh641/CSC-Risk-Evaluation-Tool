#include "GenerateSpreadsheet.hpp"

#include <OpenXLSX.hpp>
#include <windows.h>
#include <shellapi.h>

using namespace OpenXLSX;


GenerateSpreadsheet::GenerateSpreadsheet(
    const std::vector<SAMISOrganization> &samis,
    const std::vector<std::pair<std::string, RevenueResult> > &audits,
    const std::vector<OrganizationMatch> &matches,
    const std::string &y
)
    : samis(samis),
      audits(audits),
      matches(matches),
      year(y) {
    createWorkbook();
    openWorkbook();
}

static std::string toPercentage(double num) {
    std::ostringstream out;

    num *= 100.0;

    out << std::fixed << std::setprecision(2) << num;

    std::string result = out.str();

    // Remove trailing zeros
    while (!result.empty() && result.back() == '0')
        result.pop_back();

    // Remove decimal point if it is the last character
    if (!result.empty() && result.back() == '.')
        result.pop_back();

    return result + "%";
}

static std::string toCurrency(double num) {
    bool negative = (num < 0);
    num = std::abs(num);

    // Determine whether to display cents
    bool hasCents = std::fabs(num - std::round(num)) > 0.000001;

    std::ostringstream out;

    if (hasCents)
        out << std::fixed << std::setprecision(2) << num;
    else
        out << std::fixed << std::setprecision(0) << num;

    std::string str = out.str();

    // Insert commas
    std::size_t decimalPos = str.find('.');
    if (decimalPos == std::string::npos)
        decimalPos = str.length();

    for (int i = static_cast<int>(decimalPos) - 3; i > 0; i -= 3)
        str.insert(i, ",");

    if (negative)
        return "-$" + str;

    return "$" + str;
}

void GenerateSpreadsheet::createWorkbook() {
    XLDocument document;

    if (std::filesystem::exists(fileName))
        document.open(fileName);
    else
        document.create(fileName, XLForceOverwrite);

    if (!document.workbook().worksheetExists("Audit Year " + year))
        document.workbook().addWorksheet("Audit Year " + year);

    if (document.workbook().worksheetExists("Sheet1"))
        document.workbook().deleteSheet("Sheet1");

    auto wks = document.workbook().worksheet("Audit Year " + year);

    // Headers
    wks.cell("A1").value() = "Organization_Name";
    wks.cell("B1").value() = "Program_Name";
    wks.cell("C1").value() = "Total_CSC_Funding";
    wks.cell("D1").value() = "Total_Revenue";
    wks.cell("E1").value() = "Concentration";
    wks.cell("F1").value() = "Audit_Found";
    wks.cell("G1").value() = "LLM_Found_Revenue";

    int row = 2;

    for (const auto &match: matches) {
        // iterate through every match.
        // Matches should be displayed with all information
        // unmatched samis should display the org name, program name, and program funding
        // unmatched audit should display the org name, revenue, and audit found

        // ignore unmatched audits
        // if (match.samisIndex == -1 && match.auditIndex != -1) {
            // // unmatched audit
            // // just display the organization name, revenue, and audit found
            // auto &audit = audits[match.auditIndex];
            // wks.cell(row, 1).value() = audit.first;
            // wks.cell(row, 2).value() = "N/A";
            // wks.cell(row, 3).value() = "N/A";
            //
            // if (audit.second.found == NULL) {
            //     // row 4 and 5 are N/A, row 6 is No, row 7 is N/A
            //     wks.cell(row, 4).value() = "N/A";
            //     wks.cell(row, 6).value() = "No";
            //     wks.cell(row, 7).value() = "N/A";
            // } else {
            //     wks.cell(row, 4).value() = toCurrency(audit.second.revenue);
            //     wks.cell(row, 6).value() = "Yes";
            //     wks.cell(row, 7).value() = audit.second.found ? "Yes" : "No";
            // }
            //
            // wks.cell(row, 5).value() = "N/A"; // funding is unavailable
            //
            // row += 2;
        // }
        if (match.samisIndex != -1 && match.auditIndex == -1) {
            // unmatched samis
            // iterate through programs and fundingList
            auto &sam = samis[match.samisIndex];
            if (sam.programNames.size() > 1) {
                bool firstProg = true;
                for (int i = 0; i < sam.programNames.size(); i++) {
                    if (firstProg) {
                        firstProg = false;
                        wks.cell(row, 1).value() = sam.organizationName;
                    } else
                        wks.cell(row, 1).value() = "";
                    wks.cell(row, 2).value() = sam.programNames[i];
                    wks.cell(row, 3).value() = toCurrency(sam.fundingList[i]);
                    wks.cell(row, 4).value() = "";
                    wks.cell(row, 5).value() = "";
                    wks.cell(row, 6).value() = "";
                    row++;
                }
                // write total row
                wks.cell(row, 1).value() = "";
                wks.cell(row, 2).value() = "Total";
                wks.cell(row, 3).value() = toCurrency(sam.totalFunding);
                wks.cell(row, 4).value() = "N/A";
                wks.cell(row, 5).value() = "N/A";
                wks.cell(row, 6).value() = "N/A";
                wks.cell(row, 7).value() = "N/A";
            }
            else if (sam.programNames.size() == 1) {
                wks.cell(row, 1).value() = sam.organizationName;
                wks.cell(row, 2).value() = sam.programNames[0];
                wks.cell(row, 3).value() = toCurrency(sam.totalFunding);
                wks.cell(row, 4).value() = "N/A";
                wks.cell(row, 5).value() = "N/A";
                wks.cell(row, 6).value() = "N/A";
                wks.cell(row, 7).value() = "N/A";
            }

            row += 2; // delimiter between orgs
        }
        else if (match.samisIndex != -1 && match.auditIndex != -1) {
            // matched

            auto &sam = samis[match.samisIndex];
            auto &audit = audits[match.auditIndex];
            if (sam.programNames.size() > 1) {
                bool firstProg = true;
                for (int i = 0; i < sam.programNames.size(); i++) {
                    if (firstProg) {
                        firstProg = false;
                        wks.cell(row, 1).value() = sam.organizationName;
                    }
                    else
                        wks.cell(row, 1).value() = "";
                    wks.cell(row, 2).value() = sam.programNames[i];
                    wks.cell(row, 3).value() = toCurrency(sam.fundingList[i]);
                    wks.cell(row, 4).value() = "";
                    wks.cell(row, 5).value() = "";
                    wks.cell(row, 6).value() = "";
                    row++;
                }
                // write total row
                wks.cell(row, 1).value() = "";
                wks.cell(row, 2).value() = "Total";
                wks.cell(row, 3).value() = toCurrency(sam.totalFunding);

                if (audit.second.found == NULL) {
                    // row 4 and 5 are N/A, row 6 is No, row 7 is N/A
                    wks.cell(row, 4).value() = "N/A";
                    wks.cell(row, 5).value() = "N/A";
                    wks.cell(row, 6).value() = "No";
                    wks.cell(row, 7).value() = "N/A";
                }
                else {
                    wks.cell(row, 4).value() = toCurrency(audit.second.revenue);
                    wks.cell(row, 5).value() = toPercentage((sam.totalFunding / audit.second.revenue));
                    wks.cell(row, 6).value() = "Yes";
                    wks.cell(row, 7).value() = audit.second.found ? "Yes" : "No";
                }
            }
            else if (sam.programNames.size() == 1) {
                wks.cell(row, 1).value() = sam.organizationName;
                wks.cell(row, 2).value() = sam.programNames[0];
                wks.cell(row, 3).value() = toCurrency(sam.fundingList[0]);

                if (audit.second.found == NULL) {
                    // row 4 and 5 are N/A, row 6 is No, row 7 is N/A
                    wks.cell(row, 4).value() = "N/A";
                    wks.cell(row, 5).value() = "N/A";
                    wks.cell(row, 6).value() = "No";
                    wks.cell(row, 7).value() = "N/A";
                }
                else {
                    wks.cell(row, 4).value() = toCurrency(audit.second.revenue);
                    wks.cell(row, 5).value() = toPercentage((sam.totalFunding / audit.second.revenue));
                    wks.cell(row, 6).value() = "Yes";
                    wks.cell(row, 7).value() = audit.second.found ? "Yes" : "No";
                }
            }

            row += 2;
        }
    }

    document.save();

    document.close();
}

void GenerateSpreadsheet::openWorkbook() {
    ShellExecuteA(
        nullptr,
        "open",
        fileName.c_str(),
        nullptr,
        nullptr,
        SW_SHOWNORMAL
    );
}
