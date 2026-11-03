//
// Copyright (C) 2026 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#include <gtest/gtest.h>
#include <libgsi/libgsi.h>
#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <android-base/file.h>

namespace android {
namespace gsi {

TEST(LibGsiTest, IsValidSlotName) {
    EXPECT_TRUE(IsValidSlotName("dsu"));
    EXPECT_TRUE(IsValidSlotName("dsu2"));
    EXPECT_TRUE(IsValidSlotName("dsu_test"));
    EXPECT_TRUE(IsValidSlotName("dsu-test"));
    EXPECT_TRUE(IsValidSlotName("dsu.test"));

    EXPECT_FALSE(IsValidSlotName(""));
    EXPECT_FALSE(IsValidSlotName("."));
    EXPECT_FALSE(IsValidSlotName(".."));
    EXPECT_FALSE(IsValidSlotName("booted"));
    EXPECT_FALSE(IsValidSlotName("active"));
    EXPECT_FALSE(IsValidSlotName("lp_names"));
    EXPECT_FALSE(IsValidSlotName("install_status"));
    EXPECT_FALSE(IsValidSlotName("one_shot_boot"));
    EXPECT_FALSE(IsValidSlotName("avb"));
    EXPECT_FALSE(IsValidSlotName("avb_enforce"));

    EXPECT_FALSE(IsValidSlotName("foo/bar"));
    EXPECT_FALSE(IsValidSlotName("foo@bar"));
}

TEST(LibGsiTest, IsGsiRunning) {
    // This test requires root to modify /metadata/gsi/dsu/booted
    if (getuid() != 0) {
        GTEST_SKIP() << "Skipping test that requires root";
    }

    std::string booted_path = kGsiBootedIndicatorFile;
    struct stat s;
    bool was_booted = (lstat(booted_path.c_str(), &s) == 0);
    std::string backup_path = booted_path + ".bak";

    if (was_booted) {
        ASSERT_EQ(rename(booted_path.c_str(), backup_path.c_str()), 0);
    }

    // Ensure it returns false when it doesn't exist
    EXPECT_FALSE(IsGsiRunning());

    // Ensure it returns false when it is a directory (the bug scenario)
    ASSERT_EQ(mkdir(booted_path.c_str(), 0755), 0);
    EXPECT_FALSE(IsGsiRunning());
    ASSERT_EQ(rmdir(booted_path.c_str()), 0);

    // Ensure it returns false when it is a symlink (even to a regular file)
    std::string temp_file = "/data/local/tmp/gsi_temp";
    int fd = open(temp_file.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);
    close(fd);
    ASSERT_EQ(symlink(temp_file.c_str(), booted_path.c_str()), 0);
    EXPECT_FALSE(IsGsiRunning());
    ASSERT_EQ(unlink(booted_path.c_str()), 0);
    ASSERT_EQ(unlink(temp_file.c_str()), 0);

    // Ensure it returns true when it is a regular file
    fd = open(booted_path.c_str(), O_CREAT | O_WRONLY, 0644);
    ASSERT_GE(fd, 0);
    close(fd);
    EXPECT_TRUE(IsGsiRunning());
    ASSERT_EQ(unlink(booted_path.c_str()), 0);

    // Restore
    if (was_booted) {
        ASSERT_EQ(rename(backup_path.c_str(), booted_path.c_str()), 0);
    }
}

} // namespace gsi
} // namespace android
