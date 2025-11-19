#include "task2.h"

using std::vector;
using std::string;

namespace lab7 {

#ifndef Z_BUFFER_H
#define Z_BUFFER_H

#include <vector>
#include <limits>
#include <algorithm>
#include <cmath>
#include <imgui.h>

    static constexpr float INF = std::numeric_limits<float>::max();

    struct ZBuffer {
        int width, height;
        std::vector<float> buffer;

        ZBuffer(int w, int h) : width(w), height(h), buffer(w* h, INF) {}

        void clear() {
            std::fill(buffer.begin(), buffer.end(), INF);
        }

        bool testAndSet(int x, int y, float z) {
            if (x < 0 || x >= width || y < 0 || y >= height) return false;
            int idx = y * width + x;
            if (z < buffer[idx]) {
                buffer[idx] = z;
                return true;
            }
            return false;
        }
    };

    // Function to compute face normal
    inline Vec3 computeFaceNormal(const Mesh& mesh, const Face& face, const Mat4& model) {
        if (face.idx.size() < 3) return { 0, 0, 0 };

        Vec3 v0 = xform({ mesh.V[face.idx[0]].x, mesh.V[face.idx[0]].y, mesh.V[face.idx[0]].z }, model);
        Vec3 v1 = xform({ mesh.V[face.idx[1]].x, mesh.V[face.idx[1]].y, mesh.V[face.idx[1]].z }, model);
        Vec3 v2 = xform({ mesh.V[face.idx[2]].x, mesh.V[face.idx[2]].y, mesh.V[face.idx[2]].z }, model);

        Vec3 edge1 = v1 - v0;
        Vec3 edge2 = v2 - v0;
        return norm(cross(edge1, edge2));
    }

    // Function to check if face is visible (backface culling)
    inline bool isFaceVisible(const Vec3& normal, const Vec3& viewDir) {
        return dot(normal, viewDir) < 0;
    }

    // Function to rasterize a triangle using z-buffer
    static void rasterizeTriangle(ZBuffer& zBuffer, const std::vector<Vec3>& screenCoords,
        const std::vector<float>& depths, ImU32 color, const AppState& S) {
        // Find bounding box
        int minX = S.proj.cx * 2, maxX = 0;
        int minY = S.proj.cy * 2, maxY = 0;

        for (const auto& p : screenCoords) {
            minX = std::min(minX, (int)p.x);
            maxX = std::max(maxX, (int)p.x);
            minY = std::min(minY, (int)p.y);
            maxY = std::max(maxY, (int)p.y);
        }

        // Clamp to screen bounds
        minX = std::max(0, minX);
        maxX = std::min(zBuffer.width - 1, maxX);
        minY = std::max(0, minY);
        maxY = std::min(zBuffer.height - 1, maxY);

        // Get triangle vertices
        const Vec3& v0 = screenCoords[0];
        const Vec3& v1 = screenCoords[1];
        const Vec3& v2 = screenCoords[2];

        // Compute triangle area
        float area = (v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y);
        if (std::abs(area) < 1e-6f) return;

        float invArea = 1.0f / area;

        // Rasterize
        ImDrawList* dl = ImGui::GetBackgroundDrawList();

        for (int y = minY; y <= maxY; ++y) {
            for (int x = minX; x <= maxX; ++x) {
                // Compute barycentric coordinates
                float w0 = ((v1.x - x) * (v2.y - y) - (v2.x - x) * (v1.y - y)) * invArea;
                float w1 = ((v2.x - x) * (v0.y - y) - (v0.x - x) * (v2.y - y)) * invArea;
                float w2 = ((v0.x - x) * (v1.y - y) - (v1.x - x) * (v0.y - y)) * invArea;

                // Check if point is inside triangle
                if (w0 >= 0 && w1 >= 0 && w2 >= 0) {
                    // Interpolate depth
                    float z = w0 * depths[0] + w1 * depths[1] + w2 * depths[2];

                    // Z-buffer test
                    if (zBuffer.testAndSet(x, y, z)) {
                        dl->AddRectFilled(ImVec2((float)x, (float)y),
                            ImVec2((float)x + 1, (float)y + 1), color);
                    }
                }
            }
        }
    }

