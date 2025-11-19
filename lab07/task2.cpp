#include "task2.h"
#include "../lab06/lab.h"
#include <functional>
#include <map>

using std::vector;
using std::string;

namespace lab7 {
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

    int run() {
        const int W = 1200, H = 800;
        if (!glfwInit()) return 1;
#ifdef __APPLE__
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
        GLFWwindow* win = glfwCreateWindow(W, H, "Surface of Revolution & Function Graph", nullptr, nullptr);
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

        static vector<Vec3> generatrix = { {100.f, 0.f, 0.f}, {100.f, 100.f, 0.f} };
        static int subdivisions = 12;
        static int axisIndex = 2;
        const char* axisNames[] = { "X", "Y", "Z" };

        static int funcIndex = 0;
        const char* funcNames[] = { "Plane z=0", "Paraboloid", "Saddle", "Wave", "Hill" };
        static float x0 = -200.0f, x1 = 200.0f;
        static float y0 = -200.0f, y1 = 200.0f;
        static int subdivX = 20, subdivY = 20;

        while (!glfwWindowShouldClose(win)) {
            glfwPollEvents();
            applyKeyOps(win, S);

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
                    S.viewVec.x = vv[0]; S.viewVec.y = vv[1]; S.viewVec.z = vv[2];
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
                if (S.kind == PolyKind::Cube)  S.base = makeCube(150.f);
                if (S.kind == PolyKind::Octa)  S.base = makeOcta(150.f);
                if (S.kind == PolyKind::Ico)   S.base = makeIcosa(150.f);
                if (S.kind == PolyKind::Dode)  S.base = makeDodeca(150.f);
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
                    string objName = string("Revol@") + axisNames[axisIndex] + string("/") + std::to_string(subdivisions);
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
                case 0: selectedFunc = funcPlane; break;
                case 1: selectedFunc = funcParaboloid; break;
                case 2: selectedFunc = funcSaddle; break;
                case 3: selectedFunc = funcWave; break;
                case 4: selectedFunc = funcHill; break;
                default: selectedFunc = funcPlane;
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
            S.proj.cx = ImGui::GetIO().DisplaySize.x * 0.5f;
            S.proj.cy = ImGui::GetIO().DisplaySize.y * 0.5f;
            if (showAxes) drawAxes(S.proj, 250.f);
            if (S.shadingMode == 0) {
                drawWireImGui(S.base, S.modelMat, S, IM_COL32(20, 20, 20, 255), 1.8f);
            }
            else {
                drawShadedImGui(S.base, S.modelMat, S);
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
