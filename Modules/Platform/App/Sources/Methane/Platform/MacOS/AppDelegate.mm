/******************************************************************************

Copyright 2019-2020 Evgeny Gorodetskiy

Licensed under the Apache License, Version 2.0 (the "License"),
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*******************************************************************************

FILE: Methane/Platform/MacOS/AppDelegate.mm
MacOS application delegate implementation.

******************************************************************************/

#import "AppDelegate.hh"
#import "WindowDelegate.hh"

#include <Methane/Platform/MacOS/AppMac.hh>
#include <Methane/Platform/MacOS/Types.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

using namespace Methane;
using namespace Methane::Platform;

@implementation AppDelegate

@synthesize window = m_window;

- (id) initWithApp : (AppMac*) p_app andSettings : (const AppBase::Settings*) p_settings
{
    META_FUNCTION_TASK();

    self = [super init];
    if (!self || !p_settings)
        return nil;

    NSScreen* ns_main_screen = [NSScreen mainScreen];
    CGFloat frame_width = p_settings->width < 1.0 ? ns_main_screen.frame.size.width * (p_settings->width > 0.0 ? p_settings->width : 0.7)
                                                  : static_cast<CGFloat>(p_settings->width);
    CGFloat frame_height = p_settings->height < 1.0 ? ns_main_screen.frame.size.height * (p_settings->height > 0.0 ? p_settings->height : 0.7)
                                                  : static_cast<CGFloat>(p_settings->height);
    NSRect frame = NSMakeRect(0, 0, frame_width, frame_height);

    NSUInteger style_mask = NSWindowStyleMaskTitled |
                            NSWindowStyleMaskResizable |
                            NSWindowStyleMaskClosable |
                            NSWindowStyleMaskMiniaturizable;

    NSBackingStoreType backing = NSBackingStoreBuffered;

    m_window = [[NSWindow alloc] initWithContentRect:frame styleMask:style_mask backing:backing defer:YES];
    m_window.contentMinSize = NSMakeSize(static_cast<CGFloat>(p_settings->min_width), static_cast<CGFloat>(p_settings->min_height));
    m_window.title    = MacOS::ConvertToNsType<std::string, NSString*>(p_settings->name);
    m_window.delegate = [[WindowDelegate alloc] initWithApp:p_app];
    [m_window center];
    
    NSRect backing_frame = [ns_main_screen convertRectToBacking:frame];
    self.viewController = [[AppViewController alloc] initWithApp:p_app andFrameRect:backing_frame];
    
    p_app->SetWindow(m_window);
    
    return self;
}

- (void) run
{
    META_FUNCTION_TASK();
    [m_window setContentViewController: self.viewController];
    [m_window setAcceptsMouseMovedEvents:YES];
}

- (void) alert : (NSString*) ns_title withInformation: (NSString*) ns_info andStyle: (NSAlertStyle) ns_alert_style
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(ns_title);
    META_CHECK_ARG_NOT_NULL(ns_info);
    
    NSAlert *ns_alert = [[NSAlert alloc] init];
    [ns_alert addButtonWithTitle:ns_alert_style == NSAlertStyleCritical ? @"Exit" : @"OK"];
    [ns_alert setMessageText:ns_title];
    [ns_alert setInformativeText:ns_info];
    [ns_alert setAlertStyle:ns_alert_style];
    [ns_alert runModal];
    
    if (ns_alert.alertStyle == NSAlertStyleCritical)
    {
        [NSApp terminate:self];
    }
}

- (void) applicationWillFinishLaunching:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    [m_window makeKeyAndOrderFront:self];
}

- (void) applicationDidFinishLaunching:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    [m_window makeFirstResponder: self.viewController.view];
    [m_window makeKeyAndOrderFront: nil];
}

- (void) windowWillEnterFullScreen:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    AppMac* p_app = [self.viewController getApp];
    META_CHECK_ARG_NOT_NULL(p_app);
    p_app->SetFullScreenInternal(true);
}

- (void) windowWillExitFullScreen:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    AppMac* p_app = [self.viewController getApp];
    META_CHECK_ARG_NOT_NULL(p_app);
    p_app->SetFullScreenInternal(false);
}

- (void) applicationWillTerminate:(NSNotification *)notification
{
    META_FUNCTION_TASK();
    #pragma unused(notification)
    // Insert code here to tear down your application
}

- (BOOL) applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    META_FUNCTION_TASK();
    #pragma unused(sender)
    return YES;
}

@end
