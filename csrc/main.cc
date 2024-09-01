#include <stdio.h>
#include <fstream>
#include <iostream>
#include <chrono>

#include "tr_wrapper.h"
#include "tr_worker.h"

// third party libraries
#include "httplib.h"
#include "json.hpp"
#include "stb_image.h"
#include "base64.h"

using json = nlohmann::json;
using namespace httplib;

void test_inference() {
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

    std::vector<TrResult> results = process_results(line_num, rect, unicode, prob);

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
};

std::string dump_headers(const Headers &headers) {
    std::string s;
    char buf[BUFSIZ];

    for (const auto &x : headers) {
        snprintf(buf, sizeof(buf), "%s: %s\n", x.first.c_str(), x.second.c_str());
        s += buf;
    }

    return s;
}

std::string dump_multipart_files(const MultipartFormDataMap &files) {
    std::string s;
    char buf[BUFSIZ];

    s += "--------------------------------\n";

    for (const auto &x : files) {
        const auto &name = x.first;
        const auto &file = x.second;

        snprintf(buf, sizeof(buf), "name: %s\n", name.c_str());
        s += buf;

        snprintf(buf, sizeof(buf), "filename: %s\n", file.filename.c_str());
        s += buf;

        snprintf(buf, sizeof(buf), "content type: %s\n", file.content_type.c_str());
        s += buf;

        snprintf(buf, sizeof(buf), "text length: %zu\n", file.content.size());
        s += buf;

        s += "----------------\n";
    }

    return s;
}

std::string log(const Request &req, const Response &res) {
    std::string s;
    char buf[BUFSIZ];

    s += "================================\n";

    snprintf(buf, sizeof(buf), "%s %s %s", req.method.c_str(),
            req.version.c_str(), req.path.c_str());
    s += buf;

    std::string query;
    for (auto it = req.params.begin(); it != req.params.end(); ++it) {
        const auto &x = *it;
        snprintf(buf, sizeof(buf), "%c%s=%s",
                (it == req.params.begin()) ? '?' : '&', x.first.c_str(),
                x.second.c_str());
        query += buf;
    }
    snprintf(buf, sizeof(buf), "%s\n", query.c_str());
    s += buf;

    s += dump_headers(req.headers);
    s += dump_multipart_files(req.files);

    s += "--------------------------------\n";

    snprintf(buf, sizeof(buf), "%d\n", res.status);
    s += buf;
    s += dump_headers(res.headers);

    return s;
}

