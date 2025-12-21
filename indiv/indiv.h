#include "../provider.h"
#include <numeric>

namespace Indiv {
    static double EPS = 0.0001;
    static void glfw_error_callback(int error, const char* description);

    vector<vector<cv::Point>> polygons;
    const cv::Scalar BLACK = cv::Scalar(0, 0, 0);
    cv::Mat canvas;

    void drawLine(cv::Mat& img, cv::Point p1, cv::Point p2, cv::Scalar color, int thickness = 1) {
        cv::line(img, p1, p2, color, thickness, cv::LINE_AA);
    }

    void redrawScene() {
        canvas = cv::Mat(canvas.size(), CV_8UC3, cv::Scalar(255, 255, 255));
        for (size_t i = 0; i < polygons.size(); ++i) {
            const auto& poly = polygons[i];
            for (size_t j = 0; j < poly.size(); ++j) {
                drawLine(canvas, poly[j], poly[(j + 1) % poly.size()], BLACK);
            }
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

    static double cross(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c) {
        return (double)(b.x - a.x) * (c.y - a.y) - (double)(b.y - a.y) * (c.x - a.x);
    }

    static double distPointLine(const cv::Point2f& p, const cv::Point2f& a, const cv::Point2f& b) {
        double num = std::fabs(cross(a, b, p));
        double dx = (double)(b.x - a.x);
        double dy = (double)(b.y - a.y);
        double den = std::hypot(dx, dy);
        return (den < EPS) ? 0.0 : (num / den);
    }

    static bool pointInTriangleInclusive(const cv::Point2f& p, const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c) {
        double c1 = cross(a, b, p);
        double c2 = cross(b, c, p);
        double c3 = cross(c, a, p);
        bool has_neg = (c1 < -EPS) || (c2 < -EPS) || (c3 < -EPS);
        bool has_pos = (c1 > EPS) || (c2 > EPS) || (c3 > EPS);
        return !(has_neg && has_pos);
    }

    static int orient(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c) {
        double v = cross(a, b, c);
        if (v > EPS) return 1;
        if (v < -EPS) return -1;
        return 0;
    }
    static bool onSegment(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& p) {
        return std::min(a.x, b.x) - EPS <= p.x && p.x <= std::max(a.x, b.x) + EPS &&
            std::min(a.y, b.y) - EPS <= p.y && p.y <= std::max(a.y, b.y) + EPS;
    }

    static bool segmentsIntersect(const cv::Point2f& a, const cv::Point2f& b, const cv::Point2f& c, const cv::Point2f& d) {
        int o1 = orient(a, b, c), o2 = orient(a, b, d), o3 = orient(c, d, a), o4 = orient(c, d, b);
        if (o1 != o2 && o3 != o4) return true;
        if (o1 == 0 && onSegment(a, b, c)) return true;
        if (o2 == 0 && onSegment(a, b, d)) return true;
        if (o3 == 0 && onSegment(c, d, a)) return true;
        if (o4 == 0 && onSegment(c, d, b)) return true;
        return false;
    }

    static bool isSimplePolygon(const std::vector<cv::Point>& poly) {
        int n = (int)poly.size();
        if (n < 3) return false;
        std::vector<cv::Point2f> p(n);
        for (int i = 0; i < n; ++i) p[i] = poly[i];
        for (int i = 0; i < n; ++i) {
            cv::Point2f a = p[i], b = p[(i + 1) % n];
            for (int j = i + 1; j < n; ++j) {
                if (j == i || j == (i + 1) % n || (i == 0 && j == n - 1)) continue;
                cv::Point2f c = p[j], d = p[(j + 1) % n];
                if (segmentsIntersect(a, b, c, d)) return false;
            }
        }
        return true;
    }

    static void removeCollinear(std::vector<cv::Point>& poly) {
        if (poly.size() < 3) return;
        std::vector<cv::Point> out;
        int n = (int)poly.size();
        for (int i = 0; i < n; ++i) {
            cv::Point2f prev = poly[(i - 1 + n) % n];
            cv::Point2f cur = poly[i];
            cv::Point2f next = poly[(i + 1) % n];
            if (std::fabs(cross(prev, cur, next)) > EPS) {
                out.push_back(poly[i]);
            }
        }
        poly.swap(out);
    }

    static int leftmostVertexIndex(const std::vector<cv::Point2f>& pts, const std::vector<int>& idx) {
        int best = idx[0];
        for (int id : idx) {
            if (pts[id].x < pts[best].x - EPS) best = id;
            else if (std::fabs(pts[id].x - pts[best].x) <= EPS && pts[id].y < pts[best].y) best = id;
        }
        return best;
    }

    static std::vector<int> sliceCyclic(const std::vector<int>& poly, int fromPos, int toPos) {
        std::vector<int> out;
        int n = (int)poly.size();
        int p = fromPos;
        while (true) {
            out.push_back(poly[p]);
            if (p == toPos) break;
            p = (p + 1) % n;
        }
        return out;
    }

    static void triangulateIntrudingRec(const std::vector<cv::Point2f>& pts, const std::vector<int>& polyIdx, std::vector<std::array<int, 3>>& out) {
        int n = (int)polyIdx.size();
        if (n < 3) return;
        if (n == 3) {
            out.push_back({ polyIdx[0], polyIdx[1], polyIdx[2] });
            return;
        }

        int leftId = polyIdx[0];
        for (int id : polyIdx) if (pts[id].x < pts[leftId].x - EPS) leftId = id;
        int iPos = -1;
        for (int k = 0; k < n; ++k) if (polyIdx[k] == leftId) { iPos = k; break; }

        int prevPos = (iPos - 1 + n) % n;
        int nextPos = (iPos + 1) % n;
        int aId = polyIdx[prevPos];
        int bId = polyIdx[iPos];
        int cId = polyIdx[nextPos];

        const auto& A = pts[aId];
        const auto& B = pts[bId];
        const auto& C = pts[cId];

        int intruder = -1;
        double bestDist = -1.0;
        for (int v : polyIdx) {
            if (v == aId || v == bId || v == cId) continue;
            if (pointInTriangleInclusive(pts[v], A, B, C)) {
                double d = distPointLine(pts[v], A, C);
                if (d > bestDist + EPS) {
                    bestDist = d;
                    intruder = v;
                }
            }
        }

        if (intruder == -1) {
            out.push_back({ aId, bId, cId });
            std::vector<int> reduced = polyIdx;
            reduced.erase(reduced.begin() + iPos);
            triangulateIntrudingRec(pts, reduced, out);
            return;
        }

        int dPos = -1;
        for (int k = 0; k < n; ++k) if (polyIdx[k] == intruder) { dPos = k; break; }

        auto poly1 = sliceCyclic(polyIdx, iPos, dPos);
        auto poly2 = sliceCyclic(polyIdx, dPos, iPos);

        triangulateIntrudingRec(pts, poly1, out);
        triangulateIntrudingRec(pts, poly2, out);
    }

    static std::vector<std::array<int, 3>> triangulateIntruding(const std::vector<cv::Point>& polygon) {
        std::vector<std::array<int, 3>> res;
        int n = (int)polygon.size();
        if (n < 3) return res;
        std::vector<cv::Point2f> pts(n);
        for (int i = 0; i < n; ++i) pts[i] = polygon[i];
        std::vector<int> idx(n);
        std::iota(idx.begin(), idx.end(), 0);
        triangulateIntrudingRec(pts, idx, res);
        return res;
    }

    static void drawTrianglesOnCanvas(cv::Mat& canvasMat, const std::vector<cv::Point>& poly, const std::vector<std::array<int, 3>>& tris) {
        if (tris.empty()) return;
        // overlay, чтобы сделать полупрозрачные заливки
        cv::Mat overlay = canvasMat.clone();
        cv::Scalar fillColor(200, 120, 80); // можно поменять
        for (auto& t : tris) {
            std::vector<cv::Point> tri = { poly[t[0]], poly[t[1]], poly[t[2]] };
            const cv::Point* ptsArr[1] = { tri.data() };
            int npts[] = { 3 };
            cv::fillPoly(overlay, ptsArr, npts, 1, fillColor, cv::LINE_AA);
            cv::polylines(overlay, ptsArr, npts, 1, true, cv::Scalar(0, 0, 0), 1, cv::LINE_AA);
        }
        // blend
        double alpha = 0.55;
        cv::addWeighted(overlay, alpha, canvasMat, 1.0 - alpha, 0.0, canvasMat);
    }


    void Triangulation() {

    }

    void onMouse(int event, int x, int y, int flags, void* userdata) {
        if (event == cv::EVENT_LBUTTONDOWN) {
            cv::Point mousePoint(x, y);
            if (polygons.empty() || polygons.back().empty() || polygons.back().back() != mousePoint) {
                if (polygons.empty()) {
                    polygons.push_back({});
                }
                polygons.back().push_back(mousePoint);
                redrawScene();
            }
        }
    }

    int run() {
        glfwSetErrorCallback(glfw_error_callback);
        if (!glfwInit())
            return 1;

#if defined(IMGUI_IMPL_OPENGL_ES2)
        const char* glsl_version = "#version 100";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
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
                redrawScene();
            }

            ImGui::SameLine();
            if (ImGui::Button("New polygon")) {
                polygons.push_back({});
            }

            if (ImGui::Button("Triangulation")) {
                for (size_t pi = 0; pi < polygons.size(); ++pi) {
                    auto poly = polygons[pi];
                    if (poly.size() < 3) continue;

                    removeCollinear(poly);
                    if (poly.size() < 3) continue;

                    if (!isSimplePolygon(poly)) {
                        cv::polylines(canvas, poly, true, cv::Scalar(0, 0, 200), 2, cv::LINE_AA);
                        continue;
                    }
                    auto tris = triangulateIntruding(poly);

                    drawTrianglesOnCanvas(canvas, poly, tris);
                }
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