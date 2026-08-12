#include "LinkWindow.hpp"

#include <iostream>
#include <unordered_map>
#include <windows.h>
#include <fstream>

LinkWindow::LinkWindow(
    const std::vector<SAMISOrganization> &samis,
    const std::vector<OrganizationAudit> &audits,
    std::vector<OrganizationMatch> &matches,
    const std::string &year) : window(sf::VideoMode({1400, 900}), "Link Organizations"),
                               samis(samis), audits(audits), matches(matches), year(year) {
    font.loadFromFile("resources/font.ttf");

    // create generateButton
    generateButton.setSize({280.f, 70.f});
    generateButton.setPosition({560.f, 690.f});
    generateButton.setFillColor(sf::Color(35, 98, 181));
    generateButton.setOutlineThickness(2.f);
    generateButton.setOutlineColor(sf::Color(25, 80, 180));

    createCards();
}

void LinkWindow::run() {
    // instructions
    MessageBoxA(
        window.getSystemHandle(),
        "Your job is to make sure that all the organization names from the SAMIS file are correctly matched with organization names from the CSC Organizations folder."
        " The top two boxes represent organization names that are already matched together. You can drag a card from an unmatched box to a matched name to swap the names."
        " You can also press Backspace or Delete on your keyboard to unmatch the selected names. Select an unmatched audit file and an unmatched SAMIS file and press enter"
        " to create a new match. When all organization names are correctly matched, press the \"Generate Spreadsheet\" button.",
        "INSTRUCTIONS",
        MB_OK | MB_ICONINFORMATION
    );

    while (window.isOpen()) {
        processEvents();

        // update();

        render();
    }
}

void LinkWindow::processEvents() {
    sf::Event event;


    while (window.pollEvent(event)) {
        if (event.type ==
            sf::Event::Closed) {
            window.close();
        }
        else if (event.type ==
            sf::Event::MouseButtonPressed) {
            handleMousePressed(
                window.mapPixelToCoords(
                    sf::Mouse::getPosition(window)
                )
            );
        }
        else if (event.type ==
            sf::Event::MouseButtonReleased) {
            handleMouseReleased(
                window.mapPixelToCoords(
                    sf::Mouse::getPosition(window)
                )
            );
        }
        else if (event.type ==
            sf::Event::MouseMoved) {
            handleMouseMoved(
                window.mapPixelToCoords(
                    sf::Mouse::getPosition(window)
                )
            );
        }
        else if (event.type == sf::Event::MouseWheelScrolled) {
            handleMouseWheel(
                window.mapPixelToCoords(
                    sf::Mouse::getPosition(window)),
                event.mouseWheelScroll.delta
            );
        }
        else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Enter) {
                createMatchFromSelection();
            }
            else if (event.key.code == sf::Keyboard::Backspace ||
                       event.key.code == sf::Keyboard::Delete) {
                // del selected match if a match is selected
                deleteMatch();
            }
        }
    }
}

static void saveMap(const std::unordered_map<std::string, std::string> &map, const std::string &filename) {
    std::ofstream file(filename, std::ios::binary);

    if (!file)
        throw std::runtime_error("Failed to open file.");

    size_t count = map.size();
    file.write(reinterpret_cast<const char *>(&count), sizeof(count));

    for (const auto &[key, value]: map) {
        size_t keyLength = key.size();
        file.write(reinterpret_cast<const char *>(&keyLength), sizeof(keyLength));
        file.write(key.data(), keyLength);

        size_t valueLength = value.size();
        file.write(reinterpret_cast<const char *>(&valueLength), sizeof(valueLength));
        file.write(value.data(), valueLength);
    }
}

void LinkWindow::saveMatches() {
    // generate unordered map of samis orgs to audit orgs
    std::unordered_map<std::string, std::string> map;
    for (auto &match: matches) {
        if (match.auditIndex == -1 || match.samisIndex == -1)
            continue;

        auto sam = samis[match.samisIndex];
        auto audit = audits[match.auditIndex];

        map[sam.organizationName] = audit.organizationName; // think this works
    }

    // now that map is populated with the final matches, save in txt file
    saveMap(map, "resources/SerializedMatches.txt");
}

