#include "tr_wrapper.h"

int tr_run_image_from_local(const char* image_path, 
                            int ctpn_id, int crnn_id,
                            float* rect, int* unicode, float* prob) {
    int line_nums = tr_run(ctpn_id, crnn_id, 
                          (void*)image_path, 0, 0, 0,
                          2, 
                          rect, 512,
                          unicode, prob, 512);
    return line_nums;
}

int tr_run_image_from_ndarray(void* array, 
                             int ctpn_id, int crnn_id,
                             int height, int width,
                             int CV_TYPE,
                             int rotate_flag,
                             float* rect, int max_lines,
                             int* unicode, float* prob, int max_width){
    return 0;
}