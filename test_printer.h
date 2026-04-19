#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <unistd.h>
#include <vector>

#define C1 "\e[31m"
#define C2 "\e[32m"
#define C3 "\e[33m"
#define C6 "\e[36m"
#define CR "\e[m"

namespace fs = std::filesystem;

// Gets the position of an element in a vector. Returns -1 if not found.
template <typename T> int position(const std::vector<T> &vec, const T &query) {
    for (int i = 0, n = vec.size(); i < n; ++i) {
        if (vec[i] == query) {
            return i;
        }
    }
    return -1;
}

// Counts the number of components in a path.
inline int components(const fs::path &path) {
    return std::distance(path.begin(), path.end());
}

class MinimalistPrinter : public testing::EmptyTestEventListener {
    void OnTestStart(const testing::TestInfo &t) override {
        std::cout << t.test_suite_name() << '.' << t.name() << " ... ";
        std::flush(std::cout);
    }

    // Called after a failed assertion or a SUCCESS().
    void OnTestPartResult(const testing::TestPartResult &tpr) override {
        if (tpr.skipped() || !tpr.failed()) {
            return;
        }
#define SEP "--"
        std::cout << std::endl << SEP << std::endl;
        if (is_atty_) {
            std::cout << C1 << "Assertion Error in " << CR;
            std::cout << C3 << tpr.file_name() << ':' << tpr.line_number()
                      << CR;
        } else {
            std::cout << "Assertion Error in ";
            std::cout << tpr.file_name() << ':' << tpr.line_number();
        }
        std::cout << std::endl << SEP << std::endl;
        std::cout << tpr.summary();
        std::cout << SEP << std::endl;
    }

    void OnTestEnd(const testing::TestInfo &t) override {
        if (is_atty_) {
            std::cout << C2 << "[ok]" << CR;
        } else {
            std::cout << "[ok]";
        }
        long mils = t.result()->elapsed_time();
        if (mils != 0) {
            std::cout << " (" << mils << "ms)";
        }
        std::cout << std::endl;
    }

    void OnTestProgramEnd(const testing::UnitTest &u) override {
        if (u.Passed()) {
            if (is_atty_) {
                std::cout << C2 "All tests passed! " CR;
            } else {
                std::cout << "All tests passed! ";
            }
            std::cout << u.elapsed_time() << "ms elapsed." << std::endl;
        }
    }

  private:
    const bool is_atty_{isatty(STDOUT_FILENO) ? true : false};
};
