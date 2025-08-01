/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef BASE_EVENTHANDLER_FRAMEWORKS_ASYNC_CALLBACK_MANAGER_H
#define BASE_EVENTHANDLER_FRAMEWORKS_ASYNC_CALLBACK_MANAGER_H

#include "napi_async_callback_manager.h"
#include "ani_async_callback_manager.h"

namespace OHOS {
namespace AppExecFwk {

class AsyncCallbackManager {
public:
    DISALLOW_COPY_AND_MOVE(AsyncCallbackManager);
    static AsyncCallbackManager& GetInstance();

    /**
     * Delete all callback info of given event id.
     *
     * @param eventIdValue event id.
     */
    void DeleteCallbackInfoByEventId(const InnerEvent::EventId &eventIdValue);

    /**
     * Get all callback info counts of given event id.
     *
     * @param eventId event id.
     * @return Counts of callback info.
     */
    uint32_t GetListenerCountByEventId(const InnerEvent::EventId &eventId);

    /**
     * Find whether exists valid callback.
     *
     * @param eventId event id.
     * @return Returns true if exists valid callback.
     */
    bool IsExistValidCallback(const InnerEvent::EventId &eventId);

    /**
     * Insert callback.
     *
     * @param env A pointer to the environment structure.
     * @param eventId Event id.
     * @param once Whether subscribe once. if true, subscribe once.
     * @param callback Event's callback.
     * @param dataType Data type of callback's parameter.
     */
    void InsertCallbackInfo(
        ani_env *env, InnerEvent::EventId eventId, bool once, ani_ref callback, ani_string dataType);

    /**
     * Delete callback of given event id and callback object.
     *
     * @param env A pointer to the environment structure.
     * @param eventIdValue Event id.
     * @param argv Event's callback.
     */
    void DeleteCallbackInfo(napi_env env, const InnerEvent::EventId &eventIdValue, napi_value argv);
        
    /**
     * Delete callback of given event id and callback object.
     *
     * @param env A pointer to the environment structure.
     * @param eventIdValue Event id.
     * @param callback Event's callback.
     */
    void DeleteCallbackInfo(ani_env *env, const InnerEvent::EventId &eventIdValue, ani_ref callback);

    /**
     * Execute callback.
     *
     * @param event Event Emitted by user.
     */
    void DoCallback(const InnerEvent::Pointer& event);

    /**
     * Whether needs to cross runtime.
     *
     * @param eventId event id.
     * @param envType The type of runtime environment.
     * @return Returns true if needs to cross runtime.
     */
    bool IsCrossRuntime(const InnerEvent::EventId &eventId, EnvType envType);
private:
    AsyncCallbackManager() = default;

private:
    AniAsyncCallbackManager aniAsyncCallbackManager_;
    NapiAsyncCallbackManager napiAsyncCallbackManager_;
};

} // namespace AppExecFwk
} // namespace OHOS

#endif // BASE_EVENTHANDLER_FRAMEWORKS_ASYNC_CALLBACK_MANAGER_H