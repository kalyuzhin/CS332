// GLFW + ImGui wireframe (матрицы, каркас линиями ImGui). macOS/Windows ready.
// Никаких рисовалок OpenGL для геометрии: всю 3D-математику и проекции считаем сами.

#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdint>

// ==== GLFW ====
#include <GLFW/glfw3.h>

// ==== Dear ImGui ====
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"

// --- ВАЖНО: OpenGL loader для бэкенда ImGui ---
// macOS: системный OpenGL, ничего не нужно
// Windows/Linux: используем GLAD
#ifdef APPLE
#define IMGUI_IMPL_OPENGL_LOADER_OSX
#endif
#include "backends/imgui_impl_opengl3.h"
using std::vector;

static constexpr float PI = 3.14159265358979323846f;
inline float deg2rad(float a) { return a * PI / 180.0f; }

// ========================= Алгебра =========================
struct Vec3 { float x{}, y{}, z{}; };
struct Vec4 { float x{}, y{}, z{}, w{}; };

struct Mat4 {
    // row-vector convention: p' = p * M
    float m[4][4]{};
    static Mat4 I() { Mat4 M{}; for (int i = 0; i < 4; ++i) M.m[i][i] = 1.f; return M; }
    static Mat4 T(float a, float b, float c) { Mat4 M = I(); M.m[3][0] = a; M.m[3][1] = b; M.m[3][2] = c; return M; }
    static Mat4 S(float sx, float sy, float sz) { Mat4 M{}; M.m[0][0] = sx; M.m[1][1] = sy; M.m[2][2] = sz; M.m[3][3] = 1.f; return M; }
    static Mat4 Rx(float deg) {
        float c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg)); Mat4 R = I();
        R.m[1][1] = c; R.m[1][2] = s; R.m[2][1] = -s; R.m[2][2] = c; return R;
    }
    static Mat4 Ry(float deg) {
        float c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg)); Mat4 R = I();
        R.m[0][0] = c; R.m[0][2] = -s; R.m[2][0] = s; R.m[2][2] = c; return R;
    }
    static Mat4 Rz(float deg) {
        float c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg)); Mat4 R = I();
        R.m[0][0] = c; R.m[0][1] = s; R.m[1][0] = -s; R.m[1][1] = c; return R;
    }
    static Mat4 Raxis(float l, float m, float n, float deg) { // Rodrigues
        float a = deg2rad(deg), c = std::cos(a), s = std::sin(a), t = 1.f - c; Mat4 R = I();
        R.m[0][0] = t * l * l + c;   R.m[0][1] = t * l * m + s * n; R.m[0][2] = t * l * n - s * m;
        R.m[1][0] = t * m * l - s * n; R.m[1][1] = t * m * m + c;   R.m[1][2] = t * m * n + s * l;
        R.m[2][0] = t * n * l + s * m; R.m[2][1] = t * n * m - s * l; R.m[2][2] = t * n * n + c;
        return R;
    }
    static Mat4 RefXY() { Mat4 M = I(); M.m[2][2] = -1.f; return M; }
    static Mat4 RefYZ() { Mat4 M = I(); M.m[0][0] = -1.f; return M; }
    static Mat4 RefXZ() { Mat4 M = I(); M.m[1][1] = -1.f; return M; }
};
inline Mat4 operator*(const Mat4& A, const Mat4& B) {
    Mat4 C{};
    for (int r = 0; r < 4; ++r) for (int c = 0; c < 4; ++c) {
        float s = 0.f; for (int k = 0; k < 4; ++k) s += A.m[r][k] * B.m[k][c]; C.m[r][c] = s;
    }
    return C;
}
inline Vec4 operator*(const Vec4& v, const Mat4& M) {
    Vec4 r{};
    r.x = v.x * M.m[0][0] + v.y * M.m[1][0] + v.z * M.m[2][0] + v.w * M.m[3][0];
    r.y = v.x * M.m[0][1] + v.y * M.m[1][1] + v.z * M.m[2][1] + v.w * M.m[3][1];
    r.z = v.x * M.m[0][2] + v.y * M.m[1][2] + v.z * M.m[2][2] + v.w * M.m[3][2];
    r.w = v.x * M.m[0][3] + v.y * M.m[1][3] + v.z * M.m[2][3] + v.w * M.m[3][3];
    return r;
}
inline float vlen(const Vec3& v) { return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z); }
inline Vec3  norm(const Vec3& v) { float L = vlen(v); return (L == 0) ? Vec3{ 0,0,0 } : Vec3{ v.x / L,v.y / L,v.z / L }; }

