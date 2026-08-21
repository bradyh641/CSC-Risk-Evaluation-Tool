#include "MainWindow.hpp"
#include "FilePicker.hpp"
#include "../scanner/FileScanner.hpp"
#include "../scanner/SheetScanner.hpp"
#include "../parser/PDFParser.hpp"
#include "../extraction/LLMCaller.hpp"
#include "../gui/LinkWindow.hpp"
#include "../matching/CombineResults.hpp"

#include <windows.h>
#include <iostream>

std::string statusToString(AuditStatus status) {
    switch (status) {
        case AuditStatus::Found:
            return "Found";

        case AuditStatus::MissingOrganizationalDocuments:
            return "Missing Organizational Docs";

        case AuditStatus::MissingIndependentAudits:
            return "Missing Independent Audits";

        case AuditStatus::MissingAuditFile:
            return "Missing Audit File";
    }

    return "Unknown";
}

std::string getClipboardText() {
    if (!OpenClipboard(nullptr))
        return "";

    HANDLE handle = GetClipboardData(CF_TEXT);

    if (handle == nullptr) {
        CloseClipboard();
        return "";
    }

    char *text = static_cast<char *>(GlobalLock(handle));

    if (text == nullptr) {
        CloseClipboard();
        return "";
    }

    std::string result(text);

    GlobalUnlock(handle);
    CloseClipboard();

    return result;
}

MainWindow::MainWindow() : window(sf::VideoMode(800, 600), "CSC Organization Audit") {
    window.setFramerateLimit(60);

    screen = loadingScreens::Input;
    loadingStep = loadingSteps::None;

    if (!font.loadFromFile("resources/font.ttf")) {
        std::cerr << "Font failed to load\n";
    }

    // title
    titleText.setFont(font);
    titleText.setFillColor(sf::Color(30, 30, 40));
    titleText.setString("CSC Organization Audit");
    titleText.setCharacterSize(30);
    auto bounds = titleText.getLocalBounds();
    titleText.setOrigin(bounds.left + bounds.width / 2.f, bounds.top + bounds.height / 2.f);
    titleText.setPosition(400, 40);

    // Labels
    fundingLabel.setFont(font);
    fundingLabel.setFillColor(sf::Color(60, 60, 70));
    fundingLabel.setString("Funding File");
    fundingLabel.setCharacterSize(18);
    fundingLabel.setPosition(115, 100);

    cscPathLabel.setFont(font);
    cscPathLabel.setFillColor(sf::Color(60, 60, 70));
    cscPathLabel.setString("CSC Organizations Path");
    cscPathLabel.setCharacterSize(18);
    cscPathLabel.setPosition(115, 190);

    yearLabel.setFont(font);
    yearLabel.setFillColor(sf::Color(60, 60, 70));
    yearLabel.setString("Audit Year");
    yearLabel.setCharacterSize(18);
    yearLabel.setPosition(115, 280);

    // input boxes
    fundingInputBox.setSize({450, 40});
    fundingInputBox.setPosition(115, 125);
    fundingInputBox.setFillColor(sf::Color::White);
    fundingInputBox.setOutlineThickness(2.f);
    fundingInputBox.setOutlineColor(sf::Color(180, 185, 195));

    cscPathInputBox.setSize({450, 40});
    cscPathInputBox.setPosition(115, 215);
    cscPathInputBox.setFillColor(sf::Color::White);
    cscPathInputBox.setOutlineThickness(2.f);
    cscPathInputBox.setOutlineColor(sf::Color(180, 185, 195));

    yearInputBox.setSize({450, 40});
    yearInputBox.setPosition(115, 305);
    yearInputBox.setFillColor(sf::Color::White);
    yearInputBox.setOutlineThickness(2.f);
    yearInputBox.setOutlineColor(sf::Color(180, 185, 195));

    // Input Text
    fundingInputText.setFont(font);
    fundingInputText.setFillColor(sf::Color::Black);
    fundingInputText.setCharacterSize(16);
    fundingInputText.setPosition(125, 135);

    cscInputText.setFont(font);
    cscInputText.setFillColor(sf::Color::Black);
    cscInputText.setCharacterSize(16);
    cscInputText.setPosition(125, 225);

    yearInputText.setFont(font);
    yearInputText.setFillColor(sf::Color::Black);
    yearInputText.setCharacterSize(16);
    yearInputText.setPosition(125, 315);

    // Browse buttons
    fundingBrowseButton.setSize({120, 40});
    fundingBrowseButton.setPosition(580, 125);
    fundingBrowseButton.setFillColor(sf::Color(52, 120, 246));
    fundingBrowseButton.setOutlineThickness(2.f);
    fundingBrowseButton.setOutlineColor(sf::Color(25, 80, 180));

    cscBrowseButton.setSize({120, 40});
    cscBrowseButton.setPosition(580, 215);
    cscBrowseButton.setFillColor(sf::Color(52, 120, 246));
    cscBrowseButton.setOutlineThickness(2.f);
    cscBrowseButton.setOutlineColor(sf::Color(25, 80, 180));

    fundingBrowseText.setFont(font);
    fundingBrowseText.setFillColor(sf::Color::White);
    fundingBrowseText.setString("Browse");
    fundingBrowseText.setCharacterSize(16);
    fundingBrowseText.setPosition(605, 135);

    cscBrowseText.setFont(font);
    cscBrowseText.setFillColor(sf::Color::White);
    cscBrowseText.setString("Browse");
    cscBrowseText.setCharacterSize(16);
    cscBrowseText.setPosition(605, 225);

    // Start button
    startButton.setSize({250, 55});
    startButton.setPosition(275, 425);
    startButton.setFillColor(sf::Color(52, 120, 246));
    startButton.setOutlineThickness(2.f);
    startButton.setOutlineColor(sf::Color(25, 80, 180));

    startButtonText.setFont(font);
    startButtonText.setFillColor(sf::Color::White);
    startButtonText.setString("Start");
    startButtonText.setCharacterSize(22);
    bounds = startButtonText.getLocalBounds();

    startButtonText.setOrigin(
        bounds.left + bounds.width / 2.f,
        bounds.top + bounds.height / 2.f
    );

    startButtonText.setPosition(
        startButton.getPosition().x + startButton.getSize().x / 2.f,
        startButton.getPosition().y + startButton.getSize().y / 2.f
    );
}

