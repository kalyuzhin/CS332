#include "provider.h"
#include "App.h"

namespace Lab04 {
    static double EPS = 0.0001;
    static void glfw_error_callback(int error, const char* description);

    vector<vector<cv::Point>> polygons;
    vector<cv::Scalar> polygonColors;
    cv::Scalar currentColor = cv::Scalar(0, 0, 0);
    cv::Mat canvas;

    int currentTransform = 0;
    float transformDx = 0.0f;
    float transformDy = 0.0f;
    float rotationAngle = 0.0f;
    float scaleX = 1.0f;
    float scaleY = 1.0f;

    int currentSearch = 0;
    cv::Point clickedPointForSearch = cv::Point(-1, -1);
    vector<cv::Point> intersectionEdgePoints;
    vector<cv::Point2f> intersectionPoints;
    bool isDrawingIntersectionEdge = false;
    bool isDeterminingPointPosition = false;
    bool isCheckingPointInPolygon = false;

    void drawPoint(cv::Mat& img, cv::Point p, cv::Scalar color, int radius = 3) {
        cv::circle(img, p, radius, color, -1);
    }

    void drawLine(cv::Mat& img, cv::Point p1, cv::Point p2, cv::Scalar color, int thickness = 1) {
        cv::line(img, p1, p2, color, thickness, cv::LINE_AA);
    }

    void redrawScene() {
        canvas = cv::Mat(canvas.size(), CV_8UC3, cv::Scalar(255, 255, 255));
        for (size_t i = 0; i < polygons.size(); ++i) {
            const auto& poly = polygons[i];
            const auto& color = polygonColors[i];
            for (size_t j = 0; j < poly.size(); ++j) {
                drawLine(canvas, poly[j], poly[(j + 1) % poly.size()], color);
            }
        }

        for (const auto& pt : intersectionPoints) {
            drawPoint(canvas, pt, cv::Scalar(255, 0, 0), 5);
        }

        if (isDeterminingPointPosition && clickedPointForSearch.x != -1) {
            drawPoint(canvas, clickedPointForSearch, cv::Scalar(0, 0, 255));
        }
        if (isDrawingIntersectionEdge && intersectionEdgePoints.size() == 1) {
            drawPoint(canvas, intersectionEdgePoints[0], cv::Scalar(0, 255, 0));
        }
        if (isDrawingIntersectionEdge && intersectionEdgePoints.size() == 2) {
            drawLine(canvas, intersectionEdgePoints[0], intersectionEdgePoints[1], cv::Scalar(0, 255, 0));
        }
        if (isCheckingPointInPolygon && clickedPointForSearch.x != -1) {
            drawPoint(canvas, clickedPointForSearch, cv::Scalar(0, 0, 255));
        }
    }

    cv::Point2f applyTransformation(const cv::Point& p, const cv::Mat& transformMatrix) {
        cv::Mat pointMat = (cv::Mat_<double>(3, 1) << p.x, p.y, 1.0);
        cv::Mat transformedPointMat = transformMatrix * pointMat;
        return cv::Point2f(transformedPointMat.at<double>(0, 0), transformedPointMat.at<double>(1, 0));
    }

    cv::Point getPolygonCenter(const std::vector<cv::Point>& poly) {
        if (poly.empty()) return cv::Point(0, 0);
        double sumX = 0, sumY = 0;
        for (const auto& p : poly) {
            sumX += p.x;
            sumY += p.y;
        }
        return cv::Point(static_cast<int>(sumX / poly.size()), static_cast<int>(sumY / poly.size()));
    }

    void applyAffineTransformation(int transformType, cv::Point transformRefPoint = cv::Point(0, 0)) {
        if (polygons.empty()) return;

        cv::Mat transformMatrix = cv::Mat::eye(3, 3, CV_64F);

        if (transformType == 0) {
            transformMatrix = (cv::Mat_<double>(3, 3) <<
                1, 0, transformDx,
                0, 1, transformDy,
                0, 0, 1);
        }
        else if (transformType == 1 || transformType == 2) {
            double angleRad = rotationAngle * CV_PI / 180.0;
            cv::Point center = (transformType == 1) ? transformRefPoint : getPolygonCenter(polygons.back());

            cv::Mat T1 = (cv::Mat_<double>(3, 3) << 1, 0, -center.x, 0, 1, -center.y, 0, 0, 1);
            cv::Mat R = (cv::Mat_<double>(3, 3) << cos(angleRad), -sin(angleRad), 0, sin(angleRad), cos(
                angleRad), 0, 0, 0, 1);
            cv::Mat T2 = (cv::Mat_<double>(3, 3) << 1, 0, center.x, 0, 1, center.y, 0, 0, 1);
            transformMatrix = T2 * R * T1;
        }
        else if (transformType == 3 || transformType == 4) {
            cv::Point center = (transformType == 3) ? transformRefPoint : getPolygonCenter(polygons.back());

            cv::Mat T1 = (cv::Mat_<double>(3, 3) << 1, 0, -center.x, 0, 1, -center.y, 0, 0, 1);
            cv::Mat S = (cv::Mat_<double>(3, 3) << scaleX, 0, 0, 0, scaleY, 0, 0, 0, 1);
            cv::Mat T2 = (cv::Mat_<double>(3, 3) << 1, 0, center.x, 0, 1, center.y, 0, 0, 1);
            transformMatrix = T2 * S * T1;
        }

        if (!polygons.empty()) {
            std::vector<cv::Point>& currentPoly = polygons.back();
            for (auto& p : currentPoly) {
                p = applyTransformation(p, transformMatrix);
            }
        }
        redrawScene();
    }