// избегаем конфликта со std::apply — называем xform
inline Vec3 xform(const Vec3& p, const Mat4& M) {
    Vec4 v{ p.x,p.y,p.z,1.f };
    Vec4 r = v * M;
    if (std::abs(r.w) < 1e-6f) return { r.x,r.y,r.z };
    return { r.x / r.w,r.y / r.w,r.z / r.w };
}

// ========================= Геометрия =========================
struct Vertex { float x{}, y{}, z{}; };
struct Face { vector<int> idx; };

struct Polyhedron {
    vector<Vertex> V; vector<Face> F;

    Vec3 center() const {
        Vec3 c{ 0,0,0 }; if (V.empty()) return c;
        for (auto& p : V) { c.x += p.x; c.y += p.y; c.z += p.z; }
        c.x /= V.size(); c.y /= V.size(); c.z /= V.size(); return c;
    }
    Polyhedron transformed(const Mat4& M) const {
        Polyhedron o = *this;
        for (auto& p : o.V) { Vec3 t = xform({ p.x,p.y,p.z }, M); p.x = t.x; p.y = t.y; p.z = t.z; }
        return o;
    }Polyhedron move(float dx, float dy, float dz) const { return transformed(Mat4::T(dx, dy, dz)); }
    Polyhedron scale(float sx, float sy, float sz) const { return transformed(Mat4::S(sx, sy, sz)); }
    Polyhedron rotateX(float a) const { return transformed(Mat4::Rx(a)); }
    Polyhedron rotateY(float a) const { return transformed(Mat4::Ry(a)); }
    Polyhedron rotateZ(float a) const { return transformed(Mat4::Rz(a)); }
    Polyhedron reflectXY() const { return transformed(Mat4::RefXY()); }
    Polyhedron reflectYZ() const { return transformed(Mat4::RefYZ()); }
    Polyhedron reflectXZ() const { return transformed(Mat4::RefXZ()); }

    Polyhedron scaledAroundCenter(float sx, float sy, float sz) const {
        Vec3 c = center();
        return transformed(Mat4::T(-c.x, -c.y, -c.z) * Mat4::S(sx, sy, sz) * Mat4::T(c.x, c.y, c.z));
    }
    Polyhedron rotateAroundAxisThroughCenter(char axis, float deg) const {
        Vec3 c = center();
        Mat4 R = (axis == 'x' || axis == 'X') ? Mat4::Rx(deg) : (axis == 'y' || axis == 'Y') ? Mat4::Ry(deg) : Mat4::Rz(deg);
        return transformed(Mat4::T(-c.x, -c.y, -c.z) * R * Mat4::T(c.x, c.y, c.z));
    }
    Polyhedron rotateAroundLine(const Vec3& P0, const Vec3& P1, float deg) const {
        Vec3 d = norm({ P1.x - P0.x,P1.y - P0.y,P1.z - P0.z });
        Mat4 R = Mat4::Raxis(d.x, d.y, d.z, deg);
        return transformed(Mat4::T(-P0.x, -P0.y, -P0.z) * R * Mat4::T(P0.x, P0.y, P0.z));
    }
};

