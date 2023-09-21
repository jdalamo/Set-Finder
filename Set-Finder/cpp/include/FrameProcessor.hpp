//
//  FrameProcessor.hpp
//  Set-Finder
//
//  Created by JD del Alamo on 7/19/23.
//

#pragma once

#include "SetGame.hpp"
#include "ThreadPool.hpp"

#include <opencv2/opencv.hpp>

#include <pthread.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace tp = ThreadPool;

const int GAUSSIAN_RADIUS = 101;

typedef std::tuple<int, std::vector<cv::Point>> IndexedContour;
class ClassifyShapeArg : public tp::PoolTaskArg<std::vector<IndexedContour>> {
public:
   ClassifyShapeArg(
      const std::vector<cv::Vec4i>& _hierarchy,
      const cv::Mat& _frame,
      std::unordered_map<int, std::vector<SetGame::Shape>>& _cardIndexToShapesMap,
      pthread_mutex_t* _mapMutex) :
         hierarchy(_hierarchy),
         frame(_frame),
         cardIndexToShapesMap(_cardIndexToShapesMap),
         mapMutex(_mapMutex) {}

   ClassifyShapeArg() = delete;

   const std::vector<cv::Vec4i>& hierarchy; // Read-only
   const cv::Mat& frame; // Read-only
   std::unordered_map<int, std::vector<SetGame::Shape>>&
      cardIndexToShapesMap; // Write
   pthread_mutex_t* mapMutex;
};

class FrameProcessor {
public:
   FrameProcessor(
      int maxThreads) : _threadPool(maxThreads) {}

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

   static void classifyShapes(
      void* voidArg);

   static void classifyShape(
      const std::tuple<int, std::vector<cv::Point>>& indexedShape,
      const std::vector<cv::Vec4i>& hierarchy,
      const cv::Mat& frame,
      std::unordered_map<int, std::vector<SetGame::Shape>>& cardIndexToShapeMap,
      pthread_mutex_t* mapMutex);

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
   bool _initialized = false;
   tp::ThreadPool _threadPool;
   float _minCardArea = 0;
   float _minShapeArea = 0;
   int _frameNum = 0;
   cv::Size _gaussianSize = cv::Size(GAUSSIAN_RADIUS, GAUSSIAN_RADIUS);
   cv::Mat _gaussianResult;
   int _thresholdVal = 0;
};
