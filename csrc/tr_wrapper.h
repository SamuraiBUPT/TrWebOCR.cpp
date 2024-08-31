#include "tr.h"

// Tr OCR for a local image, the first param is the local path of the image
// the last 3 params are also output params, rect, unicode and probs
// 
// rect: [max_lines * 6] -> 512 * 6
// unicode: [max_lines * max_width] -> 512 * 512
// probs: [max_lines * max_width] -> 512 * 512
int tr_run_image_from_local(const char* image_path, 
                            int ctpn_id, int crnn_id,
                            float* rect, int* unicode, float* prob);

// Tr OCR for a ndarray image, the first param is the ndarray pointer. Not implemented yet.
int tr_run_image_from_ndarray(unsigned char* array, 
                             int ctpn_id, int crnn_id,
                             int height, int width,
                             int CV_TYPE,
                             int rotate_flag,
                             float* rect, int max_lines,
                             int* unicode, float* prob, int max_width);

