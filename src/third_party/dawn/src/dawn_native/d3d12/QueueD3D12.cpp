// Copyright 2017 The Dawn Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "dawn_native/d3d12/QueueD3D12.h"

#include "dawn_native/d3d12/CommandBufferD3D12.h"
#include "dawn_native/d3d12/D3D12Error.h"
#include "dawn_native/d3d12/DeviceD3D12.h"

namespace dawn_native { namespace d3d12 {

    Queue::Queue(Device* device) : QueueBase(device) {
    }

    MaybeError Queue::SubmitImpl(uint32_t commandCount, CommandBufferBase* const* commands) {
        Device* device = ToBackend(GetDevice());

        device->Tick();

        DAWN_TRY(mCommandContext.Open(device->GetD3D12Device().Get(),
                                      device->GetCommandAllocatorManager()));
        for (uint32_t i = 0; i < commandCount; ++i) {
            DAWN_TRY(ToBackend(commands[i])->RecordCommands(&mCommandContext, i));
        }

        DAWN_TRY(device->ExecuteCommandContext(&mCommandContext));

        DAWN_TRY(device->NextSerial());
        return {};
    }

}}  // namespace dawn_native::d3d12
