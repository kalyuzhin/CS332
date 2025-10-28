#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <random>
#include <stack>
#include <queue>
#include <opencv2/opencv.hpp>

namespace Lab05 {

    struct PointF {
        float x, y;
        PointF() : x(0), y(0) {}
        PointF(float x, float y) : x(x), y(y) {}
    };

    class LSystem {
    public:
        std::string axiom;
        double angle;
        double startDirection;
        std::unordered_map<char, std::string> rules;

        LSystem(const std::string& filePath);
    };

    class LSystemGenerator {
    private:
        std::shared_ptr<LSystem> lSystem;

    public:
        LSystemGenerator(std::shared_ptr<LSystem> lsystem);
        std::string generateSequence(int iterations);
    };

    class FractalDrawer {
    private:
        cv::Mat canvas;
        PointF currentPosition;
        double currentDirection;
        double minX, maxX, minY, maxY;
        double scaleCoef;

        std::stack<std::tuple<PointF, double, float, int, float>> stateStack;
        std::queue<double> randomAngles;
        std::mt19937 rng;

    public:
        FractalDrawer(cv::Mat targetCanvas);

        PointF calculateNextPosition(float stepLength, PointF position, double direction);
        void drawLine(const PointF& p1, const PointF& p2, int colorValue, float thickness);
        void calculateBounds(const std::string& sequence, double angleIncrement, float stepLength, float stepDecreasePercent = 0.0f);
        void draw(const std::string& sequence, double angleIncrement, float stepLength);
    };

    class App {
    public:
        int run();
    };

}