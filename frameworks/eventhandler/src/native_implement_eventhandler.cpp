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

#include "native_implement_eventhandler.h"

#include <cstdint>                         // for int32_t, uint32_t
#include <memory>                           // for shared_ptr, operator!=

#include "event_runner.h"
#include "errors.h"                         // for ErrCode
#include "event_handler_errors.h"           // for EVENT_HANDLER_ERR_NO_EVEN...
#include "event_queue.h"                    // for EventQueue
#include "file_descriptor_listener.h"

struct FileDescriptorCallbacks {
    const FileFDCallback readableCallback;
    const FileFDCallback writableCallback;
    const FileFDCallback shutdownCallback;
    const FileFDCallback exceptionCallback;
};

class NativeFileDescriptorListener : public OHOS::AppExecFwk::FileDescriptorListener {
public:
    explicit NativeFileDescriptorListener(const struct FileDescriptorCallbacks *fileDescriptorCallbacks)
    {
        if (fileDescriptorCallbacks->readableCallback != nullptr) {
            onReadableCallback_ = fileDescriptorCallbacks->readableCallback;
        }
        if (fileDescriptorCallbacks->writableCallback != nullptr) {
            onWritableCallback_ = fileDescriptorCallbacks->writableCallback;
        }
        if (fileDescriptorCallbacks->shutdownCallback != nullptr) {
            onShutdownCallback_ = fileDescriptorCallbacks->shutdownCallback;
        }
        if (fileDescriptorCallbacks->exceptionCallback != nullptr) {
            onExceptionCallback_ = fileDescriptorCallbacks->exceptionCallback;
        }
    }

    ~NativeFileDescriptorListener()
    {}

    /**
     * Called while file descriptor is readable.
     *
     * @param fileDescriptor File descriptor which is readable.
     */
    void OnReadable(int32_t filedescriptor)
    {
        if (onReadableCallback_ != nullptr) {
            onReadableCallback_(filedescriptor);
        }
    }

    /**
     * Called while file descriptor is writable.
     *
     * @param fileDescriptor File descriptor which is writable.
     */
    void OnWritable(int32_t filedescriptor)
    {
        if (onWritableCallback_ != nullptr) {
            onWritableCallback_(filedescriptor);
        }
    }

    /**
     * Called while shutting down this file descriptor.
     *
     * @param fileDescriptor File descriptor which is shutting down.
     */
    void OnShutdown(int32_t filedescriptor)
    {
        if (onShutdownCallback_ != nullptr) {
            onShutdownCallback_(filedescriptor);
        }
    }

    /**
     * Called while error happened on this file descriptor.
     *
     * @param fileDescriptor Error happened on this file descriptor.
     */
    void OnException(int32_t filedescriptor)
    {
        if (onExceptionCallback_ != nullptr) {
            onExceptionCallback_(filedescriptor);
        }
    }

    NativeFileDescriptorListener(const NativeFileDescriptorListener &) = delete;
    NativeFileDescriptorListener &operator=(const NativeFileDescriptorListener &) = delete;
    NativeFileDescriptorListener(NativeFileDescriptorListener &&) = delete;
    NativeFileDescriptorListener &operator=(NativeFileDescriptorListener &&) = delete;

private:
    FileFDCallback onReadableCallback_ = nullptr;
    FileFDCallback onWritableCallback_ = nullptr;
    FileFDCallback onShutdownCallback_ = nullptr;
    FileFDCallback onExceptionCallback_ = nullptr;
};

EventRunnerNativeImplement::EventRunnerNativeImplement(bool current)
{
    if (current) {
        eventRunner_ = EventRunner::Current();
    } else {
        eventRunner_ = EventRunner::Create(false);
    }
}

EventRunnerNativeImplement::~EventRunnerNativeImplement()
{
    eventRunner_ = nullptr;
}

const EventRunnerNativeImplement *EventRunnerNativeImplement::GetEventRunnerNativeObj()
{
    return new (std::nothrow) EventRunnerNativeImplement(true);
}

const EventRunnerNativeImplement *EventRunnerNativeImplement::CreateEventRunnerNativeObj()
{
    return new (std::nothrow) EventRunnerNativeImplement(false);
}

ErrCode EventRunnerNativeImplement::RunEventRunnerNativeObj() const
{
    if (eventRunner_ != nullptr) {
        return eventRunner_->Run();
    }
    return OHOS::AppExecFwk::EVENT_HANDLER_ERR_NO_EVENT_RUNNER;
}

ErrCode EventRunnerNativeImplement::StopEventRunnerNativeObj() const
{
    if (eventRunner_ != nullptr) {
        return eventRunner_->Stop();
    }
    return OHOS::AppExecFwk::EVENT_HANDLER_ERR_NO_EVENT_RUNNER;
}

ErrCode EventRunnerNativeImplement::AddFileDescriptorListener(
    int32_t fileDescriptor, uint32_t events, const FileDescriptorCallbacks *fdCallbacks) const
{
    auto nativeFileDescriptorListener = std::make_shared<NativeFileDescriptorListener>(fdCallbacks);
    return eventRunner_->GetEventQueue()->AddFileDescriptorListener(
        fileDescriptor, events, nativeFileDescriptorListener, "EventRunnerNativeTask");
}

void EventRunnerNativeImplement::RemoveFileDescriptorListener(int32_t fileDescriptor) const
{
    eventRunner_->GetEventQueue()->RemoveFileDescriptorListener(fileDescriptor);
}