void MainWindow::run() {
    while (window.isOpen()) {
        handleEvents();
        updateTextFields();
        render();
    }
}

void MainWindow::handleEvents() {
    sf::Event event;

    while (window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            window.close();
        }

        // Mouse Click
        if (event.type == sf::Event::MouseButtonPressed) {
            sf::Vector2f mouse(
                event.mouseButton.x,
                event.mouseButton.y
            );

            if (fundingInputBox.getGlobalBounds().contains(mouse)) {
                activeField = 1;
                fundingCursorPosition = fundingPath.length();
            } else if (cscPathInputBox.getGlobalBounds().contains(mouse)) {
                activeField = 2;
                cscCursorPosition = cscOrganizationsPath.length();
            } else if (yearInputBox.getGlobalBounds().contains(mouse)) {
                activeField = 3;
                yearCursorPosition = auditYear.length();
            } else {
                activeField = 0;
            }

            if (fundingBrowseButton.getGlobalBounds().contains(mouse)) {
                std::string path = FilePicker::openFile();

                if (!path.empty()) {
                    fundingPath = path;
                }

                activeField = 0;
            }

            if (cscBrowseButton.getGlobalBounds().contains(mouse)) {
                std::string path = FilePicker::openFolder();

                if (!path.empty()) {
                    cscOrganizationsPath = path;
                }

                activeField = 0;
            }

            if (startButton.getGlobalBounds().contains(mouse)) {
                if (fundingPath.empty()) {
                    MessageBoxA(
                        nullptr,
                        "Please select the Funding file for the desired year.",
                        "Missing Information",
                        MB_OK | MB_ICONWARNING
                    );
                } else if (cscOrganizationsPath.empty()) {
                    MessageBoxA(
                        nullptr,
                        "Please select the path to your \"CSC Organizations\" folder.",
                        "Missing Information",
                        MB_OK | MB_ICONWARNING
                    );
                } else if (auditYear.empty()) {
                    MessageBoxA(
                        nullptr,
                        "Please input an audit year.",
                        "Missing Information",
                        MB_OK | MB_ICONWARNING
                    );
                } else {
                    startPressed();
                }
            }
        }

        // keyboard button
        if (event.type == sf::Event::KeyPressed) {
            // Ctrl + V
            if (event.key.control && event.key.code == sf::Keyboard::V) {
                std::string paste = getClipboardText();


                if (activeField == 1) {
                    fundingPath.insert(fundingCursorPosition, paste);
                    fundingCursorPosition += paste.length();
                }


                if (activeField == 2) {
                    cscOrganizationsPath.insert(cscCursorPosition, paste);
                    cscCursorPosition += paste.length();
                }

                if (activeField == 3) {
                    auditYear.insert(yearCursorPosition, paste);
                    yearCursorPosition += paste.length();
                }
            }

            // Left Arrow
            if (event.key.code == sf::Keyboard::Left) {
                if (activeField == 1 && fundingCursorPosition > 0)
                    fundingCursorPosition--;

                if (activeField == 2 && cscCursorPosition > 0)
                    cscCursorPosition--;

                if (activeField == 3 && yearCursorPosition > 0)
                    yearCursorPosition--;
            }

            // Right Arrow
            if (event.key.code == sf::Keyboard::Right) {
                if (activeField == 1 && fundingCursorPosition < fundingPath.length())
                    fundingCursorPosition++;

                if (activeField == 2 && cscCursorPosition < cscOrganizationsPath.length())
                    cscCursorPosition++;

                if (activeField == 3 && yearCursorPosition < auditYear.length())
                    yearCursorPosition++;
            }

            // Backspace
            if (event.key.code == sf::Keyboard::Backspace) {
                if (activeField == 1 && fundingCursorPosition > 0) {
                    fundingPath.erase(
                        fundingCursorPosition - 1,
                        1
                    );

                    fundingCursorPosition--;
                }


                if (activeField == 2 && cscCursorPosition > 0) {
                    cscOrganizationsPath.erase(
                        cscCursorPosition - 1,
                        1
                    );

                    cscCursorPosition--;
                }

                if (activeField == 3 && yearCursorPosition > 0) {
                    auditYear.erase(
                        yearCursorPosition - 1,
                        1
                    );

                    yearCursorPosition--;
                }
            }
        }

        if (event.type == sf::Event::TextEntered) {
            if (event.text.unicode < 128 && event.text.unicode >= 32) {
                char character = static_cast<char>(event.text.unicode);

                if (activeField == 1) {
                    fundingPath.insert(fundingCursorPosition, 1, character);
                    fundingCursorPosition++;
                }

                if (activeField == 2) {
                    cscOrganizationsPath.insert(cscCursorPosition, 1, character);
                    cscCursorPosition++;
                }

                if (activeField == 3) {
                    auditYear.insert(yearCursorPosition, 1, character);
                    yearCursorPosition++;
                }
            }
        }
        if (event.key.code == sf::Keyboard::Enter) {
            if (fundingPath.empty()) {
                MessageBoxA(
                    nullptr,
                    "Please select the Funding file for the desired year.",
                    "Missing Information",
                    MB_OK | MB_ICONWARNING
                );
            } else if (cscOrganizationsPath.empty()) {
                MessageBoxA(
                    nullptr,
                    "Please select the path to your \"CSC Organizations\" folder.",
                    "Missing Information",
                    MB_OK | MB_ICONWARNING
                );
            } else if (auditYear.empty()) {
                MessageBoxA(
                    nullptr,
                    "Please input an audit year.",
                    "Missing Information",
                    MB_OK | MB_ICONWARNING
                );
            } else {
                startPressed();
            }
        }
    }
}

