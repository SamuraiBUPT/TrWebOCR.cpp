#include "tr_wrapper.h"

int tr_run_image_from_local(const char* image_path, 
                            int ctpn_id, int crnn_id,
                            float* rect, int* unicode, float* prob) {
    // int line_nums = tr_run(ctpn_id, crnn_id, 
    //                       image_path, 0, 0, 0,
    //                       2, 
    //                       rect, 512,
    //                       unicode, prob, 512);
    int line_nums = 1;
    return line_nums;
}

int tr_run_image_from_ndarray(unsigned char* array, 
                             int ctpn_id, int crnn_id,
                             int height, int width,
                             int CV_TYPE,
                             int rotate_flag,
                             float* rect, int max_lines,
                             int* unicode, float* prob, int max_width){
    int line_nums = tr_run(ctpn_id, crnn_id, 
                          array, height, width, CV_TYPE,
                          rotate_flag, 
                          rect, max_lines,
                          unicode, prob, max_width);
    return line_nums;
}