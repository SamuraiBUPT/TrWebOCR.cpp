#include <stdio.h>
#include <fstream>
#include <iostream>
#include <chrono>

#include "tr_wrapper.h"
#include "tr_worker.h"

int main() {
    printf("Initializing TR engine...\n");
    tr_init(0, 0, (void*)(CTPN_PATH), NULL);
    tr_init(0, 1, (void*)(CRNN_PATH), NULL);
    // int h = 119;
    // int w = 643;
    int CV_TYPE = 0;
    int flag = 2;
    int max_lines = 512;
    int max_width = 512;

    auto allocate_start = std::chrono::high_resolution_clock::now();
    float* rect = new float[max_lines * 6]();
    int* unicode = new int[max_lines * max_width]();
    float* prob = new float[max_lines * max_width]();
    auto allocate_end = std::chrono::high_resolution_clock::now();

    const char* img_path = "../img.png";

    // int line_num = tr_run(0, 1, 
    //     img, h, w, CV_TYPE, 
    //     flag, 
    //     rect, max_lines,
    //     unicode, prob, max_width);

    auto start = std::chrono::high_resolution_clock::now();

    int line_num = tr_run_image_from_local(img_path, 0, 1, rect, unicode, prob);

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "耗时：" << duration.count() / 1000.0 << "毫秒" << std::endl;

    std::cout << "检测到 " << line_num << " 行文本。" << std::endl;

    std::vector<Result> results = process_results(line_num, rect, unicode, prob);

    for (const auto& result : results) {
        const auto& rect = std::get<0>(result);
        const auto& txt = std::get<1>(result);
        const auto& confidence = std::get<2>(result);

        std::cout << "Rect: ";
        for (float r : rect) {
            std::cout << r << " ";
        }
        std::cout << ", Text: " << txt << ", Confidence: " << confidence << std::endl;
    }


    // 使用完成后释放内存
    auto deallocate_start = std::chrono::high_resolution_clock::now();
    delete[] rect;
    delete[] unicode;
    delete[] prob;
    auto deallocate_end = std::chrono::high_resolution_clock::now();

    auto allocate_duration = std::chrono::duration_cast<std::chrono::microseconds>(allocate_end - allocate_start);
    auto deallocate_duration = std::chrono::duration_cast<std::chrono::microseconds>(deallocate_end - deallocate_start);
    std::cout << "内存分配耗时：" << allocate_duration.count() / 1000.0 << "毫秒" << std::endl;
    std::cout << "内存释放耗时：" << deallocate_duration.count() / 1000.0 << "毫秒" << std::endl;

    return 0;
}