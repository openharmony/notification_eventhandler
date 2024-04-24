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

#ifndef BASE_EVENTHANDLER_INTERFACES_INNER_API_EVENT_QUEUE_H
#define BASE_EVENTHANDLER_INTERFACES_INNER_API_EVENT_QUEUE_H

#include <array>
#include <list>
#include <map>
#include <mutex>

#include "inner_event.h"
#include "event_handler_errors.h"
#include "file_descriptor_listener.h"
#include "dumper.h"

namespace OHOS {
namespace AppExecFwk {
class IoWaiter;
class EventHandler;
class EpollIoWaiter;

struct CurrentRunningEvent {
    InnerEvent::TimePoint beginTime_;
    std::weak_ptr<EventHandler> owner_;
    uint64_t senderKernelThreadId_{0};
    InnerEvent::TimePoint sendTime_;
    InnerEvent::TimePoint handleTime_;
    int64_t param_{0};
    bool hasTask_{false};
    std::string taskName_;
    InnerEvent::EventId innerEventId_ = 0u;
    CurrentRunningEvent();
    CurrentRunningEvent(InnerEvent::TimePoint time, InnerEvent::Pointer &event);
};

enum class EventInsertType: uint32_t {
    // Insert event at end
    AT_END = 0,
    // Insert event at front
    AT_FRONT
};

class EventQueue final {
public:
    // Priority for the events
    enum class Priority : uint32_t {
        // Event that should be distributed at once if possible.
        IMMEDIATE = 0,
        // High priority event, sorted by handle time, should be distributed before low priority event.
        HIGH,
        // Normal event, sorted by handle time.
        LOW,
        // Event that should be distributed only if no other event right now.
        IDLE,
        // The highest priority queue, should be distributed until the tasks in the queue are completed.
        VIP,
    };

    EventQueue();
    explicit EventQueue(const std::shared_ptr<IoWaiter> &ioWaiter);
    ~EventQueue();
    DISALLOW_COPY_AND_MOVE(EventQueue);

    /**
     * Insert an event into event queue with different priority.
     * The events will be sorted by handle time.
     *
     * @param event Event instance which should be added into event queue.
     * @param Priority Priority of the event
     * @param insertType The type of insertint event to queue
     *
     * @see #Priority
     */
    void Insert(InnerEvent::Pointer &event, Priority priority = Priority::LOW,
        EventInsertType insertType = EventInsertType::AT_END);

    /**
     * Remove events if its owner is invalid.
     */
    void RemoveOrphan();

    /**
     * Remove all events.
     */
    void RemoveAll();

    /**
     * Remove events with specified requirements.
     *
     * @param owner Owner of the event which is point to an instance of 'EventHandler'.
     */
    void Remove(const std::shared_ptr<EventHandler> &owner);

    /**
     * Remove events with specified requirements.
     *
     * @param owner Owner of the event which is point to an instance of 'EventHandler'.
     * @param innerEventId Remove events by event id.
     */
    void Remove(const std::shared_ptr<EventHandler> &owner, uint32_t innerEventId);

    /**
     * Remove events with specified requirements.
     *
     * @param owner Owner of the event which is point to an instance of 'EventHandler'.
     * @param innerEventId Remove events by event id.
     * @param param Remove events by value of param.
     */
    void Remove(const std::shared_ptr<EventHandler> &owner, uint32_t innerEventId, int64_t param);

    /**
     * Remove events with specified requirements.
     *
     * @param owner Owner of the event which is point to an instance of 'EventHandler'.
     * @param name Remove events by name of the task.
     */
    void Remove(const std::shared_ptr<EventHandler> &owner, const std::string &name);

    /**
     * Add file descriptor listener for a file descriptor.
     *
     * @param fileDescriptor File descriptor.
     * @param events Events from file descriptor, such as input, output, error
     * @param listener Listener callback.
     * @return Return 'ERR_OK' on success.
     */
    ErrCode AddFileDescriptorListener(int32_t fileDescriptor, uint32_t events,
        const std::shared_ptr<FileDescriptorListener> &listener, const std::string &taskName,
        Priority priority = Priority::HIGH);

