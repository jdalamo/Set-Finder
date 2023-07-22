//
//  FrameProcessorWrapper.hpp
//  Set-Finder
//
//  Created by JD del Alamo on 7/19/23.
//

#pragma once

#import <UIKit/UIKit.h>
#import <Foundation/Foundation.h>

@interface FrameProcessorWrapper : NSObject {
@private
    void* _frameProcessor;
}
- (FrameProcessorWrapper*) init;
- (UIImage*) process: (UIImage*) image;
@end
