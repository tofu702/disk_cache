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

@interface DCDiskCache ()

@property DCCache cache;


@end

@implementation DCDiskCache



+ (id)loadCacheWithPath:(NSString *)cachePath {
  
}

- (id)initWithPath:(NSString *)cachePath numLines:(NSUInteger)numLines maxBytes:(NSUInteger) maxBytes {
  self = [super init];
  if (self) {
    self.cache = DCMake((char*)[cachePath cStringUsingEncoding:NSASCIIStringEncoding],
                        (uint32_t)numLines,
                        (uint64_t)maxBytes);
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


@end