static std::string truncateText(const std::string &text, std::size_t maxLength = 42) {
    if (text.length() <= maxLength)
        return text;

    return text.substr(0, maxLength - 3) + "...";
}

void MainWindow::updateTextFields() {
    std::string fundingDisplay = fundingPath;
    std::string cscDisplay = cscOrganizationsPath;
    std::string yearDisplay = auditYear;

    if (activeField == 1) {
        fundingDisplay.insert(fundingCursorPosition, "|");
        fundingInputBox.setOutlineColor(sf::Color(52, 120, 246));
    } else
        fundingInputBox.setOutlineColor(sf::Color(180, 185, 195));

    if (activeField == 2) {
        cscDisplay.insert(cscCursorPosition, "|");
        cscPathInputBox.setOutlineColor(sf::Color(52, 120, 246));
    } else
        cscPathInputBox.setOutlineColor(sf::Color(180, 185, 195));

    if (activeField == 3) {
        yearDisplay.insert(yearCursorPosition, "|");
        yearInputBox.setOutlineColor(sf::Color(52, 120, 246));
    } else
        yearInputBox.setOutlineColor(sf::Color(180, 185, 195));

    fundingInputText.setString(truncateText(fundingDisplay));
    cscInputText.setString(truncateText(cscDisplay));
    yearInputText.setString(truncateText(yearDisplay));
}