int main() {
    printf("Initializing TR binary...\n");
    int ctpn_id = 0;
    int crnn_id = 1;

    tr_init(0, ctpn_id, (void*)(CTPN_PATH), NULL);
    tr_init(0, crnn_id, (void*)(CRNN_PATH), NULL);

    // test_inference();

    TrThreadPool tr_task_pool(6);

    int port = 8008;
    Server svr;

    svr.Get("/hi", [](const Request& req, Response& res) {
        res.set_content("Hello World!", "text/plain");
    });

    svr.Post("/api/trocr", [&tr_task_pool, ctpn_id, crnn_id](const Request& req, Response& res) {
        try {
            int width, height, channels;
            unsigned char* image_data = nullptr;

            if (req.has_file("file")) {
                const auto& file = req.get_file_value("file");
                // 从文件内容中加载图像
                // 注意我们在加载的时候会被load为灰度图像
                image_data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(file.content.data()), file.content.size(), &width, &height, &channels, 0);
            } else if (req.has_param("img_base64")) {
                std::string img_base64 = req.get_param_value("img_base64");
                std::string decoded_data = base64_decode(img_base64);  // 解码Base64数据

                // 从解码后的数据中加载图像
                image_data = stbi_load_from_memory(reinterpret_cast<const unsigned char*>(decoded_data.data()), decoded_data.size(), &width, &height, &channels, 0);
            } else {
                res.set_content("Missing file or img_base64 parameter", "text/plain");
                return;
            }

            if (!image_data) {
                res.set_content("Failed to load image", "text/plain");
                return;
            }

            // // 创建或打开一个txt文件
            // std::ofstream out_file("origin_gray_image_cpp.txt");

            // if (out_file.is_open()) {
            //     // 将灰度图像数据写入文件
            //     for (int i = 0; i < width * height; ++i) {
            //         out_file << static_cast<int>(image_data[i]) << " "; // 写入每个像素的灰度值
            //         if ((i + 1) % width == 0) {
            //             out_file << "\n";  // 每行写满一个图像宽度后换行
            //         }
            //     }
            //     out_file.close(); // 关闭文件
            // } else {
            //     std::cerr << "无法打开文件" << std::endl;
            // }

            if (channels == 3) {
                unsigned char* gray_data = new unsigned char[width * height];
                for (int i = 0; i < width * height; ++i) {
                    int r = image_data[i * channels];
                    int g = image_data[i * channels + 1];
                    int b = image_data[i * channels + 2];
                    gray_data[i] = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
                }
                stbi_image_free(image_data);  // 释放原始图像数据
                image_data = gray_data;  // 更新指针为灰度图像数据
                channels = 1;  // 更新通道数为1
            } else if (channels == 4) {
                unsigned char* gray_data = new unsigned char[width * height];
                for (int i = 0; i < width * height; ++i) {
                    int r = image_data[i * channels];
                    int g = image_data[i * channels + 1];
                    int b = image_data[i * channels + 2];
                    int a = image_data[i * channels + 3];

                    // 使用预乘Alpha的方式计算灰度值
                    r = r * a / 255;
                    g = g * a / 255;
                    b = b * a / 255;

                    gray_data[i] = static_cast<unsigned char>(0.299 * r + 0.587 * g + 0.114 * b);
                }
                stbi_image_free(image_data);  // 释放原始图像数据
                image_data = gray_data;  // 更新指针为灰度图像数据
                channels = 1;  // 更新通道数为1
            }

            // 把image_data数组的数据写入txt
            // 创建或打开一个txt文件
            std::ofstream out_file2("gray_image_cpp.txt");

            if (out_file2.is_open()) {
                // 将灰度图像数据写入文件
                for (int i = 0; i < width * height; ++i) {
                    out_file2 << static_cast<int>(image_data[i]) << " "; // 写入每个像素的灰度值
                    if ((i + 1) % width == 0) {
                        out_file2 << "\n";  // 每行写满一个图像宽度后换行
                    }
                }
                out_file2.close(); // 关闭文件
            } else {
                std::cerr << "无法打开文件" << std::endl;
            }

            // 现在开始处理图像数据
            // 调用任务池中的任务接口（假设任务接口能够处理 image_data 指针）
            auto start = std::chrono::high_resolution_clock::now();
            auto future = tr_task_pool.enqueue(image_data, width, height, channels, ctpn_id, crnn_id);
            std::vector<TrResult> results = future.get();   // inference
            auto end = std::chrono::high_resolution_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
            std::cout << "time: " << duration.count() / 1000.0 << " ms" << std::endl;

            // 处理结果并返回响应
            std::string result_str;
            for (const auto& result : results) {
                std::vector<float> rect = std::get<0>(result);
                std::string txt = std::get<1>(result);
                // std::cout << txt << std::endl;
                float confidence = std::get<2>(result);
                result_str += txt + "\n";
            }
            res.set_content(result_str, "text/plain");

            // 释放图像数据
            stbi_image_free(image_data);

        } catch (const std::exception& e) {
            res.set_content("Invalid JSON data", "text/plain");
        }
    });

    // svr.set_logger(
    //   [](const Request &req, const Response &res) { std::cout << log(req, res); });

    printf("Server listening on http://localhost:%d\n", port);

    svr.listen("localhost", port);

    return 0;
}