void LinkWindow::deleteMatch() {
    if (selectedSamisMatchIndex != selectedAuditMatchIndex ||
        selectedSamisMatchIndex == -1 || selectedAuditMatchIndex == -1) // will only be false when a match is selected
        return;

    // "delete" this match and put them back to their respective unmatched boxes
    // make a new OrganizationMatch and swap the value of one's audit/samis index with the other?

    auto &currMatch = matches[selectedSamisMatchIndex];

    OrganizationMatch newAuditMatch;
    newAuditMatch.auditIndex = matches[selectedSamisMatchIndex].auditIndex;
    newAuditMatch.userModified = true;
    matches.push_back(newAuditMatch);

    currMatch.auditIndex = -1;
    currMatch.userModified = true;

    selectedAuditMatchIndex = matches.size() - 1; // should point to newAuditMatch since we push_back

    createCards();
}

void LinkWindow::createMatchFromSelection() {
    if ((selectedSamisMatchIndex == -1 || selectedAuditMatchIndex == -1) ||
        (selectedSamisMatchIndex == selectedAuditMatchIndex)) {
        return;
    }

    auto &samisMatch =
            matches[selectedSamisMatchIndex];

    auto &auditMatch =
            matches[selectedAuditMatchIndex];

    samisMatch.auditIndex =
            auditMatch.auditIndex;

    auditMatch.auditIndex = -1;

    samisMatch.userModified = true;
    auditMatch.userModified = true;

    // this is where we could remove auditMatch from matches
    matches.erase(matches.begin() + selectedAuditMatchIndex);

    selectedAuditMatchIndex = selectedSamisMatchIndex;

    createCards();
}

