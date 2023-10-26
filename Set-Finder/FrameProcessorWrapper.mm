//
//  FrameProcessorWrapper.mm
//  Set-Finder
//
//  Created by JD del Alamo on 7/19/23.
//

#import <opencv2/opencv.hpp>
#import <opencv2/imgcodecs/ios.h>
#import "FrameProcessorWrapper.hpp"
#import "FrameProcessor.hpp"
#import <Foundation/Foundation.h>

@implementation FrameProcessorWrapper

- (FrameProcessorWrapper*) init: (int) maxThreads {
   // Create C++ instance
   _frameProcessor = (void*)new FrameProcessor(maxThreads);
   return self;
}

- (UIImage*) process: (UIImage*) image {
   cv::Mat frame;

   // Convert UIImage to OpenCV Mat type
   UIImageToMat(image, frame, true);

   // Convert colorspace
   cv::cvtColor(frame, frame, cv::COLOR_RGBA2BGR);

   FrameProcessor* frameProcessor = (FrameProcessor*)_frameProcessor;
   frameProcessor->process(frame);

   // Convert colorspace back
   cv::cvtColor(frame, frame, cv::COLOR_BGR2RGBA);

   return MatToUIImage(frame);
}

@end
