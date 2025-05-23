/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef EMITTER_HANDLER_IMPL_H
#define EMITTER_HANDLER_IMPL_H

#include "event_handler.h"

namespace OHOS::EventsEmitter {
using EventHandler = OHOS::AppExecFwk::EventHandler;
using EventRunner = OHOS::AppExecFwk::EventRunner;
using InnerEvent = OHOS::AppExecFwk::InnerEvent;

class EventHandlerImpl : public EventHandler {
public:
    explicit EventHandlerImpl(const std::shared_ptr<EventRunner>& runner);
    static std::shared_ptr<EventHandlerImpl> GetEventHandler();
    ~EventHandlerImpl() override;
    void ProcessEvent(const InnerEvent::Pointer& event) override;
};
}

#endif