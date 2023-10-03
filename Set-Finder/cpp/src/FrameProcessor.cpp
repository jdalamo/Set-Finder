//
//  FrameProcessor.cpp
//  Set-Finder
//
//  Created by JD del Alamo on 7/19/23.
//

#include "FrameProcessor.hpp"
#include "SetGame.hpp"

const float MIN_CARD_AREA_PERCENTAGE = 0.007;

// Thresholding constants
/**
 * BLOCK_SIZE defines the size of the pixel neighborhood used to
 * calculate the mean intensity to threshold a given pixel.
 */
const int BLOCK_SIZE = 11;
/**
 * C defines a constant that is subtracted from the calculated mean.
 */
const int C = 8;

const int CHILD_HIERARCHY_INDEX = 2;
const float CARD_APPROX_ACCURACY = 0.04;
const float MIN_ASPECT_RATIO = 1.0;
const float MAX_ASPECT_RATIO = 2.0;
const float MIN_CARD_INTENSITY_PERCENT = 0.45;

const int PARENT_HIERARCHY_INDEX = 3;
const float SHAPE_APPROX_ACCURACY = 0.08;

// Color constants
const int RED_MIN = 340;
const int RED_MAX = 65;
const int PURPLE_MIN = 260;
const int PURPLE_MAX = 340;
const int GREEN_MIN = 65;
const int GREEN_MAX = 180;

const float SHAPE_MATCH_DIAMOND_THRESHOLD = 0.065;
const float SOLIDITY_SQUIGGLE_PILL_THRESHOLD = 0.9;
const int RED_BLUE_DIFF_PURPLE_THRESHOLD = 15;
const float SOLID_SHADING_PERCENT_THRESHOLD = 0.53;
const float SOLID_SHADING_PURPLE_PERCENT_THRESHOLD = 0.25;

const float BORDER_CONTOUR_SCALAR = -0.2;
const float FILL_CONTOUR_SCALAR = -0.4;
const float OUTLINE_CONTOUR_EXTERIOR_SCALAR = 0.3;
const float OUTLINE_CONTOUR_INTERIOR_SCALAR = 0.1;

const int OPEN_SHADING_CONTRAST_THRESHOLD = 25;
const int STRIPED_SHADING_CONTRAST_THRESHOLD = 125;

const float HIGHLIGHT_SCALE_FACTOR = 0.2;

const std::vector<cv::Scalar> SET_HIGHLIGHT_COLORS = {
   cv::Scalar(255,255,0),
   cv::Scalar(255,97,237),
   cv::Scalar(0,229,255),
   cv::Scalar(255,106,0),
   cv::Scalar(21,255,0),
   cv::Scalar(144,0,255)
};

