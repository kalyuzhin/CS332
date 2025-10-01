#include <opencv2/opencv.hpp>
#include <iostream>

using namespace std;

#include <vector>
cv::Mat image;
vector<cv::Point> contour;

const cv::Vec3b borderColor = cv::Vec3b(255, 255, 255);
const cv::Vec3b areaColor = cv::Vec3b(0, 0, 0);
const cv::Vec3b red = cv::Vec3b(0, 0, 255);
const string windowName = "Task 1.3";
int start_x, start_y;

const int dx[8] = {  0, -1, -1, -1,  0,  1, 1, 1 };
const int dy[8] = {  1,  1,  0, -1, -1, -1, 0, 1 };


void traceContour()
{
    int h = image.rows, w = image.cols;
    int current_y = start_y, current_x = start_x;
    int dir = 0;

    contour.push_back(cv::Point(current_x, current_y));

    do {
        bool found = false;
        for (int k = 0; k < 8; ++k)
        {
            int new_dir = (dir + k) % 8;
            int new_y = current_y + dy[new_dir];
            int new_x = current_x + dx[new_dir];
            if (new_y >= 0 && new_y < h && new_x >= 0 && new_x < w && image.at<cv::Vec3b>(cv::Point(new_x, new_y)) == borderColor)
            {
                contour.push_back(cv::Point(new_x, new_y));
                current_y = new_y; current_x = new_x;
                dir = (new_dir + 6) % 8;
                found = true;
                break;
            }
        }
        if (!found) break; 
    } while (!(current_y == start_y && current_x == start_x && contour.size() > 1));
    for (cv::Point p : contour) {
        image.at < cv::Vec3b >(p) = red;
    }
    
}



void findStartPoint(int x, int y) {
    while (image.at<cv::Vec3b>(cv::Point(x, y)) == borderColor) {
        x++;
    }
    x--;
    start_x = x; start_y = y;
}

void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == cv::EVENT_LBUTTONDOWN) {
        // x - col; y - row
        findStartPoint(x, y);
        traceContour();
        imshow(windowName, image);
    }
}

int main()
{
    string filename;
    /*cout << "Enter filename: ";
    cin >> filename;*/
    filename = "kchau.png";
    cv::namedWindow(windowName);
    cv::setMouseCallback(windowName, mouseCallback, NULL);
    image = cv::imread(filename);
    if (image.empty()) {
        std::cout << "Could not open image!" << std::endl;
        return -1;
    }
    cout << image.size << endl;
    imshow(windowName, image);

    
    

    cv::waitKey(0);
    return 0;
}