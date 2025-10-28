//
// Created by Марк Калюжин on 26.10.2025.
//

#ifndef CS332_LAB_H
#define CS332_LAB_H

#define GL_SILENCE_DEPRECATION

#include "../provider.h"

#ifdef __APPLE__
#define IMGUI_IMPL_OPENGL_LOADER_OSX
#endif

using std::vector;

static constexpr float PI = 3.14159265358979323846f;

inline float deg2rad(float a) { return a * PI / 180.0f; }

// ========================= Линал =========================
struct Vec3 {
    float x{}, y{}, z{};
};
struct Vec4 {
    float x{}, y{}, z{}, w{};
};

inline Vec3 operator+(const Vec3 &a, const Vec3 &b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }

inline Vec3 operator-(const Vec3 &a, const Vec3 &b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }

inline Vec3 operator*(const Vec3 &a, float s) { return {a.x * s, a.y * s, a.z * s}; }

inline float dot(const Vec3 &a, const Vec3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

inline Vec3 cross(const Vec3 &a, const Vec3 &b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

inline float vlen(const Vec3 &v) { return std::sqrt(dot(v, v)); }

inline Vec3 norm(const Vec3 &v) {
    float L = vlen(v);
    return (L == 0) ? Vec3{0, 0, 0} : Vec3{v.x / L, v.y / L, v.z / L};
}

struct Mat4 {
    // row-vector convention: p' = p * M
    float m[4][4]{};

    static Mat4 I() {
        Mat4 M{};
        for (int i = 0; i < 4; ++i) M.m[i][i] = 1.f;
        return M;
    }

    static Mat4 T(float a, float b, float c) {
        Mat4 M = I();
        M.m[3][0] = a;
        M.m[3][1] = b;
        M.m[3][2] = c;
        return M;
    }

    static Mat4 S(float sx, float sy, float sz) {
        Mat4 M{};
        M.m[0][0] = sx;
        M.m[1][1] = sy;
        M.m[2][2] = sz;
        M.m[3][3] = 1.f;
        return M;
    }

    static Mat4 Rx(float deg) {
        float c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg));
        Mat4 R = I();
        R.m[1][1] = c;
        R.m[1][2] = s;
        R.m[2][1] = -s;
        R.m[2][2] = c;
        return R;
    }

    static Mat4 Ry(float deg) {
        float c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg));
        Mat4 R = I();
        R.m[0][0] = c;
        R.m[0][2] = -s;
        R.m[2][0] = s;
        R.m[2][2] = c;
        return R;
    }

    static Mat4 Rz(float deg) {
        float c = std::cos(deg2rad(deg)), s = std::sin(deg2rad(deg));
        Mat4 R = I();
        R.m[0][0] = c;
        R.m[0][1] = s;
        R.m[1][0] = -s;
        R.m[1][1] = c;
        return R;
    }

    static Mat4 Raxis(const Vec3 &u, float deg) { // u — единичный
        float a = deg2rad(deg), c = std::cos(a), s = std::sin(a), t = 1.f - c;
        float l = u.x, m = u.y, n = u.z;
        Mat4 R = I();
        R.m[0][0] = t * l * l + c;
        R.m[0][1] = t * l * m + s * n;
        R.m[0][2] = t * l * n - s * m;
        R.m[1][0] = t * m * l - s * n;
        R.m[1][1] = t * m * m + c;
        R.m[1][2] = t * m * n + s * l;
        R.m[2][0] = t * n * l + s * m;
        R.m[2][1] = t * n * m - s * l;
        R.m[2][2] = t * n * n + c;
        return R;
    }

    static Mat4 RefXY() {
        Mat4 M = I();
        M.m[2][2] = -1.f;
        return M;
    }

    static Mat4 RefYZ() {
        Mat4 M = I();
        M.m[0][0] = -1.f;
        return M;
    }

    static Mat4 RefXZ() {
        Mat4 M = I();
        M.m[1][1] = -1.f;
        return M;
    }
};

