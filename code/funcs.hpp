#ifndef FUNCS_HPP
#define FUNCS_HPP

#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdio>
#include <vector>

struct Pendulum {
    float A;    
    float b;
    float omega;
    float quality_factor;
    float T_exp;
    float T_teo;
    float erro;
    float init_pos;
};

int largest_contour_index(std::vector<std::vector<cv::Point>> vec);
void find_and_store_peaks(std::vector<cv::Point2f> &positions, std::vector<cv::Point2f> &amplitudes);
void filter_amplitudes(std::vector<cv::Point2f> &amplitudes);
void print_pendulum(Pendulum pend);
Pendulum calculate_variables(std::vector<cv::Point2f> &amplitudes, float l, float pxpm, double y_eq_px);

#endif