    // Function to project point with camera support
    static bool projectPointWithCamera(const AppState& S, const Vec3& pw, int& X, int& Y, float& depth) {
        const Projector& proj = S.proj;

        if (proj.perspective) {
            if (S.useCamera) {
                Vec3 fwd = norm(S.camera.target - S.camera.pos);
                if (vlen(fwd) < 1e-6f) return false;

                Vec3 up = S.camera.up;
                if (vlen(up) < 1e-6f) up = { 0.f, 1.f, 0.f };

                Vec3 right = norm(cross(fwd, up));
                if (vlen(right) < 1e-6f) {
                    up = { 0.f, 1.f, 0.f };
                    right = norm(cross(fwd, up));
                }
                up = cross(right, fwd);

                Vec3 d = pw - S.camera.pos;
                float x_cam = dot(d, right);
                float y_cam = dot(d, up);
                float z_cam = dot(d, fwd);

                if (z_cam <= 1e-3f) return false;

                float f = proj.f;
                float x = (x_cam * f / z_cam) * (proj.scale / f) + proj.cx;
                float y = (y_cam * f / z_cam) * (proj.scale / f) + proj.cy;

                X = (int)std::lround(x);
                Y = (int)std::lround(y);
                depth = z_cam; // Use camera space depth for z-buffer
                return true;
            }

            float denom = proj.f + pw.z;
            if (denom <= 1e-3f) return false;

            float x = (pw.x * proj.f / denom) * (proj.scale / proj.f) + proj.cx;
            float y = (pw.y * proj.f / denom) * (proj.scale / proj.f) + proj.cy;

            X = (int)std::lround(x);
            Y = (int)std::lround(y);
            depth = pw.z + proj.f; // Use perspective-corrected depth
            return true;
        }

        Vec3 q = proj.axo(pw);
        float x = q.x * proj.scale + proj.cx;
        float y = q.y * proj.scale + proj.cy;
        X = (int)std::lround(x);
        Y = (int)std::lround(y);
        depth = q.z; // Use z-coordinate in view space for parallel projection
        return true;
    }

    // Function to draw mesh with z-buffer and camera support
    static void drawMeshZBuffer(const Mesh& mesh, const Mat4& model, const AppState& S,
        ZBuffer& zBuffer, ImU32 color, bool showWireframe) {

        // Compute view direction based on AppState settings
        Vec3 viewDir{ 0,0,1 };
        if (S.useCustomView) {
            viewDir = norm(S.viewVec);
        }
        else if (S.proj.perspective && !S.useCamera) {
            // For perspective without camera, use default view direction
            viewDir = { 0, 0, 1 };
        }
        else if (!S.proj.perspective) {
            // For parallel projection
            Vec4 v{ 0.f, 0.f, 1.f, 0.f };
            Mat4 R = Mat4::Rx(S.proj.ax) * Mat4::Ry(S.proj.ay);
            Vec4 r = v * R;
            viewDir = norm(Vec3{ r.x, r.y, r.z });
        }

        Vec3 meshC_object = centroid(mesh);
        Vec3 meshC = xform(meshC_object, model);
        const float EPS = 1e-6f;

        for (size_t fi = 0; fi < mesh.F.size(); ++fi) {
            const auto& face = mesh.F[fi];
            if (face.idx.size() < 3) continue;

            // Compute face normal and check visibility using AppState's backface culling logic
            Vec3 normal = computeFaceNormal(mesh, face, model);

            // Compute face centroid for orientation and perspective
            Vec3 fc{ 0,0,0 };
            for (int vidx : face.idx) {
                Vec3 v = xform({ mesh.V[vidx].x, mesh.V[vidx].y, mesh.V[vidx].z }, model);
                fc.x += v.x; fc.y += v.y; fc.z += v.z;
            }
            fc = fc * (1.f / (float)face.idx.size());

            // Orient normal outward
            Vec3 faceV = fc - meshC;
            if (dot(normal, faceV) < 0.f) {
                normal = normal * -1.f;
            }

            // Determine final view direction for this face
            Vec3 finalViewDir = viewDir;
            if (S.useCamera) {
                finalViewDir = norm(S.camera.pos - fc);
            }
            else if (S.proj.perspective && !S.useCustomView) {
                Vec3 cam{ 0.f, 0.f, -S.proj.f };
                finalViewDir = norm(cam - fc);
            }

            // Apply backface culling if enabled
            bool frontFacing = dot(normal, finalViewDir) > EPS;
            if (S.backfaceCull && !frontFacing) {
                continue;
            }

            if (face.idx.size() == 3) {
                // Triangle face
                std::vector<Vec3> screenCoords;
                std::vector<float> depths;
                bool allVisible = true;

                for (int vi : face.idx) {
                    Vec3 worldPos = xform({ mesh.V[vi].x, mesh.V[vi].y, mesh.V[vi].z }, model);
                    int sx, sy;
                    float depth;
                    if (!projectPointWithCamera(S, worldPos, sx, sy, depth)) {
                        allVisible = false;
                        break;
                    }
                    screenCoords.push_back({ (float)sx, (float)sy, 0 });
                    depths.push_back(depth);
                }

                if (allVisible && screenCoords.size() == 3) {
                    rasterizeTriangle(zBuffer, screenCoords, depths, color, S);

                    if (showWireframe) {
                        ImDrawList* dl = ImGui::GetBackgroundDrawList();
                        for (size_t i = 0; i < 3; ++i) {
                            size_t next = (i + 1) % 3;
                            dl->AddLine(
                                ImVec2(screenCoords[i].x, screenCoords[i].y),
                                ImVec2(screenCoords[next].x, screenCoords[next].y),
                                IM_COL32(0, 0, 0, 255), 1.0f
                            );
                        }
                    }
                }
            }
            else {
                // Polygon face - triangulate by fan method
                for (size_t i = 1; i < face.idx.size() - 1; ++i) {
                    std::vector<int> triIndices = { face.idx[0], face.idx[i], face.idx[i + 1] };
                    std::vector<Vec3> screenCoords;
                    std::vector<float> depths;
                    bool allVisible = true;

                    for (int vi : triIndices) {
                        Vec3 worldPos = xform({ mesh.V[vi].x, mesh.V[vi].y, mesh.V[vi].z }, model);
                        int sx, sy;
                        float depth;
                        if (!projectPointWithCamera(S, worldPos, sx, sy, depth)) {
                            allVisible = false;
                            break;
                        }
                        screenCoords.push_back({ (float)sx, (float)sy, 0 });
                        depths.push_back(depth);
                    }

                    if (allVisible && screenCoords.size() == 3) {
                        rasterizeTriangle(zBuffer, screenCoords, depths, color, S);
                    }
                }

                if (showWireframe) {
                    // Use original draw function for wireframe
                    drawWireImGui(mesh, model, S, IM_COL32(0, 0, 0, 255), 1.0f);
                }
            }

            // Draw face normals if enabled
            if (S.showFaceNormals) {
                Vec3 nstart = fc;
                Vec3 nend = fc + normal * 30.f;
                int xs, ys, xe, ye;
                float depth;
                if (projectPointWithCamera(S, nstart, xs, ys, depth) &&
                    projectPointWithCamera(S, nend, xe, ye, depth)) {
                    ImDrawList* dl = ImGui::GetBackgroundDrawList();
                    dl->AddLine(ImVec2((float)xs, (float)ys), ImVec2((float)xe, (float)ye),
                        IM_COL32(200, 30, 30, 255), 1.2f);
                }
            }
        }
    }

#endif // Z_BUFFER_H

