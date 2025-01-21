/*
 * Copyright (c) 2021-2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "event_queue.h"

#include <algorithm>
#include <iterator>
#include <mutex>

#include "deamon_io_waiter.h"
#include "epoll_io_waiter.h"
#include "event_handler.h"
#include "event_handler_utils.h"
#include "event_logger.h"
#include "none_io_waiter.h"
#include "parameters.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
static const bool MONITOR_FLAG =
    system::GetBoolParameter("const.sys.param_file_description_monitor", false);
static const int32_t VSYNC_TASK_DELAYMS =
    system::GetIntParameter("const.sys.param_vsync_delayms", 50);
DEFINE_EH_HILOG_LABEL("EventQueue");

// Help to remove file descriptor listeners.
template<typename T>
void RemoveFileDescriptorListenerLocked(std::map<int32_t, std::shared_ptr<FileDescriptorListener>> &listeners,
    const std::shared_ptr<IoWaiter>& ioWaiter, const T &filter, bool useDeamonIoWaiter_)
{
    for (auto it = listeners.begin(); it != listeners.end();) {
        if (filter(it->second)) {
            if (useDeamonIoWaiter_ || (it->second->GetIsDeamonWaiter() && MONITOR_FLAG)) {
                DeamonIoWaiter::GetInstance().RemoveFileDescriptor(it->first);
            } else if (ioWaiter) {
                ioWaiter->RemoveFileDescriptor(it->first);
            }
            it = listeners.erase(it);
        } else {
            ++it;
        }
    }
}
}  // unnamed namespace

EventQueue::EventQueue() : ioWaiter_(std::make_shared<NoneIoWaiter>())
{
    HILOGD("enter");
    epollTimePoint_ = InnerEvent::Clock::now();
}

EventQueue::EventQueue(const std::shared_ptr<IoWaiter> &ioWaiter)
    : ioWaiter_(ioWaiter ? ioWaiter : std::make_shared<NoneIoWaiter>())
{
    HILOGD("enter");
    if (ioWaiter_->SupportListeningFileDescriptor()) {
        // Set callback to handle events from file descriptors.
        ioWaiter_->SetFileDescriptorEventCallback(
            std::bind(&EventQueue::HandleFileDescriptorEvent, this, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3, std::placeholders::_4));
    }
    epollTimePoint_ = InnerEvent::Clock::now();
}

EventQueue::~EventQueue()
{
    EH_LOGI_LIMIT("EventQueue is unavailable hence");
}

InnerEvent::Pointer EventQueue::GetEvent()
{
    return InnerEvent::Pointer(nullptr, nullptr);
}

InnerEvent::Pointer EventQueue::GetExpiredEvent(InnerEvent::TimePoint &nextExpiredTime)
{
    return InnerEvent::Pointer(nullptr, nullptr);
}

void EventQueue::WaitUntilLocked(const InnerEvent::TimePoint &when, std::unique_lock<std::mutex> &lock)
{
    // Get a temp reference of IO waiter, otherwise it maybe released while waiting.
    auto ioWaiterHolder = ioWaiter_;
    if (!ioWaiterHolder->WaitFor(lock, TimePointToTimeOut(when))) {
        HILOGE("Failed to call wait, reset IO waiter");
        ioWaiter_ = std::make_shared<NoneIoWaiter>();
        listeners_.clear();
    }
}

void EventQueue::CheckFileDescriptorEvent()
{
    InnerEvent::TimePoint now = InnerEvent::Clock::now();
    std::unique_lock<std::mutex> lock(queueLock_);
    WaitUntilLocked(now, lock);
}

bool EventQueue::AddFileDescriptorByFd(int32_t fileDescriptor, uint32_t events, const std::string &taskName,
    const std::shared_ptr<FileDescriptorListener>& listener, EventQueue::Priority priority)
{
    if (useDeamonIoWaiter_ || (listener && listener->GetIsDeamonWaiter() && MONITOR_FLAG)) {
        if (listener && listener->IsVsyncListener()) {
            vsyncPriority_ = priority;
            listener->SetDelayTime(UINT32_MAX);
        }
        return DeamonIoWaiter::GetInstance().AddFileDescriptor(fileDescriptor, events, taskName,
            listener, priority);
    }
    if (ioWaiter_) {
        return ioWaiter_->AddFileDescriptor(fileDescriptor, events, taskName, listener, priority);
    }
    return false;
}

bool EventQueue::EnsureIoWaiterLocked(const std::shared_ptr<FileDescriptorListener>& listener)
{
    HILOGD("enter");
    if (useDeamonIoWaiter_ || (listener && listener->GetIsDeamonWaiter() && MONITOR_FLAG)) {
        if (!DeamonIoWaiter::GetInstance().Init()) {
            HILOGE("Failed to initialize deamon waiter");
            return false;
        }
        DeamonIoWaiter::GetInstance().StartEpollIoWaiter();
        return true;
    }

    if (ioWaiter_->SupportListeningFileDescriptor()) {
        return true;
    }

    auto newIoWaiter = std::make_shared<EpollIoWaiter>();
    if (!newIoWaiter->Init()) {
        HILOGE("Failed to initialize epoll");
        return false;
    }

    // Set callback to handle events from file descriptors.
    newIoWaiter->SetFileDescriptorEventCallback(
        std::bind(&EventQueue::HandleFileDescriptorEvent, this, std::placeholders::_1, std::placeholders::_2,
        std::placeholders::_3, std::placeholders::_4));

    ioWaiter_->NotifyAll();
    ioWaiter_ = newIoWaiter;
    return true;
}

ErrCode EventQueue::AddFileDescriptorListenerBase(int32_t fileDescriptor, uint32_t events,
    const std::shared_ptr<FileDescriptorListener> &listener, const std::string &taskName, Priority priority)
{
    if (!usable_.load()) {
        HILOGW("EventQueue is unavailable.");
        return EVENT_HANDLER_ERR_NO_EVENT_RUNNER;
    }
    auto it = listeners_.find(fileDescriptor);
    if (it != listeners_.end()) {
        HILOGE("File descriptor %{public}d is already in listening", fileDescriptor);
        return EVENT_HANDLER_ERR_FD_ALREADY;
    }

    HILOGD("Add file descriptor %{public}d to io waiter %{public}d", fileDescriptor, useDeamonIoWaiter_);
    if (!EnsureIoWaiterLocked(listener)) {
        return EVENT_HANDLER_ERR_FD_NOT_SUPPORT;
    }

    if (!AddFileDescriptorByFd(fileDescriptor, events, taskName, listener, priority)) {
        HILOGE("Failed to add file descriptor into IO waiter");
        return EVENT_HANDLER_ERR_FD_FAILED;
    }

    listeners_.emplace(fileDescriptor, listener);
    return ERR_OK;
}

void EventQueue::HandleFileDescriptorEvent(int32_t fileDescriptor, uint32_t events,
    const std::string &taskName, Priority priority) __attribute__((no_sanitize("cfi")))
{
    std::shared_ptr<FileDescriptorListener> listener;
    {
        std::lock_guard<std::mutex> lock(queueLock_);
        if (!usable_.load()) {
            HILOGW("EventQueue is unavailable.");
            return;
        }
        auto it = listeners_.find(fileDescriptor);
        if (it == listeners_.end()) {
            HILOGW("Can not found listener, maybe it is removed");
            return;
        }
        // Hold instance of listener.
        listener = it->second;
        if (!listener) {
            return;
        }
    }

    auto handler = listener->GetOwner();
    if (!handler) {
        HILOGW("Owner of listener is released");
        return;
    }

    std::weak_ptr<FileDescriptorListener> wp = listener;
    auto f = [fileDescriptor, events, wp]() {
        auto listener = wp.lock();
        if (!listener) {
            HILOGW("Listener is released");
            return;
        }

        if ((events & FILE_DESCRIPTOR_INPUT_EVENT) != 0) {
            listener->OnReadable(fileDescriptor);
        }

        if ((events & FILE_DESCRIPTOR_OUTPUT_EVENT) != 0) {
            listener->OnWritable(fileDescriptor);
        }

        if ((events & FILE_DESCRIPTOR_SHUTDOWN_EVENT) != 0) {
            listener->OnShutdown(fileDescriptor);
        }

        if ((events & FILE_DESCRIPTOR_EXCEPTION_EVENT) != 0) {
            listener->OnException(fileDescriptor);
        }
    };

    HILOGD("Post fd %{public}d, task %{public}s, priority %{public}d.", fileDescriptor,
        taskName.c_str(), priority);
    // Post a high priority task to handle file descriptor events.
    handler->PostTask(f, taskName, 0, priority);
}

void EventQueue::RemoveListenerByOwner(const std::shared_ptr<EventHandler> &owner)
{
    if (!usable_.load()) {
        HILOGW("EventQueue is unavailable.");
        return;
    }
    auto listenerFilter = [&owner](const std::shared_ptr<FileDescriptorListener> &listener) {
        if (!listener) {
            return false;
        }
        return listener->GetOwner() == owner;
    };
    RemoveFileDescriptorListenerLocked(listeners_, ioWaiter_, listenerFilter, useDeamonIoWaiter_);
}

void EventQueue::RemoveListenerByFd(int32_t fileDescriptor)
{
    if (!usable_.load()) {
        HILOGW("EventQueue is unavailable.");
        return;
    }
    auto it = listeners_.find(fileDescriptor);
    std::shared_ptr<FileDescriptorListener> listener;
    if (it != listeners_.end()) {
        listener = it->second;
    }
    if (listeners_.erase(fileDescriptor) > 0) {
        if (useDeamonIoWaiter_ || (listener && listener->GetIsDeamonWaiter() && MONITOR_FLAG)) {
            DeamonIoWaiter::GetInstance().RemoveFileDescriptor(fileDescriptor);
            return;
        }
        if (ioWaiter_) {
            ioWaiter_->RemoveFileDescriptor(fileDescriptor);
        }
    }
}

void EventQueue::RemoveInvalidFileDescriptor()
{
    // Remove all listeners which lost its owner.
    auto listenerFilter = [](const std::shared_ptr<FileDescriptorListener> &listener) {
        if (!listener) {
            return true;
        }
        HILOGD("Start get to GetOwner");
        return !listener->GetOwner();
    };

    RemoveFileDescriptorListenerLocked(listeners_, ioWaiter_, listenerFilter, useDeamonIoWaiter_);
}

void EventQueue::SetVsyncLazyMode(bool isLazy)
{
    if (!MONITOR_FLAG) {
        return;
    }
    std::lock_guard<std::mutex> lock(queueLock_);
    if (!usable_.load()) {
        HILOGW("%{public}s, EventQueue is unavailable!", __func__);
        return;
    }
    if (isLazyMode_ == isLazy) {
        return;
    }

    for (auto it = listeners_.begin(); it != listeners_.end(); ++it) {
        auto listener = it->second;
        if (!listener || !listener->GetIsDeamonWaiter() || !listener->IsVsyncListener()) {
            continue;
        }
        listener->SetDelayTime(isLazy? UINT32_MAX : VSYNC_TASK_DELAYMS);
        HILOGD("%{public}s successful! fd = %{public}d, isLazy = %{public}d",
            __func__, it->first, isLazy);
    }
    isLazyMode_ = isLazy;
}

void EventQueue::PrepareBase()
{
    if (!usable_.load()) {
        HILOGW("EventQueue is unavailable.");
        return;
    }
    finished_ = false;
}

void EventQueue::FinishBase()
{
    if (!usable_.load()) {
        HILOGW("EventQueue is unavailable.");
        return;
    }
    finished_ = true;
    ioWaiter_->NotifyAll();
}

void EventQueue::NotifyObserverVipDone(const InnerEvent::Pointer &event)
{
    HILOGD("eventQueue NotifyObserverVipDone enter.");
}
}  // namespace AppExecFwk
}  // namespace OHOS