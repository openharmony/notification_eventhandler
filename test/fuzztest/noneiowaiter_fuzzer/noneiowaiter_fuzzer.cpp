/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "none_io_waiter.h"
#include "noneiowaiter_fuzzer.h"
#include "securec.h"

namespace OHOS {
namespace {
    constexpr size_t U32_AT_SIZE = 4;
}
class NoneFileDescriptorListener : public AppExecFwk::FileDescriptorListener {
public:
    NoneFileDescriptorListener()
    {}
    ~NoneFileDescriptorListener()
    {}

    /* @param int32_t fileDescriptor */
    void OnReadable(int32_t)
    {}

    /* @param int32_t fileDescriptor */
    void OnWritable(int32_t)
    {}

    /* @param int32_t fileDescriptor */
    void OnException(int32_t)
    {}

    NoneFileDescriptorListener(const NoneFileDescriptorListener &) = delete;
    NoneFileDescriptorListener &operator=(const NoneFileDescriptorListener &) = delete;
    NoneFileDescriptorListener(NoneFileDescriptorListener &&) = delete;
    NoneFileDescriptorListener &operator=(NoneFileDescriptorListener &&) = delete;
};
bool DoSomethingInterestingWithMyAPI(const char* data, size_t size)
{
    uint32_t events = *data;
    int32_t fileDescriptor = U32_AT(reinterpret_cast<const uint8_t*>(data));
    AppExecFwk::IoWaiter::FileDescriptorEventCallback callback;
    AppExecFwk::NoneIoWaiter noneIoWaiter;
    auto listener = std::make_shared<NoneFileDescriptorListener>();
    noneIoWaiter.AddFileDescriptor(fileDescriptor, events, "DoSomethingInterestingWithMyAPI",
        listener, AppExecFwk::EventQueue::Priority::HIGH);
    noneIoWaiter.RemoveFileDescriptor(fileDescriptor);
    noneIoWaiter.SetFileDescriptorEventCallback(callback);
    return true;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    /* Run your code on data */
    if (data == nullptr) {
        return 0;
    }

    if (size < OHOS::U32_AT_SIZE) {
        return 0;
    }

    char* ch = (char *)malloc(size + 1);
    if (ch == nullptr) {
        return 0;
    }

    (void)memset_s(ch, size + 1, 0x00, size + 1);
    if (memcpy_s(ch, size, data, size) != EOK) {
        free(ch);
        ch = nullptr;
        return 0;
    }

    OHOS::DoSomethingInterestingWithMyAPI(ch, size);
    free(ch);
    ch = nullptr;
    return 0;
}
