//
//  SetGame.hpp
//  Set-Spotter
//
//  Created by JD del Alamo on 7/19/23.
//

#pragma once

#include <vector>
#include <string>

namespace SetGame {

enum class Color {
   RED = 0,
   GREEN = 1,
   PURPLE = 2,
   UNKNOWN = 3
};

const std::vector<std::string> COLOR_TO_STRING = { "RED", "GREEN", "PURPLE", "UNKNOWN" };

enum class Symbol {
   DIAMOND = 0,
   SQUIGGLE = 1,
   OVAL = 2,
   UNKNOWN = 3
};

const std::vector<std::string> SYMBOL_TO_STRING = { "DIAMOND", "SQUIGGLE", "OVAL", "UNKNOWN" };

enum class Shading {
   SOLID = 0,
   STRIPED = 1,
   OPEN = 2,
   UNKNOWN = 3
};

const std::vector<std::string> SHADING_TO_STRING = { "SOLID", "STRIPED", "OPEN", "UNKNOWN" };

struct Shape {
   Shape(Color color, Symbol symbol, Shading shading) :
      color(color), symbol(symbol), shading(shading) {}

   bool operator==(const Shape& other) const;
   bool operator!=(const Shape& other) const;
   bool operator<(const Shape& other) const;

   std::string toString() const;

   Color color;
   Symbol symbol;
   Shading shading;
};

struct Card {
   Card(Shape shape, int count, int contourIndex) :
      shape(shape), count(count), contourIndex(contourIndex) {}

   bool operator<(const Card& other) const;

   std::string toString() const;

   Shape shape;
   int count;
   int contourIndex;
};

struct Set {
   Set(std::vector<Card> cards) : cards(cards) {
      std::sort(this->cards.begin(), this->cards.end());
   }

   static bool isSet(
      const Card& c0,
      const Card& c1,
      const Card& c2);

   bool operator<(const Set& other) const;

   std::string toString() const;

   std::vector<Card> cards;
};

} // namespace SetGame
