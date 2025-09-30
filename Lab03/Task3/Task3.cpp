#include <opencv2/opencv.hpp>
#include <iostream>
#include <windows.h>
#include <random>
#include <cstdlib>
#include <ctime>

using namespace cv;
using namespace std;
const int WIDTH = 800;
const int HEIGHT = 600;
const int MAX_COLOR = 256;

struct MyPoint
{
    int x, y;
    int r, g, b;
};

Mat colorMatrix;
Mat image;
vector<MyPoint> clickPoints;

int findMinX(const vector<MyPoint>& vec) {
    if (vec.empty()) {
        throw std::runtime_error("Vector is empty");
    }
    int minX = vec[0].x;
    for (const MyPoint& v : vec) {
        if (v.x < minX) {
            minX = v.x;
        }
    }
    return minX;
}

int findMaxX(const vector<MyPoint>& vec) {
    if (vec.empty()) {
        throw std::runtime_error("Vector is empty");
    }
    int maxX = vec[0].x;
    for (const MyPoint& v : vec) {
        if (v.x > maxX) {
            maxX = v.x;
        }
    }
    return maxX;
}

int findMinY(const vector<MyPoint>& vec) {
    if (vec.empty()) {
        throw std::runtime_error("Vector is empty");
    }
    int minY = vec[0].y;
    for (const MyPoint& v : vec) {
        if (v.y < minY) {
            minY = v.y;
        }
    }
    return minY;
}

int findMaxY(const vector<MyPoint>& vec) {
    if (vec.empty()) {
        throw std::runtime_error("Vector is empty");
    }
    int maxY = vec[0].y;
    for (const MyPoint& v : vec) {
        if (v.y > maxY) {
            maxY = v.y;
        }
    }
    return maxY;
}

void mouseCallback(int event, int x, int y, int flags, void* userdata) {
    if (event == EVENT_LBUTTONDOWN) {
        if (clickPoints.size() == 3) {
            return;
        }
        
        int g = rand() % MAX_COLOR;
        int b = rand() % MAX_COLOR;
        int r = rand() % MAX_COLOR;
        clickPoints.push_back(MyPoint(x, y, r, g, b));
        cout << "Coordinates: (" << x << ", " << y << ")" << endl;
        image.at<cv::Vec3b>(Point(x, y)) = cv::Vec3b(g, b, r);
        imshow("Canvas Window", image);
    }
    else if (event == EVENT_RBUTTONDOWN) {
        clickPoints.clear();
        image = Mat::zeros(HEIGHT, WIDTH, CV_8UC3);
        imshow("Canvas Window", image);
        cout << "Canvas cleared" << endl;
    }
    
}

void createPolygon() {
    int min_x = findMinX(clickPoints);
    int max_x = findMaxX(clickPoints);
    int min_y = findMinY(clickPoints);
    int max_y = findMaxY(clickPoints);
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            cv::Mat A = (cv::Mat_<float>(3, 3) <<
                clickPoints[0].x, clickPoints[1].x, clickPoints[2].x,
                clickPoints[0].y, clickPoints[1].y, clickPoints[2].y,
                1, 1, 1);
            cv::Mat B = (cv::Mat_<float>(3, 1) << x, y, 1);
            cv::Mat coords = A.inv() * B;

            if (cv::checkRange(coords, true, 0, 0.0, 1.0)) {
                auto expr = colorMatrix * coords;
                cv::Mat res = expr;
                image.at<cv::Vec3b>(Point(x, y)) = cv::Vec3b(res.at<float>(0, 0),
                    res.at<float>(0, 1),
                    res.at<float>(0, 2));
            }
        }
    }
}

void createTriangle() {
    if (clickPoints.size() == 3) {
        colorMatrix = Mat::zeros(3, 3, CV_32FC1);
        for (int i = 0; i < 3; i++) {
            colorMatrix.col(i).at<float>(0) = clickPoints[i].g;
            colorMatrix.col(i).at<float>(1) = clickPoints[i].b;
            colorMatrix.col(i).at<float>(2) = clickPoints[i].r;
        }
        createPolygon();
        imshow("Canvas Window", image);
    }
}

int main() {
    image = Mat::zeros(HEIGHT, WIDTH, CV_8UC3);
    cout << image.size() << endl;
    srand(time(NULL));
    namedWindow("Canvas Window");
    setMouseCallback("Canvas Window", mouseCallback, NULL);

    imshow("Canvas Window", image);

    cout << "Interactive canvas ready!" << endl;
    cout << "Left click - add point" << endl;
    cout << "right click - clear canvas" << endl;

    while (true) {
        int key = cv::waitKey(1) & 0xFF;
        if (key == 27) { // ESC - выход
            break;
        }
        else if (key == 32) { // SPACE - создание треугольника
            cout << "space pressed" << endl;
            createTriangle();
        }
    }

    waitKey(0);
    return 0;
}
