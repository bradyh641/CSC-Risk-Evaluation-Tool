#include "FileScanner.hpp"

#include <iostream>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

FileScanner::FileScanner(const std::string &rootDirectory)
    : root(rootDirectory) {
}

static std::filesystem::path makeExtendedPath(const std::filesystem::path &path) {
    std::string pathString = path.string();

    if (pathString.rfind("\\\\?\\", 0) == 0) {
        return path;
    }

    return std::filesystem::path(
        "\\\\?\\" + pathString
    );
}

static fs::path createLocalCopy(const fs::path &source) {
    fs::path extendedSource = makeExtendedPath(source);

    try {
        fs::path tempFolder =
                fs::current_path() / "Instance_Storage" / "PDFs";

        if (!fs::exists(tempFolder)) {
            fs::create_directories(tempFolder);
        }

        std::string organizationName = extendedSource.parent_path().parent_path().parent_path().filename().string();

        fs::path destination = tempFolder / (organizationName + " - " + source.filename().string());

        std::ifstream test(extendedSource, std::ios::binary);

        if (!test) {
            std::cerr << "Source PDF unavailable: "
                    << extendedSource
                    << "\n";

            return {};
        }

        std::ifstream input(
            extendedSource,
            std::ios::binary
        );

        if (!input) {
            std::cerr << "Could not open source PDF\n";
            return {};
        }


        std::ofstream output(
            destination,
            std::ios::binary
        );

        if (!output) {
            std::cerr << "Could not create destination PDF\n";
            return {};
        }


        output << input.rdbuf();


        input.close();
        output.close();


        return destination;
    } catch (const fs::filesystem_error &e) {
        std::cerr
                << "Failed to copy PDF: "
                << e.what()
                << "\n";

        return {};
    }
}

// returns the path of the new independent audits folder with the files in it
// static fs::path copyIndepAuditsFolder(const fs::path& parent) {
//     // look through every folder and copy locally. Need to test whether we can read the name even if the folder doesn't exist
//     std::regex auditFolderPattern1("^.*(Independent Audits){1}$");
//     std::regex auditFolderPattern2("^.*(Independent Audit){1}$");
//
//     for (const auto& entry : fs::directory_iterator(parent)) {
//         if (std::regex_match(entry.path().filename().string(), auditFolderPattern1) ||
//             std::regex_match(entry.path().filename().string(), auditFolderPattern2)) {
//             // this is the folder we are looking for
//             // copy this folder over. Create a directory
//             std::string orgName = parent.parent_path().filename().string();
//
//             // path where we attach all audit files
//             fs::path indepAuditPath = fs::current_path() / "temp" / "dirs" / orgName;
//
//             if (!fs::exists(indepAuditPath)) {
//                 fs::create_directories(indepAuditPath); // should hopefully create the directories all the way down to org names
//             }
//
//             fs::path temp = makeExtendedPath(entry.path());
//
//             // Copy files into the local dir
//             for (const auto& file : fs::directory_iterator(temp)) {
//                 // here, file should point to the literal location on the box drive
//                 // indepAuditPath points to where we want the files to go
//                 // entry.path() is still the location of the independent audits folder on the box drive
//                 if (file.path().extension() != ".pdf")
//                     continue;
//
//                 fs::path sourceFile = makeExtendedPath(file.path());
//                 fs::path destination = indepAuditPath / sourceFile.filename();
//                 std::ifstream test(sourceFile, std::ios::binary);
//
//                 if(!test)
//                 {
//                     std::cerr << "Source PDF unavailable: "
//                               << sourceFile
//                               << "\n";
//
//                     return {};
//                 }
//
//                 std::ifstream input(
//                     sourceFile,
//                     std::ios::binary
//                 );
//
//                 if(!input)
//                 {
//                     std::cerr << "Could not open source PDF\n";
//                     return {};
//                 }
//
//
//                 std::ofstream output(
//                     destination,
//                     std::ios::binary
//                 );
//
//                 if(!output)
//                 {
//                     std::cerr << "Could not create destination PDF\n";
//                     return {};
//                 }
//
//
//                 output << input.rdbuf();
//
//
//                 input.close();
//                 output.close();
//             }
//             return indepAuditPath;
//         }
//     }
//     return {};
// }

std::filesystem::path FileScanner::findFolderEndingWith(
    const fs::path &parent,
    const std::string &suffix) {
    if (!fs::exists(parent))
        return {};

    for (const auto &entry: fs::directory_iterator(parent)) {
        if (!entry.is_directory())
            continue;

        std::string name = entry.path().filename().string();

        if (name.size() >= suffix.size() &&
            name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0) {
            return entry.path();
        }
    }

    return {};
}

std::filesystem::path FileScanner::findAuditFile(
    const fs::path &independentAuditFolder,
    const std::regex &pattern) {
    if (!fs::exists(independentAuditFolder))
        return {};

    for (const auto &entry: fs::directory_iterator(independentAuditFolder)) {
        if (entry.path().extension() != ".pdf")
            continue;

        std::string filename = entry.path().stem().string();

        if (std::regex_match(filename, pattern))
            return fs::absolute(entry.path());
    }

    return {};
}

std::vector<OrganizationAudit> FileScanner::scan(
    const std::string &auditYear) {
    std::vector<OrganizationAudit> organizations;

    if (!fs::exists(root)) {
        std::cerr << "Root directory does not exist.\n";
        return organizations;
    }

    // this is me being lazy, it works tho
    std::string regexString =
            "^(FYE )?" +
            auditYear +
            ".*Independent( Consolidated Financial Report)? Audit( Report)?( & Mgmt)?( Letter)?( All Requirements)?$";

    std::regex auditRegex(regexString);

    // fs::path extRoot = makeExtendedPath(root);

    for (const auto &organization: fs::directory_iterator(root)) {
        if (!organization.is_directory())
            continue;

        OrganizationAudit audit;

        audit.organizationName = organization.path().filename().string();
        audit.auditFile.clear();
        audit.status = AuditStatus::MissingOrganizationalDocuments;

        // Find Organizational Documents
        fs::path organizationalDocs =
                findFolderEndingWith(
                    organization.path(),
                    "Organizational Docs");

        if (!organizationalDocs.empty()) {
            audit.status = AuditStatus::MissingIndependentAudits;

            // copy independent audits contents to local folder temp->dirs->org_name->[audits]
            // fs::path independentAudits = copyIndepAuditsFolder(organizationalDocs);

            // Find Independent Audits
            fs::path independentAudits =
                    findFolderEndingWith(
                        organizationalDocs,
                        "Independent Audits");

            if (independentAudits.empty())
                independentAudits = findFolderEndingWith(organizationalDocs, "Independent Audit");

            if (!independentAudits.empty()) {
                audit.status = AuditStatus::MissingAuditFile;

                // Find audit PDF
                fs::path auditFile =
                        findAuditFile(
                            independentAudits,
                            auditRegex);

                if (!auditFile.empty()) {
                    // create local copy
                    audit.auditFile = createLocalCopy(auditFile);

                    audit.status = AuditStatus::Found;
                }
            }
        }

        organizations.push_back(std::move(audit));
    }

    return organizations;
}