void
FrameProcessor::process(cv::Mat& frame)
{
   /**
    * If this is the first frame processed we need to set some member
    * variables.
    */
   if (!_initialized) {
      _minCardArea = frame.size().width * frame.size().height * MIN_CARD_AREA_PERCENTAGE;
      _minShapeArea = _minCardArea / 7;
      _initialized = true;
   }

   cv::Mat grayScaleFrame;
   cv::cvtColor(frame, grayScaleFrame, cv::COLOR_BGR2GRAY);

   cv::Mat threshold;
   cv::adaptiveThreshold(grayScaleFrame, threshold, 255, cv::ADAPTIVE_THRESH_MEAN_C, cv::THRESH_BINARY,
      BLOCK_SIZE, C);
   std::vector<std::vector<cv::Point>> contours;
   std::vector<cv::Vec4i> hierarchy;
   cv::findContours(threshold, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_NONE);
   if (contours.empty()) return;

   // Create indexed contours
   int contourIndex = 0;
   std::vector<IndexedContour> indexedContours;
   std::transform(contours.begin(), contours.end(), std::back_inserter(indexedContours),
      [&](std::vector<cv::Point>& contour) {
         contourIndex++;
         return IndexedContour { contourIndex - 1, contour };
      }
   );

   // Filter cards
   std::vector<IndexedContour> indexedCardContours;
   std::unordered_set<int> cardIndices;
   std::copy_if(indexedContours.begin(), indexedContours.end(), std::back_inserter(indexedCardContours),
      [&](IndexedContour& indexedContour) {
         return cardFilter(indexedContour,
                           hierarchy,
                           cardIndices);
      }
   );
   if (indexedCardContours.empty()) return;

   // Filter shapes
   std::vector<IndexedContour> indexedShapeContours;
   std::copy_if(indexedContours.begin(), indexedContours.end(), std::back_inserter(indexedShapeContours),
      [&](IndexedContour& indexedContour) {
         return shapeFilter(indexedContour,
                            hierarchy,
                            cardIndices);
      }
   );
   if (indexedShapeContours.empty()) return;

   // Classify shapes
   std::unordered_map<int, std::vector<SetGame::Shape>> cardIndexToShapesMap;
   pthread_mutex_t mapMutex;
   pthread_mutex_init(&mapMutex, NULL);
   _threadPool.parallelize<std::vector<IndexedContour>>(classifyShapes, indexedShapeContours,
      [&]() -> ClassifyShapeArg* {
         ClassifyShapeArg* arg = new ClassifyShapeArg(
            hierarchy, frame, cardIndexToShapesMap, &mapMutex);

         return arg;
      }
   );
   pthread_mutex_destroy(&mapMutex);
   if (cardIndexToShapesMap.empty()) return;

   // Verify shapes and construct cards
   std::vector<SetGame::Card> indexedCards;
   for (const auto& entry : cardIndexToShapesMap) {
      int cardIndex = entry.first;
      const std::vector<SetGame::Shape>& shapes = entry.second;

      // The max number of shapes per card is 3
      if (shapes.size() > 3) continue;

      // Check that all shapes within the same card are equal
      bool allEqual = std::adjacent_find(shapes.begin(), shapes.end(),
         [](const SetGame::Shape& shape1, const SetGame::Shape& shape2) {
            return shape1 != shape2;
         }
      ) == shapes.end();
      if (!allEqual) continue;

      // TODO: check shape positions relative to card and compare to number of shapes
      SetGame::Card card(shapes[0], shapes.size(), cardIndex);
      indexedCards.push_back(card);
   }

   // Get sets
   std::vector<SetGame::Set> sets = getSets(indexedCards);

   // Highlight sets
   std::sort(sets.begin(), sets.end());
   std::unordered_set<int> highlightedCards;
   int i = 0;
   for (const auto& set : sets) {
      int colorIndex = i % SET_HIGHLIGHT_COLORS.size();
      cv::Scalar color = SET_HIGHLIGHT_COLORS[colorIndex];
      for (const auto& card : set.cards) {
         if (highlightedCards.find(card.contourIndex) != highlightedCards.end()) {
            // We've already highlighted this card--expand the contour
            scaleContour(contours[card.contourIndex], HIGHLIGHT_SCALE_FACTOR);
         }
         cv::drawContours(frame, contours, card.contourIndex, color, 9);
         highlightedCards.insert(card.contourIndex);
      }
      i++;
   }
}

bool
FrameProcessor::cardFilter(
   const IndexedContour& indexedContour,
   const std::vector<cv::Vec4i>& hierarchy,
   std::unordered_set<int>& cardIndices) const
{
   int index = std::get<0>(indexedContour);
   const std::vector<cv::Point>& contour = std::get<1>(indexedContour);

   if (hierarchy[index][CHILD_HIERARCHY_INDEX] < 0) {
      // Contour has no child contours
      return false;
   }

   // Area check
   if (cv::contourArea(contour) < _minCardArea) return false;

   // Approximate contour is rectangle check
   double peri = cv::arcLength(contour, true) * CARD_APPROX_ACCURACY;
   std::vector<cv::Point> approx;
   cv::approxPolyDP(contour, approx, peri, true);
   if (approx.size() != 4) return false;

   // Aspect ratio check
   cv::Rect rect = cv::boundingRect(contour);
   float aspectRatio = ((float)std::max(rect.height, rect.width) / std::min(rect.height, rect.width));
   if (aspectRatio < MIN_ASPECT_RATIO || aspectRatio > MAX_ASPECT_RATIO) return false;

   cardIndices.insert(index);
   return true;
}

bool
FrameProcessor::shapeFilter(
   const IndexedContour& indexedContour,
   const std::vector<cv::Vec4i>& hierarchy,
   const std::unordered_set<int>& cardIndices) const
{
   int index = std::get<0>(indexedContour);
   const std::vector<cv::Point>& contour = std::get<1>(indexedContour);

   int parentIndex = hierarchy[index][PARENT_HIERARCHY_INDEX];
   if (cardIndices.find(parentIndex) == cardIndices.end()) {
      // Current contour is not contained within a card
      return false;
   }

   // Area check
   if (cv::contourArea(contour) < _minShapeArea) return false;

   // Approximate contour is rectangle check
   double peri = cv::arcLength(contour, true) * SHAPE_APPROX_ACCURACY;
   std::vector<cv::Point> approx;
   cv::approxPolyDP(contour, approx, peri, true);
   if (approx.size() != 4) return false;

   return true;
}

void
FrameProcessor::classifyShapes(
   void* voidArg)
{
   ClassifyShapeArg* arg = (ClassifyShapeArg*)voidArg;
   std::for_each(arg->start, arg->end,
      [&](const IndexedContour& indexedShape) {
         classifyShape(indexedShape, arg->hierarchy, arg->frame,
            arg->cardIndexToShapesMap, arg->mapMutex);
      }
   );
}