    static Mesh buildRevolution(const vector<Vec3>& gen, char axis, int subdivisions) {
        Mesh mesh;
        if (gen.size() < 2 || subdivisions < 3) {
            return mesh;
        }
        int n = static_cast<int>(gen.size());
        int m = subdivisions;
        mesh.V.reserve(n * m);
        float stepDeg = 360.0f / static_cast<float>(m);
        for (int i = 0; i < n; ++i) {
            const Vec3& p = gen[i];
            for (int k = 0; k < m; ++k) {
                float angle = stepDeg * static_cast<float>(k);
                float rad = deg2rad(angle);
                Vec3 rotated;
                if (axis == 'X' || axis == 'x') {
                    float c = std::cos(rad);
                    float s = std::sin(rad);
                    rotated.x = p.x;
                    rotated.y = p.y * c - p.z * s;
                    rotated.z = p.y * s + p.z * c;
                }
                else if (axis == 'Y' || axis == 'y') {
                    float c = std::cos(rad);
                    float s = std::sin(rad);
                    rotated.x = p.x * c + p.z * s;
                    rotated.y = p.y;
                    rotated.z = -p.x * s + p.z * c;
                }
                else {
                    float c = std::cos(rad);
                    float s = std::sin(rad);
                    rotated.x = p.x * c - p.y * s;
                    rotated.y = p.x * s + p.y * c;
                    rotated.z = p.z;
                }
                mesh.V.push_back({ rotated.x, rotated.y, rotated.z });
            }
        }
        for (int i = 0; i < n - 1; ++i) {
            for (int k = 0; k < m; ++k) {
                int kNext = (k + 1) % m;
                int v0 = i * m + k;
                int v1 = (i + 1) * m + k;
                int v2 = (i + 1) * m + kNext;
                int v3 = i * m + kNext;
                Face f;
                f.idx = { v0, v1, v2, v3 };
                mesh.F.push_back(std::move(f));
            }
        }
        return mesh;
    }

