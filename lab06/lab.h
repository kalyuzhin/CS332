//
// Created by Марк Калюжин on 26.10.2025.
//

#ifndef CS332_LAB_H
#define CS332_LAB_H

#define GL_SILENCE_DEPRECATION

#include "../provider.h"

struct Vec3 {
    float x{}, y{}, z{};
};
struct Vec4 {
    float x{}, y{}, z{}, w{1};
};

struct Mat4 {
    float m[4][4]{};

    static Mat4 identity() {
        Mat4 I{};
        for (int i = 0; i < 4; ++i) I.m[i][i] = 1.0f;
        return I;
    }

    static Mat4 translation(float a, float b, float c) {
        Mat4 T = identity();
        T.m[3][0] = a;
        T.m[3][1] = b;
        T.m[3][2] = c;
        return T;
    }

    static Mat4 scaling(float sx, float sy, float sz) {
        Mat4 S{};
        S.m[0][0] = sx;
        S.m[1][1] = sy;
        S.m[2][2] = sz;
        S.m[3][3] = 1.0f;
        return S;
    }

    static Mat4 rotX(float deg) {
        float r = deg * float(M_PI) / 180.0f, c = std::cos(r), s = std::sin(r);
        Mat4 R = identity();
        R.m[1][1] = c;
        R.m[1][2] = s;
        R.m[2][1] = -s;
        R.m[2][2] = c;
        return R;
    }

    static Mat4 rotY(float deg) {
        float r = deg * float(M_PI) / 180.0f, c = std::cos(r), s = std::sin(r);
        Mat4 R = identity();
        R.m[0][0] = c;
        R.m[0][2] = -s;
        R.m[2][0] = s;
        R.m[2][2] = c;
        return R;
    }

    static Mat4 rotZ(float deg) {
        float r = deg * float(M_PI) / 180.0f, c = std::cos(r), s = std::sin(r);
        Mat4 R = identity();
        R.m[0][0] = c;
        R.m[0][1] = s;
        R.m[1][0] = -s;
        R.m[1][1] = c;
        return R;
    }

    static Mat4 reflectXY() {
        Mat4 R = identity();
        R.m[2][2] = -1;
        return R;
    }

    static Mat4 reflectYZ() {
        Mat4 R = identity();
        R.m[0][0] = -1;
        return R;
    }

    static Mat4 reflectXZ() {
        Mat4 R = identity();
        R.m[1][1] = -1;
        return R;
    }

    static Mat4 axisAngle(float l, float m, float n, float deg) {
        float r = deg * float(M_PI) / 180.0f, c = std::cos(r), s = std::sin(r);
        Mat4 R = identity();
        R.m[0][0] = l * l + c * (1 - l * l);
        R.m[0][1] = l * (1 - c) * m + n * s;
        R.m[0][2] = l * (1 - c) * n - m * s;
        R.m[1][0] = l * (1 - c) * m - n * s;
        R.m[1][1] = m * m + c * (1 - m * m);
        R.m[1][2] = m * (1 - c) * n + l * s;
        R.m[2][0] = l * (1 - c) * n + m * s;
        R.m[2][1] = m * (1 - c) * n - l * s;
        R.m[2][2] = n * n + c * (1 - n * n);
        return R;
    }

    static Mat4 rotateAroundLine(const Vec3 &A, float l, float m, float n, float deg) {
        return translation(-A.x, -A.y, -A.z) * axisAngle(l, m, n, deg) * translation(A.x, A.y, A.z);
    }

    Mat4 operator*(const Mat4 &B) const {
        Mat4 C{};
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                for (int k = 0; k < 4; ++k)
                    C.m[i][j] += m[i][k] * B.m[k][j];
        return C;
    }
};

static inline Vec4 toVec4(const Vec3 &v) { return {v.x, v.y, v.z, 1.0f}; }

static inline Vec3 toVec3(const Vec4 &v) { return {v.x, v.y, v.z}; }

static inline Vec4 mul(const Vec4 &v, const Mat4 &M) {
    Vec4 r{};
    r.x = v.x * M.m[0][0] + v.y * M.m[1][0] + v.z * M.m[2][0] + v.w * M.m[3][0];
    r.y = v.x * M.m[0][1] + v.y * M.m[1][1] + v.z * M.m[2][1] + v.w * M.m[3][1];
    r.z = v.x * M.m[0][2] + v.y * M.m[1][2] + v.z * M.m[2][2] + v.w * M.m[3][2];
    r.w = v.x * M.m[0][3] + v.y * M.m[1][3] + v.z * M.m[2][3] + v.w * M.m[3][3];
    return r;
}

