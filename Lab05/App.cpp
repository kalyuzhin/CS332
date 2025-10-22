#include "provider.h"
#include "Lab05/App.h"
#include <random>
#include <chrono>
#include <fstream>
#include <sstream>
#include <cmath>
#include <stack>
#include <queue>
#include <thread>
#include <filesystem>

using namespace Lab05;

static std::shared_ptr<LSystem> currentLSystem;
static std::shared_ptr<LSystemGenerator> currentGenerator;
static std::string currentSequence;
static int currentIterations = 1;
static std::vector<std::string> lSystemFiles = {
    "../Lab05/LSystems/Sierpinski Curve.txt",
    "../Lab05/LSystems/Koch Curve.txt",
    "../Lab05/LSystems/Simple Tree.txt",
    "../Lab05/LSystems/Random Tree.txt",
    "../Lab05/LSystems/Dragon.txt",
    "../Lab05/LSystems/Koch Island.txt",
    "../Lab05/LSystems/Hilbert Curve.txt",
    "../Lab05/LSystems/Gosper Curve.txt",
    "../Lab05/LSystems/Hexagonal Tiling.txt",
    "../Lab05/LSystems/Advanced Tree.txt"
};
static std::vector<int> iterationsCounts = { 1, 1, 1, 1, 1, 1, 1, 1, 1, 4 };
static int selectedLSystem = 0;

static bool needsRedraw = false;
static bool lsystem_running = false;
static int current_draw_step = 0;
static int total_draw_steps = 0;

static cv::Mat midpointCanvas;
static cv::Mat lsystemCanvas;
const int CANVAS_WIDTH = 800;
const int CANVAS_HEIGHT = 600;

void printCurrentDirectory() {
    try {
        std::string currentPath = std::filesystem::current_path().string();
        printf("Current working directory: %s\n", currentPath.c_str());
    }
    catch (const std::exception& e) {
        printf("Error getting current directory: %s\n", e.what());
    }
}

std::string findLSystemFile(const std::string& filename) {
    std::vector<std::string> paths = {
        filename,
        "../" + filename,
        "../../" + filename,
        "../../../" + filename,
        "Lab05/LSystems/" + filename,
        "../Lab05/LSystems/" + filename,
        "../../Lab05/LSystems/" + filename,
        "../../../Lab05/LSystems/" + filename
    };

    printf("Searching for file: %s\n", filename.c_str());

    for (const auto& path : paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            file.close();
            printf("Found file: %s\n", path.c_str());
            return path;
        }
        else {
            printf("  Not found: %s\n", path.c_str());
        }
    }

    throw std::runtime_error("Cannot find L-system file: " + filename);
}

void checkLSystemFiles() {
    printf("=== CHECKING L-SYSTEM FILES ===\n");
    for (const auto& filename : lSystemFiles) {
        printf("Checking file: %s\n", filename.c_str());

        try {
            std::string fullPath = findLSystemFile(filename);
            std::ifstream file(fullPath);
            if (!file.is_open()) {
                printf("  ERROR: Cannot open file!\n");
                continue;
            }

            std::string line;
            int lineNum = 0;
            while (std::getline(file, line)) {
                printf("  Line %d: '%s'\n", lineNum, line.c_str());
                lineNum++;
            }
            file.close();
            printf("  --- End of file ---\n");
        }
        catch (const std::exception& e) {
            printf("  ERROR: %s\n", e.what());
        }
    }
}

LSystem::LSystem(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open L-system file: " + filePath);
    }

    std::string line;
    if (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string axiomStr, angleStr, directionStr;

        if (!(iss >> axiomStr >> angleStr >> directionStr)) {
            throw std::runtime_error("Invalid first line format in L-system file");
        }

        axiom = axiomStr;
        angle = std::stod(angleStr);
        startDirection = std::stod(directionStr);

        printf("Parsed: axiom='%s', angle=%.1f, startDirection=%.1f\n",
            axiom.c_str(), angle, startDirection);
    }

    int ruleCount = 0;
    while (std::getline(file, line)) {
        if (line.empty()) continue;

        size_t start = line.find_first_not_of(" \t");
        if (start == std::string::npos) continue;

        size_t end = line.find_last_not_of(" \t");
        std::string trimmedLine = line.substr(start, end - start + 1);

        size_t pos = trimmedLine.find('>');
        if (pos != std::string::npos && pos > 0) {
            char key = trimmedLine[0];
            std::string value = trimmedLine.substr(pos + 1);

            size_t valueStart = value.find_first_not_of(" \t");
            if (valueStart != std::string::npos) {
                value = value.substr(valueStart);
            }

            rules[key] = value;
            ruleCount++;
            printf("Rule %d: '%c' -> '%s'\n", ruleCount, key, value.c_str());
        }
    }

    printf("Loaded L-system: %zu rules total\n", rules.size());
}