void LinkWindow::handleMousePressed(sf::Vector2f mouse) {
    auto findCard = [&](auto &cards) {
        for (auto &card: cards) {
            if (card.contains(mouse) && card.isVisible) {
                draggedCard = &card;

                card.dragging = true;

                card.offset = mouse - card.rectangle.getPosition();

                if (card.isSamis) { // if this card is samis
                    if (matches[card.matchIndex].auditIndex != -1) { // matched; deselect all other cards and select this card and its counterpart
                        for (auto c: matchedSamisCards) { // deselect all matched samis cards
                            c.selected = false;
                        }
                        for (auto c: unmatchedSamisCards) {
                            c.selected = false;
                        }
                        for (auto c: unmatchedAuditCards) {
                            c.selected = false;
                        }
                        // find other by iterating through matchedAuditCards and matching the matchIndex while deselecting all matched audit cards
                        for (int i = 0; i < matchedAuditCards.size(); i++) {
                            matchedAuditCards[i].selected = false;
                            if (matchedAuditCards[i].matchIndex == card.matchIndex) {
                                matchedAuditCards[i].selected = true;
                                break;
                            }
                        }

                        selectedSamisMatchIndex = card.matchIndex;
                        selectedAuditMatchIndex = card.matchIndex; // they have the same match index
                    }
                    else {
                        // samis card is not matched, deselect all samis cards and matched audit cards
                        for (auto c: unmatchedSamisCards) {
                            c.selected = false;
                        }
                        for (auto c: matchedSamisCards) {
                            c.selected = false;
                        }
                        for (auto c: matchedAuditCards) {
                            if (c.matchIndex == selectedAuditMatchIndex) { // one of the matched values was selected, deselect it
                                selectedAuditMatchIndex = -1;
                                break;
                            }
                            c.selected = false;
                        }
                        selectedSamisMatchIndex = card.matchIndex;
                    }
                }
                else { // audit card
                    if (matches[card.matchIndex].samisIndex != -1) { // if this is in matched, deselect all other cards in matchedAuditCards and select this card and its counterpart
                        for (auto c: matchedAuditCards) {
                            c.selected = false;
                        }
                        for (auto c: unmatchedSamisCards) {
                            c.selected = false;
                        }
                        for (auto c: unmatchedAuditCards) {
                            c.selected = false;
                        }
                        // find other by iterating through matchedSamisCards and matching the matchIndex
                        for (int i = 0; i < matchedSamisCards.size(); i++) {
                            matchedSamisCards[i].selected = false;
                            if (matchedSamisCards[i].matchIndex == card.matchIndex) {
                                matchedSamisCards[i].selected = true;
                                break;
                            }
                        }

                        selectedSamisMatchIndex = card.matchIndex;
                        selectedAuditMatchIndex = card.matchIndex; // they have the same match index
                    }
                    else {
                        // card is not matched, deselect all audit cards, and all matched samis cards
                        for (auto c: unmatchedAuditCards) {
                            c.selected = false;
                        }
                        for (auto c: matchedAuditCards) {
                            c.selected = false;
                        }
                        for (auto c: matchedSamisCards) {
                            if (c.matchIndex == selectedSamisMatchIndex) { // one of the matched values was selected
                                selectedSamisMatchIndex = -1;
                                break;
                            }
                            c.selected = false;
                        }
                        selectedAuditMatchIndex = card.matchIndex;
                    }
                }
                card.selected = true;

                return true;
            }
        }

        return false;
    };

    // std::cout << mouse.x << ", " << mouse.y << std::endl;

    if (generateButton.getGlobalBounds().contains(mouse)) {
        // Generate the spreadsheet. Call the spreadsheet
        window.close();

        std::cout << "Saving matches to memory..." << std::endl;
        saveMatches();

        // Parse audits and feed into LLM
        // loadingStep = loadingSteps::ReadingPDFs;
        std::cout << "Parsing Documents..." << std::endl;
        render();
        std::vector<std::pair<std::string, RevenueResult>> results;
        PDFParser parser;
        LLMCaller llm;
        for (int i = 0; i < audits.size(); i++) {

            // for each of these audits, find their matched samisIndex. Iterate through matches until i matches j
            int j = 0;
            for (;j < matches.size(); j++) {
                if (matches[j].auditIndex == i) {
                    break;
                }
            }

            if (audits[i].status != AuditStatus::Found || matches[j].samisIndex == -1) {
                results.push_back({audits[i].organizationName, {0, 0, NULL}});
                continue;
            }

            auto pages = parser.extractRelevantPages(audits[i].auditFile, false); // w/o OCR
            if (pages.empty())
                pages = parser.extractRelevantPages(audits[i].auditFile, true); // run OCR

            auto result = llm.extractRevenue(pages);

            // store result
            results.push_back({audits[i].organizationName, result});
            // results.push_back({audits[i].organizationName, {1234567,.90,true}}); // testing
        }

        std::cout << "Generating Spreadsheet..." << std::endl;
        GenerateSpreadsheet(samis, results, matches, year);

        return;
    }

    // std::cout << "unmatchedSamis" << std::endl;
    if (findCard(unmatchedSamisCards))
        return;

    // std::cout << "unmatchedAudit" << std::endl;
    if (findCard(unmatchedAuditCards))
        return;

    // std::cout << "matched samis" << std::endl;
    if (findCard(matchedSamisCards))
        return;

    // std::cout << "matched audit" << std::endl;
    if (findCard(matchedAuditCards))
        return;
}

void LinkWindow::handleMouseMoved(
    sf::Vector2f mouse) {
    if (draggedCard == nullptr)
        return;

    // std::cout << "dragging" << std::endl;
    draggedCard->setPosition(
        mouse -
        draggedCard->offset);
}

void LinkWindow::handleMouseReleased(sf::Vector2f mouse) {
    if (draggedCard == nullptr)
        return;

    draggedCard->dragging = false;

    OrganizationCard *targetCard = nullptr;

    auto findTarget = [&](auto &cards) {
        for (auto &card: cards) {
            if (card.contains(mouse) && &card != draggedCard && card.isVisible) {
                targetCard = &card;
                return true;
            }
        }

        return false;
    };


    bool foundTarget =
            findTarget(matchedSamisCards) ||
            findTarget(matchedAuditCards) ||
            findTarget(unmatchedSamisCards) ||
            findTarget(unmatchedAuditCards);

    // std::cout << foundTarget << std::endl;

    if (foundTarget && targetCard != nullptr) {
        if (draggedCard->matchIndex < 0 ||
            targetCard->matchIndex < 0) {
            draggedCard = nullptr;
            createCards();
            return;
        }


        auto &source =
                matches[draggedCard->matchIndex];


        auto &destination =
                matches[targetCard->matchIndex];

        // std::cout << "destination: " << audits[destination.auditIndex].first << std::endl;
        // std::cout << "source: " << samis[source.samisIndex].organizationName << std::endl;

        swapCards(*draggedCard, *targetCard);
    }


    draggedCard = nullptr;


    createCards();
}