struct Face {
    std::vector<int> idx;
};

struct Polyhedron {
    std::vector<Vec3> V;
    std::vector<Face> F;

    Vec3 faceCenter(const Face &f) const {
        float x = 0, y = 0, z = 0;
        for (int i: f.idx) {
            x += V[i].x;
            y += V[i].y;
            z += V[i].z;
        }
        float n = float(f.idx.size());
        return {x / n, y / n, z / n};
    }

    Vec3 centroidObject() const {
        float x = 0, y = 0, z = 0;
        for (auto &p: V) {
            x += p.x;
            y += p.y;
            z += p.z;
        }
        float n = float(V.size());
        return {x / n, y / n, z / n};
    }

    static Polyhedron Cube() {
        Polyhedron p;
        p.V = {{-1, -1, -1},
               {+1, -1, -1},
               {+1, +1, -1},
               {-1, +1, -1},
               {-1, -1, +1},
               {+1, -1, +1},
               {+1, +1, +1},
               {-1, +1, +1}};
        p.F = {{{0, 1, 2, 3}},
               {{4, 5, 6, 7}},
               {{0, 1, 5, 4}},
               {{3, 2, 6, 7}},
               {{1, 2, 6, 5}},
               {{0, 3, 7, 4}}};
        return p;
    }

    static Polyhedron Tetrahedron() {
        Polyhedron t;
        t.V = {{-1, +1, -1},
               {+1, -1, -1},
               {+1, +1, +1},
               {-1, -1, +1}};
        t.F = {{{0, 1, 2}},
               {{0, 1, 3}},
               {{0, 2, 3}},
               {{1, 2, 3}}};
        return t;
    }

    static Polyhedron Octahedron() {
        Polyhedron o;
        o.V = {{+1, 0, 0},
               {-1, 0, 0},
               {0,  +1, 0},
               {0,  -1, 0},
               {0,  0, +1},
               {0,  0, -1}};
        o.F = {{{0, 2, 4}},
               {{2, 1, 4}},
               {{1, 3, 4}},
               {{3, 0, 4}},
               {{0, 2, 5}},
               {{2, 1, 5}},
               {{1, 3, 5}},
               {{3, 0, 5}}};
        return o;
    }

    static Polyhedron Icosahedron() {
        Polyhedron ico;
        std::vector<std::pair<Vec3, int>> bottom, top;
        bottom.reserve(5);
        top.reserve(5);
        double ang = -90.0;
        int num = 1;
        for (int i = 0; i < 5; ++i) {
            double rad = ang * M_PI / 180.0;
            bottom.push_back({{float(std::cos(rad)), -0.5f, float(std::sin(rad))}, num});
            ang += 72;
            num += 2;
        }
        ang = -54.0;
        num = 2;
        for (int i = 0; i < 5; ++i) {
            double rad = ang * M_PI / 180.0;
            top.push_back({{float(std::cos(rad)), +0.5f, float(std::sin(rad))}, num});
            ang += 72;
            num += 2;
        }
        auto all = bottom;
        all.insert(all.end(), top.begin(), top.end());
        std::sort(all.begin(), all.end(), [](auto &a, auto &b) { return a.second < b.second; });
        ico.V.reserve(12);
        for (auto &pr: all) ico.V.push_back(pr.first);
        for (int i = 1; i <= 8; ++i) ico.F.push_back(Face{{i - 1, i, i + 1}});
        ico.F.push_back(Face{{8, 9, 0}});
        ico.F.push_back(Face{{9, 0, 1}});
        ico.V.push_back({0, -float(std::sqrt(5.0)) / 2.0f, 0}); // 10
        ico.V.push_back({0, +float(std::sqrt(5.0)) / 2.0f, 0}); // 11
        int k = 1;
        for (int i = 0; i < 4; ++i) {
            ico.F.push_back(Face{{10, k - 1, k + 1}});
            k += 2;
        }
        ico.F.push_back(Face{{10, 8, 0}});
        k = 2;
        for (int i = 0; i < 4; ++i) {
            ico.F.push_back(Face{{11, k - 1, k + 1}});
            k += 2;
        }
        ico.F.push_back(Face{{11, 9, 1}});
        return ico;
    }

