//
// Created by Марк Калюжин on 19.11.2025.
//

#include "task3.h"

#define GL_SILENCE_DEPRECATION

#include <cmath>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__

#include <OpenGL/gl3.h>

#endif

using std::vector;

namespace lab8 {

    struct Vec3 {
        float x{}, y{}, z{};
    };

    struct Vec4 {
        float x{}, y{}, z{}, w{};
    };

    struct Mat4 {
        float m[4][4]{};

        static Mat4 identity() {
            Mat4 r{};
            r.m[0][0] = 1.0f;
            r.m[1][1] = 1.0f;
            r.m[2][2] = 1.0f;
            r.m[3][3] = 1.0f;
            return r;
        }
    };

    struct Camera {
        Vec3 position{0.0f, 3.0f, 5.0f};
        Vec3 target{0.0f, 0.0f, 0.0f};
        Vec3 up{0.0f, 1.0f, 0.0f};
        float fovY_deg{60.0f};
        float z_near{0.1f};
        float z_far{100.0f};
        float aspect{16.0f / 9.0f};
    };

    static inline float deg2rad(float a) {
        return a * 3.14159265358979323846f / 180.0f;
    }

    static Vec3 operator+(const Vec3 &a, const Vec3 &b) {
        return {a.x + b.x, a.y + b.y, a.z + b.z};
    }

    static Vec3 operator-(const Vec3 &a, const Vec3 &b) {
        return {a.x - b.x, a.y - b.y, a.z - b.z};
    }

    static Vec3 operator*(const Vec3 &v, float s) {
        return {v.x * s, v.y * s, v.z * s};
    }

    static float dot(const Vec3 &a, const Vec3 &b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    static Vec3 cross(const Vec3 &a, const Vec3 &b) {
        return {
                a.y * b.z - a.z * b.y,
                a.z * b.x - a.x * b.z,
                a.x * b.y - a.y * b.x
        };
    }

    static Vec3 normalize(const Vec3 &v) {
        float len = std::sqrt(dot(v, v));
        if (len < 1e-6f) return {0.0f, 0.0f, 0.0f};
        float inv = 1.0f / len;
        return {v.x * inv, v.y * inv, v.z * inv};
    }

    static Mat4 operator*(const Mat4 &a, const Mat4 &b) {
        Mat4 r{};
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                float s = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    s += a.m[i][k] * b.m[k][j];
                }
                r.m[i][j] = s;
            }
        }
        return r;
    }

    static Vec4 operator*(const Mat4 &m, const Vec4 &v) {
        Vec4 r{};
        r.x = m.m[0][0] * v.x + m.m[0][1] * v.y + m.m[0][2] * v.z + m.m[0][3] * v.w;
        r.y = m.m[1][0] * v.x + m.m[1][1] * v.y + m.m[1][2] * v.z + m.m[1][3] * v.w;
        r.z = m.m[2][0] * v.x + m.m[2][1] * v.y + m.m[2][2] * v.z + m.m[2][3] * v.w;
        r.w = m.m[3][0] * v.x + m.m[3][1] * v.y + m.m[3][2] * v.z + m.m[3][3] * v.w;
        return r;
    }

    static Mat4 make_look_at(const Vec3 &eye, const Vec3 &center, const Vec3 &up) {
        Vec3 f = normalize(center - eye);
        Vec3 s = normalize(cross(f, up));
        Vec3 u = cross(s, f);

        Mat4 r = Mat4::identity();

        r.m[0][0] = s.x;
        r.m[0][1] = s.y;
        r.m[0][2] = s.z;
        r.m[0][3] = -dot(s, eye);

        r.m[1][0] = u.x;
        r.m[1][1] = u.y;
        r.m[1][2] = u.z;
        r.m[1][3] = -dot(u, eye);

        r.m[2][0] = -f.x;
        r.m[2][1] = -f.y;
        r.m[2][2] = -f.z;
        r.m[2][3] = dot(f, eye);

        r.m[3][0] = 0.0f;
        r.m[3][1] = 0.0f;
        r.m[3][2] = 0.0f;
        r.m[3][3] = 1.0f;

        return r;
    }

    static Mat4 make_perspective(float fovY_deg, float aspect, float z_near, float z_far) {
        float fov = deg2rad(fovY_deg);
        float f = 1.0f / std::tan(fov * 0.5f);
        Mat4 r{};

        r.m[0][0] = f / aspect;
        r.m[1][1] = f;
        r.m[2][2] = (z_far + z_near) / (z_near - z_far);
        r.m[2][3] = (2.0f * z_far * z_near) / (z_near - z_far);
        r.m[3][2] = -1.0f;
        r.m[3][3] = 0.0f;

        return r;
    }

    static Vec3 spherical_to_cartesian(float radius, float yaw, float pitch) {
        float cp = std::cos(pitch);
        float sp = std::sin(pitch);
        float cy = std::cos(yaw);
        float sy = std::sin(yaw);
        return {radius * cp * cy, radius * sp, radius * cp * sy};
    }

    struct Cube {
        vector <Vec3> vertices;
        vector <std::pair<int, int>> edges;
    };

    static Cube make_unit_cube() {
        Cube c;
        c.vertices = {
                {-1.0f, -1.0f, -1.0f},
                {1.0f,  -1.0f, -1.0f},
                {1.0f,  1.0f,  -1.0f},
                {-1.0f, 1.0f,  -1.0f},
                {-1.0f, -1.0f, 1.0f},
                {1.0f,  -1.0f, 1.0f},
                {1.0f,  1.0f,  1.0f},
                {-1.0f, 1.0f,  1.0f}
        };

        c.edges = {
                {0, 1},
                {1, 2},
                {2, 3},
                {3, 0},
                {4, 5},
                {5, 6},
                {6, 7},
                {7, 4},
                {0, 4},
                {1, 5},
                {2, 6},
                {3, 7}
        };

        return c;
    }

    static ImVec2 project_point_to_canvas(
            const Vec3 &p,
            const Mat4 &vp,
            const ImVec2 &origin,
            const ImVec2 &size
    ) {
        Vec4 w{p.x, p.y, p.z, 1.0f};
        Vec4 c = vp * w;

        if (std::fabs(c.w) < 1e-6f) {
            return origin;
        }

        float x_ndc = c.x / c.w;
        float y_ndc = c.y / c.w;

        float sx = (x_ndc * 0.5f + 0.5f) * size.x + origin.x;
        float sy = (1.0f - (y_ndc * 0.5f + 0.5f)) * size.y + origin.y;

        return {sx, sy};
    }

    static void draw_axes(
            const Mat4 &vp,
            const ImVec2 &origin,
            const ImVec2 &size,
            ImDrawList *dl
    ) {
        Vec3 o{0.0f, 0.0f, 0.0f};
        Vec3 x{2.0f, 0.0f, 0.0f};
        Vec3 y{0.0f, 2.0f, 0.0f};
        Vec3 z{0.0f, 0.0f, 2.0f};

        ImVec2 po = project_point_to_canvas(o, vp, origin, size);
        ImVec2 px = project_point_to_canvas(x, vp, origin, size);
        ImVec2 py = project_point_to_canvas(y, vp, origin, size);
        ImVec2 pz = project_point_to_canvas(z, vp, origin, size);

        dl->AddLine(po, px, IM_COL32(255, 0, 0, 255), 2.0f);
        dl->AddLine(po, py, IM_COL32(0, 255, 0, 255), 2.0f);
        dl->AddLine(po, pz, IM_COL32(0, 128, 255, 255), 2.0f);
    }

    static void draw_cube_wireframe(
            const Cube &cube,
            const Mat4 &vp,
            const ImVec2 &origin,
            const ImVec2 &size,
            ImDrawList *dl
    ) {
        for (auto [i0, i1]: cube.edges) {
            Vec3 p0 = cube.vertices[i0];
            Vec3 p1 = cube.vertices[i1];

            ImVec2 s0 = project_point_to_canvas(p0, vp, origin, size);
            ImVec2 s1 = project_point_to_canvas(p1, vp, origin, size);

            dl->AddLine(s0, s1, IM_COL32(220, 220, 220, 255), 2.0f);
        }
    }

    static void glfw_error_callback(int error, const char *description) {
        fprintf(stderr, "GLFW Error %d: %s\n", error, description);
    }

    void run_task3() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit()) return;