void
FrameProcessor::classifyShape(
   const IndexedContour& indexedShape,
   const std::vector<cv::Vec4i>& hierarchy,
   const cv::Mat& frame,
   std::unordered_map<int, std::vector<SetGame::Shape>>& cardIndexToShapeMap,
   pthread_mutex_t* mapMutex)
{
   int index = std::get<0>(indexedShape);
   const std::vector<cv::Point>& contour = std::get<1>(indexedShape);

   // Detect contour's symbol
   double peri = cv::arcLength(contour, true) * SHAPE_APPROX_ACCURACY;
   std::vector<cv::Point> approx;
   cv::approxPolyDP(contour, approx, peri, true);
   double shapeMatchRatio = cv::matchShapes(contour, approx, cv::CONTOURS_MATCH_I1, 0);
   SetGame::Symbol symbol;
   if (shapeMatchRatio < SHAPE_MATCH_DIAMOND_THRESHOLD) {
      symbol = SetGame::Symbol::DIAMOND;
   } else {
      std::vector<cv::Point> hull;
      cv::convexHull(contour, hull);
      double solidityRatio = cv::contourArea(contour) / cv::contourArea(hull);
      symbol = (solidityRatio < SOLIDITY_SQUIGGLE_PILL_THRESHOLD) ?
         SetGame::Symbol::SQUIGGLE :
         SetGame::Symbol::OVAL;
   }

   // Detect contour's color
   cv::Moments M = cv::moments(contour);
   int cx = (int)(M.m10 / M.m00);
   int cy = (int)(M.m01 / M.m00);

   std::vector<cv::Point> borderContour;
   std::transform(contour.begin(), contour.end(), std::back_inserter(borderContour),
      [&](const cv::Point& point) {
         return scalePoint(point, cx, cy, BORDER_CONTOUR_SCALAR);
      }
   );
   std::vector<std::vector<cv::Point>> border = { borderContour };

   cv::Mat borderContourMask = cv::Mat::zeros(frame.size(), CV_8U);
   std::vector<std::vector<cv::Point>> c = { contour };
   cv::drawContours(borderContourMask, c, 0, cv::Scalar(255), -1);
   cv::drawContours(borderContourMask, border, 0, cv::Scalar(0), -1);
   cv::Scalar meanColor = cv::mean(frame, borderContourMask);

   int blue = (int)meanColor[0];
   int green = (int)meanColor[1];
   int red = (int)meanColor[2];

   std::tuple<int, int, int> hsv =
      bgrToHsv(blue, green, red);
   const int hue = std::get<0>(hsv);
   SetGame::Color color;
   if (hue > RED_MIN || hue <= RED_MAX) {
      color = SetGame::Color::RED;
   } else if (hue > PURPLE_MIN && hue <= PURPLE_MAX) {
      color = SetGame::Color::PURPLE;
   } else if (hue > GREEN_MIN && hue <= GREEN_MAX) {
      color = SetGame::Color::GREEN;
   } else {
      std::cout << "ERROR: undetected color: " << hue << "|" <<
         std::get<1>(hsv) << "|" << std::get<2>(hsv) << std::endl;
      color = SetGame::Color::UNKNOWN;
   }

   // Detect contour's shading
   std::vector<cv::Point> fillContour, outlineContourExterior, outlineContourInterior;
   std::transform(contour.begin(), contour.end(), std::back_inserter(fillContour),
      [&](const cv::Point& point) {
         return scalePoint(point, cx, cy, FILL_CONTOUR_SCALAR);
      }
   );
   std::transform(contour.begin(), contour.end(), std::back_inserter(outlineContourExterior),
      [&](const cv::Point& point) {
         return scalePoint(point, cx, cy, OUTLINE_CONTOUR_EXTERIOR_SCALAR);
      }
   );
   std::transform(contour.begin(), contour.end(), std::back_inserter(outlineContourInterior),
      [&](const cv::Point& point) {
         return scalePoint(point, cx, cy, OUTLINE_CONTOUR_INTERIOR_SCALAR);
      }
   );

   std::vector<std::vector<cv::Point>> outlineExterior = { outlineContourExterior };
   std::vector<std::vector<cv::Point>> outlineInterior = { outlineContourInterior };
   cv::Mat outlineMask = cv::Mat::zeros(frame.size(), CV_8U);
   cv::drawContours(outlineMask, outlineExterior, 0, cv::Scalar(255), -1);
   cv::drawContours(outlineMask, outlineInterior, 0, cv::Scalar(0), -1);
   cv::Scalar meanBorderColor = cv::mean(frame, outlineMask);

   std::vector<std::vector<cv::Point>> fill = { fillContour };
   cv::Mat fillMask = cv::Mat::zeros(frame.size(), CV_8U);
   cv::drawContours(fillMask, fill, 0, cv::Scalar(255), -1);
   cv::Scalar meanFillColor = cv::mean(frame, fillMask);

   const double colorDiff = colorDifference(meanBorderColor, meanFillColor);
   SetGame::Shading shading;
   if (colorDiff < OPEN_SHADING_CONTRAST_THRESHOLD) {
      shading = SetGame::Shading::OPEN;
   } else if (colorDiff < STRIPED_SHADING_CONTRAST_THRESHOLD) {
      shading = SetGame::Shading::STRIPED;
   } else {
      shading = SetGame::Shading::SOLID;
   }

   SetGame::Shape shape(color, symbol, shading);
   const int parentIndex = hierarchy[index][PARENT_HIERARCHY_INDEX];
   pthread_mutex_lock(mapMutex);
   cardIndexToShapeMap[parentIndex].push_back(shape);
   pthread_mutex_unlock(mapMutex);
}

