//
//  ViewController.m
//  DiskCacheTest
//
//  Created by Steven Stanek on 9/8/13.
//  Copyright (c) 2013 stevenstanek. All rights reserved.
//

#import "ViewController.h"

@interface ViewController ()

@property (nonatomic, assign) NSUInteger totalRetrievals;

- (void)imageFetchCallbackWithURL:(NSString *)url data:(NSData *)data;
- (void)setStringInTextBlock:(NSString *) s;
@end

@implementation ViewController

static const NSUInteger NUM_RETRIEVALS = 1024;

- (void)viewDidLoad
{
    [super viewDidLoad];
  [self attachKeyboardAccessoryViews];
  self.cache = [DCDiskCache loadOrCreateCacheWithDefaultPath];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}

#pragma mark -
#pragma mark ViewDidLoad Helpers

- (UIToolbar*)makeStandardCloseToolBarWithSelector:(SEL) selector {
  UIToolbar *toolbar = [[UIToolbar alloc] initWithFrame:CGRectMake(0, 0, 320, 50)];
  toolbar.items = @[[[UIBarButtonItem alloc]
                     initWithTitle:@"Close"
                     style:UIBarButtonItemStyleDone
                     target:self
                     action:selector]];
  return toolbar;
}

- (void) attachKeyboardAccessoryViews {
  self.urlTextField.inputAccessoryView = [self makeStandardCloseToolBarWithSelector:@selector(closeURLKeyboardClicked)];
  self.cacheLinesTextField.inputAccessoryView = [self makeStandardCloseToolBarWithSelector:@selector(closeCacheLinesKeyboardClicked)];
  self.cacheKBTextField.inputAccessoryView = [self makeStandardCloseToolBarWithSelector:@selector(closeCacheKBKeyboardClicked)];
}



#pragma mark -
#pragma mark Keyboard Selectors

- (void)closeURLKeyboardClicked {
  [self.urlTextField resignFirstResponder];
}

- (void)closeCacheLinesKeyboardClicked {
  [self.cacheLinesTextField resignFirstResponder];
}

- (void)closeCacheKBKeyboardClicked {
  [self.cacheKBTextField resignFirstResponder];
}


#pragma mark -
#pragma mark IBActions

- (IBAction)startFullFetch {
  [self crawlOnly];
  self.totalRetrievals = 0;
  [self.crawler bulkAsyncFetchImagesWithSelector:@selector(imageFetchCallbackWithURL:data:) target:self];
}

- (IBAction)crawlOnly {
  self.crawler = [[ImageCrawler alloc] initWithImagePageURL:self.urlTextField.text];
  [self.crawler fetchImagePageAndParseImageURLs];
}

- (IBAction)startRetrievalTest {
  NSUInteger num_hits = 0;
  NSUInteger num_urls = [self.crawler.imageFileURLS count];
  
  NSDate *startDate = [NSDate date];
  
  for(NSUInteger i=0; i < NUM_RETRIEVALS; i++) {
    NSUInteger idx = rand() % num_urls;
    
     // Pick a random URL, and try to retrieve it
    NSString *key = [self.crawler.imageFileURLS objectAtIndex:idx];
    
    @autoreleasepool {
      if([self.cache itemForKey:key]) {
        num_hits ++;
      }
    }
  }
  
  NSDate *endDate = [NSDate date];
  NSTimeInterval delta = [endDate timeIntervalSinceDate:startDate];
  NSUInteger delta_micros = delta * 1000*1000;
  
  self.testSummaryTextView.text = [NSString stringWithFormat:
                                   @"%d hits out of %d retrievals; %f%%;\n"
                                    "Took %d microsec, %d microsec/retrieval , %d microsec/hit",
                                   num_hits,
                                   NUM_RETRIEVALS,
                                   (num_hits * 100.0f) / NUM_RETRIEVALS,
                                   delta_micros,
                                   delta_micros / NUM_RETRIEVALS,
                                   (num_hits == 0) ? 0 : delta_micros / num_hits];
}

- (IBAction)recreateCache {
  if (self.cache) {
    NSString *pathToRemove = [self.cache cachePath];
    self.cache = nil;

    [[NSFileManager defaultManager] removeItemAtPath:pathToRemove error:nil];
    self.testSummaryTextView.text = [NSString stringWithFormat:@"Recreated cache at %@", pathToRemove];
  }
  self.cache = [DCDiskCache loadOrCreateCacheWithPath:[DCDiskCache defaultCachePath]
                                      desiredNumLines:[self.cacheLinesTextField.text integerValue]
                                      desiredMaxBytes:[self.cacheKBTextField.text integerValue] * 1024];
}


#pragma mark -
#pragma Callbacks

- (void)imageFetchCallbackWithURL:(NSURL*)url data:(NSData *)data {
  NSString *str_url =[url absoluteString];
  [self.cache setItem:data forKey: str_url];
  self.totalRetrievals += 1;
  NSString *s = [NSString stringWithFormat:@"%d/%d Retrievals\nFetched Image %@ of len: %d",
                self.totalRetrievals,
                [self.crawler.imageFileURLS count],
                url,
                [data length]];
  [self performSelector:@selector(setStringInTextBlock:)
               onThread:[NSThread mainThread]
             withObject:s
          waitUntilDone:NO];
}

- (void)setStringInTextBlock:(NSString *) s{
  self.testSummaryTextView.text = s;
}

@end
