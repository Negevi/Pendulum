#include <opencv2/opencv.hpp>
#include <iostream>
#include <cstdio>

const char* STREAM_URL = "http://192.168.1.5:4747/video";
const int DEBUG = 0; // Set this to true for debug mode
const int M = 173; // g
const int L = 50; // cm
const float PXCM = 0.12; // cm em 1 px
const float FPS = 30;


int largest_contour_index(std::vector<std::vector<cv::Point>> vec) {
    int area = 0, index = 0; // Returns 0 if null
    for(int i = 0; i < vec.size(); i++) {
        int comparator = cv::contourArea(vec[i]);
        if(area < comparator) {
            area = comparator;
            index = i;
        }
    }
    return index;
}

int main() {

    cv::VideoCapture cap(STREAM_URL);

    if (!cap.isOpened()) {
        printf("Error; cannot open stream. Check URL?");
        return -1;
    }

    cv::Mat frame, treated, mask, hsv;
    cv::namedWindow("Video", cv::WINDOW_AUTOSIZE);

    int cx, cy;
    float radius;
    cv::Point2f center;

    if(DEBUG) {
        printf("Entering Debug mode... (Change DEBUG to 0 if this was not intentional)\n");

        while (true) {
            cap >> frame;           
            if (frame.empty()) break; 
            if (cv::waitKey(10) == 27) break; // ESC to quit

            // Essentially, HSV is a color steriod that then gets masked into a color range
            cv::GaussianBlur(frame, treated, cv::Size(5,5), 0);
            cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

            // Essential lines for fine tuning. Would be nice to turn this number range into strings for user
            cv::Scalar lower(0, 100, 90); // lower bound 
            cv::Scalar upper(80, 255, 255); // upper bound
            // FINE TUNE THIS FOR READING!!!

            cv::inRange(hsv, lower, upper, mask);

            std::vector<std::vector<cv::Point>> contour_vec; // stores ALL cont
            std::vector<cv::Vec4i> hierarchy; // needed by findContours
            cv::findContours(mask, contour_vec, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            // findContour finds all possivble contours. Need to be filtered (finds largest)

            cv::imshow("Video", frame); 
            cv::imshow("HSV", hsv);             
            cv::imshow("Mask", mask); 
        }

    } else {

        FILE* gnuplotPipe = popen("gnuplot -persistent", "w");

        if (!gnuplotPipe) {
            printf("Error opening pipe to GNUplot");
            return -1;
        }

        fprintf(gnuplotPipe,
            "set title 'Pendulum'\n"
            "set xlabel 'Time'\n"
            "set ylabel 'X position'\n"
            "set grid\n"
            "set autoscale x\n"
            "set yrange [0:*]\n"
            "plot '-' using 1:2 with lines title 'Pendulum X(t)' lc rgb 'blue'\n"
        );
        fflush(gnuplotPipe);

        static int frameCount = 0;
        std::vector<cv::Point2f> positions;


        while (true) {
            cap >> frame;           
            if (frame.empty() || cv::waitKey(10) == 27) { // ESC to quit
                fprintf(gnuplotPipe, "e\n"); // signal end of data
                pclose(gnuplotPipe);
                break;
            } 

            // Essentially, HSV is a color steriod that then gets masked into a color range
            cv::GaussianBlur(frame, treated, cv::Size(5,5), 0);
            cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

            // Essential lines for fine tuning. Would be nice to turn this number range into strings for user
            cv::Scalar lower(0, 100, 90); // lower bound 
            cv::Scalar upper(80 , 255, 255); // upper bound
            // FINE TUNE THIS FOR READING!!!

            cv::inRange(hsv, lower, upper, mask);

            std::vector<std::vector<cv::Point>> contour_vec; // stores ALL cont
            std::vector<cv::Vec4i> hierarchy; // needed by findContours
            cv::findContours(mask, contour_vec, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

            // findContour finds all possivble contours. Need to be filtered (finds largest)
            int contour_index = largest_contour_index(contour_vec);
            
            if(contour_index == -1 || contour_vec.empty()) {
                printf("No pendulum found.... retring in 3 seconds...\n");
                cv::waitKey(3000);
                continue;
            }

            cv::Moments M = cv::moments(contour_vec[contour_index]); // M gives information on the contour found

            if(M.m00 == 0) M.m00 = 1; // Not dividing by zero

            // Calculates center positions
            cx = int (M.m10 / M.m00);
            cy = int (M.m01 / M.m00);

            cv::minEnclosingCircle(contour_vec[contour_index], center, radius);

            cv::circle(frame, center, (int)radius, cv::Scalar(0, 255, 0), 2);
            cv::circle(frame, cv::Point(cx, cy), 5, cv::Scalar(150, 0, 255), -1);
            
            fprintf(gnuplotPipe, "%f %f\n", frameCount/FPS, cx/PXCM);
            fflush(gnuplotPipe);

            frameCount++;

            cv::imshow("Video", frame); 
        }

        fprintf(gnuplotPipe, "e\n"); // end of data stream
        pclose(gnuplotPipe);
    }   
}