#if defined(__APPLE__)
        const char *glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

        GLFWwindow *window = glfwCreateWindow(1280, 720, "CS332 - Lab07 - Task3 Camera", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            return;
        }
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO &io = ImGui::GetIO();
        (void) io;

        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        Camera cam;
        Cube cube = make_unit_cube();

        float radius = 6.0f;
        float yaw = 0.7f;
        float pitch = 0.5f;
        bool auto_rotate = true;
        float auto_speed = 0.5f;

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Camera settings");
            ImGui::SliderFloat("Radius", &radius, 2.0f, 15.0f);
            ImGui::SliderFloat("Yaw", &yaw, -3.1415f, 3.1415f);
            ImGui::SliderFloat("Pitch", &pitch, -1.2f, 1.2f);
            ImGui::SliderFloat("FOV Y", &cam.fovY_deg, 30.0f, 100.0f);
            ImGui::Checkbox("Auto rotate", &auto_rotate);
            ImGui::SliderFloat("Auto speed", &auto_speed, -2.0f, 2.0f);
            ImGui::Text("Camera pos: (%.2f, %.2f, %.2f)", cam.position.x, cam.position.y, cam.position.z);
            ImGui::End();

            if (auto_rotate) {
                yaw += auto_speed * io.DeltaTime;
            }

            cam.position = spherical_to_cartesian(radius, yaw, pitch) + cam.target;

            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            if (display_h == 0) display_h = 1;
            cam.aspect = static_cast<float>(display_w) / static_cast<float>(display_h);

            Mat4 view = make_look_at(cam.position, cam.target, cam.up);
            Mat4 proj = make_perspective(cam.fovY_deg, cam.aspect, cam.z_near, cam.z_far);
            Mat4 vp = proj * view;

            glViewport(0, 0, display_w, display_h);
            glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT);

            ImGui::Begin("Camera view");
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();
            if (canvas_size.x < 50.0f) canvas_size.x = 50.0f;
            if (canvas_size.y < 50.0f) canvas_size.y = 50.0f;
            ImVec2 canvas_pos_max = ImVec2(canvas_pos.x + canvas_size.x, canvas_pos.y + canvas_size.y);

            ImDrawList *draw_list = ImGui::GetWindowDrawList();
            draw_list->AddRectFilled(canvas_pos, canvas_pos_max, IM_COL32(15, 15, 20, 255));
            draw_list->AddRect(canvas_pos, canvas_pos_max, IM_COL32(255, 255, 255, 255));

            draw_axes(vp, canvas_pos, canvas_size, draw_list);
            draw_cube_wireframe(cube, vp, canvas_pos, canvas_size, draw_list);

            ImGui::End();

            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
    }

}