// Платоновы тела
Polyhedron makeCube(float s = 1.f) {
    Polyhedron P; float a = s;
    P.V = { {-a,-a,-a},{ a,-a,-a},{ a, a,-a},{-a, a,-a},{-a,-a, a},{ a,-a, a},{ a, a, a},{-a, a, a} };
    P.F = { {{0,1,2,3}},{{4,5,6,7}},{{0,1,5,4}},{{3,2,6,7}},{{1,2,6,5}},{{0,3,7,4}} }; return P;
}
Polyhedron makeTetra(float s = 1.f) {
    Polyhedron P; float a = s;
    P.V = { {-a, a,-a},{ a,-a,-a},{ a, a, a},{-a,-a, a} };
    P.F = { {{0,1,2}},{{0,1,3}},{{0,2,3}},{{1,2,3}} }; return P;
}
Polyhedron makeOcta(float s = 1.f) {
    Polyhedron P; float a = s;
    P.V = { {0,0,a},{0,0,-a},{0,a,0},{0,-a,0},{a,0,0},{-a,0,0} };
    P.F = { {{0,2,4}},{{0,4,3}},{{0,3,5}},{{0,5,2}},{{1,4,2}},{{1,3,4}},{{1,5,3}},{{1,2,5}} }; return P;
}
Polyhedron makeIcosa(float s = 1.f) {
    Polyhedron P; const float phi = (1.f + std::sqrt(5.f)) * 0.5f; float a = s, b = s / phi;
    P.V = { {-a, b,0},{ a, b,0},{-a,-b,0},{ a,-b,0},{0,-a, b},{0, a, b},{0,-a,-b},{0, a,-b},{ b,0,-a},{ b,0, a},{-b,0,-a},{-b,0, a} };
    P.F = { {{0,11,5}},{{0,5,1}},{{0,1,7}},{{0,7,10}},{{0,10,11}},{{1,5,9}},{{5,11,4}},{{11,10,2}},{{10,7,6}},{{7,1,8}},{{3,9,4}},{{3,4,2}},{{3,2,6}},{{3,6,8}},{{3,8,9}},{{4,9,5}},{{2,4,11}},{{6,2,10}},{{8,6,7}},{{9,8,1}} };
    return P;
}
Polyhedron makeDodeca(float s = 1.f) {
    Polyhedron ico = makeIcosa(s); vector<Vec3> C; C.reserve(ico.F.size());
    for (const auto& f : ico.F) {
        Vec3 c{ 0,0,0 }; for (int i : f.idx) { c.x += ico.V[i].x; c.y += ico.V[i].y; c.z += ico.V[i].z; }
        float inv = 1.f / f.idx.size(); c.x *= inv; c.y *= inv; c.z *= inv; C.push_back(c);
    }
    Polyhedron P; for (auto& c : C) P.V.push_back({ c.x,c.y,c.z });
    const int D[12][5] = { {0,1,5,4,2},{1,6,14,9,5},{6,7,16,15,14},{7,3,13,12,16},{3,0,2,8,13},{2,4,10,11,8},{5,9,19,18,10},{14,15,17,19,9},{16,12,20,17,15},{13,8,11,21,20},{0,3,7,6,1},{10,18,22,21,11} };
    for (const auto& face : D) { Face f; f.idx.assign(face, face + 5); P.F.push_back(f); }
    return P;
}

// ========================= Проекции =========================
struct Projector {
    bool perspective = true;
    float d = 600.f;        // «фокус» перспективы
    float ax = 45.264f;     // аксонометрия: поворот X
    float ay = 45.f;        // аксонометрия: поворот Y
    float scale = 160.f;
    float cx = 600.f, cy = 400.f; // центр экрана

