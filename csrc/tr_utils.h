#ifndef TRUTILS_H
#define TRUTILS_H
#include <string>
#include <iostream>
#include <fstream>

template<typename T>
void print_matrix(T* matrix, int rows, int cols) {
    for(int i=0; i<rows; i++) {
        for(int j=0; j<cols; j++) {
            std::cout << matrix[i*cols+j] << " ";
        }
        std::cout << std::endl;
    }
}

template<typename T>
void save_matrix(T* matrix, int rows, int cols, std::string filename) {
    std::ofstream outfile(filename);
    for(int i=0; i<rows; i++) {
        for(int j=0; j<cols; j++) {
            outfile << static_cast<int>(matrix[i * cols + j]) << " ";
        }
        outfile << std::endl;
    }
    outfile.close();
}

#endif