inline Mat4 operator*(const Mat4 &A, const Mat4 &B) {
    Mat4 C{};
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c) {
            float s = 0.f;
            for (int k = 0; k < 4; ++k) s += A.m[r][k] * B.m[k][c];
            C.m[r][c] = s;
        }
    return C;
}

inline Vec4 operator*(const Vec4 &v, const Mat4 &M) {
    Vec4 r{};
    r.x = v.x * M.m[0][0] + v.y * M.m[1][0] + v.z * M.m[2][0] + v.w * M.m[3][0];
    r.y = v.x * M.m[0][1] + v.y * M.m[1][1] + v.z * M.m[2][1] + v.w * M.m[3][1];
    r.z = v.x * M.m[0][2] + v.y * M.m[1][2] + v.z * M.m[2][2] + v.w * M.m[3][2];
    r.w = v.x * M.m[0][3] + v.y * M.m[1][3] + v.z * M.m[2][3] + v.w * M.m[3][3];
    return r;
}

inline Vec3 xform(const Vec3 &p, const Mat4 &M) {
    Vec4 v{p.x, p.y, p.z, 1.f};
    Vec4 r = v * M;
    if (std::abs(r.w) < 1e-6f) return {r.x, r.y, r.z};
    return {r.x / r.w, r.y / r.w, r.z / r.w};
}

// ========================= Геометрия =========================
struct Vertex {
    float x{}, y{}, z{};
};
struct Face {
    vector<int> idx;
};
struct Mesh {
    vector<Vertex> V;
    vector<Face> F;
};

Vec3 centroid(const Mesh &m) {
    Vec3 c{0, 0, 0};
    if (m.V.empty()) return c;
    for (auto &p: m.V) {
        c.x += p.x;
        c.y += p.y;
        c.z += p.z;
    }
    float inv = 1.f / static_cast<float>(m.V.size());
    return {c.x * inv, c.y * inv, c.z * inv};
}

// Platonic solids
Mesh makeCube(float s = 1.f) {
    Mesh P;
    float a = s;
    P.V = {{-a, -a, -a},
           {a,  -a, -a},
           {a,  a,  -a},
           {-a, a,  -a},
           {-a, -a, a},
           {a,  -a, a},
           {a,  a,  a},
           {-a, a,  a}};
    P.F = {{{0, 1, 2, 3}},
           {{4, 5, 6, 7}},
           {{0, 1, 5, 4}},
           {{3, 2, 6, 7}},
           {{1, 2, 6, 5}},
           {{0, 3, 7, 4}}};
    return P;
}

Mesh makeTetra(float s = 1.f) {
    Mesh P;
    float a = s;
    P.V = {{-a, a,  -a},
           {a,  -a, -a},
           {a,  a,  a},
           {-a, -a, a}};
    P.F = {{{0, 1, 2}},
           {{0, 1, 3}},
           {{0, 2, 3}},
           {{1, 2, 3}}};
    return P;
}

Mesh makeOcta(float s = 1.f) {
    Mesh P;
    float a = s;
    P.V = {{0,  0,  a},
           {0,  0,  -a},
           {0,  a,  0},
           {0,  -a, 0},
           {a,  0,  0},
           {-a, 0,  0}};
    P.F = {{{0, 2, 4}},
           {{0, 4, 3}},
           {{0, 3, 5}},
           {{0, 5, 2}},
           {{1, 4, 2}},
           {{1, 3, 4}},
           {{1, 5, 3}},
           {{1, 2, 5}}};
    return P;
}

