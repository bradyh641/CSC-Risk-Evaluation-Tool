#include "CombineResults.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

std::string CombineResults::normalizeName(
    const std::string &name) {
    std::string normalized;

    for (char c: name) {
        if (std::isalnum(static_cast<unsigned char>(c))) {
            normalized +=
                    static_cast<char>(
                        std::tolower(c));
        }
    }

    auto removeWord =
            [&](const std::string &word) {
        size_t pos;

        while ((pos = normalized.find(word))
               != std::string::npos) {
            normalized.erase(pos, word.size());
        }
    };

    removeWord("the");
    removeWord("inc");
    removeWord("llc");
    removeWord("corp");
    removeWord("corporation");
    removeWord("incorporated");
    removeWord("foundation");

    return normalized;
}

double CombineResults::similarityScore(
    const std::string &first,
    const std::string &second) {
    std::unordered_map<char, int> firstMap;
    std::unordered_map<char, int> secondMap;

    for (char c: first)
        firstMap[c]++;

    for (char c: second)
        secondMap[c]++;

    std::unordered_set<char> letters;

    for (auto &p: firstMap)
        letters.insert(p.first);

    for (auto &p: secondMap)
        letters.insert(p.first);

    int difference = 0;

    for (char c: letters) {
        int count1 = 0;
        int count2 = 0;

        if (firstMap.contains(c))
            count1 = firstMap.at(c);

        if (secondMap.contains(c))
            count2 = secondMap.at(c);

        difference +=
                std::abs(count1 - count2);
    }

    int totalCharacters =
            first.length() +
            second.length();

    if (totalCharacters == 0)
        return 0.0;

    double differenceRatio =
            static_cast<double>(difference)
            /
            totalCharacters;

    // std::cout << first << ", " << second << ", " << 1.0 - differenceRatio << std::endl;

    return 1.0 - differenceRatio;
}

std::unordered_map<std::string, std::string> loadMap(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary);

    if (!file)
        throw std::runtime_error("Failed to open file.");

    std::unordered_map<std::string, std::string> map;

    size_t count;
    file.read(reinterpret_cast<char *>(&count), sizeof(count));

    for (size_t i = 0; i < count; ++i) {
        size_t keyLength;
        file.read(reinterpret_cast<char *>(&keyLength), sizeof(keyLength));

        std::string key(keyLength, '\0');
        file.read(key.data(), keyLength);

        size_t valueLength;
        file.read(reinterpret_cast<char *>(&valueLength), sizeof(valueLength));

        std::string value(valueLength, '\0');
        file.read(value.data(), valueLength);

        map.emplace(std::move(key), std::move(value));
    }

    return map;
}

void CombineResults::scanMemory(const std::vector<FundingOrganization> &funding,
                                const std::vector<OrganizationAudit> &audits,
                                std::vector<OrganizationMatch> &matches,
                                std::vector<bool> &fundingMatched, std::vector<bool> &auditMatched) {
    // scan ../resources/SerializedMatches.txt for matches from the last run
    std::unordered_map<std::string, std::string> map;
    try {
        map = loadMap("resources/SerializedMatches.txt");
    } catch (std::runtime_error &e) {
        std::cerr << "No matches stored in memory." << std::endl;
    }

    if (map.empty())
        return;

    // iterate through funding
    for (int i = 0; i < funding.size(); ++i) {
        if (map.contains(funding[i].organizationName)) {
            // iterate through audits until we find the matching name
            std::string auditName = map[funding[i].organizationName];
            for (int j = 0; j < audits.size(); ++j) {
                if (audits[j].organizationName == auditName) {
                    // both of these funding and audit names exist in the current run. Add them to matches
                    OrganizationMatch newMatch;
                    newMatch.fundingIndex = i;
                    newMatch.auditIndex = j;
                    newMatch.autoMatched = true;
                    matches.push_back(newMatch);

                    fundingMatched[i] = true;
                    auditMatched[j] = true;

                    break;
                }
            }
        }
    }
}

std::vector<OrganizationMatch>
CombineResults::matchOrganizations(
    const std::vector<FundingOrganization> &funding,
    const std::vector<OrganizationAudit> &audits) {
    std::vector<OrganizationMatch> matches;

    std::vector<bool> fundingMatched(
        funding.size(),
        false);

    std::vector<bool> auditMatched(
        audits.size(),
        false);

    scanMemory(funding, audits, matches, fundingMatched, auditMatched);

    for (size_t i = 0; i < funding.size(); i++) {
        if (fundingMatched[i])
            continue;

        double bestScore = 0.0;
        int bestAudit = -1;

        std::string fundingName = normalizeName(funding[i].organizationName);

        for (size_t j = 0; j < audits.size(); j++) {
            if (auditMatched[j])
                continue;

            std::string auditName = normalizeName(audits[j].organizationName);

            double score = similarityScore(fundingName, auditName);

            if (score > bestScore) {
                bestScore = score;
                bestAudit = j;
            }
        }

        if (bestAudit != -1 &&
            bestScore >= 0.60) {
            OrganizationMatch match;

            match.fundingIndex = i;
            match.auditIndex = bestAudit;
            match.similarity = bestScore;
            match.autoMatched = true;

            matches.push_back(match);

            fundingMatched[i] = true;
            auditMatched[bestAudit] = true;
        }
    }

    //
    // Add unmatched funding entries
    //

    for (size_t i = 0; i < funding.size(); i++) {
        if (!fundingMatched[i]) {
            OrganizationMatch match;

            match.fundingIndex = i;

            matches.push_back(match);
        }
    }

    //
    // Add unmatched audit entries
    //

    for (size_t i = 0; i < audits.size(); i++) {
        if (!auditMatched[i]) {
            OrganizationMatch match;

            match.auditIndex = i;

            matches.push_back(match);
        }
    }

    return matches;
}