    std::string classifyPointRelativeToEdge(cv::Point p, cv::Point p1, cv::Point p2) {
        double cross_product = (p.x - p1.x) * (p2.y - p1.y) - (p.y - p1.y) * (p2.x - p1.x);
        if (cross_product > 0) return "Слева";
        if (cross_product < 0) return "Справа";
        return "На ребре";
    }

    cv::Point2f findIntersection(cv::Point p1, cv::Point p2, cv::Point p3, cv::Point p4) {
        float s1_x, s1_y, s2_x, s2_y;
        s1_x = p2.x - p1.x;
        s1_y = p2.y - p1.y;
        s2_x = p4.x - p3.x;
        s2_y = p4.y - p3.y;

        float s, t;
        s = (-s1_y * (p1.x - p3.x) + s1_x * (p1.y - p3.y)) / (-s2_x * s1_y + s1_x * s2_y);
        t = (s2_x * (p1.y - p3.y) - s2_y * (p1.x - p3.x)) / (-s2_x * s1_y + s1_x * s2_y);

        if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
            float ix = p1.x + (t * s1_x);
            float iy = p1.y + (t * s1_y);
            return cv::Point2f(ix, iy);
        }

        return cv::Point2f(-1, -1);
    }

    bool isPointInPolygon(cv::Point p, const std::vector<cv::Point>& poly) {
        if (poly.empty()) return false;
        int intersections = 0;
        for (size_t i = 0; i < poly.size(); ++i) {
            cv::Point p1 = poly[i];
            cv::Point p2 = poly[(i + 1) % poly.size()];

            if (((p1.y <= p.y && p.y < p2.y) || (p2.y <= p.y && p.y < p1.y)) &&
                (p.x < (p2.x - p1.x) * (p.y - p1.y) / (p2.y - p1.y) + p1.x)) {
                intersections++;
            }
        }
        return intersections % 2 == 1;
    }

    void onMouse(int event, int x, int y, int flags, void* userdata) {
        if (event == cv::EVENT_LBUTTONDOWN) {
            cv::Point mousePoint(x, y);

            if (isDeterminingPointPosition) {
                clickedPointForSearch = mousePoint;
                redrawScene();
                if (!polygons.empty() && !polygons.back().empty()) {
                    cv::Point edgeStart = polygons.back()[0];
                    cv::Point edgeEnd = polygons.back()[1 % polygons.back().size()];
                    std::string position = classifyPointRelativeToEdge(mousePoint, edgeStart, edgeEnd);
                    printf("Точка (%d, %d) находится %s относительно ребра ((%d, %d) - (%d, %d)).\n",
                        mousePoint.x, mousePoint.y, position.c_str(), edgeStart.x, edgeStart.y, edgeEnd.x, edgeEnd.y);
                }
            }
            else if (isDrawingIntersectionEdge) {
                intersectionEdgePoints.push_back(mousePoint);
                redrawScene();
                if (intersectionEdgePoints.size() == 2) {
                    cv::Point pA = intersectionEdgePoints[0];
                    cv::Point pB = intersectionEdgePoints[1];
                    bool foundIntersection = false;
                    for (const auto& poly : polygons) {
                        for (size_t i = 0; i < poly.size(); ++i) {
                            cv::Point pC = poly[i];
                            cv::Point pD = poly[(i + 1) % poly.size()];
                            cv::Point2f intersection = findIntersection(pA, pB, pC, pD);
                            if (intersection.x != -1) {
                                intersectionPoints.push_back(intersection);
                                printf("Точка пересечения: (%.0f, %.0f)\n", intersection.x, intersection.y);
                                foundIntersection = true;
                            }
                        }
                    }
                    if (!foundIntersection) {
                        printf("Пересечений не найдено.\n");
                    }
                    intersectionEdgePoints.clear();
                    isDrawingIntersectionEdge = false;
                    redrawScene();
                }
            }
            else if (isCheckingPointInPolygon) {
                clickedPointForSearch = mousePoint;
                redrawScene();
                if (!polygons.empty()) {
                    bool inPoly = isPointInPolygon(mousePoint, polygons.back());
                    if (inPoly) {
                        printf("Точка (%d, %d) находится внутри полигона.\n", mousePoint.x, mousePoint.y);
                    }
                    else {
                        printf("Точка (%d, %d) не принадлежит полигону.\n", mousePoint.x, mousePoint.y);
                    }
                }
            }
            else {
                if (polygons.empty() || polygons.back().empty() || polygons.back().back() != mousePoint) {
                    if (polygons.empty() || polygonColors.size() < polygons.size()) {
                        polygons.push_back({});
                        polygonColors.push_back(currentColor);
                    }
                    polygons.back().push_back(mousePoint);
                    redrawScene();
                }
            }
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

        canvas = cv::Mat(720, 1280, CV_8UC3, cv::Scalar(255, 255, 255));
        cv::namedWindow("Canvas", cv::WINDOW_AUTOSIZE);
        cv::setMouseCallback("Canvas", onMouse, NULL);

        while (!glfwWindowShouldClose(window)) {
            glfwPollEvents();

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
            ImGui::Begin("Controls");

            if (ImGui::Button("Clear")) {
                polygons.clear();
                polygonColors.clear();
                clickedPointForSearch = cv::Point(-1, -1);
                intersectionEdgePoints.clear();
                intersectionPoints.clear();
                isDrawingIntersectionEdge = false;
                isDeterminingPointPosition = false;
                isCheckingPointInPolygon = false;
                redrawScene();
            }

            ImGui::SameLine();
            if (ImGui::Button("New polygon")) {
                currentColor = cv::Scalar(rand() % 256, rand() % 256, rand() % 256);
                polygons.push_back({});
                polygonColors.push_back(currentColor);
                isDeterminingPointPosition = false;
                isDrawingIntersectionEdge = false;
                isCheckingPointInPolygon = false;
                clickedPointForSearch = cv::Point(-1, -1);
                intersectionEdgePoints.clear();
            }

            ImGui::ColorEdit3("Color", (float*)&currentColor);

            ImGui::Separator();
            ImGui::Text("Affine transformations");
            const char* transform_items[] = { "Rotation dx, dy", "Rotation around a user-specified point",
                                             "Rotation around its center", "Scaling relative to a given point",
                                             "Scaling relative to its center" };
            ImGui::Combo("Conversion", &currentTransform, transform_items, IM_ARRAYSIZE(transform_items));

            if (currentTransform == 0) {
                ImGui::InputFloat("dx", &transformDx);
                ImGui::InputFloat("dy", &transformDy);
            }
            else if (currentTransform == 1 || currentTransform == 2) {
                ImGui::InputFloat("Angle (degrees)", &rotationAngle);
            }
            else if (currentTransform == 3 || currentTransform == 4) {
                ImGui::InputFloat("Scale X", &scaleX);
                ImGui::InputFloat("Scale Y", &scaleY);
            }

            if (ImGui::Button("Apply")) {
                cv::Point refPoint = cv::Point(canvas.cols / 2, canvas.rows / 2);
                if (clickedPointForSearch.x != -1) {
                    refPoint = clickedPointForSearch;
                }
                applyAffineTransformation(currentTransform, refPoint);
            }

            ImGui::Separator();
            ImGui::Text("Vector algorithms");
            const char* search_items[] = { "Point position", "Intersection point", "Point && polygon" };
            ImGui::Combo("Algorithm", &currentSearch, search_items, IM_ARRAYSIZE(search_items));

            if (ImGui::Button("Select mode")) {
                isDeterminingPointPosition = false;
                isDrawingIntersectionEdge = false;
                isCheckingPointInPolygon = false;
                clickedPointForSearch = cv::Point(-1, -1);
                intersectionEdgePoints.clear();
                redrawScene();

                if (currentSearch == 0) {
                    isDeterminingPointPosition = true;
                }
                else if (currentSearch == 1) {
                    isDrawingIntersectionEdge = true;
                    intersectionPoints.clear();
                    printf("Режим: Поиск пересечения. Кликните дважды на холсте, чтобы задать ребро.\n");
                }
                else if (currentSearch == 2) {
                    isCheckingPointInPolygon = true;
                    printf("Режим: Принадлежность точки полигону. Кликните на холсте, чтобы выбрать точку.\n");
                }
                ImGui::CloseCurrentPopup();
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