Mesh makeIcosa(float s = 1.f) {
    // Стандартные координаты: (0, ±1, ±φ), (±1, ±φ, 0), (±φ, 0, ±1)
    const float phi = (1.f + std::sqrt(5.f)) * 0.5f;
    vector<Vec3> W = {
            {0,    -1,   -phi},
            {0,    -1,   +phi},
            {0,    +1,   -phi},
            {0,    +1,   +phi},
            {-1,   -phi, 0},
            {-1,   +phi, 0},
            {+1,   -phi, 0},
            {+1,   +phi, 0},
            {-phi, 0,    -1},
            {+phi, 0,    -1},
            {-phi, 0,    +1},
            {+phi, 0,    +1}
    };
    // Нормируем к радиусу s (окруж. сфера)
    float maxr = 0.f;
    for (auto &p: W) maxr = std::max(maxr, vlen(p));
    float k = s / maxr;
    Mesh P;
    for (auto &p: W) P.V.push_back({p.x * k, p.y * k, p.z * k});
    // 20 треугольников (согласованы с этим порядком вершин)
    P.F = {
            {{0, 8,  2}},
            {{0, 2,  9}},
            {{0, 9,  6}},
            {{0, 6,  1}},
            {{0, 1,  8}},
            {{1, 6,  11}},
            {{1, 11, 10}},
            {{1, 10, 8}},
            {{2, 8,  5}},
            {{2, 5,  7}},
            {{2, 7,  9}},
            {{3, 5,  8}},
            {{3, 11, 5}},
            {{3, 10, 11}},
            {{3, 4,  10}},
            {{3, 8,  4}},
            {{4, 8,  10}},
            {{5, 11, 7}},
            {{6, 9,  11}},
            {{7, 11, 9}}
    };
    return P;
}

// Додекаэдр как дуал к икосаэдру: вершины — центры треугольных граней (нормализованные),
// грани — по 5 центров граней, инцидентных каждой вершине икосаэдра, в циклическом порядке.
Mesh makeDodeca(float s = 1.f) {
    Mesh ico = makeIcosa(1.f);

    // центры граней (будут вершинами додека)
    vector<Vec3> centers;
    centers.reserve(ico.F.size());
    for (auto &f: ico.F) {
        Vec3 c{0, 0, 0};
        for (int i: f.idx) { c = c + Vec3{ico.V[i].x, ico.V[i].y, ico.V[i].z}; }
        float inv = 1.f / f.idx.size();
        c = c * inv;
        // нормируем к единичной сфере
        c = norm(c);
        centers.push_back(c);
    }
    // масштаб к радиусу s
    for (auto &c: centers) { c = c * s; }

    // соответствие: индекс новой вершины = индекс исходной грани
    Mesh dode;
    for (auto &c: centers) dode.V.push_back({c.x, c.y, c.z});

    // для каждого ВЕРШИНЫ икосаэдра собираем все ГРАНИ, которые её содержат (их 5),
    // сортируем циклически вокруг направления этой вершины, формируем пятиугольную грань
    std::vector<std::vector<int>> incidentFaces(ico.V.size());
    for (int fi = 0; fi < (int) ico.F.size(); ++fi) {
        for (int vi: ico.F[fi].idx) {
            incidentFaces[vi].push_back(fi);
        }
    }
    for (int vi = 0; vi < (int) ico.V.size(); ++vi) {
        auto &faces = incidentFaces[vi]; // 5 штук
        if ((int) faces.size() != 5) continue;

        // базис в касательной плоскости возле вершины vi
        Vec3 Nv = norm({ico.V[vi].x, ico.V[vi].y, ico.V[vi].z}); // нормаль наружу
        Vec3 tmp = (std::fabs(Nv.z) < 0.9f) ? Vec3{0, 0, 1} : Vec3{0, 1, 0};
        Vec3 e1 = norm(cross(tmp, Nv)); // касательный
        Vec3 e2 = cross(Nv, e1);

        struct Item {
            int fidx;
            float ang;
        };
        std::vector<Item> items;
        items.reserve(5);

        // угол от e1/e2
        for (int fidx: faces) {
            Vec3 C = centers[fidx]; // уже на сфере
            Vec3 v = C - Nv * dot(C, Nv); // проекция в касательной плоскости
            float x = dot(v, e1), y = dot(v, e2);
            float ang = std::atan2(y, x);
            items.push_back({fidx, ang});
        }
        std::sort(items.begin(), items.end(), [](const Item &a, const Item &b) { return a.ang < b.ang; });

        Face pent;
        for (auto &it: items) pent.idx.push_back(it.fidx);
        dode.F.push_back(std::move(pent));
    }
    return dode;
}

