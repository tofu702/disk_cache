//
//  DCDiskCache.m
//  DiskCacheTest
//
//  Created by Steven Stanek on 9/16/13.
//  Copyright (c) 2013 stevenstanek. All rights reserved.
//

#import "DCDiskCache.h"
#import "disk_cache.h"

const NSUInteger DC_NO_CACHE_LIMIT_MAX_BYTES = 0;
const NSUInteger DC_DEFAULT_NUM_LINES = 32*128;
const NSUInteger DC_DEFAULT_MAX_BYTES = 32*1024*1024;

@interface DCDiskCache ()

@property DCCache cache;

+ (NSString *)defaultCachePath;

@end

@implementation DCDiskCache


+ (id)loadOrCreateCacheWithDefaultPath {
  NSString *cachePath = [DCDiskCache defaultCachePath];
  NSLog(@"Using path: %@", cachePath);
  return [DCDiskCache loadOrCreateCacheWithPath:cachePath
                                desiredNumLines:DC_DEFAULT_NUM_LINES
                                desiredMaxBytes:DC_DEFAULT_MAX_BYTES];
}

+ (id)loadOrCreateCacheWithPath:(NSString *)cachePath
                desiredNumLines:(NSUInteger)numLines
                desiredMaxBytes:(NSUInteger)maxBytes {
  return [[DCDiskCache alloc] initWithPath:cachePath numLines:numLines maxBytes:maxBytes];
}

- (id)initWithPath:(NSString *)cachePath numLines:(NSUInteger)numLines maxBytes:(NSUInteger) maxBytes {
  self = [super init];
  if (self) {
    char *cString = (char *) [cachePath cStringUsingEncoding:NSASCIIStringEncoding];
    self.cache = DCLoad(cString);
    if (!self.cache) {
      // If this path doesn't exist; create it
      [[NSFileManager defaultManager] createDirectoryAtPath:cachePath
                                withIntermediateDirectories:YES
                                                 attributes:nil
                                                      error:nil];
      self.cache = DCMake((char*)[cachePath cStringUsingEncoding:NSASCIIStringEncoding],
                          (uint32_t)numLines,
                          (uint64_t)maxBytes);
    }
  }
  return self;
}

- (void)dealloc {
  DCCloseAndFree(self.cache);
}

- (void)setItem:(id<NSCoding>)item forKey:(NSString*)key {
  NSData *archivedData = [NSKeyedArchiver archivedDataWithRootObject:item];
  DCAdd(self.cache,
        (char *)[key cStringUsingEncoding:NSASCIIStringEncoding],
        (uint8_t *) [archivedData bytes],
        (uint64_t)[archivedData length]);
  
}

- (id)itemForKey:(NSString *)key {
  DCData foundEntry = DCLookup(self.cache, (char *)[key cStringUsingEncoding:NSASCIIStringEncoding]);
  if (!foundEntry) {
    return nil;
  }
  
  NSData *data = [NSData dataWithBytes:foundEntry->data length:foundEntry->data_len];
  return [NSKeyedUnarchiver unarchiveObjectWithData:data];
}


- (void)removeItemForKey:(NSString *)key {
    //TODO: Implement
}


#pragma mark -
#pragma mark Private Helpers


+ (NSString *)defaultCachePath {
  NSFileManager *filemanager = [NSFileManager defaultManager];
  NSArray *arr = [filemanager URLsForDirectory:NSCachesDirectory inDomains:NSUserDomainMask];
  
  if ([arr count] == 0) {
    return nil;
  }

  NSURL *baseURL = [arr objectAtIndex:0];
  NSURL *finalURL = [baseURL URLByAppendingPathComponent:@"default_DCDiskCache"];
  return [finalURL path];
}

@end