    /**
     * Remove all file descriptor listeners for a specified owner.
     *
     * @param owner Owner of the event which is point to an instance of 'FileDescriptorListener'.
     */
    void RemoveFileDescriptorListener(const std::shared_ptr<EventHandler> &owner);

    /**
     * Remove file descriptor listener for a file descriptor.
     *
     * @param fileDescriptor File descriptor.
     */
    void RemoveFileDescriptorListener(int32_t fileDescriptor);

    /**
     * Prepare event queue, before calling {@link #GetEvent}.
     * If {@link #Finish} is called, prepare event queue again, before calling {@link #GetEvent}.
     */
    void Prepare();

    /**
     * Exit from blocking in {@link #GetEvent}, and mark the event queue finished.
     * After calling {@link #Finish}, {@link #GetEvent} never returns any event, until {@link #Prepare} is called.
     */
    void Finish();

    /**
     * Get event from event queue one by one.
     * Before calling this method, developers should call {@link #Prepare} first.
     * If none should be handled right now, the thread will be blocked in this method.
     * Call {@link #Finish} to exit from blocking.
     *
     * @return Returns nullptr if event queue is not prepared yet, or {@link #Finish} is called.
     * Otherwise returns event instance.
     */
    InnerEvent::Pointer GetEvent();

    /**
     * Get expired event from event queue one by one.
     * Before calling this method, developers should call {@link #Prepare} first.
     *
     * @param nextExpiredTime Output the expired time for the next event.
     * @return Returns nullptr if none in event queue is expired.
     * Otherwise returns event instance.
     */
    InnerEvent::Pointer GetExpiredEvent(InnerEvent::TimePoint &nextExpiredTime);

    /**
     * Prints out the internal information about an object in the specified format,
     * helping you diagnose internal errors of the object.
     *
     * @param dumper The Dumper object you have implemented to process the output internal information.
     */
    void Dump(Dumper &dumper);

    /**
     * Print out the internal information about an object in the specified format,
     * helping you diagnose internal errors of the object.
     *
     * @param queueInfo queue Info.
     */
    void DumpQueueInfo(std::string& queueInfo);

    /**
     * Checks whether the current EventHandler is idle.
     *
     * @return Returns true if all events have been processed; returns false otherwise.
     */
    bool IsIdle();

    /**
     * Check whether this event queue is empty.
     *
     * @return If queue is empty return true otherwise return false.
     */
    bool IsQueueEmpty();

    /**
     * Check whether an event with the given ID can be found among the events that have been sent but not processed.
     *
     * @param owner Owner of the event which is point to an instance of 'EventHandler'.
     * @param innerEventId The id of the event.
     */
    bool HasInnerEvent(const std::shared_ptr<EventHandler> &owner, uint32_t innerEventId);

    /**
     * Check whether an event carrying the given param can be found among the events that have been sent but not
     * processed.
     *
     * @param owner The owner of the event which is point to an instance of 'EventHandler'.
     * @param param The basic parameter of the event.
     */
    bool HasInnerEvent(const std::shared_ptr<EventHandler> &owner, int64_t param);

    void PushHistoryQueueBeforeDistribute(const InnerEvent::Pointer &event);

    void PushHistoryQueueAfterDistribute();

    bool HasPreferEvent(int basePrio);

    std::string DumpCurrentQueueSize();

    /**
     * Check whether there are currenty file descriptors is need to be processed.
     */
    void CheckFileDescriptorEvent();

    /**
     * Set epoll waiter mode
     */
    void SetNoWaitEpollWaiter(bool useNoWaitEpollWaiter)
    {
        useNoWaitEpollWaiter_ = useNoWaitEpollWaiter;
    }
private:
    using RemoveFilter = std::function<bool(const InnerEvent::Pointer &)>;
    using HasFilter = std::function<bool(const InnerEvent::Pointer &)>;

    struct HistoryEvent {
        uint64_t senderKernelThreadId{0};
        std::string taskName;
        InnerEvent::EventId innerEventId = 0u;
        bool hasTask{false};
        InnerEvent::TimePoint sendTime;
        InnerEvent::TimePoint handleTime;
        InnerEvent::TimePoint triggerTime;
        InnerEvent::TimePoint completeTime;
    };