// ========================= Проекции =========================
struct Projector {
    bool perspective = true;   // true — перспектива, false — аксонометрия
    float f = 600.f;           // фокусное расстояние (для перспективы)
    float ax = 35.264f;        // аксонометрия: поворот X
    float ay = 45.f;           // аксонометрия: поворот Y
    float scale = 160.f;
    float cx = 600.f, cy = 400.f; // центр экрана

    // аксонометрическая ориентация (ортографическая проекция после поворотов)
    Vec3 axo(const Vec3 &p) const {
        Vec3 r = xform(p, Mat4::Rx(ax) * Mat4::Ry(ay));
        return r;
    }

    // Проецирование мировой точки p -> экранные int координаты
    bool project(const Vec3 &pw, int &X, int &Y) const {
        if (perspective) {
            // Камера в (0,0,-f), плоскость z=0. Эквивалент: x' = f*x/(f+z).
            float denom = f + pw.z;
            if (denom <= 1e-3f) return false; // за "камерой" — отбрасываем
            float x = (pw.x * f / denom) * (scale / f) + cx; // scale ~ пикс/ед. на плоскости
            float y = (pw.y * f / denom) * (scale / f) + cy;
            X = (int) std::lround(x);
            Y = (int) std::lround(y);
            return true;
        } else {
            Vec3 q = axo(pw);
            float x = q.x * scale + cx, y = q.y * scale + cy;
            X = (int) std::lround(x);
            Y = (int) std::lround(y);
            return true;
        }
    }
};

// ========================= Приложение =========================
enum class PolyKind {
    Tetra = 0, Cube, Octa, Ico, Dode
};

struct AppState {
    PolyKind kind = PolyKind::Cube;
    Mesh base = makeCube(1.f);   // локальная геометрия
    Mat4 modelMat = Mat4::I();   // локальные -> мировые
    Projector proj;

    // произвольная линия в мировых координатах
    Vec3 P0{-200, 0, 0}, P1{200, 0, 0};

    // текущая ось для вращения вокруг центра (мировые оси)
    char axisSel = 'X';
};

// Мировое центр-объекта (из локального центра)
Vec3 worldCenter(const Mesh &base, const Mat4 &M) {
    Vec3 cL = centroid(base);
    return xform(cL, M);
}

// Применение трансформаций К МОДЕЛИ
// ВАЖНО: для мировой операции нужно ПРЕ-домножение: M := T * M (row-vector, world space first)
void worldTranslate(AppState &S, float dx, float dy, float dz) {
    S.modelMat = Mat4::T(dx, dy, dz) * S.modelMat;
}

void worldScaleAround(AppState &S, const Vec3 &Cw, float sx, float sy, float sz) {
    S.modelMat = Mat4::T(-Cw.x, -Cw.y, -Cw.z) * Mat4::S(sx, sy, sz) * Mat4::T(Cw.x, Cw.y, Cw.z) * S.modelMat;
}

void worldRotateAroundAxisThrough(AppState &S, const Vec3 &Cw, const Vec3 &axisUnit, float deg) {
    S.modelMat = Mat4::T(-Cw.x, -Cw.y, -Cw.z) * Mat4::Raxis(axisUnit, deg) * Mat4::T(Cw.x, Cw.y, Cw.z) * S.modelMat;
}

void worldReflectPlane(AppState &S, const Mat4 &R) {
    S.modelMat = R * S.modelMat;
}