LSystemGenerator::LSystemGenerator(std::shared_ptr<LSystem> lsystem)
    : lSystem(lsystem) {
}

std::string LSystemGenerator::generateSequence(int iterations) {
    std::string current = lSystem->axiom;

    for (int i = 0; i < iterations; i++) {
        std::string next;

        for (char symbol : current) {
            auto it = lSystem->rules.find(symbol);
            if (it != lSystem->rules.end()) {
                next += it->second;
            }
            else {
                next += symbol;
            }
        }

        current = next;
    }

    return current;
}

FractalDrawer::FractalDrawer(cv::Mat targetCanvas)
    : canvas(targetCanvas), currentPosition(PointF(0, 0)),
    currentDirection(0), rng(std::random_device{}()) {
}

PointF FractalDrawer::calculateNextPosition(float stepLength, PointF position, double direction) {
    double radianAngle = direction * (3.14159 / 180.0);
    float nextX = position.x + static_cast<float>(stepLength * cos(radianAngle));
    float nextY = position.y - static_cast<float>(stepLength * sin(radianAngle));
    return PointF(nextX, nextY);
}

void FractalDrawer::drawLine(const PointF& p1, const PointF& p2, int colorValue, float thickness) {
    cv::Point cvP1(static_cast<int>(p1.x), static_cast<int>(p1.y));
    cv::Point cvP2(static_cast<int>(p2.x), static_cast<int>(p2.y));

    int color = std::max(0, std::min(255, colorValue));
    cv::Scalar lineColor(color, color, color);

    cv::line(canvas, cvP1, cvP2, lineColor, static_cast<int>(thickness));
}

void FractalDrawer::calculateBounds(const std::string& sequence, double angleIncrement, float stepLength, float stepDecreasePercent) {
    std::stack<std::tuple<PointF, double, float>> stack;
    randomAngles = std::queue<double>();

    minX = maxX = currentPosition.x;
    minY = maxY = currentPosition.y;

    PointF currentPos = currentPosition;
    double currentDir = currentDirection;
    float currentStep = stepLength;
    double initialAngle = angleIncrement;

    for (char symbol : sequence) {
        if (isalpha(symbol)) {
            currentStep -= currentStep * (stepDecreasePercent / 100.0f);
            PointF nextPos = calculateNextPosition(currentStep, currentPos, currentDir);

            minX = std::min(minX, static_cast<double>(nextPos.x));
            minY = std::min(minY, static_cast<double>(nextPos.y));
            maxX = std::max(maxX, static_cast<double>(nextPos.x));
            maxY = std::max(maxY, static_cast<double>(nextPos.y));

            currentPos = nextPos;
        }
        else {
            switch (symbol) {
            case '+':
                currentDir += angleIncrement;
                break;
            case '-':
                currentDir -= angleIncrement;
                break;
            case '[':
                stack.push(std::make_tuple(currentPos, currentDir, currentStep));
                break;
            case ']':
                if (!stack.empty()) {
                    auto savedState = stack.top();
                    stack.pop();
                    currentPos = std::get<0>(savedState);
                    currentDir = std::get<1>(savedState);
                    currentStep = std::get<2>(savedState);
                }
                break;
            case '@':
                std::uniform_real_distribution<double> dist(0, initialAngle);
                double randomAngle = dist(rng);
                randomAngles.push(randomAngle);
                angleIncrement = randomAngle;
                break;
            }
        }
    }
}