    static Polyhedron Dodecahedron() {
        Polyhedron ico = Icosahedron();
        Polyhedron dod;
        dod.V.reserve(ico.F.size());
        for (auto &f: ico.F) dod.V.push_back(ico.faceCenter(f));
        for (int i = 0; i < 12; ++i) {
            std::vector<std::pair<float, int>> d;
            d.reserve(dod.V.size());
            for (int j = 0; j < (int) dod.V.size(); ++j) {
                auto &vv = dod.V[j];
                float dx = vv.x - ico.V[i].x, dy = vv.y - ico.V[i].y, dz = vv.z - ico.V[i].z;
                d.push_back({dx * dx + dy * dy + dz * dz, j});
            }
            std::sort(d.begin(), d.end(), [](auto &a, auto &b) { return a.first < b.first; });
            std::array<int, 5> five{};
            for (int t = 0; t < 5; ++t) five[t] = d[t].second;
            int first = five[0], next = five[1];
            std::vector<int> rest = {five[2], five[3], five[4]};
            auto d2 = [&](const Vec3 &A, const Vec3 &B) {
                float dx = A.x - B.x, dy = A.y - B.y, dz = A.z - B.z;
                return dx * dx + dy * dy + dz * dz;
            };
            std::sort(rest.begin(), rest.end(),
                      [&](int a, int b) { return d2(dod.V[a], dod.V[next]) < d2(dod.V[b], dod.V[next]); });
            Face face;
            face.idx = {first, next, rest[0], rest[1], rest[2]};
            dod.F.push_back(face);
        }
        return dod;
    }
};