void worldRotateAroundLine(AppState &S, const Vec3 &P0w, const Vec3 &P1w, float deg) {
    Vec3 axis = norm(P1w - P0w);
    S.modelMat = Mat4::T(-P0w.x, -P0w.y, -P0w.z) * Mat4::Raxis(axis, deg) * Mat4::T(P0w.x, P0w.y, P0w.z) * S.modelMat;
}

// Рисование осей мировых координат
static void drawAxes(const Projector &proj, float len = 250.f) {
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    Vec3 O{0, 0, 0};
    Vec3 X{len, 0, 0}, Y{0, len, 0}, Z{0, 0, len};
    int ox, oy, xx, xy, yx, yy, zx, zy;
    if (proj.project(O, ox, oy) && proj.project(X, xx, xy))
        dl->AddLine(ImVec2(ox, oy), ImVec2(xx, xy), IM_COL32(255, 0, 0, 255), 2.0f);
    if (proj.project(O, ox, oy) && proj.project(Y, yx, yy))
        dl->AddLine(ImVec2(ox, oy), ImVec2(yx, yy), IM_COL32(0, 200, 0, 255), 2.0f);
    if (proj.project(O, ox, oy) && proj.project(Z, zx, zy))
        dl->AddLine(ImVec2(ox, oy), ImVec2(zx, zy), IM_COL32(0, 128, 255, 255), 2.0f);

    // подписи
    ImGui::GetForegroundDrawList()->AddText(ImVec2((float) xx, (float) xy), IM_COL32(255, 0, 0, 255), "X");
    ImGui::GetForegroundDrawList()->AddText(ImVec2((float) yx, (float) yy), IM_COL32(0, 200, 0, 255), "Y");
    ImGui::GetForegroundDrawList()->AddText(ImVec2((float) zx, (float) zy), IM_COL32(0, 128, 255, 255), "Z");
}

// Wireframe через ImGui
static void drawWireImGui(const Mesh &base, const Mat4 &model, const Projector &proj,
                          ImU32 color = IM_COL32(0, 0, 0, 255), float thick = 1.6f) {
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    // Проецируем рёбра граней
    for (const auto &f: base.F) {
        for (size_t i = 0; i < f.idx.size(); ++i) {
            int i0 = f.idx[i], i1 = f.idx[(i + 1) % f.idx.size()];
            Vec3 aL{base.V[i0].x, base.V[i0].y, base.V[i0].z};
            Vec3 bL{base.V[i1].x, base.V[i1].y, base.V[i1].z};
            Vec3 aW = xform(aL, model);
            Vec3 bW = xform(bL, model);

            int x0, y0, x1, y1;
            if (proj.project(aW, x0, y0) && proj.project(bW, x1, y1))
                dl->AddLine(ImVec2((float) x0, (float) y0), ImVec2((float) x1, (float) y1), color, thick);
        }
    }
}