// change these hardcoded bounds to be set to the boxes bounds.
// Make the sf::RectangleShapes part of the obj and initialize in constructor like generate button
void LinkWindow::handleMouseWheel(
    sf::Vector2f mouse,
    float delta) {
    float scrollSpeed = 40.f;


    if (mouse.x >= 40 &&
        mouse.x <= 1360 &&
        mouse.y >= 80 &&
        mouse.y <= 530) {
        matchedScrollOffset -=
                delta * scrollSpeed;
    } else if (mouse.x >= 40 &&
               mouse.x <= 540 &&
               mouse.y >= 600 &&
               mouse.y <= 850) {
        unmatchedSamisScrollOffset -=
                delta * scrollSpeed;
    } else if (mouse.x >= 860 &&
               mouse.x <= 1360 &&
               mouse.y >= 600 &&
               mouse.y <= 850) {
        unmatchedAuditScrollOffset -=
                delta * scrollSpeed;
    }


    clampScroll();
}

void LinkWindow::clampScroll() {
    auto clamp =
            [](float &value, float max) {
        if (value < 0)
            value = 0;

        if (value > max)
            value = max;
    };


    float matchedContentHeight =
            matchedSamisCards.size() * 45.f;


    float unmatchedSamisHeight =
            unmatchedSamisCards.size() * 45.f;


    float unmatchedAuditHeight =
            unmatchedAuditCards.size() * 45.f;


    clamp(
        matchedScrollOffset,
        std::max(
            0.f,
            matchedContentHeight - 450.f
        )
    );


    clamp(
        unmatchedSamisScrollOffset,
        std::max(
            0.f,
            unmatchedSamisHeight - 250.f
        )
    );


    clamp(
        unmatchedAuditScrollOffset,
        std::max(
            0.f,
            unmatchedAuditHeight - 250.f
        )
    );
}

void LinkWindow::render() {
    window.clear(sf::Color(245, 248, 252));

    drawMatchedColumns();

    drawUnmatchedBoxes();

    drawCards();

    drawButtons();

    window.display();
}

void LinkWindow::drawScrollBar(
    float x,
    float y,
    float width,
    float height,
    float contentHeight,
    float scrollOffset) {
    // No scrollbar needed if everything fits.
    if (contentHeight <= height)
        return;

    // Background track.
    sf::RectangleShape track({width, height});
    track.setPosition({x, y});
    track.setFillColor(sf::Color(235, 235, 235));

    window.draw(track);

    // Thumb size.
    float thumbHeight = height * (height / contentHeight);

    // Don't let it become impossibly tiny.
    thumbHeight = std::max(30.f, thumbHeight);

    float maxScroll = contentHeight - height;

    float thumbY =
            y +
            (scrollOffset / maxScroll) *
            (height - thumbHeight);

    sf::RectangleShape thumb({width, thumbHeight});
    thumb.setPosition({x, thumbY});
    thumb.setFillColor(sf::Color(120, 120, 120));

    window.draw(thumb);
}

void LinkWindow::drawButtons() {
    window.draw(generateButton);

    sf::Text buttonText;
    buttonText.setFont(font);
    buttonText.setCharacterSize(22);
    buttonText.setFillColor(sf::Color::White);
    buttonText.setString("Generate Spreadsheet");

    // Center the text
    sf::FloatRect bounds = buttonText.getLocalBounds();
    buttonText.setOrigin(
        bounds.left + bounds.width / 2.f,
        bounds.top + bounds.height / 2.f
    );

    buttonText.setPosition(
        generateButton.getPosition().x + generateButton.getSize().x / 2.f,
        generateButton.getPosition().y + generateButton.getSize().y / 2.f
    );

    window.draw(buttonText);
}