static inline Vec3 operator+(const Vec3 &a, const Vec3 &b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

static inline Vec3 operator-(const Vec3 &a, const Vec3 &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

static inline Vec3 operator*(const Vec3 &a, float s) { return {a.x * s, a.y * s, a.z * s}; }

static inline Vec3 normalized(const Vec3 &v) {
    float L = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (L == 0) return {0, 0, 1};
    return {v.x / L, v.y / L, v.z / L};
}

static inline Vec3 apply(const Vec3 &p, const Mat4 &M) { return toVec3(mul(toVec4(p), M)); }

enum class Solid {
    Cube, Tetra, Octa, Icosa, Dodeca
};
enum class Projection {
    Perspective, Axonometric
};

struct PerspectiveParams {
    float d = 400.0f;
};
struct AxonometricParams {
    float ax_deg = 35.264f;
    float ay_deg = 45.0f;
};

static inline ImVec2 projectPerspective(const Vec3 &v, const ImVec2 &center, const PerspectiveParams &P) {
    Vec4 row{v.x - center.x, v.y - center.y, v.z, 1.0f};
    Mat4 M = Mat4::identity();
    M.m[2][2] = 0.0f;
    M.m[2][3] = 1.0f / P.d;
    Vec4 out = mul(row, M);
    float w = (out.w != 0.0f) ? out.w : 1.0f;
    return {out.x / w + center.x, out.y / w + center.y};
}

static inline ImVec2 projectAxonometric(const Vec3 &v, const ImVec2 &center, const AxonometricParams &A) {
    float ax = A.ax_deg * float(M_PI) / 180.0f, ay = A.ay_deg * float(M_PI) / 180.0f;
    float cosX = std::cos(ax), sinX = std::sin(ax), cosY = std::cos(ay), sinY = std::sin(ay);
    Mat4 M{};
    M.m[0][0] = cosY;
    M.m[0][1] = sinX * sinY;
    M.m[1][1] = cosX;
    M.m[2][0] = sinY;
    M.m[2][1] = -sinX * cosY;
    M.m[3][3] = 1.0f;
    Vec4 out = mul(toVec4(v - Vec3{center.x, center.y, 0}), M);
    return {out.x + center.x, out.y + center.y};
}

static Vec3 worldCentroid(const Polyhedron &P, const Mat4 &M) {
    float x = 0, y = 0, z = 0;
    for (auto &v: P.V) {
        auto w = apply(v, M);
        x += w.x;
        y += w.y;
        z += w.z;
    }
    float n = float(P.V.size());
    return {x / n, y / n, z / n};
}

static Polyhedron makeSolid(Solid s) {
    switch (s) {
        case Solid::Cube:
            return Polyhedron::Cube();
        case Solid::Tetra:
            return Polyhedron::Tetrahedron();
        case Solid::Octa:
            return Polyhedron::Octahedron();
        case Solid::Icosa:
            return Polyhedron::Icosahedron();
        case Solid::Dodeca:
            return Polyhedron::Dodecahedron();
    }
    return Polyhedron::Cube();
}

struct AppState {
    Polyhedron base = Polyhedron::Cube();
    Mat4 model = Mat4::identity();
    Solid current = Solid::Cube;
    Projection proj = Projection::Perspective;
    PerspectiveParams persp{};
    AxonometricParams axono{};
    float move[3]{0, 0, 0};
    float scale[3]{1, 1, 1};
    float rotXYZ[3]{0, 0, 0};
    float axisAngleDeg = 0.0f;
    int axisIndex = 0;
    float L_P0[3]{0, 0, 0};
    float L_P1[3]{0, 100, 0};
    float L_angle = 0.0f;
    int reflectIndex = 0;
};

static void resetPlacement(AppState &S, const ImVec2 &center) {
    S.model = Mat4::identity();
    S.model = S.model * Mat4::rotX(10.0f) * Mat4::rotY(10.0f);
    S.model = S.model * Mat4::scaling(120, 120, 120);
    S.model = S.model * Mat4::translation(center.x, center.y, 0);
}

static void drawPolyhedronWire(const Polyhedron &P, const Mat4 &model, const ImVec2 &origin, const ImVec2 &size,
                               Projection proj, const PerspectiveParams &persp, const AxonometricParams &ax) {
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 center{origin.x + size.x * 0.5f, origin.y + size.y * 0.5f};
    std::vector<Vec3> W;
    W.reserve(P.V.size());
    for (auto &v: P.V) W.push_back(apply(v, model));
    auto project = [&](const Vec3 &w) {
        return (proj == Projection::Perspective) ? projectPerspective(w, center, persp)
                                                 : projectAxonometric(w, center, ax);
    };
    for (auto &f: P.F) {
        std::vector<ImVec2> poly2d;
        poly2d.reserve(f.idx.size() + 1);
        for (int id: f.idx) poly2d.push_back(project(W[id]));
        poly2d.push_back(poly2d.front());
        dl->AddPolyline(poly2d.data(), (int) poly2d.size(), IM_COL32(0, 0, 0, 255), false, 2.0f);
    }
}

int run_lab6() {
    if (!glfwInit()) return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow *window = glfwCreateWindow(1280, 800, "CG Lab 7–9 (ImGui + GLFW)", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 2;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    AppState S;
    S.base = makeSolid(S.current);
    resetPlacement(S, {640, 400});

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Controls");
        if (ImGui::BeginCombo("Solid", "Select")) {
            if (ImGui::Selectable("Cube", S.current == Solid::Cube)) {
                S.current = Solid::Cube;
                S.base = makeSolid(S.current);
                resetPlacement(S, {640, 400});
            }
            if (ImGui::Selectable("Tetrahedron", S.current == Solid::Tetra)) {
                S.current = Solid::Tetra;
                S.base = makeSolid(S.current);
                resetPlacement(S, {640, 400});
            }
            if (ImGui::Selectable("Octahedron", S.current == Solid::Octa)) {
                S.current = Solid::Octa;
                S.base = makeSolid(S.current);
                resetPlacement(S, {640, 400});
            }
            if (ImGui::Selectable("Icosahedron", S.current == Solid::Icosa)) {
                S.current = Solid::Icosa;
                S.base = makeSolid(S.current);
                resetPlacement(S, {640, 400});
            }
            if (ImGui::Selectable("Dodecahedron", S.current == Solid::Dodeca)) {
                S.current = Solid::Dodeca;
                S.base = makeSolid(S.current);
                resetPlacement(S, {640, 400});
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Reset")) resetPlacement(S, {640, 400});
        ImGui::Separator();

        ImGui::Text("Projection");
        int projIdx = (S.proj == Projection::Perspective ? 0 : 1);
        if (ImGui::RadioButton("Perspective", projIdx == 0)) S.proj = Projection::Perspective;
        ImGui::SameLine();
        if (ImGui::RadioButton("Axonometric", projIdx == 1)) S.proj = Projection::Axonometric;
        if (S.proj == Projection::Perspective) {
            ImGui::SliderFloat("d (focal)", &S.persp.d, 100.0f, 1500.0f, "%.1f");
        } else {
            ImGui::SliderFloat("ax (deg)", &S.axono.ax_deg, 0.0f, 90.0f, "%.3f");
            ImGui::SliderFloat("ay (deg)", &S.axono.ay_deg, 0.0f, 90.0f, "%.3f");
        }
        ImGui::Separator();

        ImGui::Text("Move (dx,dy,dz)");
        ImGui::DragFloat3("##move", S.move, 1.0f);
        if (ImGui::Button("Apply Move")) S.model = S.model * Mat4::translation(S.move[0], S.move[1], S.move[2]);

        ImGui::Text("Scale (sx,sy,sz) about center");
        ImGui::DragFloat3("##scale", S.scale, 0.01f, 0.01f, 10.0f);
        if (ImGui::Button("Apply Scale about center")) {
            Vec3 C = worldCentroid(S.base, S.model);
            S.model =
                    S.model * Mat4::translation(-C.x, -C.y, -C.z) * Mat4::scaling(S.scale[0], S.scale[1], S.scale[2]) *
                    Mat4::translation(C.x, C.y, C.z);
        }

        ImGui::Text("Rotate (deg) around X/Y/Z about center");
        ImGui::DragFloat3("##rotxyz", S.rotXYZ, 0.1f);
        if (ImGui::Button("Apply XYZ rotation about center")) {
            Vec3 C = worldCentroid(S.base, S.model);
            S.model =
                    S.model * Mat4::translation(-C.x, -C.y, -C.z) * Mat4::rotX(S.rotXYZ[0]) * Mat4::rotY(S.rotXYZ[1]) *
                    Mat4::rotZ(S.rotXYZ[2]) * Mat4::translation(C.x, C.y, C.z);
        }

        ImGui::Text("Rotate around center-parallel axis");
        ImGui::SliderInt("Axis (0=X,1=Y,2=Z)", &S.axisIndex, 0, 2);
        ImGui::DragFloat("Angle (deg)", &S.axisAngleDeg, 0.1f);
        if (ImGui::Button("Apply Center-Axis Rotation")) {
            Vec3 C = worldCentroid(S.base, S.model);
            Vec3 dir = (S.axisIndex == 0 ? Vec3{1, 0, 0} : (S.axisIndex == 1 ? Vec3{0, 1, 0} : Vec3{0, 0, 1}));
            dir = normalized(dir);
            S.model = S.model * Mat4::rotateAroundLine(C, dir.x, dir.y, dir.z, S.axisAngleDeg);
        }

        ImGui::Text("Rotate about arbitrary line P0->P1");
        ImGui::DragFloat3("P0", S.L_P0, 0.5f);
        ImGui::DragFloat3("P1", S.L_P1, 0.5f);
        ImGui::DragFloat("Angle", &S.L_angle, 0.1f);
        if (ImGui::Button("Apply Line Rotation")) {
            Vec3 P0{S.L_P0[0], S.L_P0[1], S.L_P0[2]};
            Vec3 P1{S.L_P1[0], S.L_P1[1], S.L_P1[2]};
            Vec3 d = normalized(P1 - P0);
            S.model = S.model * Mat4::rotateAroundLine(P0, d.x, d.y, d.z, S.L_angle);
        }

        ImGui::Separator();
        ImGui::Text("Reflect about plane");
        ImGui::RadioButton("XY", &S.reflectIndex, 0);
        ImGui::SameLine();
        ImGui::RadioButton("YZ", &S.reflectIndex, 1);
        ImGui::SameLine();
        ImGui::RadioButton("XZ", &S.reflectIndex, 2);
        if (ImGui::Button("Apply Reflection")) {
            Mat4 R = (S.reflectIndex == 0 ? Mat4::reflectXY() : (S.reflectIndex == 1 ? Mat4::reflectYZ()
                                                                                     : Mat4::reflectXZ()));
            Vec3 C = worldCentroid(S.base, S.model);
            S.model = S.model * Mat4::translation(-C.x, -C.y, -C.z) * R * Mat4::translation(C.x, C.y, C.z);
        }
        ImGui::End();

        ImGui::Begin("Viewport");
        ImVec2 p0 = ImGui::GetCursorScreenPos();
        ImVec2 avail = ImGui::GetContentRegionAvail();
        if (avail.x < 100) avail.x = 100;
        if (avail.y < 100) avail.y = 100;
        ImDrawList *dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(p0, {p0.x + avail.x, p0.y + avail.y}, IM_COL32(245, 245, 245, 255));
        drawPolyhedronWire(S.base, S.model, p0, avail, S.proj, S.persp, S.axono);
        ImGui::Dummy(avail);
        ImGui::End();

        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(1, 1, 1, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}


#endif //CS332_LAB_H