    static float funcPlane(float x, float y) {
        return 0.0f;
    }

    static float funcParaboloid(float x, float y) {
        return (x * x + y * y) / 100.0f;
    }

    static float funcSaddle(float x, float y) {
        return (x * x - y * y) / 100.0f;
    }

    static float funcWave(float x, float y) {
        return 25.0f * std::sin(std::sqrt(x * x + y * y) / 25.0f);
    }

    static float funcHill(float x, float y) {
        return 200.0f * std::exp(-(x * x + y * y) / 10000.0f);
    }

    static Mesh buildFunctionSurface(std::function<float(float, float)> func,
        float x0, float x1, float y0, float y1,
        int subdivisionsX, int subdivisionsY) {
        Mesh mesh;

        if (subdivisionsX < 1 || subdivisionsY < 1) {
            return mesh;
        }

        float stepX = (x1 - x0) / subdivisionsX;
        float stepY = (y1 - y0) / subdivisionsY;

        for (int i = 0; i <= subdivisionsY; ++i) {
            float y = y0 + i * stepY;
            for (int j = 0; j <= subdivisionsX; ++j) {
                float x = x0 + j * stepX;
                float z = func(x, y);
                mesh.V.push_back({ x, y, z });
            }
        }

        int pointsPerRow = subdivisionsX + 1;
        for (int i = 0; i < subdivisionsY; ++i) {
            for (int j = 0; j < subdivisionsX; ++j) {
                int v0 = i * pointsPerRow + j;
                int v1 = i * pointsPerRow + (j + 1);
                int v2 = (i + 1) * pointsPerRow + (j + 1);
                int v3 = (i + 1) * pointsPerRow + j;

                Face f;
                f.idx = { v0, v1, v2, v3 };
                mesh.F.push_back(std::move(f));
            }
        }

        return mesh;
    }

    // Функция для создания нескольких объектов для демонстрации перекрытия
    static vector<pair<Mesh, Mat4>> createDemoObjects(const AppState& S) {
        vector<pair<Mesh, Mat4>> objects;

        // Центральный объект - текущий выбранный объект
        objects.push_back({ S.base, S.modelMat });

        // Дополнительные объекты вокруг центрального для демонстрации Z-буфера
        float offset = 250.f;

        // Объект справа
        objects.push_back({ makeCube(80.f), Mat4::T(offset, 0.f, 0.f) * S.modelMat });

        // Объект слева
        objects.push_back({ makeTetra(70.f), Mat4::T(-offset, 0.f, 0.f) * S.modelMat });

        // Объект сверху
        objects.push_back({ makeOcta(60.f), Mat4::T(0.f, offset, 0.f) * S.modelMat });

        // Объект снизу
        objects.push_back({ makeIcosa(50.f), Mat4::T(0.f, -offset, 0.f) * S.modelMat });

        // Объект спереди
        objects.push_back({ makeDodeca(40.f), Mat4::T(0.f, 0.f, offset) * S.modelMat });

        return objects;
    }

