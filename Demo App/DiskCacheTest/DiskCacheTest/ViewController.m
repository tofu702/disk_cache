//
//  ViewController.m
//  DiskCacheTest
//
//  Created by Steven Stanek on 9/8/13.
//  Copyright (c) 2013 stevenstanek. All rights reserved.
//

#import "ViewController.h"

@interface ViewController ()

- (void)imageFetchCallbackWithURL:(NSString *)url data:(NSData *)data;

@end

@implementation ViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
  [self attachKeyboardAccessoryViews];
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

- (IBAction)startFetch {
  self.crawler = [[ImageCrawler alloc] initWithImagePageURL:self.urlTextField.text];
  [self.crawler fetchImagePageAndParseImageURLs];
  [self.crawler bulkAsyncFetchImagesWithSelector:@selector(imageFetchCallbackWithURL:data:) target:self];
}

- (IBAction)startRetrievalTest {
  
}

- (IBAction)clearCache {
  
}


#pragma mark -
#pragma Callbacks

- (void)imageFetchCallbackWithURL:(NSString *)url data:(NSData *)data {
  NSLog(@"Fetched image: %@ of len: %d", url, [data length]);
}

@end
