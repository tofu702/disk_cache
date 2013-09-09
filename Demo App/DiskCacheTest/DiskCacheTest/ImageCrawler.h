//
//  ImageCrawler.h
//  DiskCacheTest
//
//  Created by Steven Stanek on 9/8/13.
//  Copyright (c) 2013 stevenstanek. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface ImageCrawler : NSObject

@property (nonatomic) NSString *imagePageURL;
@property (nonatomic) NSArray *imageFileURLS;



/**
 * Set the url to crawl for images
 */
- (id)initWithImagePageURL:(NSString *)imagePageURL;

/**
 * Get Images from the provided imagePageURL and populate the imageFileURLS dictionary
 */
- (void)fetchImagePageAndParseImageURLs;




@end