    int run_lab_7() {
        const int W = 1200, H = 800;
        if (!glfwInit()) return 1;
#ifdef __APPLE__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        GLFWwindow* win = glfwCreateWindow(W, H, "Surface of Revolution & Function Graph with Z-Buffer", nullptr, nullptr);
        if (!win) {
            glfwTerminate();
            return 1;
        }
        glfwMakeContextCurrent(win);
        glfwSwapInterval(1);
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        (void)io;
        ImGui::StyleColorsLight();
        ImGui_ImplGlfw_InitForOpenGL(win, true);
#ifdef __APPLE__
        ImGui_ImplOpenGL3_Init("#version 150");
#else
        ImGui_ImplOpenGL3_Init(nullptr);
#endif

        AppState S;
        bool showAxes = true;
        bool useZBuffer = true; // Включить Z-буфер по умолчанию
        bool showMultipleObjects = false; // Показывать несколько объектов для демонстрации перекрытия

        // Создаем Z-буфер
        ZBuffer zBuffer(W, H);

        // Настройка камеры по умолчанию
        S.useCamera = true;
        S.cameraOrbit = true;
        S.camRadius = 600.f;
        S.camYaw = 45.f;
        S.camPitch = 30.f;
        updateCameraOrbit(S);

        ImGui::FileBrowser saveFileDialog(
            ImGuiFileBrowserFlags_SelectDirectory |
            ImGuiFileBrowserFlags_CloseOnEsc |
            ImGuiFileBrowserFlags_ConfirmOnEnter
        );
        ImGui::FileBrowser openFileDialog(
            ImGuiFileBrowserFlags_CloseOnEsc |
            ImGuiFileBrowserFlags_ConfirmOnEnter
        );
        openFileDialog.SetTypeFilters({ ".obj" });

        static vector<Vec3> generatrix = { {100.f, 0.f,   0.f},
                                          {100.f, 100.f, 0.f} };
        static int subdivisions = 12;
        static int axisIndex = 2;
        const char* axisNames[] = { "X", "Y", "Z" };

        static int funcIndex = 0;
        const char* funcNames[] = { "Plane z=0", "Paraboloid", "Saddle", "Wave", "Hill" };
        static float x0 = -200.0f, x1 = 200.0f;
        static float y0 = -200.0f, y1 = 200.0f;
        static int subdivX = 20, subdivY = 20;

        // Переменные для управления преобразованиями
        static float translateX = 0.f, translateY = 0.f, translateZ = 0.f;
        static float rotateX = 0.f, rotateY = 0.f, rotateZ = 0.f;
        static float scaleX = 1.f, scaleY = 1.f, scaleZ = 1.f;

        // Переменные для управления камерой
        static bool autoRotateCamera = false;
        static float cameraSpeed = 1.0f;

        while (!glfwWindowShouldClose(win)) {
            glfwPollEvents();
            applyKeyOps(win, S);

            // Автоматическое вращение камеры
            if (autoRotateCamera && S.useCamera && S.cameraOrbit) {
                S.camYaw += 0.5f * cameraSpeed;
                if (S.camYaw > 180.f) S.camYaw -= 360.f;
                updateCameraOrbit(S);
            }

            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            ImGui::Begin("Controls");
            auto meshNames = S.getMeshNamesForImGui();
            if (ImGui::Combo("Polyhedron", &S.polyIdx, meshNames.data(), meshNames.size())) {
                S.modelMat = Mat4::I();
                auto meshPair = S.meshes[S.polyIdx];
                S.kind = meshPair.first;
                S.base = meshPair.second;
            }

            ImGui::SeparatorText("Z-Buffer Settings");
            ImGui::Checkbox("Use Z-Buffer", &useZBuffer);
            ImGui::Checkbox("Show Multiple Objects", &showMultipleObjects);
            if (ImGui::Button("Clear Z-Buffer")) {
                zBuffer.clear();
            }

            ImGui::SeparatorText("Camera Control");
            ImGui::Checkbox("Use camera", &S.useCamera);
            ImGui::Checkbox("Auto Rotate Camera", &autoRotateCamera);
            if (autoRotateCamera) {
                ImGui::SliderFloat("Rotation Speed", &cameraSpeed, 0.1f, 3.0f);
            }

            if (S.useCamera) {
                S.proj.perspective = true;

                ImGui::Checkbox("Orbit around object", &S.cameraOrbit);
                if (S.cameraOrbit) {
                    ImGui::SliderFloat("Radius", &S.camRadius, 100.f, 2000.f);
                    ImGui::SliderFloat("Yaw", &S.camYaw, -180.f, 180.f);
                    ImGui::SliderFloat("Pitch", &S.camPitch, -80.f, 80.f);
                    updateCameraOrbit(S);
                }
                else {
                    ImGui::InputFloat3("Cam position", &S.camera.pos.x);
                    ImGui::InputFloat3("Cam target", &S.camera.target.x);
                }

                Vec3 dir = norm(S.camera.target - S.camera.pos);
                ImGui::Text("Dir: (%.1f, %.1f, %.1f)", dir.x, dir.y, dir.z);
                ImGui::Text("Pos: (%.1f, %.1f, %.1f)", S.camera.pos.x, S.camera.pos.y, S.camera.pos.z);
            }

            ImGui::SeparatorText("Transformations");
            ImGui::Text("Translation:");
            ImGui::InputFloat("X", &translateX);
            ImGui::InputFloat("Y", &translateY);
            ImGui::InputFloat("Z", &translateZ);

            ImGui::Text("Rotation (degrees):");
            ImGui::InputFloat("Rotate X", &rotateX);
            ImGui::InputFloat("Rotate Y", &rotateY);
            ImGui::InputFloat("Rotate Z", &rotateZ);

            ImGui::Text("Scale:");
            ImGui::InputFloat("Scale X", &scaleX);
            ImGui::InputFloat("Scale Y", &scaleY);
            ImGui::InputFloat("Scale Z", &scaleZ);

            if (ImGui::Button("Apply Transformations")) {
                // Применяем преобразования к текущему объекту
                Mat4 transform = Mat4::I();
                transform = transform * Mat4::T(translateX, translateY, translateZ);
                transform = transform * Mat4::Rx(rotateX) * Mat4::Ry(rotateY) * Mat4::Rz(rotateZ);
                transform = transform * Mat4::S(scaleX, scaleY, scaleZ);
                S.modelMat = S.modelMat * transform;
            }

            ImGui::SameLine();
            if (ImGui::Button("Reset Transform")) {
                S.modelMat = Mat4::I();
                translateX = translateY = translateZ = 0.f;
                rotateX = rotateY = rotateZ = 0.f;
                scaleX = scaleY = scaleZ = 1.f;
            }

            ImGui::Checkbox("Perspective", &S.proj.perspective);
            ImGui::Checkbox("Show axes", &showAxes);
            ImGui::SliderFloat("Scale", &S.proj.scale, 20.f, 400.f);
            if (!S.proj.perspective) {
                ImGui::SliderFloat("Axon X", &S.proj.ax, 0.f, 90.f);
                ImGui::SliderFloat("Axon Y", &S.proj.ay, 0.f, 90.f);
            }
            else {
                ImGui::SliderFloat("Focus f", &S.proj.f, 100.f, 2000.f);
            }

            ImGui::SeparatorText("Back-face culling");
            ImGui::Checkbox("Back-face culling", &S.backfaceCull);
            ImGui::Checkbox("Show face normals", &S.showFaceNormals);
            ImGui::Checkbox("Use custom view vector", &S.useCustomView);
            if (S.useCustomView) {
                float vv[3] = { S.viewVec.x, S.viewVec.y, S.viewVec.z };
                if (ImGui::InputFloat3("View vector", vv)) {
                    S.viewVec.x = vv[0];
                    S.viewVec.y = vv[1];
                    S.viewVec.z = vv[2];
                }
            }

            ImGui::SeparatorText("Lighting");
            const char* shadingItems[] = { "Wireframe", "Gouraud (Lambert)", "Phong toon" };
            ImGui::Combo("Shading", &S.shadingMode, shadingItems, IM_ARRAYSIZE(shadingItems));

            float lightPosArr[3] = { S.lightPos.x, S.lightPos.y, S.lightPos.z };
            if (ImGui::InputFloat3("Light position", lightPosArr)) {
                S.lightPos.x = lightPosArr[0];
                S.lightPos.y = lightPosArr[1];
                S.lightPos.z = lightPosArr[2];
            }

            ImGui::ColorEdit3("Object color", &S.objectColor.x);
            ImGui::SliderFloat("Ambient", &S.ambientK, 0.0f, 1.0f);
            ImGui::SliderInt("Toon levels", &S.toonLevels, 2, 6);

            ImGui::SeparatorText("Rotate around center");
            if (ImGui::Button("X +5")) {
                Vec3 Cw = worldCenter(S.base, S.modelMat);
                worldRotateAroundAxisThrough(S, Cw, { 1, 0, 0 }, +5.f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Y +5")) {
                Vec3 Cw = worldCenter(S.base, S.modelMat);
                worldRotateAroundAxisThrough(S, Cw, { 0, 1, 0 }, +5.f);
            }
            ImGui::SameLine();
            if (ImGui::Button("Z +5")) {
                Vec3 Cw = worldCenter(S.base, S.modelMat);
                worldRotateAroundAxisThrough(S, Cw, { 0, 0, 1 }, +5.f);
            }
            ImGui::SeparatorText("Reflect");
            if (ImGui::Button("XY")) worldReflectPlane(S, Mat4::RefXY());
            ImGui::SameLine();
            if (ImGui::Button("YZ")) worldReflectPlane(S, Mat4::RefYZ());
            ImGui::SameLine();
            if (ImGui::Button("XZ")) worldReflectPlane(S, Mat4::RefXZ());
            ImGui::SeparatorText("Arbitrary line");
            ImGui::Text("P0 coordinates:");
            ImGui::InputFloat("x P0", &S.P0.x);
            ImGui::InputFloat("y P0", &S.P0.y);
            ImGui::InputFloat("z P0", &S.P0.z);
            ImGui::Text("P1 coordinates:");
            ImGui::InputFloat("x P1", &S.P1.x);
            ImGui::InputFloat("y P1", &S.P1.y);
            ImGui::InputFloat("z P1", &S.P1.z);
            ImGui::Text("P0(%.1f, %.1f, %.1f) P1(%.1f, %.1f, %.1f)", S.P0.x, S.P0.y, S.P0.z, S.P1.x, S.P1.y, S.P1.z);
            if (ImGui::Button("Rotate P0-P1 +5")) worldRotateAroundLine(S, S.P0, S.P1, +5.f);
            ImGui::SameLine();
            if (ImGui::Button("Rotate P0-P1 -5")) worldRotateAroundLine(S, S.P0, S.P1, -5.f);
            if (ImGui::Button("Reset [C]")) {
                if (S.kind == PolyKind::Tetra) S.base = makeTetra(150.f);
                if (S.kind == PolyKind::Cube) S.base = makeCube(150.f);
                if (S.kind == PolyKind::Octa) S.base = makeOcta(150.f);
                if (S.kind == PolyKind::Ico) S.base = makeIcosa(150.f);
                if (S.kind == PolyKind::Dode) S.base = makeDodeca(150.f);
                S.modelMat = Mat4::I();
                S.proj.perspective = true;
                S.proj.ax = 35.264f;
                S.proj.ay = 45.f;
                S.proj.scale = 160.f;
            }
            ImGui::SeparatorText("File");
            static char newFilename[256] = "";
            ImGui::InputText("Filename", newFilename, IM_ARRAYSIZE(newFilename));
            if (ImGui::Button("Open file")) {
                openFileDialog.Open();
            }
            ImGui::SameLine();
            if (strlen(newFilename) == 0) {
                ImGui::BeginDisabled();
                ImGui::Button("Save file");
                ImGui::EndDisabled();
                if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) {
                    ImGui::SetTooltip("Enter filename");
                }
            }
            else {
                if (ImGui::Button("Save file")) {
                    saveFileDialog.Open();
                }
            }
            ImGui::SeparatorText("Surface of revolution");
            ImGui::Combo("Axis", &axisIndex, axisNames, IM_ARRAYSIZE(axisNames));
            ImGui::InputInt("Subdivisions", &subdivisions);
            if (subdivisions < 3) subdivisions = 3;
            ImGui::Text("Generatrix points:");
            for (size_t i = 0; i < generatrix.size(); ++i) {
                float xyz[3] = { generatrix[i].x, generatrix[i].y, generatrix[i].z };
                char label[32];
                snprintf(label, sizeof(label), "P%zu", i);
                if (ImGui::InputFloat3(label, xyz)) {
                    generatrix[i].x = xyz[0];
                    generatrix[i].y = xyz[1];
                    generatrix[i].z = xyz[2];
                }
            }
            if (ImGui::Button("Add point")) {
                if (!generatrix.empty()) {
                    generatrix.push_back(generatrix.back());
                }
                else {
                    generatrix.push_back({ 0.f, 0.f, 0.f });
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Remove last") && generatrix.size() > 1) {
                generatrix.pop_back();
            }
            if (ImGui::Button("Build revolution")) {
                char axisChar = axisNames[axisIndex][0];
                Mesh newMesh = buildRevolution(generatrix, axisChar, subdivisions);
                if (!newMesh.V.empty()) {
                    S.kind = PolyKind::UserObj;
                    S.base = newMesh;
                    S.modelMat = Mat4::I();
                    string objName =
                        string("Revol@") + axisNames[axisIndex] + string("/") + std::to_string(subdivisions);
                    S.meshesNames.push_back(objName);
                    S.meshes.push_back({ PolyKind::UserObj, newMesh });
                    S.polyIdx = static_cast<int>(S.meshes.size() - 1);
                }
            }

            ImGui::SeparatorText("Function Surface z = f(x, y)");
            ImGui::Combo("Function", &funcIndex, funcNames, IM_ARRAYSIZE(funcNames));
            ImGui::InputFloat("X min", &x0);
            ImGui::InputFloat("X max", &x1);
            ImGui::InputFloat("Y min", &y0);
            ImGui::InputFloat("Y max", &y1);
            ImGui::InputInt("Subdivisions X", &subdivX);
            ImGui::InputInt("Subdivisions Y", &subdivY);
            if (subdivX < 1) subdivX = 1;
            if (subdivY < 1) subdivY = 1;

            if (ImGui::Button("Build Function Surface")) {
                std::function<float(float, float)> selectedFunc;

                switch (funcIndex) {
                case 0:
                    selectedFunc = funcPlane;
                    break;
                case 1:
                    selectedFunc = funcParaboloid;
                    break;
                case 2:
                    selectedFunc = funcSaddle;
                    break;
                case 3:
                    selectedFunc = funcWave;
                    break;
                case 4:
                    selectedFunc = funcHill;
                    break;
                default:
                    selectedFunc = funcPlane;
                }

                Mesh newMesh = buildFunctionSurface(selectedFunc, x0, x1, y0, y1, subdivX, subdivY);
                if (!newMesh.V.empty()) {
                    S.kind = PolyKind::UserObj;
                    S.base = newMesh;
                    S.modelMat = Mat4::I();
                    string objName = string("Func@") + funcNames[funcIndex] +
                        string("/") + std::to_string(subdivX) + "x" + std::to_string(subdivY);
                    S.meshesNames.push_back(objName);
                    S.meshes.push_back({ PolyKind::UserObj, newMesh });
                    S.polyIdx = static_cast<int>(S.meshes.size() - 1);
                }
            }
            ImGui::End();

            saveFileDialog.Display();
            if (saveFileDialog.HasSelected()) {
                std::filesystem::path dir_path = saveFileDialog.GetDirectory();
                std::filesystem::path fullpath = dir_path / std::filesystem::path(newFilename);
                Mesh newMesh = transformMesh(S.meshes[S.polyIdx].second, S.modelMat);
                saveObject(fullpath.string(), S, newMesh);
                saveFileDialog.ClearSelected();
            }
            openFileDialog.Display();
            if (openFileDialog.HasSelected()) {
                string filenameToOpen = openFileDialog.GetSelected().string();
                Mesh newMesh;
                openObject(filenameToOpen, S, newMesh);
                openFileDialog.ClearSelected();
            }

            // Обновляем размеры Z-буфера
            ImVec2 displaySize = ImGui::GetIO().DisplaySize;
            if (zBuffer.width != (int)displaySize.x || zBuffer.height != (int)displaySize.y) {
                zBuffer = ZBuffer((int)displaySize.x, (int)displaySize.y);
            }

            S.proj.cx = displaySize.x * 0.5f;
            S.proj.cy = displaySize.y * 0.5f;

            // Очищаем Z-буфер каждый кадр
            if (useZBuffer) {
                zBuffer.clear();
            }

            if (showAxes) drawAxes(S, 250.f);

            // Создаем демонстрационные объекты
            vector<pair<Mesh, Mat4>> demoObjects = createDemoObjects(S);
            vector<ImU32> demoColors = {
                IM_COL32(255, 0, 0, 255),    // Красный - центральный объект
                IM_COL32(0, 255, 0, 255),    // Зеленый
                IM_COL32(0, 0, 255, 255),    // Синий
                IM_COL32(255, 255, 0, 255),  // Желтый
                IM_COL32(255, 0, 255, 255),  // Пурпурный
                IM_COL32(0, 255, 255, 255)   // Голубой
            };

            // Отрисовка объектов
            if (showMultipleObjects) {
                // Рисуем несколько объектов для демонстрации перекрытия
                for (size_t i = 0; i < demoObjects.size(); ++i) {
                    if (useZBuffer) {
                        drawMeshZBuffer(demoObjects[i].first, demoObjects[i].second, S,
                            zBuffer, demoColors[i], (S.shadingMode == 0));
                    }
                    else {
                        if (S.shadingMode == 0) {
                            drawWireImGui(demoObjects[i].first, demoObjects[i].second, S,
                                demoColors[i], 1.8f);
                        }
                        else {
                            drawShadedImGui(demoObjects[i].first, demoObjects[i].second, S);
                        }
                    }
                }
            }
            else {
                // Рисуем только основной объект
                if (useZBuffer) {
                    ImU32 color = IM_COL32(
                        (int)(S.objectColor.x * 255),
                        (int)(S.objectColor.y * 255),
                        (int)(S.objectColor.z * 255),
                        255
                    );
                    drawMeshZBuffer(S.base, S.modelMat, S, zBuffer, color, (S.shadingMode == 0));
                }
                else {
                    if (S.shadingMode == 0) {
                        drawWireImGui(S.base, S.modelMat, S, IM_COL32(20, 20, 20, 255), 1.8f);
                    }
                    else {
                        drawShadedImGui(S.base, S.modelMat, S);
                    }
                }
            }

            int fbw, fbh;
            glfwGetFramebufferSize(win, &fbw, &fbh);
            ImGui::Render();
            glViewport(0, 0, fbw, fbh);
            glClearColor(0.96f, 0.96f, 0.96f, 1.f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
            glfwSwapBuffers(win);
        }
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(win);
        glfwTerminate();
        return 0;
    }

}