void
FrameProcessor::scaleContour(
   std::vector<cv::Point>& contour,
   const float scalar)
{
   cv::Moments M = cv::moments(contour);
   int cx = (int)(M.m10 / M.m00);
   int cy = (int)(M.m01 / M.m00);

   std::for_each(contour.begin(), contour.end(),
      [&](cv::Point& point) {
         point = scalePoint(point, cx, cy, scalar);
      }
   );
}

cv::Point
FrameProcessor::scalePoint(
   const cv::Point& point,
   const int cx,
   const int cy,
   const float scalar)
{
   int newX = (int)(point.x - ((cx - point.x) * scalar));
   int newY = (int)(point.y - ((cy - point.y) * scalar));

   return cv::Point(newX, newY);
}

double
FrameProcessor::colorDifference(
   const cv::Scalar& color1,
   const cv::Scalar& color2)
{
   // https://en.wikipedia.org/wiki/Color_difference

   int red1 = (int)color1[0];
   int green1 = (int)color1[1];
   int blue1 = (int)color1[2];

   int red2 = (int)color2[0];
   int green2 = (int)color2[1];
   int blue2 = (int)color2[2];

   double redAvg = (red1 + red2) / 2;
   int redDelta = red1 - red2;
   int greenDelta = green1 - green2;
   int blueDelta = blue1 - blue2;

   double redDiff = (redAvg / 256 + 2) * pow(redDelta, 2);
   int greenDiff = pow(greenDelta, 2) * 4;
   double blueDiff = ((255 - redAvg) / 256 + 2) * pow(blueDelta, 2);

   return std::sqrt(redDiff + greenDiff + blueDiff);
}

std::tuple<int, int, int>
FrameProcessor::bgrToHsv(
   const int b,
   const int g,
   const int r)
{
   double blue = b / 255.0;
   double green = g / 255.0;
   double red = r / 255.0;

   double cMax = std::max({ blue, green, red });
   double cMin = std::min({ blue, green, red });
   double cDiff = cMax - cMin;

   int hue;
   int saturation;
   int value;

   if (cMax == cMin) {
      hue = 0;
   } else if (cMax == red) {
      hue = (int)(60 * ((green - blue) / cDiff) + 360) % 360;
   } else if (cMax == green) {
      hue = (int)(60 * ((blue - red) / cDiff) + 120) % 360;
   } else if (cMax == blue) {
      hue = (int)(60 * ((red - green) / cDiff) + 240) % 360;
   }

   if (cMax == 0) {
      saturation = 0;
   } else {
      saturation = std::round(cDiff / cMax * 100);
   }

   value = std::round(cMax * 100);

   std::tuple<int, int, int> output(hue, saturation, value);
   return output;
}

std::vector<SetGame::Set>
FrameProcessor::getSets(
   const std::vector<SetGame::Card> indexedCards) const
{
   std::vector<SetGame::Set> sets;
   for (int i = 0; i < indexedCards.size(); i++) {
      const SetGame::Card& cardI = indexedCards[i];
      for (int j = i + 1; j < indexedCards.size(); j++) {
         const SetGame::Card& cardJ = indexedCards[j];
         for (int k = j + 1; k < indexedCards.size(); k++) {
            const SetGame::Card& cardK = indexedCards[k];
            if (SetGame::Set::isSet(cardI, cardJ, cardK)) {
               std::vector<SetGame::Card> cards = { cardI, cardJ, cardK };
               SetGame::Set set(cards);
               sets.push_back(set);
            }
         }
      }
   }

   return sets;
}
