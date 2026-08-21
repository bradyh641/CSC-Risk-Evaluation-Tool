#pragma once

#include <SFML/Graphics.hpp>

struct OrganizationCard
{
    int matchIndex = -1;

    bool isFunding = false;

    bool dragging = false;

    bool selected = false;

    bool isVisible = false;

    sf::RectangleShape rectangle;

    sf::Text text;

    sf::Vector2f offset;


    bool contains(sf::Vector2f point) const
    {
        return rectangle.getGlobalBounds()
            .contains(point);
    }

    void setPosition(sf::Vector2f position)
    {
        rectangle.setPosition(position);

        text.setPosition(
            position.x + 10,
            position.y + 7
        );
    }

    bool operator<(const OrganizationCard& other) const
    {
        return this->text.getString().toAnsiString() < other.text.getString().toAnsiString();
    }

    bool operator>(const OrganizationCard& other) const
    {
        return this->text.getString().toAnsiString() > other.text.getString().toAnsiString();
    }

    bool operator!=(const OrganizationCard& other) const {
        return this->text.getString().toAnsiString() != other.text.getString().toAnsiString();
    }
};