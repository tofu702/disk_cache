//
//  ImageCrawler.m
//  DiskCacheTest
//
//  Created by Steven Stanek on 9/8/13.
//  Copyright (c) 2013 stevenstanek. All rights reserved.
//

#import "ImageCrawler.h"

@interface ImageCrawler ()

@property (nonatomic) NSString *baseURL;

- (NSData *)fetchDataFromURL:(NSString *) url;
- (NSArray *)findURLsInString:(NSString *) s;
- (NSString *)transformURLStringToAbsoluteURL:(NSString *)potentialRelativeURL;

@end

@implementation ImageCrawler


@synthesize imagePageURL;



- (id)initWithImagePageURL:(NSString *)aImagePageURL {
  self = [super init];
  if (self) {
    self.imagePageURL = aImagePageURL;
  }
  return self;
}

- (void)fetchImagePageAndParseImageURLs {
  NSData *data = [self fetchDataFromURL:self.imagePageURL];
  NSString *message = [NSString stringWithFormat:@"%d bytes retrieved", [data length]];
  [[[UIAlertView alloc] initWithTitle:@"Data Retrieved"
                             message:message
                            delegate:nil
                   cancelButtonTitle:@"OK"
                    otherButtonTitles:nil] show];
  NSString *dataAsString = [[NSString alloc] initWithData:data encoding:NSASCIIStringEncoding];
  self.imageFileURLS = [self findURLsInString:dataAsString];
}


#pragma mark Setter/Getter Overrides
#pragma mark -


- (void)setImagePageURL:(NSString *)aImagePageURL {
  imagePageURL = aImagePageURL;
  
  if ([imagePageURL characterAtIndex:[imagePageURL length] -1] == '/') {
    self.baseURL = aImagePageURL;
  } else {
    NSURL *url = [NSURL URLWithString:aImagePageURL];
    self.baseURL = [url URLByDeletingLastPathComponent].absoluteString;
  }
}



#pragma mark Private Helpers
#pragma mark -


- (NSData *)fetchDataFromURL:(NSString *) url {
  NSURLRequest *request = [NSURLRequest requestWithURL:[NSURL URLWithString:url]];
  NSURLResponse *response = [[NSURLResponse alloc] init];
  NSError *err = [[NSError alloc] init];
  
  NSData *returnme = [NSURLConnection sendSynchronousRequest:request
                                           returningResponse:&response
                                                       error:&err];
  if (!returnme) {
    NSLog(@"Failed to retrieve data for URL %@, response: %@, error: %@", url, response, err);
  }
  return returnme;
}

- (NSArray *) findURLsInString:(NSString *) s {
  NSString *re_str = @"[^= \' >\"]+.jpg";
  NSRegularExpression *re = [NSRegularExpression regularExpressionWithPattern:re_str
                                                                      options:NSRegularExpressionCaseInsensitive error:nil];
  NSArray *matches =  [re matchesInString:s options:0 range:NSMakeRange(0, [s length])];
  
  NSMutableArray *results = [NSMutableArray array];
  for (NSTextCheckingResult *res in matches) {
    NSString *sub_string = [s substringWithRange:res.range];
    [results addObject:[self transformURLStringToAbsoluteURL:sub_string]];
  }
  
  NSLog(@"Matches: %@", results);
  return results;
}

// Take a potentially relative URL and make it absolute
- (NSString *)transformURLStringToAbsoluteURL:(NSString *)potentialRelativeURL {
  // If the string starts with http:// then it's not relative
  if ([[potentialRelativeURL lowercaseString] rangeOfString:@"http://"].location != NSNotFound) {
    return potentialRelativeURL;
  }
  
  // Sigh, it is relative; let's build a good one...
  return [[NSURL URLWithString:self.baseURL] URLByAppendingPathComponent:potentialRelativeURL].absoluteString;
}


@end
