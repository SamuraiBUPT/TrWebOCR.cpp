#include <stdio.h>
#include "tr.h"
#include <fstream>
#include <iostream>

int main() {
    printf("Initializing TR engine...\n");
    tr_init(0, 0, (void*)(CTPN_PATH), NULL);
    tr_init(0, 1, (void*)(CRNN_PATH), NULL);
    int h = 119;
    int w = 643;
    int chan = 0;
    int flag = 2;
    int max_new_lines = 512;
    int max_len = 512;

    int *img = new int[h * w];

    // 打开文件
    std::ifstream file("../img.txt");
    if (!file) {
        std::cerr << "无法打开文件 img.txt" << std::endl;
        delete[] img;
        return -1;
    }

    // 读取文件内容到 img 数组中
    for (int i = 0; i < h*w; ++i) {
        if (!(file >> img[i])) {
            std::cerr << "读取文件时出错或文件中的元素不足。" << std::endl;
            delete[] img;
            return -1;
        }
    }

    file.close();

    std::cout << "第一个元素是: " << img[0] << std::endl;
    std::cout << "最后一个元素是: " << img[h*w-1] << std::endl;

    float* rect = new float[max_new_lines * 6]();
    int* unicode = new int[max_new_lines * max_len]();
    float* prob = new float[max_new_lines * max_len]();

    int line_num = tr_run(0, 1, 
        img, h, w, chan, 
        flag, 
        rect, max_new_lines,
        unicode, prob, max_len);

    std::cout << "检测到 " << line_num << " 行文本。" << std::endl;

    // 使用完成后释放内存
    delete[] img;
    delete[] rect;
    delete[] unicode;
    delete[] prob;

    return 0;
}