void FractalDrawer::drawAdvancedTree(const std::string& sequence, double angleIncrement, float stepLength) {
    printf("Starting advanced tree draw\n");

    canvas = cv::Scalar(255, 255, 255);

    currentPosition = PointF(CANVAS_WIDTH / 2, CANVAS_HEIGHT - 50);
    currentDirection = 90;

    float stepDecreasePercent = 15.0f;
    int colorChangeValue = 18;
    float penThicknessDecreasePercent = 15.0f;

    float initialStepLength = stepLength;
    int initialColor = 80;
    float initialThickness = 20.0f;

    calculateBounds(sequence, angleIncrement, stepLength, stepDecreasePercent);

    double width = maxX - minX;
    double height = maxY - minY;

    if (width == 0 || height == 0) {
        printf("Error: Zero bounds\n");
        return;
    }

    double scaleX = (CANVAS_WIDTH - 80) / width;
    double scaleY = (CANVAS_HEIGHT - 80) / height;
    scaleCoef = std::min(scaleX, scaleY);

    double offsetX = (CANVAS_WIDTH - width * scaleCoef) / 2;
    double offsetY = (CANVAS_HEIGHT - height * scaleCoef) / 2;

    currentPosition = PointF(static_cast<float>(-minX * scaleCoef + offsetX),
        static_cast<float>(-minY * scaleCoef + offsetY));
    currentDirection = 90;

    std::stack<std::tuple<PointF, double, float, int, float>> stack;
    PointF currentPos = currentPosition;
    double currentDir = currentDirection;
    float currentStep = stepLength * static_cast<float>(scaleCoef);
    int currentColor = initialColor;
    float currentThickness = initialThickness;

    double initialAngle = angleIncrement;

    for (char symbol : sequence) {
        if (isalpha(symbol)) {
            currentColor += colorChangeValue;
            currentColor = std::min(200, currentColor);

            currentThickness -= currentThickness * (penThicknessDecreasePercent / 100.0f);
            currentThickness = std::max(1.0f, currentThickness);

            currentStep -= currentStep * (stepDecreasePercent / 100.0f);

            PointF nextPos = calculateNextPosition(currentStep, currentPos, currentDir);
            drawLine(currentPos, nextPos, currentColor, currentThickness);
            currentPos = nextPos;
        }
        else {
            switch (symbol) {
            case '+':
                currentDir += angleIncrement;
                break;
            case '-':
                currentDir -= angleIncrement;
                break;
            case '[':
                stack.push(std::make_tuple(currentPos, currentDir, currentStep, currentColor, currentThickness));
                break;
            case ']':
                if (!stack.empty()) {
                    auto savedState = stack.top();
                    stack.pop();
                    currentPos = std::get<0>(savedState);
                    currentDir = std::get<1>(savedState);
                    currentStep = std::get<2>(savedState);
                    currentColor = std::get<3>(savedState);
                    currentThickness = std::get<4>(savedState);
                }
                break;
            case '@':
                if (!randomAngles.empty()) {
                    angleIncrement = randomAngles.front();
                    randomAngles.pop();
                }
                break;
            }
        }
    }

    printf("Advanced tree draw completed\n");
    lsystem_running = false;
}

void FractalDrawer::draw(const std::string& sequence, double angleIncrement, float stepLength) {
    printf("Starting draw: sequence length=%zu\n", sequence.length());

    canvas = cv::Scalar(255, 255, 255);

    currentPosition = PointF(0, 0);
    currentDirection = currentLSystem->startDirection;
    minX = minY = maxX = maxY = 0.0;

    calculateBounds(sequence, angleIncrement, stepLength, 0.0f);

    double width = maxX - minX;
    double height = maxY - minY;

    if (width == 0 || height == 0) {
        printf("Error: Zero bounds\n");
        return;
    }

    double scaleX = (CANVAS_WIDTH - 40) / width;
    double scaleY = (CANVAS_HEIGHT - 40) / height;
    scaleCoef = std::min(scaleX, scaleY);

    double offsetX = (CANVAS_WIDTH - width * scaleCoef) / 2 - minX * scaleCoef;
    double offsetY = (CANVAS_HEIGHT - height * scaleCoef) / 2 - minY * scaleCoef;

    currentPosition = PointF(static_cast<float>(offsetX), static_cast<float>(offsetY));
    currentDirection = currentLSystem->startDirection;

    std::stack<std::tuple<PointF, double, float, int, float>> stack;
    PointF currentPos = currentPosition;
    double currentDir = currentDirection;
    float currentStep = stepLength * static_cast<float>(scaleCoef);
    int currentColor = 0;
    float currentThickness = 2.0f;

    for (char symbol : sequence) {
        if (isalpha(symbol)) {
            PointF nextPos = calculateNextPosition(currentStep, currentPos, currentDir);
            drawLine(currentPos, nextPos, currentColor, currentThickness);
            currentPos = nextPos;
        }
        else {
            switch (symbol) {
            case '+':
                currentDir += angleIncrement;
                break;
            case '-':
                currentDir -= angleIncrement;
                break;
            case '[':
                stack.push(std::make_tuple(currentPos, currentDir, currentStep, currentColor, currentThickness));
                break;
            case ']':
                if (!stack.empty()) {
                    auto savedState = stack.top();
                    stack.pop();
                    currentPos = std::get<0>(savedState);
                    currentDir = std::get<1>(savedState);
                    currentStep = std::get<2>(savedState);
                    currentColor = std::get<3>(savedState);
                    currentThickness = std::get<4>(savedState);
                }
                break;
            case '@':
                if (!randomAngles.empty()) {
                    angleIncrement = randomAngles.front();
                    randomAngles.pop();
                }
                break;
            }
        }
    }

    printf("Draw completed\n");
    lsystem_running = false;
}

