//
//  DCDiskCache.h
//  DiskCacheTest
//
//  Created by Steven Stanek on 9/16/13.
//  Copyright (c) 2013 stevenstanek. All rights reserved.
//

/**
 * This file specifies the interface for a DCDiskCache. A DCDiskCache is built by storing cache entries
 * to disk as individual files and with a memory mapped file for book keeping. This file is a wrapper
 * around the C library and it should be extremely fast.
 * A few notes:
 * 1. The cache is implemented as set associative cache, meaning that a given key has several candidate positions
 *    and a new insert will automatically replace the worst entry at those candidate positions.
 * 2. The cache has a maximum size, when this size is hit, a substantial number of entries will be evicted
 *    in order to avoid doing the reasonably expensive eviction operation on every subsequent insert.
 */

#import <Foundation/Foundation.h>

FOUNDATION_EXPORT const NSUInteger DC_NO_CACHE_LIMIT_MAX_BYTES;

@interface DCDiskCache : NSObject


/**
 * A convenience classmethod to load a previously stored cache from the default path. If one does not exist,
 * create it and return it. Default values are picked for path, numLines and maxBytes.
 * NOTE: THIS IS NOT A SINGLETON! Calling this method multiple times will return multiple distinct cache 
 * objects.
 */
+ (id)loadOrCreateCacheWithDefaultPath;

/**
 * A convenience classmethod to load a previous stored cache from the provide path. If one does not exist,
 * create it with the provided number of line and maxBytes.
 * NOTE: THIS IS NOT A SINGLETON! Calling this method multiple times will return multiple distinct cache
 * objects.
 * NOTE: numLines and maxBytes are only used if creating a new disk cache
 */
+ (id)loadOrCreateCacheWithPath:(NSString *)cachePath
                desiredNumLines:(NSUInteger)numLines
                desiredMaxBytes:(NSUInteger)maxBytes;


/**
 * Designated Initializer
 * Load a disk cache at the desired path if one exists, if none does exist, create a new one with the
 * provided line count and max bytes.
 * Arguments:
 * cachePath: A valid path to a directory that the disk cache should use, this directory should be empty
 * numLines: The number of lines (the maximum number of entries) in the cache
 * maxBytes: The maximum total size of the cache, specify DC_NO_CACHE_LIMIT_MAX_BYTES
 */
- (id)initWithPath:(NSString *)cachePath numLines:(NSUInteger)numLines maxBytes:(NSUInteger) maxBytes;



/**
 *
 */
- (void)setItem:(id<NSCoding>)item forKey:(NSString*)key;

/**
 *
 */
- (id)itemForKey:(NSString *)key;

/**
 * NOT YET IMPLEMENTED
 */
- (void)removeItemForKey:(NSString *)key;

@end
