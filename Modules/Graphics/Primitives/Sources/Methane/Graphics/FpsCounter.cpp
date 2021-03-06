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

FILE: Methane/Graphics/FpsCounter.cpp
FPS counter calculates frame time duration with moving average window algorithm.

******************************************************************************/

#include <Methane/Graphics/FpsCounter.h>
#include <Methane/Instrumentation.h>

namespace Methane::Graphics
{

FpsCounter::FrameTiming::FrameTiming(double total_time_sec, double present_time_sec, double gpu_wait_time_sec) noexcept
    : m_total_time_sec(total_time_sec)
    , m_present_time_sec(present_time_sec)
    , m_gpu_wait_time_sec(gpu_wait_time_sec)
{
    META_FUNCTION_TASK();
}

FpsCounter::FrameTiming& FpsCounter::FrameTiming::operator+=(const FrameTiming& other) noexcept
{
    META_FUNCTION_TASK();
    m_total_time_sec    += other.m_total_time_sec;
    m_present_time_sec  += other.m_present_time_sec;
    m_gpu_wait_time_sec += other.m_gpu_wait_time_sec;
    return *this;
}

FpsCounter::FrameTiming& FpsCounter::FrameTiming::operator-=(const FrameTiming& other) noexcept
{
    META_FUNCTION_TASK();
    m_total_time_sec    -= other.m_total_time_sec;
    m_present_time_sec  -= other.m_present_time_sec;
    m_gpu_wait_time_sec -= other.m_gpu_wait_time_sec;
    return *this;
}

FpsCounter::FrameTiming FpsCounter::FrameTiming::operator/(double divisor) const noexcept
{
    META_FUNCTION_TASK();
    return FrameTiming(m_total_time_sec    / divisor,
                       m_present_time_sec  / divisor,
                       m_gpu_wait_time_sec / divisor);
}

FpsCounter::FrameTiming FpsCounter::FrameTiming::operator*(double multiplier) const noexcept
{
    META_FUNCTION_TASK();
    return FrameTiming(m_total_time_sec    * multiplier,
                       m_present_time_sec  * multiplier,
                       m_gpu_wait_time_sec * multiplier);
}

void FpsCounter::Reset(uint32_t averaged_timings_count) noexcept
{
    META_FUNCTION_TASK();
    m_averaged_timings_count = averaged_timings_count;
    while (!m_frame_timings.empty())
    {
        m_frame_timings.pop();
    }
    m_frame_timings_sum = FrameTiming();
    m_present_on_gpu_wait_time_sec = 0.0;
    m_frame_timer.Reset();
    m_present_timer.Reset();
}

void FpsCounter::OnCpuFramePresented() noexcept
{
    META_FUNCTION_TASK();
    if (m_frame_timings.size() >= m_averaged_timings_count)
    {
        m_frame_timings_sum -= m_frame_timings.front();
        m_frame_timings.pop();
    }

    const FrameTiming frame_timing(m_frame_timer.GetElapsedSecondsD(),
                                   m_present_timer.GetElapsedSecondsD(),
                                   m_present_on_gpu_wait_time_sec);
    
    m_frame_timings_sum += frame_timing;
    m_frame_timings.push(frame_timing);

    m_frame_timer.Reset();
}

FpsCounter::FrameTiming FpsCounter::GetAverageFrameTiming() const noexcept
{
    META_FUNCTION_TASK();
    const uint32_t averaged_timings_count = GetAveragedTimingsCount();
    return averaged_timings_count ? m_frame_timings_sum / averaged_timings_count : FrameTiming();
}

uint32_t FpsCounter::GetFramesPerSecond() const noexcept
{
    META_FUNCTION_TASK();
    double average_frame_time_sec = GetAverageFrameTiming().GetTotalTimeSec();
    return average_frame_time_sec > 0.0 ? static_cast<uint32_t>(std::round(1.0 / average_frame_time_sec)) : 0U;
}

} // namespace Methane::Graphics
