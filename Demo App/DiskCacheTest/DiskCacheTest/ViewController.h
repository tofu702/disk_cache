//
//  ViewController.h
//  DiskCacheTest
//
//  Created by Steven Stanek on 9/8/13.
//  Copyright (c) 2013 stevenstanek. All rights reserved.
//

#import <UIKit/UIKit.h>

#import "DCDiskCache.h"
#import "ImageCrawler.h"

@interface ViewController : UIViewController

// IBOutlets
@property (weak, nonatomic) IBOutlet UITextField *urlTextField;
@property (weak, nonatomic) IBOutlet UITextField *cacheLinesTextField;
@property (weak, nonatomic) IBOutlet UITextField *cacheKBTextField;
@property (weak, nonatomic) IBOutlet UITextView *testSummaryTextView;
@property (weak, nonatomic) IBOutlet UITextView *cacheStatsTextView;

//Other Properties
@property (nonatomic) ImageCrawler *crawler;
@property (nonatomic) DCDiskCache *cache;


- (IBAction)startFetch;
- (IBAction)startRetrievalTest;
- (IBAction)clearCache;

@end
