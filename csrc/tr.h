// tr.h
// Header file for the TR library, declare that these functions have been defined in the library.
// Author: SamuraiBUPT
// Date: 2024-08-28

#ifndef TR_H
#define TR_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CTPN_PATH
#define CTPN_PATH "tr/tr/ctpn.so"
#endif

#ifndef CRNN_PATH
#define CRNN_PATH "tr/tr/crnn.so"
#endif



// Function declarations based on strings found
void tr_init(int process_id, int session_id, void* model_path, void* arg);

// tr_release: 释放资源函数，接收一个int参数
void tr_release(int arg1);

// tr_detect: 检测函数，接收多个参数，返回int类型结果
int tr_detect(int ctpn_id, void* img_ptr, int height, int width, int channel, 
              int rotate_flag, void* rect_ptr, int max_lines);

// tr_recognize: 识别函数，接收多个参数，返回int类型结果
int tr_recognize(int crnn_id, void* img_ptr, int height, int width, int channel, 
                 void* unicode, void* probs, int max_width);

// tr_run: 综合运行函数，接收多个参数，返回int类型结果
int tr_run(int ctpn_id, int crnn_id, 
           void* img_ptr, int height, int width, int CV_TYPE, 
           int rotate_flag, 
           float* rect, int max_lines, 
           int* unicode, float* probs, int max_width);

// tr_crnn: CRNN算法函数，接收多个参数，返回int类型结果
int tr_crnn(int crnn_id, 
            void* img_ptr, int height, int width, 
            void* buffer, void* shape, int max_items);

#ifdef __cplusplus
}
#endif

#endif // TR_H