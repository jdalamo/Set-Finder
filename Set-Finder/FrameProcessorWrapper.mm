//
//  FrameProcessorWrapper.mm
//  Set-Finder
//
//  Created by JD del Alamo on 7/19/23.
//

#import <opencv2/opencv.hpp>
#import <opencv2/imgcodecs/ios.h>
#import "FrameProcessorWrapper.h"
#import "FrameProcessor.h"
#import <Foundation/Foundation.h>

@implementation FrameProcessorWrapper

- (FrameProcessorWrapper*) init: (int) maxThreads showSets:(bool) showSets {
   // Create C++ instance
   _frameProcessor = (void*)new FrameProcessor(maxThreads, showSets);
   return self;
}

- (UIImage*) process: (UIImage*) image {
   cv::Mat frame;

   // Convert UIImage to OpenCV Mat type
   UIImageToMat(image, frame, true);

   // Convert colorspace
   cv::cvtColor(frame, frame, cv::COLOR_RGBA2BGR);

   FrameProcessor* frameProcessor = (FrameProcessor*)_frameProcessor;
   frameProcessor->Process(frame);

   // Convert colorspace back
   cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA);

   return MatToUIImage(frame);
}

- (bool) getShowSets {
   FrameProcessor* frameProcessor = (FrameProcessor*)_frameProcessor;
   return frameProcessor->GetShowSets();
}

- (void) setShowSets: (bool) show {
   FrameProcessor* frameProcessor = (FrameProcessor*)_frameProcessor;
   frameProcessor->SetShowSets(show);
}

- (int) getNumSetsInFrame {
   FrameProcessor* frameProcessor = (FrameProcessor*)_frameProcessor;
   return frameProcessor->GetNumSetsInFrame();
}

@end
