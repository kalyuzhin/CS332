#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <opencv2/opencv.hpp>

namespace Lab05 {

    class App {
    public:
        int run();
    };

} // namespace Lab05

// L-система классы
struct LSystem {
    std::string axiom;
    double angle;
    double startDirection;
    std::map<char, std::string> rules;

    LSystem(const std::string& filePath);
};

class LSystemGenerator {
private:
    std::shared_ptr<LSystem> lSystem;

public:
    LSystemGenerator(std::shared_ptr<LSystem> lsystem);
    std::string generateSequence(int iterations);
};

struct PointF {
    float x, y;
    PointF(float x = 0, float y = 0) : x(x), y(y) {}
};

class FractalDrawer {
private:
    cv::Mat& canvas;
    PointF currentPosition;
    double currentDirection;
    double scaleCoef;
    double minX, minY, maxX, maxY;

    std::vector<double> anglesQueue;

    PointF calculateNextPosition(float stepLength, PointF position, double direction);
    void drawLine(const PointF& p1, const PointF& p2, int colorValue, float thickness);

public:
    FractalDrawer(cv::Mat& canvas) : canvas(canvas) {
        currentPosition = PointF(0, 0);
        currentDirection = 0.0;
        scaleCoef = 1.0;
        minX = minY = maxX = maxY = 0.0;
    }

    void calculateBounds(const std::string& sequence, double angleIncrement, float stepLength);
    void draw(const std::string& sequence, double angleIncrement, float stepLength);
};

// Вспомогательные функции
void drawLSystemDemo();
void loadLSystem(const std::string& filename);
void checkLSystemFiles();
std::string findLSystemFile(const std::string& filename);