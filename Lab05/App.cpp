#include "provider.h"
#include "Lab05/App.h"
#include <random>
#include <chrono>


//2. Алгоритм midpoint displacement
//Реализовать алгоритм midpoint displacement для двумерной визуализации горного массива.
//Необходимо отображать результаты последовательных шагов алгоритма.Программа должна позволять изменять параметры построения ломаной.
//поля для интерфейса: R - коэфициент изменения высот, Steps - кол-во шагов алгоритма
namespace Lab05 {
    static void glfw_error_callback(int error, const char* description);

    vector<cv::Point> points;
    vector<double> test = { 1, 2, 3, 4 };
    cv::Point lp, rp;
	static const int WIDTH = 1600;
	static const int HEIGHT = 1000;
	int steps = 5;
	float R = 0.5f;
    cv::Mat canvas;

    bool midpoint_running = false;
    int current_step = 0;
    int steps_total = 0;

    float randomFloat(float min, float max) {
        static std::mt19937 rng(std::random_device{}());
        std::uniform_real_distribution<float> dist(min, max);
        return dist(rng);
    }

    void redraw() {
        canvas = cv::Mat(canvas.size(), CV_8UC3, cv::Scalar(255, 255, 255));
        for (int i = 0; i < points.size()-1; i++) {
            cv::line(canvas, points[i], points[i + 1], cv::Scalar(0, 0, 0));
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
            it+=2;
        }
    }

    void MidpointDisplacement() {
        for (int i = 0; i < steps; i++) {
            MidpointDisplacementStep();
            redraw();
            this_thread::sleep_for(chrono::seconds(1));
        }
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

        GLFWwindow* window = glfwCreateWindow(300, 500, "CG Lab with OpenCV and ImGui", NULL, NULL);
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

        canvas = cv::Mat(HEIGHT, WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));
        lp = cv::Point(0, (int)randomFloat(0, HEIGHT));
        points.push_back(lp);
        rp = cv::Point(WIDTH, (int)randomFloat(0, HEIGHT));
        points.push_back(rp);
        cv::line(canvas, lp, rp, cv::Scalar(0, 0, 0));
        cv::namedWindow("Canvas", cv::WINDOW_AUTOSIZE);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
            ImGui::Begin("Controls");
            ImGui::SliderFloat("R", &R, 0.0f, 1.0f);
            if (R < 0.0f) R = 0.0f;
            if (R > 1.0f) R = 1.0f;

            ImGui::InputInt("Steps", &steps);

            ImGui::Separator();
            if (ImGui::Button("Apply")) {
                /*points.erase(points.begin() + 1, points.end() - 1);
                MidpointDisplacement();*/
                points.erase(points.begin() + 1, points.end() - 1);
                midpoint_running = true;
                current_step = 0;
                steps_total = steps;
                redraw();
                //last_anim_time = std::chrono::high_resolution_clock::now();
            }
            static auto last_anim_time = std::chrono::high_resolution_clock::now();
            if (midpoint_running) {
                auto now = std::chrono::high_resolution_clock::now();
                float dt = std::chrono::duration<float>(now - last_anim_time).count();
                if (dt > 0.5f) {
                    MidpointDisplacementStep();
                    redraw();
                    last_anim_time = now;
                    current_step += 1;
                    if (current_step >= steps_total)
                        midpoint_running = false;
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Random")) {
                points.clear();
                lp = cv::Point(0, (int)randomFloat(0, HEIGHT));
                points.push_back(lp);
                rp = cv::Point(WIDTH, (int)randomFloat(0, HEIGHT));
                points.push_back(rp);
                cv::line(canvas, lp, rp, cv::Scalar(0, 0, 0));
                redraw();
            }
            ImGui::End();

            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            cv::imshow("Canvas", canvas);
            cv::waitKey(1);

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