    Vec3 axo(const Vec3& p) const { return xform(p, Mat4::Rx(ax) * Mat4::Ry(ay)); }
    bool project(const Vec3& p, int& X, int& Y) const {
        Vec3 q = axo(p);
        if (perspective) {
            float zc = q.z + d;
            if (zc <= 1.f) return false;
            float x = (q.x * d / q.z) * scale / 100.f + cx;
            float y = (q.y * d / q.z) * scale / 100.f + cy;
            /*float w = 1.f + p.z / d;
            if (w <= 0.01f) return false;
            float x = (p.x / w) * scale + cx;
            float y = (p.y / w) * scale + cy;*/
            X = (int)std::lround(x); Y = (int)std::lround(y); return true;
        }
        else {
            //Vec3 q = axo(p);
            float x = q.x * scale + cx, y = q.y * scale + cy;
            X = (int)std::lround(x); Y = (int)std::lround(y); return true;
        }
    }
};

// ========================= Вспомогательное =========================
enum class PolyKind { Tetra = 0, Cube, Octa, Ico, Dode };

static void applyKeyOps(GLFWwindow* win, Polyhedron& obj, Polyhedron& base,
    PolyKind& kind, Projector& proj,
    Vec3& P0, Vec3& P1, char& axisSel)
{
    auto pressed = [&](int key) { return glfwGetKey(win, key) == GLFW_PRESS; };

    float moveStep = 20.f, rotStep = 5.f, scaleMul = 1.05f, lineRotStep = 5.f;

    // выбор тела
    if (pressed(GLFW_KEY_1)) { kind = PolyKind::Tetra; base = makeTetra(1.f); obj = base; }
    if (pressed(GLFW_KEY_2)) { kind = PolyKind::Cube;  base = makeCube(1.f);  obj = base; }
    if (pressed(GLFW_KEY_3)) { kind = PolyKind::Octa;  base = makeOcta(1.f);  obj = base; }
    if (pressed(GLFW_KEY_4)) { kind = PolyKind::Ico;   base = makeIcosa(1.f); obj = base; }
    if (pressed(GLFW_KEY_5)) { kind = PolyKind::Dode;  base = makeDodeca(1.f); obj = base; }

    // проекции
    if (pressed(GLFW_KEY_P)) proj.perspective = true;
    if (pressed(GLFW_KEY_O)) proj.perspective = false;

    // перенос
    if (pressed(GLFW_KEY_LEFT))   obj = obj.move(-moveStep, 0, 0);
    if (pressed(GLFW_KEY_RIGHT))  obj = obj.move(+moveStep, 0, 0);
    if (pressed(GLFW_KEY_UP))     obj = obj.move(0, -moveStep, 0);
    if (pressed(GLFW_KEY_DOWN))   obj = obj.move(0, +moveStep, 0);
    if (pressed(GLFW_KEY_PAGE_UP))   obj = obj.move(0, 0, +moveStep);
    if (pressed(GLFW_KEY_PAGE_DOWN)) obj = obj.move(0, 0, -moveStep);

    // масштаб (отн. центра)
    if (pressed(GLFW_KEY_LEFT_BRACKET))  obj = obj.scaledAroundCenter(1.f / scaleMul, 1.f / scaleMul, 1.f / scaleMul);
    if (pressed(GLFW_KEY_RIGHT_BRACKET)) obj = obj.scaledAroundCenter(scaleMul, scaleMul, scaleMul);

    // вращение через центр
    if (pressed(GLFW_KEY_X)) obj = obj.rotateAroundAxisThroughCenter('X', rotStep);
    if (pressed(GLFW_KEY_Y)) obj = obj.rotateAroundAxisThroughCenter('Y', rotStep);
    if (pressed(GLFW_KEY_Z)) obj = obj.rotateAroundAxisThroughCenter('Z', rotStep);

    // отражения
    if (pressed(GLFW_KEY_F1)) obj = obj.reflectXY();
    if (pressed(GLFW_KEY_F2)) obj = obj.reflectYZ();
    if (pressed(GLFW_KEY_F3)) obj = obj.reflectXZ();

    // выбор оси для вращения
    static bool tabLatch = false;
    if (pressed(GLFW_KEY_TAB)) {
        if (!tabLatch) {
            axisSel = (axisSel == 'X' ? 'Y' : axisSel == 'Y' ? 'Z' : 'X');
            std::cout << "Axis: " << axisSel << "\n";
            tabLatch = true;
        }
    }
    else tabLatch = false;

    if (pressed(GLFW_KEY_SEMICOLON))  obj = obj.rotateAroundAxisThroughCenter(axisSel, +rotStep);
    if (pressed(GLFW_KEY_APOSTROPHE)) obj = obj.rotateAroundAxisThroughCenter(axisSel, -rotStep);

    // произвольная прямая P0, P1
    if (pressed(GLFW_KEY_A)) P0.x -= moveStep;
    if (pressed(GLFW_KEY_D)) P0.x += moveStep;
    if (pressed(GLFW_KEY_W)) P0.y -= moveStep;
    if (pressed(GLFW_KEY_S)) P0.y += moveStep;
    if (pressed(GLFW_KEY_Q)) P0.z -= moveStep;
    if (pressed(GLFW_KEY_E)) P0.z += moveStep;

    if (pressed(GLFW_KEY_J)) P1.x -= moveStep;
    if (pressed(GLFW_KEY_L)) P1.x += moveStep;
    if (pressed(GLFW_KEY_I)) P1.y -= moveStep;
    if (pressed(GLFW_KEY_K)) P1.y += moveStep;
    if (pressed(GLFW_KEY_U)) P1.z -= moveStep;
    if (pressed(GLFW_KEY_O)) P1.z += moveStep;

    if (pressed(GLFW_KEY_MINUS)) obj = obj.rotateAroundLine(P0, P1, +lineRotStep);
    if (pressed(GLFW_KEY_EQUAL)) obj = obj.rotateAroundLine(P0, P1, -lineRotStep);

    // зум
    if (pressed(GLFW_KEY_COMMA))  proj.scale *= 0.98f;
    if (pressed(GLFW_KEY_PERIOD)) proj.scale *= 1.02f;

    // сброс
    if (pressed(GLFW_KEY_C)) {
        obj = base; proj.perspective = true; proj.ax = 35.264f; proj.ay = 45.f; proj.scale = 160.f;
    }
}

