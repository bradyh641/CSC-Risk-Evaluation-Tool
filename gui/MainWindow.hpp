#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include "../extraction/LLMCaller.hpp"
#include "../scanner/SheetScanner.hpp"
#include "../matching/CombineResults.hpp"


class MainWindow
{
public:

    MainWindow();

    void run();


private:

    enum loadingScreens {
        Input,
        Loading
    };

    enum loadingSteps {
        None,
        ReadingSpreadsheet,
        ExtractingPDFs,
        ReadingPDFs,
        OCR,
        CallingLLM,
        Matching,
        Finished
    };

    void handleEvents();
    void render();

    void updateTextFields();

    void startPressed();

    float getProgress() const;

    void drawLoading();

    std::string getLoadingText() const;

    sf::RenderWindow window;
    sf::Font font;

    loadingScreens screen;
    loadingSteps loadingStep;

    // Labels

    sf::Text titleText;

    sf::Text fundingLabel;
    sf::Text cscPathLabel;
    sf::Text yearLabel;

    // Input fields

    sf::RectangleShape fundingInputBox;
    sf::RectangleShape cscPathInputBox;
    sf::RectangleShape yearInputBox;

    sf::Text fundingInputText;
    sf::Text cscInputText;
    sf::Text yearInputText;

    // Buttons

    sf::RectangleShape fundingBrowseButton;
    sf::RectangleShape cscBrowseButton;
    sf::RectangleShape startButton;


    sf::Text fundingBrowseText;
    sf::Text cscBrowseText;
    sf::Text startButtonText;


    // Stored values

    std::string fundingPath;
    std::string cscOrganizationsPath;
    std::string auditYear;

    size_t fundingCursorPosition = 0;
    size_t cscCursorPosition = 0;
    size_t yearCursorPosition = 0;

    // 0 = none
    // 1 = funding path field
    // 2 = CSC path field
    // 3 = audit year field

    int activeField = 0;

    std::vector<OrganizationMatch> organizationMatches;
};