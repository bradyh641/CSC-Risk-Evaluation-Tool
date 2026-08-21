#pragma once

#include <filesystem>
#include <regex>
#include <string>
#include <vector>

enum AuditStatus
{
    Found,
    MissingOrganizationalDocuments,
    MissingIndependentAudits,
    MissingAuditFile
};

struct OrganizationAudit
{
    std::string organizationName;
    std::filesystem::path auditFile;
    AuditStatus status;
};

class FileScanner
{
public:
    explicit FileScanner(const std::string& rootDirectory);

    std::vector<OrganizationAudit> scan(const std::string& auditYear);

private:
    std::filesystem::path root;

    void copyDirectory(const std::filesystem::path& source, const std::filesystem::path& destination);

    std::filesystem::path findFolderEndingWith(
        const std::filesystem::path& parent,
        const std::string& suffix);

    std::filesystem::path findAuditFile(
        const std::filesystem::path& independentAuditFolder,
        const std::regex& pattern);
};