void loadLSystem(const std::string& filename) {
    try {
        std::string fullPath = findLSystemFile(filename);
        printf("Loading L-system from: %s\n", fullPath.c_str());
        currentLSystem = std::make_shared<LSystem>(fullPath);
        currentGenerator = std::make_shared<LSystemGenerator>(currentLSystem);
        currentSequence = currentGenerator->generateSequence(currentIterations);
        printf("Generated sequence length: %zu\n", currentSequence.length());
        needsRedraw = true;
    }
    catch (const std::exception& e) {
        printf("Error loading L-system: %s\n", e.what());
        throw;
    }
}

void drawLSystemDemo() {
    ImGui::Begin("L-System Fractals");

    if (ImGui::Combo("Fractal Type", &selectedLSystem,
        "Sierpinski Curve\0Koch Curve\0Simple Tree\0Random Tree\0Dragon Curve\0Koch Island\0Hilbert Curve\0Gosper Curve\0Hexagonal Tiling\0Advanced Tree\0")) {
        printf("Selected L-system: %d - %s\n", selectedLSystem, lSystemFiles[selectedLSystem].c_str());
        try {
            loadLSystem(lSystemFiles[selectedLSystem]);
            currentIterations = iterationsCounts[selectedLSystem];
        }
        catch (const std::exception& e) {
            printf("Failed to load L-system: %s\n", e.what());
        }
    }

    if (ImGui::SliderInt("Iterations", &currentIterations, 1, 15)) {
        if (currentGenerator) {
            currentSequence = currentGenerator->generateSequence(currentIterations);
            needsRedraw = true;
        }
    }

    if (ImGui::Button("Draw Fractal") || needsRedraw) {
        if (currentGenerator && currentLSystem && !lsystem_running) {
            printf("=== DRAWING FRACTAL ===\n");
            currentSequence = currentGenerator->generateSequence(currentIterations);
            lsystem_running = true;

            std::thread([&]() {
                FractalDrawer drawer(lsystemCanvas);
                if (selectedLSystem == 9) {
                    drawer.drawAdvancedTree(currentSequence, currentLSystem->angle, 15.0f);
                }
                else {
                    drawer.draw(currentSequence, currentLSystem->angle, 10.0f);
                }
                }).detach();

            needsRedraw = false;
        }
    }

    if (lsystem_running) {
        ImGui::SameLine();
        ImGui::Text("Drawing... %d/%d", current_draw_step, total_draw_steps);
    }

    if (currentLSystem) {
        ImGui::Separator();
        ImGui::Text("Axiom: %s", currentLSystem->axiom.c_str());
        ImGui::Text("Angle: %.1f", currentLSystem->angle);
        ImGui::Text("Start Direction: %.1f", currentLSystem->startDirection);
        ImGui::Text("Sequence length: %zu", currentSequence.length());
        ImGui::Text("Rules:");
        for (const auto& rule : currentLSystem->rules) {
            ImGui::Text("  %c -> %s", rule.first, rule.second.c_str());
        }
    }

    ImGui::End();
}

std::vector<cv::Point> points;
cv::Point lp, rp;
int steps = 5;
float R = 0.5f;

bool midpoint_running = false;
int current_step = 0;
int steps_total = 0;

float randomFloat(float min, float max) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_real_distribution<float> dist(min, max);
    return dist(rng);
}

