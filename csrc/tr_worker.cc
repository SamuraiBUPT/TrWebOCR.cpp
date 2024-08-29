#include <iostream>

#include "tr_worker.h"
#include "tr_wrapper.h"

// 将 Unicode 码点转换为字符
inline char unichr(int unicode) {
    return static_cast<char>(unicode); // 对于宽字符，使用 wchar_t 和 wstring
}

std::pair<std::string, float> parse(const int* unicode_arr, const float* prob_arr, int num) {
    std::string txt;
    float prob = 0.0;
    int unicode_pre = -1;
    int count = 0;

    for (int pos = 0; pos < num; ++pos) {
        int unicode = unicode_arr[pos];
        if (unicode >= 0) {
            if (unicode != unicode_pre) {
                txt += unichr(unicode);  // 将 Unicode 转换为字符并追加到字符串
            }
            count += 1;
            prob += prob_arr[pos];
        }
        unicode_pre = unicode;
    }

    float confidence = prob / std::max(count, 1);  // 计算平均置信度

    // std::cout << "text is: " << txt << std::endl;
    return {txt, confidence};
}

std::vector<Result> process_results(int line_num, float* rect_arr, int* unicode_arr, float* prob_arr) {
    std::vector<Result> results;

    for (int i = 0; i < line_num; ++i) {
        int num = static_cast<int>(rect_arr[i * 6 + 5] + 0.5);  // 计算 num

        // TODO: 这个地方512不是固定的，要改
        auto [txt, confidence] = parse(&unicode_arr[i * 512], &prob_arr[i * 512], num);

        // 6是RECT的固定width，相当于一个leading dimension
        std::vector<float> rect(rect_arr + i * 6, rect_arr + i * 6 + 5);  // 提取矩形区域
        results.emplace_back(rect, txt, confidence);
    }

    return results;
}

void TrThreadPool::start(size_t numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([=]() {
            // 每个线程持有的内存
            // TODO: 这里需要换成vector，方便做扩容、缩容。
            float* rect = new float[512 * 6](); // 假设最大行数为 512，RECT_SIZE 为 6
            int* unicode = new int[512 * 512](); // 假设最大行数为 512，max_width 为 512
            float* prob = new float[512 * 512](); // 同上

            while (true) {
                TrTask task;
                {
                    std::unique_lock<std::mutex> lock(eventMutex);

                    eventVar.wait(lock, [=]() { return stopFlag || !tasks.empty(); });

                    if (stopFlag && tasks.empty())
                        break;

                    task = std::move(tasks.front());
                    tasks.pop();
                }

                // 清空内存
                std::fill(rect, rect + 512 * 6, 0.0f);
                std::fill(unicode, unicode + 512 * 512, 0);
                std::fill(prob, prob + 512 * 512, 0.0f);

                // 执行任务，传递给 task 函数
                int line_num = task(rect, unicode, prob);
            }

            // 释放内存
            delete[] rect;
            delete[] unicode;
            delete[] prob;
        });
    }
};

void TrThreadPool::stop() {
    {
        std::unique_lock<std::mutex> lock(eventMutex);
        stopFlag = true;
    }

    eventVar.notify_all();

    for (std::thread &thread : threads)
        thread.join();
}

void TrThreadPool::enqueue(TrTask task, const char* image_path, int ctpn_id, int crnn_id) {
    {
        std::unique_lock<std::mutex> lock(eventMutex);
        tasks.emplace([=](float* rect, int* unicode, float* prob) {
            return tr_run_image_from_local(image_path, ctpn_id, crnn_id, rect, unicode, prob);
        });
    }
    eventVar.notify_one();
}

TrThreadPool::TrThreadPool(size_t numThreads) {
    start(numThreads);
}

TrThreadPool::~TrThreadPool() {
    stop();
}
