//
//  FrameProcessor.hpp
//  Set-Finder
//
//  Created by JD del Alamo on 7/19/23.
//

#pragma once

#include "SetGame.hpp"

#include <opencv2/opencv.hpp>

#include <unordered_map>
#include <unordered_set>
#include <vector>

const int GAUSSIAN_RADIUS = 101;

class FrameProcessor {
public:
   FrameProcessor(bool debug=false) : _debug(debug) {}

   void process(cv::Mat& frame);

private:
   bool cardFilter(
      const std::tuple<int, std::vector<cv::Point>>& indexedContour,
      const std::vector<cv::Vec4i>& hierarchy,
      std::unordered_set<int>& cardIndices) const;

   bool shapeFilter(
      const std::tuple<int, std::vector<cv::Point>>& indexedContour,
      const std::vector<cv::Vec4i>& hierarchy,
      const std::unordered_set<int>& cardIndices) const;

   static void classifyShape(
      const std::tuple<int, std::vector<cv::Point>>& indexedShape,
      const std::vector<cv::Vec4i>& hierarchy,
      const cv::Mat& frame,
      std::unordered_map<int, std::vector<SetGame::Shape>>& cardIndexToShapeMap);

   static void scaleContour(
      std::vector<cv::Point>& contour,
      const float scalar);

   static cv::Point scalePoint(
      const cv::Point& point,
      const int cx,
      const int cy,
      const float scalar);

   static double colorDifference(
      const cv::Scalar& color1,
      const cv::Scalar& color2);

   std::vector<SetGame::Set> getSets(
      const std::vector<SetGame::Card> indexedCards) const;

private:
   bool _debug;
   bool _initialized = false;
   float _minCardArea = 0;
   float _minShapeArea = 0;
   int _frameNum = 0;
   cv::Size _gaussianSize = cv::Size(GAUSSIAN_RADIUS, GAUSSIAN_RADIUS);
   cv::Mat _gaussianResult;
   int _thresholdVal = 0;
};

