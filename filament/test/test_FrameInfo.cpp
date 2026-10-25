/*
 * Copyright (C) 2026 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "../src/FrameInfo.h"
#include "../src/details/Engine.h"

#include <filament/Engine.h>
#include <filament/Renderer.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>

using namespace filament;

TEST(FrameInfo, ReusesStorageAndReturnsIndependentBoundedCopies) {
    Engine* engine = Engine::create(Engine::Backend::NOOP);
    ASSERT_NE(engine, nullptr);
    auto& implementation = *downcast(engine);
    auto& driver = implementation.getDriverApi();
    {
        FrameInfoManager manager(implementation, driver);
        manager.updateUserHistory(nullptr, driver);
        EXPECT_TRUE(manager.getFrameInfoHistory(1).empty());
        auto const* storage = manager.getFrameInfoHistorySlice().data();
        EXPECT_NE(storage, nullptr);
        utils::FixedCapacityVector<Renderer::FrameInfo> firstSnapshot;
        for (uint32_t id = 1; id <= 40; ++id) {
            manager.beginFrame(nullptr, driver, { 4 }, id,
                    std::chrono::steady_clock::now(), {}, {});
            manager.endFrame(driver);
            engine->flushAndWait();
            manager.waitForGpu();
            manager.updateUserHistory(nullptr, driver);
            auto const history = manager.getFrameInfoHistorySlice();
            EXPECT_EQ(history.data(), storage);
            EXPECT_EQ(history.size(), std::min(size_t(id), size_t(16)));
            EXPECT_EQ(history[0].frameId, id);
            for (size_t i = 1; i < history.size(); ++i) {
                EXPECT_EQ(history[i].frameId, history[i - 1].frameId - 1);
            }
            if (id == 1) {
                firstSnapshot = manager.getFrameInfoHistory(1);
            }
            EXPECT_EQ(firstSnapshot[0].frameId, 1u);
            for (size_t count : { 0u, 1u, 4u, 16u, 100u }) {
                auto const copy = manager.getFrameInfoHistory(count);
                EXPECT_EQ(copy.size(), std::min<size_t>(count, history.size()));
                EXPECT_EQ(copy.capacity(), copy.size());
                if (!copy.empty()) {
                    EXPECT_NE(copy.data(), history.data());
                    EXPECT_EQ(copy[0].frameId, id);
                }
            }
            auto const saved = manager.getFrameInfoHistory(1);
            manager.updateUserHistory(nullptr, driver);
            EXPECT_EQ(saved[0].frameId, id);
        }
        manager.terminate(implementation);
        engine->flushAndWait();
    }
    Engine::destroy(&engine);
}

int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
