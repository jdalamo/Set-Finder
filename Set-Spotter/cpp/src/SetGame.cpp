//
//  SetGame.cpp
//  Set-Spotter
//
//  Created by JD del Alamo on 7/19/23.
//

#include "SetGame.h"

namespace SetGame {

bool
Shape::operator==(
   const Shape& other) const
{
   return color == other.color &&
      symbol == other.symbol &&
      shading == other.shading;
}

bool
Shape::operator!=(
   const Shape& other) const
{
   return !(*this == other);
}

bool
Shape::operator<(
   const Shape& other) const
{
   if (static_cast<int>(color) < static_cast<int>(other.color)) return true;
   if (static_cast<int>(other.color) < static_cast<int>(color)) return false;
   if (static_cast<int>(symbol) < static_cast<int>(other.symbol)) return true;
   if (static_cast<int>(other.symbol) < static_cast<int>(symbol)) return false;

   return static_cast<int>(shading) < static_cast<int>(other.shading);
}

std::string
Shape::toString() const
{
   return SHADING_TO_STRING[static_cast<int>(shading)] + std::string(" ") +
          COLOR_TO_STRING[static_cast<int>(color)] + std::string(" ") +
          SYMBOL_TO_STRING[static_cast<int>(symbol)] + "(s)";
}

bool
Card::operator<(
   const Card& other) const
{
   if (count < other.count) return true;
   if (other.count < count) return false;

   return shape < other.shape;
}

std::string
Card::toString() const
{
   return std::to_string(count) + " " + shape.toString();
}

bool
Set::isSet(
   const Card& c0,
   const Card& c1,
   const Card& c2)
{
   auto allSame = [](int i0, int i1, int i2) {
      return i0 == i1 && i1 == i2;
   };
   auto allDiff = [](int i0, int i1, int i2) {
      return i0 != i1 && i1 != i2 && i0 != i2;
   };

   // Check counts
   if (!allSame(c0.count, c1.count, c2.count) &&
       !allDiff(c0.count, c1.count, c2.count)) return false;

   // Check colors
   if (!allSame(
         static_cast<int>(c0.shape.color),
         static_cast<int>(c1.shape.color),
         static_cast<int>(c2.shape.color)) &&
      !allDiff(
         static_cast<int>(c0.shape.color),
         static_cast<int>(c1.shape.color),
         static_cast<int>(c2.shape.color))) return false;

   // Check symbols
   if (!allSame(
         static_cast<int>(c0.shape.symbol),
         static_cast<int>(c1.shape.symbol),
         static_cast<int>(c2.shape.symbol)) &&
      !allDiff(
         static_cast<int>(c0.shape.symbol),
         static_cast<int>(c1.shape.symbol),
         static_cast<int>(c2.shape.symbol))) return false;

   // Check shadings
   if (!allSame(
         static_cast<int>(c0.shape.shading),
         static_cast<int>(c1.shape.shading),
         static_cast<int>(c2.shape.shading)) &&
      !allDiff(
         static_cast<int>(c0.shape.shading),
         static_cast<int>(c1.shape.shading),
         static_cast<int>(c2.shape.shading))) return false;

   // It's a set
   return true;
}

bool
Set::operator<(
   const Set& other) const
{
   /**
    * We only need to consider the first card in each set object's
    * "cards" vector for the following two reasons:
    *
    *   1. There are no duplicate cards in Set
    *   2. Each set object's "cards" vector is already sorted
    *
    * Because of this, we can be assured that the first card in each
    * set object's "cards" vector is different, AND it represents the
    * "smallest" card in that set object's vector.
    */
   return cards[0] < other.cards[0];
}

std::string
Set::toString() const
{
   std::string output = "Set\n====================\n";
   for (const auto& card : cards) {
      output += card.toString() + "\n";
   }
   output += "\n";
   return output;
}

} // namespace SetGame
