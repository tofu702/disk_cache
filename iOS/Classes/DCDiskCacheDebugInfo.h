//
//  DCDiskCacheDebugInfo.h
//  DiskCacheTest
//
//  Created by Steven Stanek on 10/27/13.
//  Copyright (c) 2013 stevenstanek. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface DCDiskCacheDebugInfo : NSObject

@property (nonatomic, assign) NSUInteger numLines;
@property (nonatomic, assign) NSUInteger numItems;
@property (nonatomic, assign) NSUInteger currentSizeInBytes;
@property (nonatomic, assign) NSUInteger maxSizeInBytes;


@end
