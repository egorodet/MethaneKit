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

#import "WindowDelegate.hh"

#include <Methane/Platform/MacOS/AppMac.hh>
#include <Methane/Instrumentation.h>
#include <Methane/Checks.hpp>

using namespace Methane;
using namespace Methane::Platform;

@implementation WindowDelegate
{
    AppMac*         m_p_app;
    Data::FrameSize m_frame_size;
}

- (id) initWithApp : (AppMac*) p_app
{
    META_FUNCTION_TASK();

    self = [super init];
    if (!self)
        return nil;

    m_p_app = p_app;
    
    return self;
}

- (void) windowDidEnterFullScreen:(NSNotification*) notification
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(notification)

    m_p_app->SetFullScreen(true);
}

- (void) windowDidExitFullScreen:(NSNotification*) notification
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(notification)

    m_p_app->SetFullScreen(false);
}

- (void) windowDidMiniaturize:(NSNotification*) notification
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(notification)

    m_p_app->Resize(m_p_app->GetFrameSize(), true);
}

- (void) windowDidDeminiaturize:(NSNotification*) notification
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(notification)

    m_p_app->Resize(m_p_app->GetFrameSize(), false);
}

- (void) windowWillStartLiveResize:(NSNotification*) notification
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(notification)

    m_p_app->StartResizing();
}

- (void) windowDidEndLiveResize:(NSNotification*) notification
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(notification)

    m_p_app->EndResizing();
}

- (void) windowDidBecomeKey:(NSNotification*) notification
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(notification)

    m_p_app->SetKeyboardFocus(true);
}

- (void) windowDidResignKey:(NSNotification*) notification
{
    META_FUNCTION_TASK();
    META_CHECK_ARG_NOT_NULL(m_p_app);
    #pragma unused(notification)

    m_p_app->SetKeyboardFocus(false);
}

@end