void LinkWindow::drawMatchedColumns() {
    sf::RectangleShape header({1400.f, 46.f});
    header.setFillColor(sf::Color(35, 98, 181));

    window.draw(header);

    sf::Text title;

    title.setFont(font);
    title.setFillColor(sf::Color::White);
    title.setCharacterSize(32);
    title.setString("Organization Matching");
    title.setPosition({490, 1});

    window.draw(title);

    sf::RectangleShape leftBox({500, 450});
    leftBox.setPosition({40, 90});
    leftBox.setFillColor(sf::Color::White);
    leftBox.setOutlineThickness(2);
    leftBox.setOutlineColor(sf::Color(200, 210, 225));

    window.draw(leftBox);

    sf::RectangleShape rightBox({500, 450});
    rightBox.setPosition({860, 90});
    rightBox.setOutlineThickness(2.f);
    rightBox.setOutlineColor(sf::Color(200, 210, 225));
    rightBox.setFillColor(sf::Color::White);

    window.draw(rightBox);

    sf::Text leftTitle;
    leftTitle.setFont(font);
    leftTitle.setFillColor(sf::Color(40, 40, 40));
    leftTitle.setCharacterSize(24);
    leftTitle.setString("SAMIS");
    leftTitle.setPosition({232, 55});

    window.draw(leftTitle);

    sf::Text rightTitle;
    rightTitle.setFont(font);
    rightTitle.setFillColor(sf::Color(40, 40, 40));
    rightTitle.setCharacterSize(24);
    rightTitle.setString("Audit Folder");
    rightTitle.setPosition({1030, 55});

    window.draw(rightTitle);

    drawScrollBar(
        1348.f,
        90.f,
        12.f,
        450.f,
        matchedSamisCards.size() * 45.f,
        matchedScrollOffset);


    float y = 95.f;


    for (auto &card: matchedSamisCards) {
        card.setPosition(
            {60, y - matchedScrollOffset}
        );

        // if (card.selected) {
        //     card.rectangle.setFillColor(
        //         sf::Color(100, 150, 255)
        //     );
        // } else {
        //     card.rectangle.setFillColor(
        //         sf::Color(255, 255, 255)
        //     );
        // }

        // window.draw(card.rectangle);
        // window.draw(card.text);

        y += 45.f;
    }

    y = 95.f;


    for (auto &card: matchedAuditCards) {
        card.setPosition(
            {880, y - matchedScrollOffset}
        );

        // if (card.selected) {
        //     card.rectangle.setFillColor(
        //         sf::Color(100, 150, 255)
        //     );
        // } else {
        //     card.rectangle.setFillColor(
        //         sf::Color(255, 255, 255)
        //     );
        // }
        //
        // window.draw(card.rectangle);
        // window.draw(card.text);

        y += 45.f;
    }
    // drawCards();
}

void LinkWindow::drawUnmatchedBoxes() {
    // Background boxes

    sf::RectangleShape leftBox(sf::Vector2f(500, 250));
    leftBox.setPosition(sf::Vector2f(40, 600));
    leftBox.setFillColor(sf::Color::White);
    leftBox.setOutlineThickness(2);
    leftBox.setOutlineColor(sf::Color(200, 210, 225));

    window.draw(leftBox);

    sf::RectangleShape rightBox(sf::Vector2f(500, 250));
    rightBox.setPosition(sf::Vector2f(860, 600));
    rightBox.setFillColor(sf::Color::White);
    rightBox.setOutlineThickness(2);
    rightBox.setOutlineColor(sf::Color(200, 210, 225));
    window.draw(rightBox);


    // Titles

    sf::Text leftTitle;

    leftTitle.setFont(font);
    leftTitle.setFillColor(sf::Color(40, 40, 40));
    leftTitle.setCharacterSize(22);
    leftTitle.setString(
        "Unmatched SAMIS");

    leftTitle.setPosition(
        sf::Vector2f(180, 566));

    window.draw(leftTitle);


    sf::Text rightTitle;

    rightTitle.setFont(font);
    rightTitle.setFillColor(sf::Color(40, 40, 40));
    rightTitle.setCharacterSize(22);
    rightTitle.setString(
        "Unmatched Audit");

    rightTitle.setPosition(
        sf::Vector2f(1010, 566));

    window.draw(rightTitle);

    drawScrollBar(
        530.f,
        600.f,
        10.f,
        250.f,
        unmatchedSamisCards.size() * 45.f,
        unmatchedSamisScrollOffset);

    drawScrollBar(
        1350.f,
        600.f,
        10.f,
        250.f,
        unmatchedAuditCards.size() * 45.f,
        unmatchedAuditScrollOffset);

    std::sort(unmatchedSamisCards.begin(), unmatchedSamisCards.end());
    std::sort(unmatchedAuditCards.begin(), unmatchedAuditCards.end());

    // Position and draw SAMIS cards

    float samisY = 605.f;

    for (auto &card: unmatchedSamisCards) {
        card.setPosition(
            sf::Vector2f(
                60,
                samisY - unmatchedSamisScrollOffset
            ));

        // if (card.selected) {
        //     card.rectangle.setFillColor(
        //         sf::Color(100, 150, 255)
        //     );
        // } else {
        //     card.rectangle.setFillColor(
        //         sf::Color(255, 255, 255)
        //     );
        // }

        // window.draw(card.rectangle);
        // window.draw(card.text);

        samisY += 45.f;
    }

    // Position and draw Audit cards

    float auditY = 605.f;

    for (auto &card: unmatchedAuditCards) {
        card.setPosition(
            sf::Vector2f(
                880,
                auditY - unmatchedAuditScrollOffset
            ));

        // if (card.selected) {
        //     card.rectangle.setFillColor(
        //         sf::Color(100, 150, 255)
        //     );
        // } else {
        //     card.rectangle.setFillColor(
        //         sf::Color(255, 255, 255)
        //     );
        // }

        // window.draw(card.rectangle);
        // window.draw(card.text);

        auditY += 45.f;
    }

    // drawCards();
}