    /*
     * To avoid starvation of lower priority event queue, give a chance to process lower priority events,
     * after continuous processing several higher priority events.
     */
    static const uint32_t DEFAULT_MAX_HANDLED_EVENT_COUNT = 5;

    // Sub event queues for IMMEDIATE, HIGH and LOW priority. So use value of IDLE as size.
    static const uint32_t SUB_EVENT_QUEUE_NUM = static_cast<uint32_t>(Priority::IDLE);

    struct SubEventQueue {
        std::list<InnerEvent::Pointer> queue;
        uint32_t handledEventsCount{0};
        uint32_t maxHandledEventsCount{DEFAULT_MAX_HANDLED_EVENT_COUNT};
    };

    void Remove(const RemoveFilter &filter);
    void RemoveOrphan(const RemoveFilter &filter);
    bool HasInnerEvent(const HasFilter &filter);
    InnerEvent::Pointer PickEventLocked(const InnerEvent::TimePoint &now, InnerEvent::TimePoint &nextWakeUpTime);
    InnerEvent::Pointer GetExpiredEventLocked(InnerEvent::TimePoint &nextExpiredTime);
    void WaitUntilLocked(const InnerEvent::TimePoint &when, std::unique_lock<std::mutex> &lock);
    void HandleFileDescriptorEvent(int32_t fileDescriptor, uint32_t events, const std::string &name);
    bool EnsureIoWaiterSupportListerningFileDescriptorLocked();
    std::string HistoryQueueDump(const HistoryEvent &historyEvent);
    std::string DumpCurrentRunning();
    void DumpCurrentRunningEventId(const InnerEvent::EventId &innerEventId, std::string &content);
    void DumpCurentQueueInfo(Dumper &dumper, uint32_t dumpMaxSize);
    void RemoveFileDescriptorByFd(int32_t fileDescriptor);
    bool AddFileDescriptorByFd(int32_t fileDescriptor, uint32_t events, const std::string &taskName,
        const std::shared_ptr<FileDescriptorListener>& listener, EventQueue::Priority priority);
    bool EnsureIoWaiterForDefault();
    bool EnsureIoWaiterForNoWait();
    std::mutex queueLock_;

    std::atomic_bool usable_ {true};

    // Sub event queues for different priority.
    std::array<SubEventQueue, SUB_EVENT_QUEUE_NUM> subEventQueues_;

    // Event queue for IDLE events.
    std::list<InnerEvent::Pointer> idleEvents_;
    // Event queue for VIP events.
    std::list<InnerEvent::Pointer> vipEvents_;

    // Next wake up time when block in 'GetEvent'.
    InnerEvent::TimePoint wakeUpTime_ { InnerEvent::TimePoint::max() };

    // Mark if in idle mode, and record the start time of idle.
    InnerEvent::TimePoint idleTimeStamp_ { InnerEvent::Clock::now() };

    bool isIdle_ {true};

    // Mark if the event queue is finished.
    bool finished_ {true};

    // IO waiter used to block if no events while calling 'GetEvent'.
    std::shared_ptr<IoWaiter> ioWaiter_;

    // select different epoll
    bool useNoWaitEpollWaiter_ = false;

    // Epoll io waiter for no wait mdoe
    std::shared_ptr<EpollIoWaiter> epollIoWaiter_;

    // File descriptor listeners to handle IO events.
    std::map<int32_t, std::shared_ptr<FileDescriptorListener>> listeners_;

    // current running event info
    CurrentRunningEvent currentRunningEvent_;

    static const uint8_t HISTORY_EVENT_NUM_POWER = 32; // 必须是2的幂次，使用位运算取余
    std::vector<HistoryEvent> historyEvents_;
    uint8_t historyEventIndex_ = 0;
};
}  // namespace AppExecFwk
}  // namespace OHOS

#endif  // #ifndef BASE_EVENTHANDLER_INTERFACES_INNER_API_EVENT_QUEUE_H