void redrawMidpoint() {
    midpointCanvas = cv::Scalar(255, 255, 255);
    for (size_t i = 0; i < points.size() - 1; i++) {
        cv::line(midpointCanvas, points[i], points[i + 1], cv::Scalar(0, 0, 0));
    }
}

void MidpointDisplacementStep() {
    auto it = points.begin() + 1;
    while (it != points.end()) {
        auto a = *(it - 1);
        auto b = *it;
        float mid_x = (b.x + a.x) / 2.0f;
        float length = (b.x - a.x) / 2.0f;
        float mid_y = (b.y + a.y) / 2.0f + randomFloat(-R * length, R * length);
        it = points.insert(it, cv::Point(mid_x, mid_y));
        it += 2;
    }
}

static void glfw_error_callback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int App::run() {
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    printf("=== CURRENT DIRECTORY INFO ===\n");
    printCurrentDirectory();

    checkLSystemFiles();

    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    GLFWwindow* window = glfwCreateWindow(350, 800, "CG Lab with OpenCV and ImGui", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    midpointCanvas = cv::Mat(CANVAS_HEIGHT, CANVAS_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));
    lsystemCanvas = cv::Mat(CANVAS_HEIGHT, CANVAS_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));

    printf("=== INITIALIZING L-SYSTEMS ===\n");

    try {
        loadLSystem(lSystemFiles[0]);
    }
    catch (const std::exception& e) {
        printf("Failed to load initial L-system: %s\n", e.what());
        return 1;
    }

    lp = cv::Point(0, (int)randomFloat(0, CANVAS_HEIGHT));
    points.push_back(lp);
    rp = cv::Point(CANVAS_WIDTH, (int)randomFloat(0, CANVAS_HEIGHT));
    points.push_back(rp);
    redrawMidpoint();

    cv::namedWindow("Midpoint Displacement", cv::WINDOW_AUTOSIZE);
    cv::namedWindow("L-System Fractals", cv::WINDOW_AUTOSIZE);

    printf("Windows created. Starting main loop...\n");

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(330, 100), ImGuiCond_Once);
        ImGui::Begin("Algorithm Selection");

        static int algorithm = 0;
        ImGui::RadioButton("Midpoint Displacement", &algorithm, 0);
        ImGui::RadioButton("L-System Fractals", &algorithm, 1);

        ImGui::End();

        if (algorithm == 0) {
            ImGui::SetNextWindowSize(ImVec2(330, 300), ImGuiCond_Once);
            ImGui::Begin("Midpoint Displacement Controls");

            ImGui::SliderFloat("R", &R, 0.0f, 1.0f);
            if (R < 0.0f) R = 0.0f;
            if (R > 1.0f) R = 1.0f;

            ImGui::InputInt("Steps", &steps);

            ImGui::Separator();
            if (ImGui::Button("Apply")) {
                points.erase(points.begin() + 1, points.end() - 1);
                midpoint_running = true;
                current_step = 0;
                steps_total = steps;
                redrawMidpoint();
            }

            static auto last_anim_time = std::chrono::high_resolution_clock::now();
            if (midpoint_running) {
                auto now = std::chrono::high_resolution_clock::now();
                float dt = std::chrono::duration<float>(now - last_anim_time).count();
                if (dt > 0.5f) {
                    MidpointDisplacementStep();
                    redrawMidpoint();
                    last_anim_time = now;
                    current_step += 1;
                    if (current_step >= steps_total)
                        midpoint_running = false;
                }
            }

            ImGui::SameLine();
            if (ImGui::Button("Random")) {
                points.clear();
                lp = cv::Point(0, (int)randomFloat(0, CANVAS_HEIGHT));
                points.push_back(lp);
                rp = cv::Point(CANVAS_WIDTH, (int)randomFloat(0, CANVAS_HEIGHT));
                points.push_back(rp);
                redrawMidpoint();
            }

            if (midpoint_running) {
                ImGui::Text("Step: %d/%d", current_step, steps_total);
            }

            ImGui::End();
        }
        else {
            drawLSystemDemo();
        }

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        if (algorithm == 0) {
            cv::imshow("Midpoint Displacement", midpointCanvas);
        }
        else {
            cv::imshow("L-System Fractals", lsystemCanvas);
        }

        cv::waitKey(1);

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    cv::destroyAllWindows();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}