void LinkWindow::drawCards() {

    for (auto& card: unmatchedAuditCards) {
        if (card.rectangle.getPosition().y <= 810 && card.rectangle.getPosition().y >= 600) {
            window.draw(card.rectangle);
            card.isVisible = true;
            if (card.text.getString().getSize() > 38) {
                std::string tempFullStr = card.text.getString();
                card.text.setString(tempFullStr.substr(0, 38) + "...");
                window.draw(card.text);
                card.text.setString(tempFullStr);
            } else
                window.draw(card.text);
        } else
            card.isVisible = false;
    }
    for (auto& card: unmatchedSamisCards) {
         if (card.rectangle.getPosition().y <= 810 && card.rectangle.getPosition().y >= 600) {
            window.draw(card.rectangle);
            card.isVisible = true;
            if (card.text.getString().getSize() > 38) {
                std::string tempFullStr = card.text.getString();
                card.text.setString(tempFullStr.substr(0, 38) + "...");
                window.draw(card.text);
                card.text.setString(tempFullStr);
            } else
                window.draw(card.text);
        } else
            card.isVisible = false;
    }
    for (auto& card: matchedAuditCards) {
        if (card.rectangle.getPosition().y <= 500 && card.rectangle.getPosition().y >= 90) {
            window.draw(card.rectangle);
            card.isVisible = true;
            if (card.text.getString().getSize() > 38) {
                std::string tempFullStr = card.text.getString();
                card.text.setString(tempFullStr.substr(0, 38) + "...");
                window.draw(card.text);
                card.text.setString(tempFullStr);
            }
            else
                window.draw(card.text);
        }
        else
            card.isVisible = false;
    }
    for (auto& card: matchedSamisCards) {
        if (card.rectangle.getPosition().y <= 500 && card.rectangle.getPosition().y >= 90) {
            window.draw(card.rectangle);
            card.isVisible = true;
            if (card.text.getString().getSize() > 38) {
                std::string tempFullStr = card.text.getString();
                card.text.setString(tempFullStr.substr(0, 38) + "...");
                window.draw(card.text);
                card.text.setString(tempFullStr);
            } else
                window.draw(card.text);
        }
        else
            card.isVisible = false;
    }
}

