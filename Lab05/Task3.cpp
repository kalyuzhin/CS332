#include "provider.h"
#include "Lab05/Task3.h"
#include <random>
#include <chrono>
#include <imgui_internal.h>


namespace Task3 {
    inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) {
        return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y);
    }

    inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) {
        return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y);
    }

    inline ImVec2 operator*(const ImVec2& lhs, double numb) {
        return ImVec2(lhs.x * numb, lhs.y * numb);
    }
    static void glfw_error_callback(int error, const char* description);

    static ImVec2 point = ImVec2(100, 100);
    static vector<ImVec2> points;
    static bool dragging = false;
    const int GLFW_WIDTH = 1920;
    const int GLFW_HEIGHT = 1920;
    const ImColor RED = IM_COL32(255, 0, 0, 255);
    const ImColor GREEN = IM_COL32(0, 255, 0, 255);
    const ImColor BLUE = IM_COL32(0, 0, 255, 255);
    float radius = 8.0f;
    int selected_point = -1;
    ImVec2 canvas_pos;

    void DrawBezierCurve(ImDrawList* draw_list) {
        if (points.size() < 4) return;
        for (int idx = 0; idx < points.size() - 1; idx += 3) {
            for (int i = 0; i <= 1000; i++) {
                float t = (float)i / 1000.0f;
                float u = 1.0f - t;
                ImVec2 pixel = points[idx] * (u * u * u) +
                    points[idx+1] * 3 * u * u * t +
                    points[idx+2] * 3 * u * t * t +
                    points[idx+3] * t * t * t;
                ImVec2 pixel_screen = canvas_pos + pixel;
                ImVec2 pixel_size = ImVec2(4.0f, 4.0f);
                draw_list->AddRectFilled(pixel_screen,
                    pixel_screen + pixel_size,
                    RED);
            }
        }
        
    }

    void RenderPointMover()
    {
        ImGui::Begin("Mover", nullptr, ImGuiWindowFlags_NoMove);
        canvas_pos = ImGui::GetCursorScreenPos();
        ImVec2 canvas_size(GLFW_HEIGHT, GLFW_WIDTH);
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;

        bool mouse_down = ImGui::IsMouseDown(0);

        bool left_clicked = ImGui::IsMouseClicked(0);
        bool left_down = ImGui::IsMouseDown(0);
        bool left_released = ImGui::IsMouseReleased(0);

        bool right_clicked = ImGui::IsMouseClicked(1);
        bool right_down = ImGui::IsMouseDown(1);
        bool right_released = ImGui::IsMouseReleased(1);

        //ImVec2 point_screen = canvas_pos + point;

        //bool mouse_over_point = ImLengthSqr(mouse_pos - point_screen) < radius * radius;
        

        if (left_down) {
            if (selected_point == -1) {
                for (int i = 0; i < points.size(); i++) {
                    ImVec2 point_screen = canvas_pos + points[i];
                    bool mouse_over_point = ImLengthSqr(mouse_pos - point_screen) < radius * radius;
                    if (mouse_over_point) {
                        selected_point = i;
                        break;
                    }
                }
            }
            else {
                points[selected_point] = mouse_pos - canvas_pos;
            }
        }
        else {
            selected_point = -1;
        }

        if (right_clicked) {
            ImVec2 new_point = mouse_pos - canvas_pos;
            if (points.size() != 0) {
                ImVec2 middle_point = new_point + points[points.size() - 1];
                middle_point.x /= 2; middle_point.y /= 2;
                points.push_back(middle_point);
                points.push_back(middle_point);
            }
            points.push_back(new_point);
            
        }

        draw_list->AddRect(canvas_pos, canvas_pos + canvas_size, IM_COL32(200, 200, 200, 255));
        for (int i = 0; i < points.size(); i++) {
            ImVec2 point_screen = canvas_pos + points[i];
            radius = 8.0f;
            draw_list->AddCircleFilled(point_screen, radius, GREEN);
        }

        DrawBezierCurve(draw_list);

        ImGui::Dummy(canvas_size);

        ImGui::End();
    }

    int App::run() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return 1;

#if defined(IMGUI_IMPL_OPENGL_ES2)
        const char* glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
#elif defined(__APPLE__)
        const char* glsl_version = "#version 150";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#else
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#endif

        GLFWwindow* window = glfwCreateWindow(GLFW_WIDTH, GLFW_HEIGHT, "CG Lab with OpenCV and ImGui", NULL, NULL);
        if (window == NULL)
            return 1;
        glfwMakeContextCurrent(window);
        glfwSwapInterval(1);

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

        ImGui::StyleColorsDark();

        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
        //cv::Mat canvas = cv::Mat(HEIGHT, WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));
        //cv::namedWindow("Canvas", cv::WINDOW_AUTOSIZE);
        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            RenderPointMover();

            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            //cv::imshow("Canvas", canvas);
            /*cv::waitKey(1);*/

            glfwSwapBuffers(window);
        }

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();
        return 0;
    }

    static void glfw_error_callback(int error, const char* description) {
        fprintf(stderr, "Glfw Error %d: %s\n", error, description);
    }
}