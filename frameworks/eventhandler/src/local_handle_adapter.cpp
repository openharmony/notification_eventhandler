/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "local_handle_adapter.h"
#include "event_logger.h"

namespace OHOS {
namespace AppExecFwk {
namespace {
DEFINE_EH_HILOG_LABEL("LocalHandleAdapter");
} // namespace
LocalHandleAdapter& LocalHandleAdapter::GetInstance()
{
    static LocalHandleAdapter instance;
    return instance;
}

void LocalHandleAdapter::SetLocalHandleFunc(EventOpenLocalHandleFunc openFunc, EventCloseLocalHandleFunc closeFunc)
{
    openLocalHandleFunc = openFunc;
    closeLocalHandleFunc = closeFunc;
}

LocalHandleAdapter::LocalHandleAdapter()
{
    HILOGD("create LocalHandleAdapter");
}

LocalHandleAdapter::~LocalHandleAdapter()
{
    openLocalHandleFunc = nullptr;
    closeLocalHandleFunc = nullptr;
}
}  // namespace AppExecFwk
}  // namespace OHOS