// каркас — линиями ImGui (разрешено по условию для wireframe)
static void drawWireImGui(const Polyhedron& P, const Projector& proj, ImU32 color = IM_COL32(0, 0, 0, 255), float thickness = 1.6f)
{
    ImDrawList* dl = ImGui::GetBackgroundDrawList(); // рисуем поверх всего окна
    for (const auto& f : P.F) {
        for (size_t i = 0; i < f.idx.size(); ++i) {
            int i0 = f.idx[i], i1 = f.idx[(i + 1) % f.idx.size()];
            Vec3 a{ P.V[i0].x, P.V[i0].y, P.V[i0].z };
            Vec3 b{ P.V[i1].x, P.V[i1].y, P.V[i1].z };
            int x0, y0, x1, y1;
            bool ok0 = proj.project(a, x0, y0), ok1 = proj.project(b, x1, y1);
            if (ok0 && ok1) dl->AddLine(ImVec2((float)x0, (float)y0), ImVec2((float)x1, (float)y1), color, thickness);
        }
    }
}

// ========================= main =========================
int run_main() {
    const int W = 1200, H = 800;

    // --- GLFW init ---
    if (!glfwInit()) { std::cerr << "GLFW init failed\n"; return 1; }
#ifdef APPLE
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(W, H, "GLFW+ImGui Wireframe", nullptr, nullptr);
    if (!window) { std::cerr << "GLFW window failed\n"; glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // --- ImGui init ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(
#ifdef APPLE
        "#version 150"
#else
        nullptr
#endif
    );

    // Состояние сцены
    PolyKind kind = PolyKind::Cube;
    Polyhedron base = makeCube(1.f);
    Polyhedron obj = base;

    Projector proj; proj.cx = W * 0.5f; proj.cy = H * 0.5f;

    Vec3 P0{ -200,0,0 }, P1{ 200,0,0 };
    char axisSel = 'X';

    // UI состояние
    int  polyIdx = 1; // 0..4
    bool persp = true;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        applyKeyOps(window, obj, base, kind, proj, P0, P1, axisSel);

        // --- ImGui frame begin ---
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Панель управления (слева)
        ImGui::Begin("Controls");
        const char* items[] = { "Tetrahedron","Cube","Octahedron","Icosahedron","Dodecahedron" };
        if (ImGui::Combo("Polyhedron", &polyIdx, items, IM_ARRAYSIZE(items))) {
            switch (polyIdx) {
            case 0: kind = PolyKind::Tetra; base = makeTetra(1.f); break;
            case 1: kind = PolyKind::Cube;  base = makeCube(1.f);  break;
            case 2: kind = PolyKind::Octa;  base = makeOcta(1.f);  break;
            case 3: kind = PolyKind::Ico;   base = makeIcosa(1.f); break;
            case 4: kind = PolyKind::Dode;  base = makeDodeca(1.f); break;
            }
            obj = base;
        }
        if (ImGui::Checkbox("Perspective", &persp)) proj.perspective = persp;
        ImGui::SliderFloat("Scale", &proj.scale, 20.f, 400.f);
        if (!persp) {
            ImGui::SliderFloat("Axon X", &proj.ax, 0.f, 90.f);
            ImGui::SliderFloat("Axon Y", &proj.ay, 0.f, 90.f);
        }
        else {
            ImGui::SliderFloat("Focus d", &proj.d, 100.f, 2000.f);
        }
        ImGui::SeparatorText("Rotate around center (deg)");
        if (ImGui::Button("X +5")) obj = obj.rotateAroundAxisThroughCenter('X', +5.f);
        ImGui::SameLine();
        if (ImGui::Button("Y +5")) obj = obj.rotateAroundAxisThroughCenter('Y', +5.f);
        ImGui::SameLine();
        if (ImGui::Button("Z +5")) obj = obj.rotateAroundAxisThroughCenter('Z', +5.f);

        ImGui::SeparatorText("Reflect");
        if (ImGui::Button("Reflect XY")) obj = obj.reflectXY();
        ImGui::SameLine();
        if (ImGui::Button("Reflect YZ")) obj = obj.reflectYZ();
        ImGui::SameLine();
        if (ImGui::Button("Reflect XZ")) obj = obj.reflectXZ();

        ImGui::SeparatorText("Arbitrary line");
        ImGui::Text("P0(%.1f,%.1f,%.1f)  P1(%.1f,%.1f,%.1f)", P0.x, P0.y, P0.z, P1.x, P1.y, P1.z);
        if (ImGui::Button("Rotate P0-P1 +5")) obj = obj.rotateAroundLine(P0, P1, +5.f);
        ImGui::SameLine();
        if (ImGui::Button("Rotate P0-P1 -5")) obj = obj.rotateAroundLine(P0, P1, -5.f);

        if (ImGui::Button("Reset [C]")) {
            obj = base; proj.perspective = true; proj.ax = 35.264f; proj.ay = 45.f; proj.scale = 160.f; persp = true;
        }
        ImGui::End();

        // Подгоняем центр на случай ресайза окна
        int fbw, fbh; glfwGetFramebufferSize(window, &fbw, &fbh);
        proj.cx = fbw * 0.5f; proj.cy = fbh * 0.5f;

        // Рисуем каркас (поверх всего окна)
        drawWireImGui(obj, proj, IM_COL32(0, 0, 0, 255), 1.8f);

        // вспомогательная линия P0-P1
        int x0, y0, x1, y1;
        if (proj.project(P0, x0, y0) && proj.project(P1, x1, y1)) {
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2((float)x0, (float)y0),
                ImVec2((float)x1, (float)y1),
                IM_COL32(180, 0, 0, 255), 1.8f);
        }

        // --- ImGui render ---
        ImGui::Render();
        glViewport(0, 0, fbw, fbh);
        glClearColor(0.96f, 0.96f, 0.96f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    // --- shutdown ---
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}