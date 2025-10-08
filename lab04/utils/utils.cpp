//#include "provider.h"
#include <opencv2/opencv.hpp>

using namespace cv;

Mat createRotateMatrix(double angle) {
    double radians = angle * CV_PI / 180.0;
    double cos_a = std::cos(radians);
    double sin_a = std::sin(radians);
    Mat rotationMatrix = (Mat_<double>(3, 3) <<
        cos_a,  sin_a,  0,
        -sin_a, cos_a,  0,
        0,      0,      1
    );
    return rotationMatrix;
}

Mat createScaleMatrix(double xScale, double yScale) {
    Mat rotationMatrix = (Mat_<double>(3, 3) <<
        xScale, 0,      0,
        0,      yScale, 0,
        0,      0,      1
        );
    return rotationMatrix;
}

Mat createShiftMatrix(int dx, int dy) {
    Mat rotationMatrix = (Mat_<double>(3, 3) <<
        1,  0,  0,
        0,  1,  0,
        dx, dy, 1
        );
    return rotationMatrix;
}

void applyAffine(InputArray src, OutputArray dst, InputArray m) {
    Mat input = src.getMat();
    Mat matrix = m.getMat();
    CV_Assert(matrix.rows == 3 && matrix.cols == 3);
    Mat coordinates = input * matrix;
    coordinates.copyTo(dst);
}

void convertPointToVec(Point p, OutputArray dst) {
    Mat res = (Mat_<int>(1, 3) << p.x, p.y, 1);
    res.copyTo(dst);
}

Point convertVecToPoint(InputArray src) {
    Mat mat = src.getMat();
    Point p = Point(mat.at<int>(0, 0), mat.at<int>(0, 1));
    return p;
}