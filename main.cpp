#include "funcs.hpp"

const char* STREAM_URL = "http://192.168.1.5:4747/video";
const int DEBUG = 0; // Set this to true for debug mode
const float L = 0.5; // m
const float PXPM = 1000; // px por metro
const int TOLERANCE = 5; // This value defines the range of movement that the algorith considers as the starting point.

int main() {

    cv::VideoCapture cap(STREAM_URL);

    if (!cap.isOpened()) {
        printf("Error; cannot open stream. Check URL?");
        return -1;
    }

    cv::namedWindow("Video", cv::WINDOW_AUTOSIZE);

    // Essential lines for fine tuning. Would be nice to turn this number range into strings for user
    cv::Scalar lower(0, 100, 90); // lower bound 
    cv::Scalar upper(80 , 255, 255); // upper bound
    // FINE TUNE THIS FOR READING!!!

    cv::Mat frame, treated, mask, hsv;
    std::vector<std::vector<cv::Point>> contour_vec; // stores ALL contours
    std::vector<cv::Vec4i> hierarchy; // needed by findContours
    std::vector<cv::Point2f> positions; // Stores the positions of the pendulum
    std::vector<cv::Point2f> amplitudes; // Stores the peak of the pendulums

    cv::Point2f center;
    int cx = 0, cy = 0, initial_x, movement_flag = 0;
    float radius = 0.0f, t = 0.0f, x = 0.0f;

    const float fps = cap.get(cv::CAP_PROP_FPS);

    if(DEBUG) {
        printf("Entering Debug mode... (Change DEBUG to 0 if this was not intentional)\n");

        while (true) {
            cap >> frame;           
            if (frame.empty()) break; 
            if (cv::waitKey(10) == 27) break; // ESC to quit

            // Essentially, HSV is a color steriod that then gets masked into a color range
            cv::GaussianBlur(frame, treated, cv::Size(5,5), 0);
            cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

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

    fprintf(gnuplotPipe, "set terminal wxt\n"); // sets terminal type. change this if experiencing issues

    fprintf(gnuplotPipe, "set title 'Pendulum'\n");
    fprintf(gnuplotPipe, "set xlabel 'Time (s)'\n");
    fprintf(gnuplotPipe, "set ylabel 'X position (cm)'\n");
    fprintf(gnuplotPipe, "set grid\n");
    fprintf(gnuplotPipe, "set autoscale x\n");
    fprintf(gnuplotPipe, "set autoscale y\n"); // GNUplot settings
    fflush(gnuplotPipe);

    static int frameCount = 0;
    std::vector<cv::Point2f> positions; // This vector will store all data points


    while (true) {
        cap >> frame;
        frameCount++;     

        if (frame.empty() || cv::waitKey(10) == 27) { // ESC to quit
            fflush(gnuplotPipe);
            break;
        } 


        /* For anyone readaing this, hear me out... for some odd reason, the first few frames gets read for some
        really weird values, so here i decided to skip them... I doubt the problem is in the code, most likely with
        the DroidCam app i am using and my shitty phone... Well, this is my fix for it so dont fret */

        if(frameCount < 8)
            continue;

        // Essentially, HSV is a color steriod that then gets masked into a color range
        cv::GaussianBlur(frame, treated, cv::Size(5,5), 0);
        cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

        cv::inRange(hsv, lower, upper, mask);

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

        // Centers
        cx = int (M.m10 / M.m00);
        cy = int (M.m01 / M.m00);

        cv::minEnclosingCircle(contour_vec[contour_index], center, radius);

        cv::circle(frame, center, (int)radius, cv::Scalar(0, 255, 0), 2);
        cv::circle(frame, cv::Point(cx, cy), 5, cv::Scalar(150, 0, 255), -1);
        
        t = (float)frameCount / fps;
        x = (float)cx / PXPM * 100;

        cv::Point2f current_point = cv::Point2f(t, x);
        positions.push_back(cv::Point2f(t, x));

        if(positions.size() == 1) {
            initial_x = x;
        }

        if (((x > initial_x + TOLERANCE) || (x < initial_x - TOLERANCE)) && movement_flag == 0) // Necessary so that the amplitudes vec doesnt get stored with junk
            movement_flag = 1;


        if(movement_flag)
            find_and_store_peaks(positions, amplitudes);
        
        fprintf(gnuplotPipe,
            "plot '-' using 1:2 with lines title 'Pendulum X(t)' lc rgb 'blue', "
            "'-' using 1:2 with points title 'Amplitudes' pt 7 ps 1.5 lc rgb 'red'\n");

        for (const auto& point : positions)
            fprintf(gnuplotPipe, "%f %f\n", point.x, point.y);
        fprintf(gnuplotPipe, "e\n");

        
        for (const auto& point : amplitudes)
           fprintf(gnuplotPipe, "%f %f\n", point.x, point.y);
        fprintf(gnuplotPipe, "e\n");

        fflush(gnuplotPipe);
        
        cv::imshow("Video", frame); 
    
    }

    pclose(gnuplotPipe);

    filter_amplitudes(amplitudes);
    Pendulum pend = calculate_variables(amplitudes, L, PXPM, initial_x);
    print_pendulum(pend);

    }
}