// Горячие клавиши
static void applyKeyOps(GLFWwindow *win, AppState &S) {
    auto pressed = [&](int key) { return glfwGetKey(win, key) == GLFW_PRESS; };

    float moveStep = 20.f, rotStep = 5.f, scaleMul = 1.05f, lineRotStep = 5.f;

    // выбор тела
    if (pressed(GLFW_KEY_1)) {
        S.kind = PolyKind::Tetra;
        S.base = makeTetra(150.f);
        S.modelMat = Mat4::I();
    }
    if (pressed(GLFW_KEY_2)) {
        S.kind = PolyKind::Cube;
        S.base = makeCube(150.f);
        S.modelMat = Mat4::I();
    }
    if (pressed(GLFW_KEY_3)) {
        S.kind = PolyKind::Octa;
        S.base = makeOcta(150.f);
        S.modelMat = Mat4::I();
    }
    if (pressed(GLFW_KEY_4)) {
        S.kind = PolyKind::Ico;
        S.base = makeIcosa(150.f);
        S.modelMat = Mat4::I();
    }
    if (pressed(GLFW_KEY_5)) {
        S.kind = PolyKind::Dode;
        S.base = makeDodeca(150.f);
        S.modelMat = Mat4::I();
    }

    // проекции
    if (pressed(GLFW_KEY_P)) S.proj.perspective = true;
    if (pressed(GLFW_KEY_O)) S.proj.perspective = false;

    // перенос (мировой)
    if (pressed(GLFW_KEY_LEFT)) worldTranslate(S, -moveStep, 0, 0);
    if (pressed(GLFW_KEY_RIGHT)) worldTranslate(S, +moveStep, 0, 0);
    if (pressed(GLFW_KEY_UP)) worldTranslate(S, 0, -moveStep, 0);
    if (pressed(GLFW_KEY_DOWN)) worldTranslate(S, 0, +moveStep, 0);
    if (pressed(GLFW_KEY_PAGE_UP)) worldTranslate(S, 0, 0, +moveStep);
    if (pressed(GLFW_KEY_PAGE_DOWN)) worldTranslate(S, 0, 0, -moveStep);

    // масштаб (отн. МИРОВОГО центра объекта)
    Vec3 Cw = worldCenter(S.base, S.modelMat);
    if (pressed(GLFW_KEY_LEFT_BRACKET)) worldScaleAround(S, Cw, 1.f / scaleMul, 1.f / scaleMul, 1.f / scaleMul);
    if (pressed(GLFW_KEY_RIGHT_BRACKET)) worldScaleAround(S, Cw, scaleMul, scaleMul, scaleMul);

    // вращение вокруг прямой через мировой центр, параллельной оси
    if (pressed(GLFW_KEY_X)) worldRotateAroundAxisThrough(S, Cw, {1, 0, 0}, rotStep);
    if (pressed(GLFW_KEY_Y)) worldRotateAroundAxisThrough(S, Cw, {0, 1, 0}, rotStep);
    if (pressed(GLFW_KEY_Z)) worldRotateAroundAxisThrough(S, Cw, {0, 0, 1}, rotStep);

    // отражения относительно мировых координатных плоскостей
    if (pressed(GLFW_KEY_F1)) worldReflectPlane(S, Mat4::RefXY());
    if (pressed(GLFW_KEY_F2)) worldReflectPlane(S, Mat4::RefYZ());
    if (pressed(GLFW_KEY_F3)) worldReflectPlane(S, Mat4::RefXZ());

    // выбор оси (цикл X->Y->Z)
    static bool tabLatch = false;
    if (pressed(GLFW_KEY_TAB)) {
        if (!tabLatch) {
            S.axisSel = (S.axisSel == 'X' ? 'Y' : S.axisSel == 'Y' ? 'Z' : 'X');
            std::cout << "Axis: " << S.axisSel << "\n";
            tabLatch = true;
        }
    } else tabLatch = false;

    if (pressed(GLFW_KEY_SEMICOLON)) { // ; — по часовой
        Vec3 ax = (S.axisSel == 'X') ? Vec3{1, 0, 0} : (S.axisSel == 'Y') ? Vec3{0, 1, 0} : Vec3{0, 0, 1};
        worldRotateAroundAxisThrough(S, Cw, ax, +rotStep);
    }
    if (pressed(GLFW_KEY_APOSTROPHE)) { // ' — против
        Vec3 ax = (S.axisSel == 'X') ? Vec3{1, 0, 0} : (S.axisSel == 'Y') ? Vec3{0, 1, 0} : Vec3{0, 0, 1};
        worldRotateAroundAxisThrough(S, Cw, ax, -rotStep);
    }

    // произвольная прямая P0, P1 (мировые)
    if (pressed(GLFW_KEY_A)) S.P0.x -= moveStep;
    if (pressed(GLFW_KEY_D)) S.P0.x += moveStep;
    if (pressed(GLFW_KEY_W)) S.P0.y -= moveStep;
    if (pressed(GLFW_KEY_S)) S.P0.y += moveStep;
    if (pressed(GLFW_KEY_Q)) S.P0.z -= moveStep;
    if (pressed(GLFW_KEY_E)) S.P0.z += moveStep;

    if (pressed(GLFW_KEY_J)) S.P1.x -= moveStep;
    if (pressed(GLFW_KEY_L)) S.P1.x += moveStep;
    if (pressed(GLFW_KEY_I)) S.P1.y -= moveStep;
    if (pressed(GLFW_KEY_K)) S.P1.y += moveStep;
    if (pressed(GLFW_KEY_U)) S.P1.z -= moveStep;
    if (pressed(GLFW_KEY_O)) S.P1.z += moveStep;

    if (pressed(GLFW_KEY_MINUS)) worldRotateAroundLine(S, S.P0, S.P1, +lineRotStep);
    if (pressed(GLFW_KEY_EQUAL)) worldRotateAroundLine(S, S.P0, S.P1, -lineRotStep);

    // зум
    if (pressed(GLFW_KEY_COMMA)) S.proj.scale *= 0.98f;
    if (pressed(GLFW_KEY_PERIOD)) S.proj.scale *= 1.02f;

    // сброс
    if (pressed(GLFW_KEY_C)) {
        switch (S.kind) {
            case PolyKind::Tetra:
                S.base = makeTetra(150.f);
                break;
            case PolyKind::Cube:
                S.base = makeCube(150.f);
                break;
            case PolyKind::Octa:
                S.base = makeOcta(150.f);
                break;
            case PolyKind::Ico:
                S.base = makeIcosa(150.f);
                break;
            case PolyKind::Dode:
                S.base = makeDodeca(150.f);
                break;
        }
        S.modelMat = Mat4::I();
        S.proj.perspective = true;
        S.proj.ax = 35.264f;
        S.proj.ay = 45.f;
        S.proj.scale = 160.f;
    }
}

