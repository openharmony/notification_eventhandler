/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#include <cstdlib>
#include <vector>

#include "event_handler.h"
#include "event_queue.h"
#include "event_runner.h"
#include "epoll_io_waiter.h"
#include "deamon_io_waiter.h"
#include "none_io_waiter.h"

using namespace testing::ext;
using namespace OHOS::AppExecFwk;

class LibEventHandlerEpollIoWaiterTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void LibEventHandlerEpollIoWaiterTest::SetUpTestCase(void)
{}

void LibEventHandlerEpollIoWaiterTest::TearDownTestCase(void)
{}

void LibEventHandlerEpollIoWaiterTest::SetUp(void)
{}

void LibEventHandlerEpollIoWaiterTest::TearDown(void)
{}

class IoFileDescriptorListener : public FileDescriptorListener {
public:
    IoFileDescriptorListener()
    {}
    ~IoFileDescriptorListener()
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

    IoFileDescriptorListener(const IoFileDescriptorListener &) = delete;
    IoFileDescriptorListener &operator=(const IoFileDescriptorListener &) = delete;
    IoFileDescriptorListener(IoFileDescriptorListener &&) = delete;
    IoFileDescriptorListener &operator=(IoFileDescriptorListener &&) = delete;
};

/*
 * @tc.name: Init001
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(LibEventHandlerEpollIoWaiterTest, Init001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. get event with event id and param, then get event id and param from event.
     * @tc.expected: step1. the event id and param is the same as we set.
     */
    EpollIoWaiter epollIoWaiter;
    bool result = epollIoWaiter.Init();
    EXPECT_EQ(result, true);
    epollIoWaiter.NotifyAll();
}

/*
 * @tc.name: AddFileDescriptor001
 * @tc.desc: AddFileDescriptor
 * @tc.type: FUNC
 */
HWTEST_F(LibEventHandlerEpollIoWaiterTest, AddFileDescriptor001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. get event with event id and param, then get event id and param from event.
     * @tc.expected: step1. the event id and param is the same as we set.
     */
    EpollIoWaiter epollIoWaiter;

    int32_t fileDescriptor = -1;
    uint32_t events = 1;
    auto listener = std::make_shared<IoFileDescriptorListener>();
    bool result = epollIoWaiter.AddFileDescriptor(fileDescriptor, events, "AddFileDescriptor001",
        listener, EventQueue::Priority::HIGH);
    EXPECT_EQ(result, false);
    epollIoWaiter.RemoveFileDescriptor(fileDescriptor);
}

/*
 * @tc.name: AddFileDescriptor002
 * @tc.desc: AddFileDescriptor
 * @tc.type: FUNC
 */
HWTEST_F(LibEventHandlerEpollIoWaiterTest, AddFileDescriptor002, TestSize.Level1)
{
    /**
     * @tc.steps: step1. get event with event id and param, then get event id and param from event.
     * @tc.expected: step1. the event id and param is the same as we set.
     */
    EpollIoWaiter epollIoWaiter;

    int32_t fileDescriptor = 1;
    uint32_t events = 0;
    auto listener = std::make_shared<IoFileDescriptorListener>();
    bool result = epollIoWaiter.AddFileDescriptor(fileDescriptor, events, "AddFileDescriptor002",
        listener, EventQueue::Priority::HIGH);
    EXPECT_EQ(result, false);
    epollIoWaiter.RemoveFileDescriptor(fileDescriptor);
}

/*
 * @tc.name: AddFileDescriptor003
 * @tc.desc: AddFileDescriptor
 * @tc.type: FUNC
 */
HWTEST_F(LibEventHandlerEpollIoWaiterTest, AddFileDescriptor003, TestSize.Level1)
{
    /**
     * @tc.steps: step1. get event with event id and param, then get event id and param from event.
     * @tc.expected: step1. the event id and param is the same as we set.
     */
    EpollIoWaiter epollIoWaiter;

    int32_t fileDescriptor = 1;
    uint32_t events = 1;
    auto listener = std::make_shared<IoFileDescriptorListener>();
    bool result = epollIoWaiter.AddFileDescriptor(fileDescriptor, events, "AddFileDescriptor003",
        listener, EventQueue::Priority::HIGH);
    EXPECT_EQ(result, false);
}

HWTEST_F(LibEventHandlerEpollIoWaiterTest, PostTaskForVsync001, TestSize.Level1)
{
    /**
     * @tc.steps: step1. get event with event id and param, then get event id and param from event.
     * @tc.expected: step1. the event id and param is the same as we set.
     */

    int32_t fileDescriptor = 1;
    uint32_t events = 1;
    DeamonIoWaiter::GetInstance().Init();
    auto runner = EventRunner::Create(true);
    auto handler = std::make_shared<EventHandler>(runner);

    auto listener = std::make_shared<IoFileDescriptorListener>();

    handler->AddFileDescriptorListener(fileDescriptor, events,
        listener, "PostTaskForVsync001", EventQueue::Priority::VIP);
    bool result = DeamonIoWaiter::GetInstance().AddFileDescriptor(fileDescriptor,
        events, "PostTaskForVsync001", listener, EventQueue::Priority::VIP);
    EXPECT_EQ(result, true);

    DeamonIoWaiter::GetInstance().HandleFileDescriptorEvent(fileDescriptor, events);
    usleep(500);
    listener->SetType(IoFileDescriptorListener::ListenerType::LTYPE_VSYNC);
    DeamonIoWaiter::GetInstance().HandleFileDescriptorEvent(fileDescriptor, events);
    usleep(500);
    events = FILE_DESCRIPTOR_INPUT_EVENT | FILE_DESCRIPTOR_OUTPUT_EVENT|
        FILE_DESCRIPTOR_SHUTDOWN_EVENT | FILE_DESCRIPTOR_EXCEPTION_EVENT;
    DeamonIoWaiter::GetInstance().HandleFileDescriptorEvent(fileDescriptor, events);
    usleep(500);
}

/*
 * @tc.name: NoneIoWaiter001
 * @tc.desc: NoneIoWaiter001
 * @tc.type: FUNC
 */
HWTEST_F(LibEventHandlerEpollIoWaiterTest, NoneIoWaiter001, TestSize.Level1)
{
    NoneIoWaiter ioWaiter;
    ioWaiter.RemoveFileDescriptor(1);
    auto listener = std::make_shared<IoFileDescriptorListener>();
    bool result = ioWaiter.AddFileDescriptor(1, 2, "task", listener, EventQueue::Priority::VIP);
    EXPECT_EQ(result, false);
}