void LinkWindow::createCards() {
    draggedCard = nullptr;

    matchedSamisCards.clear();
    matchedAuditCards.clear();

    unmatchedSamisCards.clear();
    unmatchedAuditCards.clear();

    // if (selectedSamisMatchIndex != -1 && selectedAuditMatchIndex != -1) {
    //     std::cout << "Selected SAMIS: " << samis[matches[selectedSamisMatchIndex].samisIndex].organizationName << std::endl;
    //     std::cout << "Selected audit: " << audits[matches[selectedAuditMatchIndex].auditIndex].first << std::endl;
    // }

    float y = 150;


    for (size_t i = 0; i < matches.size(); i++) {
        auto &match = matches[i];

        if (match.samisIndex != -1 && match.auditIndex != -1) {
            OrganizationCard left;

            if (i == selectedSamisMatchIndex)
                left.selected = true;
            left.matchIndex = i;
            left.isSamis = true;

            left.rectangle =
                    sf::RectangleShape(
                        {460, 35});

            // std::cout << "Left: " << left.selected << std::endl;
            if (left.selected)
                left.rectangle.setFillColor(sf::Color(215, 232, 255));
            else
                left.rectangle.setFillColor(sf::Color::White);


            left.text.setFont(font);
            left.text.setFillColor(sf::Color(40, 40, 40));
            left.text.setCharacterSize(18);

            left.text.setString(
                samis[match.samisIndex]
                .organizationName);


            matchedSamisCards.push_back(left);


            OrganizationCard right;

            if (i == selectedAuditMatchIndex)
                right.selected = true;
            right.matchIndex = i;
            right.isSamis = false;
            right.rectangle = sf::RectangleShape({460, 35});

            // std::cout << "Right: " << right.selected << std::endl;
            if (right.selected)
                right.rectangle.setFillColor(sf::Color(215, 232, 255));
            else
                right.rectangle.setFillColor(sf::Color::White);


            right.text.setFont(font);
            right.text.setFillColor(sf::Color(40, 40, 40));
            right.text.setCharacterSize(18);

            right.text.setString(audits[match.auditIndex].organizationName);


            matchedAuditCards.push_back(right);
        } else if (match.samisIndex != -1 && match.auditIndex == -1) {
            // unmatched samis

            OrganizationCard card;

            if (i == selectedSamisMatchIndex)
                card.selected = true;
            card.matchIndex = i;
            card.isSamis = true;

            if (selectedSamisMatchIndex == i) {
                card.selected = true;
            }

            card.rectangle =
                    sf::RectangleShape(
                        {460, 35});

            // std::cout << "Unmatched Samis: " << card.selected << std::endl;
            if (card.selected)
                card.rectangle.setFillColor(sf::Color(215, 232, 255));
            else
                card.rectangle.setFillColor(sf::Color::White);


            card.text.setFont(font);
            card.text.setFillColor(sf::Color(40, 40, 40));
            card.text.setCharacterSize(18);

            card.text.setString(
                samis[match.samisIndex]
                .organizationName);


            unmatchedSamisCards.push_back(card);
        }

        if (match.auditIndex != -1 && match.samisIndex == -1) {
            // unmatched audit

            OrganizationCard card;

            if (i == selectedAuditMatchIndex)
                card.selected = true;
            card.matchIndex = i;
            card.isSamis = false;

            if (selectedAuditMatchIndex == i) {
                card.selected = true;
            }

            card.rectangle =
                    sf::RectangleShape(
                        {460, 35});

            // std::cout << "Unmatched Audit: " << card.selected << std::endl;
            if (card.selected)
                card.rectangle.setFillColor(sf::Color(215, 232, 255));
            else
                card.rectangle.setFillColor(sf::Color::White);

            card.text.setFont(font);
            card.text.setFillColor(sf::Color(40, 40, 40));
            card.text.setCharacterSize(18);

            card.text.setString(audits[match.auditIndex].organizationName);

            unmatchedAuditCards.push_back(card);
        }
    }
}

void LinkWindow::swapCards(
    OrganizationCard &a,
    OrganizationCard &b) {
    auto &first =
            matches[a.matchIndex];

    auto &second =
            matches[b.matchIndex];


    if (a.isSamis == b.isSamis) {
        // Same side: swap organizations

        if (a.isSamis) {
            std::swap(
                first.samisIndex,
                second.samisIndex);
        } else {
            std::swap(
                first.auditIndex,
                second.auditIndex);
        }
    } else {
        // Opposite sides: create/reassign match

        if (a.isSamis) {
            int tempIndex = first.auditIndex;
            first.auditIndex = second.auditIndex;

            second.auditIndex = tempIndex;
        } else {
            int tempIndex = first.samisIndex;
            first.samisIndex = second.samisIndex;

            second.samisIndex = tempIndex;
        }
    }


    first.userModified = true;
    second.userModified = true;
}