int run() {
    const int W = 1200, H = 800;

    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return 1;
    }
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow *window = glfwCreateWindow(W, H, "GLFW+ImGui Wireframe (perspective fixed, dual dodeca)", nullptr,
                                          nullptr);
    if (!window) {
        std::cerr << "GLFW window failed\n";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
#ifdef __APPLE__
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init(nullptr);
#endif

    AppState S;
    S.proj.cx = W * 0.5f;
    S.proj.cy = H * 0.5f;
    // начальная фигура крупнее (чтоб не терялась)
    S.base = makeCube(150.f);

    int polyIdx = 1;
    bool persp = true;
    bool showAxes = true;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        applyKeyOps(window, S);

        // ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Панель
        ImGui::Begin("Controls");
        const char *items[] = {"Tetrahedron", "Cube", "Octahedron", "Icosahedron", "Dodecahedron"};
        if (ImGui::Combo("Polyhedron", &polyIdx, items, IM_ARRAYSIZE(items))) {
            S.modelMat = Mat4::I();
            switch (polyIdx) {
                case 0:
                    S.kind = PolyKind::Tetra;
                    S.base = makeTetra(150.f);
                    break;
                case 1:
                    S.kind = PolyKind::Cube;
                    S.base = makeCube(150.f);
                    break;
                case 2:
                    S.kind = PolyKind::Octa;
                    S.base = makeOcta(150.f);
                    break;
                case 3:
                    S.kind = PolyKind::Ico;
                    S.base = makeIcosa(150.f);
                    break;
                case 4:
                    S.kind = PolyKind::Dode;
                    S.base = makeDodeca(150.f);
                    break;
            }
        }
        if (ImGui::Checkbox("Perspective", &persp)) S.proj.perspective = persp;
        ImGui::Checkbox("Show axes", &showAxes);
        ImGui::SliderFloat("Scale", &S.proj.scale, 20.f, 400.f);
        if (!S.proj.perspective) {
            ImGui::SliderFloat("Axon X", &S.proj.ax, 0.f, 90.f);
            ImGui::SliderFloat("Axon Y", &S.proj.ay, 0.f, 90.f);
        } else {
            ImGui::SliderFloat("Focus f", &S.proj.f, 100.f, 2000.f);
        }
        ImGui::SeparatorText("World ops around center");
        if (ImGui::Button("Rotate X +5")) {
            Vec3 Cw = worldCenter(S.base, S.modelMat);
            worldRotateAroundAxisThrough(S, Cw, {1, 0, 0}, +5.f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Rotate Y +5")) {
            Vec3 Cw = worldCenter(S.base, S.modelMat);
            worldRotateAroundAxisThrough(S, Cw, {0, 1, 0}, +5.f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Rotate Z +5")) {
            Vec3 Cw = worldCenter(S.base, S.modelMat);
            worldRotateAroundAxisThrough(S, Cw, {0, 0, 1}, +5.f);
        }

        ImGui::SeparatorText("Reflect world planes");
        if (ImGui::Button("XY")) worldReflectPlane(S, Mat4::RefXY());
        ImGui::SameLine();
        if (ImGui::Button("YZ")) worldReflectPlane(S, Mat4::RefYZ());
        ImGui::SameLine();
        if (ImGui::Button("XZ")) worldReflectPlane(S, Mat4::RefXZ());

        ImGui::SeparatorText("Arbitrary line (world)");
        ImGui::Text("P0(%.1f,%.1f,%.1f)  P1(%.1f,%.1f,%.1f)", S.P0.x, S.P0.y, S.P0.z, S.P1.x, S.P1.y, S.P1.z);
        if (ImGui::Button("Rotate P0-P1 +5")) worldRotateAroundLine(S, S.P0, S.P1, +5.f);
        ImGui::SameLine();
        if (ImGui::Button("Rotate P0-P1 -5")) worldRotateAroundLine(S, S.P0, S.P1, -5.f);

        if (ImGui::Button("Reset [C]")) {
            switch (S.kind) {
                case PolyKind::Tetra:
                    S.base = makeTetra(150.f);
                    break;
                case PolyKind::Cube:
                    S.base = makeCube(150.f);
                    break;
                case PolyKind::Octa:
                    S.base = makeOcta(150.f);
                    break;
                case PolyKind::Ico:
                    S.base = makeIcosa(150.f);
                    break;
                case PolyKind::Dode:
                    S.base = makeDodeca(150.f);
                    break;
            }
            S.modelMat = Mat4::I();
            S.proj.perspective = true;
            S.proj.ax = 35.264f;
            S.proj.ay = 45.f;
            S.proj.scale = 160.f;
            persp = true;
        }
        ImGui::End();

        // Ресайз
        int fbw, fbh;
        ImGuiViewport* vp = ImGui::GetMainViewport();
        S.proj.cx = vp->Size.x * 0.5f;
        S.proj.cy = vp->Size.y * 0.5f;

        // Оси
        if (showAxes) drawAxes(S.proj, 250.f);

        // Wireframe объекта
        drawWireImGui(S.base, S.modelMat, S.proj, IM_COL32(20, 20, 20, 255), 1.8f);

        // Линия P0-P1
        int x0, y0, x1, y1;
        if (S.proj.project(S.P0, x0, y0) && S.proj.project(S.P1, x1, y1)) {
            ImGui::GetBackgroundDrawList()->AddLine(ImVec2((float) x0, (float) y0),
                                                    ImVec2((float) x1, (float) y1),
                                                    IM_COL32(200, 0, 0, 255), 2.0f);
        }

        // Render
        ImGui::Render();
        glViewport(0, 0, fbw, fbh);
        glClearColor(0.96f, 0.96f, 0.96f, 1.0f);
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
