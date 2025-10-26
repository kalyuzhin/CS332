//
// Created by Марк Калюжин on 13.10.2025.
//

#ifndef CS332_TASK1_H
#define CS332_TASK1_H

#define GL_SILENCE_DEPRECATION


#include "../provider.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__

#include <OpenGL/gl3.h>

#endif

#include <vector>
#include <cmath>
#include <algorithm>
#include <random>

struct Vec2 {
    float x{}, y{};

    Vec2() = default;

    Vec2(float px, float py) : x(px), y(py) {}

    Vec2 operator-(const Vec2 &o) const { return {x - o.x, y - o.y}; }
};

static inline float cross(const Vec2 &a, const Vec2 &b) { return a.x * b.y - a.y * b.x; }

static inline float cross(const Vec2 &a, const Vec2 &b, const Vec2 &c) { return cross(b - a, c - a); }

static inline float dot(const Vec2 &a, const Vec2 &b) { return a.x * b.x + a.y * b.y; }

class ConvexHull {
public:
    void addPoint(const Vec2 &m) {
        if (points.empty()) {
            points.push_back(m);
            return;
        }
        if (points.size() == 1) {
            if (!(m.x == points[0].x && m.y == points[0].y)) points.push_back(m);
            return;
        }
        if (points.size() == 2) {
            if (std::fabs(cross(points[0], points[1], m)) < 1e-6f) {
                extendSegment(m);
                return;
            }
            std::vector<Vec2> t = {points[0], points[1], m};
            if (cross(t[0], t[1], t[2]) < 0) std::swap(t[0], t[1]);
            points = t;
            return;
        }
        if (isInside(m)) return;
        updateHull(m);
    }

    const std::vector<Vec2> &getHull() const { return points; }

private:
    std::vector<Vec2> points;

    bool isInside(const Vec2 &p) const {
        int n = (int) points.size();
        for (int i = 0; i < n; ++i)
            if (cross(points[i], points[(i + 1) % n], p) < -1e-6f) return false;
        return true;
    }

    void extendSegment(const Vec2 &m) {
        Vec2 a = points[0], b = points[1];
        Vec2 d = b - a;
        if (dot(m - a, d) < 0) points[0] = m;
        else if (dot(m - b, {-d.x, -d.y}) < 0) points[1] = m;
    }

    struct Tangents {
        int left = -1, right = -1;
        bool found = false;
    };

    Tangents findTangents(const Vec2 &m) const {
        Tangents t;
        int n = (int) points.size();
        if (n < 3) return t;
        for (int i = 0; i < n; ++i) {
            bool allRight = true, allLeft = true;
            for (int j = 0; j < n; ++j) {
                if (j == i) continue;
                float c = cross(m, points[i], points[j]);
                if (c < -1e-6f) allRight = false;
                if (c > 1e-6f) allLeft = false;
            }
            if (allRight) t.left = i;
            if (allLeft) t.right = i;
        }
        t.found = (t.left != -1 && t.right != -1);
        return t;
    }

    void updateHull(const Vec2 &m) {
        Tangents t = findTangents(m);
        if (!t.found) {
            handleCollinear(m);
            return;
        }
        int n = (int) points.size();
        std::vector<Vec2> newHull;
        int i = t.right;
        newHull.push_back(points[i]);
        while (i != t.left) {
            i = (i + 1) % n;
            if (i == t.left) break;
        }
        newHull.push_back(m);
        i = t.left;
        while (i != t.right) {
            newHull.push_back(points[i]);
            i = (i + 1) % n;
            if (i == t.right) break;
        }
        newHull.push_back(points[t.right]);
        points = newHull;
    }

    void handleCollinear(const Vec2 &m) {
        Vec2 a = points[0], b = points[1];
        Vec2 d = b - a;
        auto proj = [&](const Vec2 &q) { return dot(q - a, d); };
        float minv = proj(a), maxv = proj(b), vm = proj(m);
        if (vm < minv) points[0] = m;
        else if (vm > maxv) points[1] = m;
    }
};

class ConvexHullApp {
public:
    ConvexHullApp() {
        glfwInit();
#ifdef __APPLE__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        window = glfwCreateWindow(1200, 800, "Convex Hull Step-by-Step", nullptr, nullptr);
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init("#version 150");
        ImGui::StyleColorsDark();
    }

    ~ConvexHullApp() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window);
        glfwTerminate();
    }

    void run() {
        std::mt19937 rng{std::random_device{}()};
        std::uniform_real_distribution<float> rx(60.f, 900.f);
        std::uniform_real_distribution<float> ry(60.f, 650.f);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();
            drawUI(rng, rx, ry);
            ImGui::Render();
            int w, h;
            glfwGetFramebufferSize(window, &w, &h);
            glViewport(0, 0, w, h);
            glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(window);
        }
    }

private:
    GLFWwindow *window = nullptr;
    ConvexHull hull;
    std::vector<Vec2> inputPoints;

    void drawUI(std::mt19937 &rng, auto &rx, auto &ry) {
        ImGui::SetNextWindowSize(ImVec2(90, 100));
        ImGui::Begin("Controls");
        if (ImGui::Button("Reset")) {
            inputPoints.clear();
            hull = ConvexHull();
        }
        ImGui::Text("Points: %zu", inputPoints.size());
        ImGui::Text("Hull: %zu", hull.getHull().size());
        ImGui::End();

        ImGui::Begin("Canvas");
        ImDrawList *dl = ImGui::GetWindowDrawList();
        ImVec2 p0{40, 40}, p1{940, 690};
        dl->AddRectFilled(p0, p1, IM_COL32(20, 20, 26, 255), 6);
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            ImVec2 m = ImGui::GetMousePos();
            if (m.x >= p0.x && m.x <= p1.x && m.y >= p0.y && m.y <= p1.y) {
                Vec2 p{m.x, m.y};
                inputPoints.push_back(p);
                hull.addPoint(p);
            }
        }
        for (auto &p: inputPoints)
            dl->AddCircleFilled({p.x, p.y}, 4, IM_COL32(200, 200, 230, 255));
        const auto &h = hull.getHull();
        if (h.size() >= 2)
            for (size_t i = 0; i < h.size(); ++i)
                dl->AddLine({h[i].x, h[i].y}, {h[(i + 1) % h.size()].x, h[(i + 1) % h.size()].y},
                            IM_COL32(80, 180, 240, 255), 3);
        ImGui::End();
    }
};

#endif //CS332_TASK1_H