float MainWindow::getProgress() const {
    switch (loadingStep) {
        case loadingSteps::None: return 0.0f;
        case loadingSteps::ReadingSpreadsheet: return 0.20f;
        case loadingSteps::ReadingPDFs: return 0.40f;
        case loadingSteps::OCR: return 0.60f;
        case loadingSteps::CallingLLM: return 0.80f;
        case loadingSteps::Matching: return 0.95f;
        case loadingSteps::Finished: return 1.0f;
    }

    return 0.f;
}

std::string MainWindow::getLoadingText() const {
    switch (loadingStep) {
        case loadingSteps::ReadingSpreadsheet:
            return "Reading spreadsheet...";

        case loadingSteps::ExtractingPDFs:
            return "Extracting PDFs...";

        case loadingSteps::ReadingPDFs:
            return "Reading audit PDFs and calling LLM...";

        case loadingSteps::OCR:
            return "Performing OCR...";

        case loadingSteps::CallingLLM:
            return "Extracting revenue...";

        case loadingSteps::Matching:
            return "Matching organizations...";

        case loadingSteps::Finished:
            return "Done!";

        default:
            return "";
    }
}

void MainWindow::drawLoading() {
    // background
    sf::RectangleShape outline(sf::Vector2f(400.f, 30.f));
    outline.setPosition(200.f, 285.f);
    outline.setFillColor(sf::Color(220, 220, 220));

    // Filled portion
    float progress = getProgress();

    sf::RectangleShape fill(
        sf::Vector2f(400.f * progress, 30.f));

    fill.setPosition(200.f, 285.f);
    fill.setFillColor(sf::Color(70, 170, 255));

    // Status text
    sf::Text statusText;
    statusText.setFont(font);
    statusText.setCharacterSize(24);
    statusText.setFillColor(sf::Color::Black);
    statusText.setString(getLoadingText());

    // Center beneath the progress bar
    sf::FloatRect bounds = statusText.getLocalBounds();
    statusText.setOrigin(bounds.left + bounds.width / 2.f,
                         bounds.top + bounds.height / 2.f);
    statusText.setPosition(400.f, 335.f);

    window.draw(outline);
    window.draw(fill);
    window.draw(statusText);
}

void MainWindow::render() {
    window.clear(sf::Color(245, 247, 250));

    if (screen == loadingScreens::Input) {
        window.draw(titleText);

        window.draw(fundingLabel);
        window.draw(cscPathLabel);
        window.draw(yearLabel);

        window.draw(fundingInputBox);
        window.draw(cscPathInputBox);
        window.draw(yearInputBox);

        window.draw(fundingInputText);
        window.draw(cscInputText);
        window.draw(yearInputText);

        window.draw(fundingBrowseButton);
        window.draw(cscBrowseButton);

        window.draw(fundingBrowseText);
        window.draw(cscBrowseText);

        window.draw(startButton);
        window.draw(startButtonText);
    } else {
        // draw loading screen
        drawLoading();
    }

    window.display();
}

void MainWindow::startPressed() {
    screen = loadingScreens::Loading;
    render();

    std::cout << "Scanning Files..." << std::endl;
    loadingStep = loadingSteps::ExtractingPDFs;
    render();
    FileScanner scanner(cscOrganizationsPath);
    auto audits = scanner.scan(auditYear);

    std::cout << "Scanning CSV..." << std::endl;
    loadingStep = loadingSteps::ReadingSpreadsheet;
    render();
    SheetScanner sheetScan(fundingPath);
    auto fundingData = sheetScan.scan();

    std::cout << "Checking memory and consolidating data..." << std::endl;
    render();
    organizationMatches =
            CombineResults::matchOrganizations(
                fundingData,
                audits
            );

    std::cout << "Booting Link Window..." << std::endl;
    LinkWindow linkWindow(
        fundingData,
        audits,
        organizationMatches,
        auditYear
    );

    window.close();

    linkWindow.run();
}
