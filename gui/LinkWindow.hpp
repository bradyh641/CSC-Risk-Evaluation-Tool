#pragma once

#include <SFML/Graphics.hpp>

#include <vector>
#include <string>
#include <unordered_map>

#include "../matching/CombineResults.hpp"
#include "../scanner/SheetScanner.hpp"
#include "../extraction/LLMCaller.hpp"
#include "OrganizationCard.hpp"

class LinkWindow
{
public:

    LinkWindow(
        const std::vector<SAMISOrganization>& samis,
        const std::vector<std::pair<std::string, RevenueResult>>& audits,
        std::vector<OrganizationMatch>& matches,
        const std::string& year
    );

    void run();

private:

    void processEvents();

    // void update();

    void render();

    void drawMatchedColumns();

    void drawUnmatchedBoxes();

    void drawScrollBar(float x,float y,float width,float height,float contentHeight,float scrollOffset);

    void saveMatches();

    sf::RenderWindow window;

    sf::RectangleShape generateButton;

    sf::Font font;

    std::string year;

    const std::vector<SAMISOrganization>& samis;
    const std::vector<std::pair<std::string, RevenueResult>>& audits;

    std::vector<OrganizationMatch>& matches;

    std::vector<OrganizationCard> matchedSamisCards;

    std::vector<OrganizationCard> matchedAuditCards;

    std::vector<OrganizationCard> unmatchedSamisCards;

    std::vector<OrganizationCard> unmatchedAuditCards;

    OrganizationCard* draggedCard = nullptr;

    float matchedScrollOffset = 0.f;

    float unmatchedSamisScrollOffset = 0.f;

    float unmatchedAuditScrollOffset = 0.f;

    int selectedSamisMatchIndex = -1;

    int selectedAuditMatchIndex = -1;

    void deleteMatch();

    void drawButtons();

    void createMatchFromSelection();

    void handleMouseWheel(sf::Vector2f mouse, float delta);

    void clampScroll();

    void createCards();

    void handleMousePressed(
        sf::Vector2f mouse);

    void handleMouseReleased(
        sf::Vector2f mouse);

    void handleMouseMoved(
        sf::Vector2f mouse);


    void swapCards(
        OrganizationCard& first,
        OrganizationCard& second);

    void drawCards();
};