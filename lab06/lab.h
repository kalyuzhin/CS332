#ifndef CS332_LAB_H
#define CS332_LAB_H
#define GL_SILENCE_DEPRECATION

#include <cmath>
#include <algorithm>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cstdlib>

#include "../provider.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__

#include <OpenGL/gl3.h>

#endif

using std::vector;

static constexpr float PI = 3.14159265358979323846f;

inline float deg2rad(float a) { return a * PI / 180.f; }

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
    return L ? Vec3{v.x / L, v.y / L, v.z / L} : Vec3{0, 0, 0};
}

struct Mat4 {
    float m[4][4]{};

    static Mat4 I() {
        Mat4 M{};
        for (int i = 0; i < 4; ++i)M.m[i][i] = 1.f;
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

    static Mat4 Raxis(const Vec3 &u, float deg) {
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
            for (int k = 0; k < 4; ++k)s += A.m[r][k] * B.m[k][c];
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
    return std::abs(r.w) < 1e-6f ? Vec3{r.x, r.y, r.z} : Vec3{r.x / r.w, r.y / r.w, r.z / r.w};
}

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

inline Vec3 centroid(const Mesh &m) {
    Vec3 c{0, 0, 0};
    if (m.V.empty())return c;
    for (auto &p: m.V) {
        c.x += p.x;
        c.y += p.y;
        c.z += p.z;
    }
    float inv = 1.f / (float) m.V.size();
    return {c.x * inv, c.y * inv, c.z * inv};
}

inline Mesh makeCube(float s = 1.f) {
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

inline Mesh makeTetra(float s = 1.f) {
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

inline Mesh makeOcta(float s = 1.f) {
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

inline Mesh makeIcosa(float s = 1.f) {
    const float phi = (1.f + std::sqrt(5.f)) * 0.5f;
    vector<Vec3> verts = {
            {-1,   phi,  0},
            {1,    phi,  0},
            {-1,   -phi, 0},
            {1,    -phi, 0},
            {0,    -1,   phi},
            {0,    1,    phi},
            {0,    -1,   -phi},
            {0,    1,    -phi},
            {phi,  0,    -1},
            {phi,  0,    1},
            {-phi, 0,    -1},
            {-phi, 0,    1}
    };
    float maxr = 0.f;
    for (auto &v: verts) maxr = std::max(maxr, vlen(v));
    float k = s / maxr;

    Mesh M;
    for (auto &v: verts) M.V.push_back({v.x * k, v.y * k, v.z * k});

    M.F = {
            {{0,  11, 5}},
            {{0,  5,  1}},
            {{0,  1,  7}},
            {{0,  7,  10}},
            {{0,  10, 11}},
            {{1,  5,  9}},
            {{5,  11, 4}},
            {{11, 10, 2}},
            {{10, 7,  6}},
            {{7,  1,  8}},
            {{3,  9,  4}},
            {{3,  4,  2}},
            {{3,  2,  6}},
            {{3,  6,  8}},
            {{3,  8,  9}},
            {{4,  9,  5}},
            {{2,  4,  11}},
            {{6,  2,  10}},
            {{8,  6,  7}},
            {{9,  8,  1}}
    };
    return M;
}

inline Mesh makeDodeca(float s = 1.f) {
    Mesh ico = makeIcosa(1.f);
    vector<Vec3> centers;
    centers.reserve(ico.F.size());
    for (auto &f: ico.F) {
        Vec3 c{0, 0, 0};
        for (int i: f.idx)c = c + Vec3{ico.V[i].x, ico.V[i].y, ico.V[i].z};
        c = c * (1.f / (float) f.idx.size());
        centers.push_back(norm(c));
    }
    for (auto &c: centers)c = c * s;
    Mesh dode;
    for (auto &c: centers)dode.V.push_back({c.x, c.y, c.z});
    std::vector<std::vector<int>> inc(ico.V.size());
    for (int fi = 0; fi < (int) ico.F.size(); ++fi)for (int vi: ico.F[fi].idx)inc[vi].push_back(fi);
    for (int vi = 0; vi < (int) ico.V.size(); ++vi) {
        auto &fs = inc[vi];
        if ((int) fs.size() != 5)continue;
        Vec3 Nv = norm({ico.V[vi].x, ico.V[vi].y, ico.V[vi].z});
        Vec3 tmp = std::fabs(Nv.z) < 0.9f ? Vec3{0, 0, 1} : Vec3{0, 1, 0};
        Vec3 e1 = norm(cross(tmp, Nv)), e2 = cross(Nv, e1);
        struct It {
            int f;
            float ang;
        };
        vector<It> items;
        items.reserve(5);
        for (int fidx: fs) {
            Vec3 C = centers[fidx];
            Vec3 v = C - Nv * dot(C, Nv);
            float x = dot(v, e1), y = dot(v, e2);
            items.push_back({fidx, std::atan2(y, x)});
        }
        std::sort(items.begin(), items.end(), [](const It &a, const It &b) { return a.ang < b.ang; });
        Face pent;
        for (auto &t: items)pent.idx.push_back(t.f);
        dode.F.push_back(std::move(pent));
    }
    return dode;
}

inline Mesh transformMesh(const Mesh& original, const Mat4& transform) {
    Mesh newMesh;

    newMesh.V.reserve(original.V.size());
    for (const auto& vertex : original.V) {
        Vec3 transformed = xform({ vertex.x, vertex.y, vertex.z }, transform);
        newMesh.V.push_back({ transformed.x, transformed.y, transformed.z });
    }

    newMesh.F = original.F;

    return newMesh;
}

struct Projector {
    bool perspective = true;
    float f = 600.f;
    float ax = 35.264f;
    float ay = 45.f;
    float scale = 160.f;
    float cx = 600.f, cy = 400.f;

    Vec3 axo(const Vec3 &p) const { return xform(p, Mat4::Rx(ax) * Mat4::Ry(ay)); }

    bool project(const Vec3 &pw, int &X, int &Y) const {
        if (perspective) {
            float denom = f + pw.z;
            if (denom <= 1e-3f) return false;
            float x = (pw.x * f / denom) * (scale / f) + cx;
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

enum class PolyKind {
    Tetra = 0, Cube, Octa, Ico, Dode, UserObj
};

struct MeshData {
    const char* name;
    PolyKind kind;
    Mesh mesh;
};

struct AppState {
    vector<string> meshesNames = { "Tetrahedron", "Cube", "Octahedron", "Icosahedron", "Dodecahedron" };
    vector<pair<PolyKind, Mesh>> meshes = {
        {PolyKind::Tetra, makeTetra(150.f)},
        {PolyKind::Cube, makeCube(150.f)},
        {PolyKind::Octa, makeOcta(150.f)},
        {PolyKind::Ico, makeIcosa(150.f)},
        {PolyKind::Dode, makeDodeca(150.f)}
    };
    int polyIdx = 1;
    PolyKind kind = PolyKind::Cube;
    Mesh base = makeCube(150.f);
    Mat4 modelMat = Mat4::I();
    Projector proj;
    Vec3 P0{-200, 0, 0};
    Vec3 P1{200, 0, 0};
    char axisSel = 'X';

    // new fields for back-face culling & view vector
    bool backfaceCull = true;
    bool showFaceNormals = false;
    bool useCustomView = false;
    Vec3 viewVec{0.f, 0.f, 1.f};

    vector<const char*> getMeshNamesForImGui() {
        vector<const char*> result;
        for (const auto& name : meshesNames) {
            result.push_back(name.c_str());
        }
        return result;
    }
};

inline Vec3 worldCenter(const Mesh &base, const Mat4 &M) { return xform(centroid(base), M); }

inline void worldTranslate(AppState &S, float dx, float dy, float dz) {
    S.modelMat = S.modelMat * Mat4::T(dx, dy, dz);
}

inline void worldScaleAround(AppState &S, const Vec3 &Cw, float sx, float sy, float sz) {
    S.modelMat = S.modelMat * Mat4::T(-Cw.x, -Cw.y, -Cw.z) * Mat4::S(sx, sy, sz) * Mat4::T(Cw.x, Cw.y, Cw.z);
}

inline void worldRotateAroundAxisThrough(AppState &S, const Vec3 &Cw, const Vec3 &u, float deg) {
    S.modelMat = S.modelMat * Mat4::T(-Cw.x, -Cw.y, -Cw.z) * Mat4::Raxis(u, deg) * Mat4::T(Cw.x, Cw.y, Cw.z);
}

inline void worldReflectPlane(AppState &S, const Mat4 &R) { S.modelMat = S.modelMat * R; }

inline void worldRotateAroundLine(AppState &S, const Vec3 &P0w, const Vec3 &P1w, float deg) {
    Vec3 u = norm(P1w - P0w);
    S.modelMat = S.modelMat * Mat4::T(-P0w.x, -P0w.y, -P0w.z) * Mat4::Raxis(u, deg) * Mat4::T(P0w.x, P0w.y, P0w.z);
}

static void drawAxes(const Projector &proj, float len = 250.f) {
    ImDrawList *dl = ImGui::GetBackgroundDrawList();
    Vec3 O{0, 0, 0}, X{len, 0, 0}, Y{0, len, 0}, Z{0, 0, len};
    int ox, oy, xx, xy, yx, yy, zx, zy;
    if (proj.project(O, ox, oy) && proj.project(X, xx, xy))
        dl->AddLine(ImVec2(ox, oy), ImVec2(xx, xy), IM_COL32(255, 0, 0, 255), 2.f);
    if (proj.project(O, ox, oy) && proj.project(Y, yx, yy))
        dl->AddLine(ImVec2(ox, oy), ImVec2(yx, yy), IM_COL32(0, 200, 0, 255), 2.f);
    if (proj.project(O, ox, oy) && proj.project(Z, zx, zy))
        dl->AddLine(ImVec2(ox, oy), ImVec2(zx, zy), IM_COL32(0, 128, 255, 255), 2.f);
    ImGui::GetForegroundDrawList()->AddText(ImVec2((float) xx, (float) xy), IM_COL32(255, 0, 0, 255), "X");
    ImGui::GetForegroundDrawList()->AddText(ImVec2((float) yx, (float) yy), IM_COL32(0, 200, 0, 255), "Y");
    ImGui::GetForegroundDrawList()->AddText(ImVec2((float) zx, (float) zy), IM_COL32(0, 128, 255, 255), "Z");
}

// Modified: add AppState param so we can compute view vector & culling behavior
static void
drawWireImGui(const Mesh &base, const Mat4 &model, const AppState &S, ImU32 color = IM_COL32(20, 20, 20, 255),
              float thick = 1.8f) {
    ImDrawList *dl = ImGui::GetBackgroundDrawList();

    // compute view direction in world coordinates
    Vec3 viewDirWorld{0,0,1};
    if (S.proj.perspective) {
        // camera positioned at (0,0,-f) in world coordinates according to projection formula
        Vec3 cam{0.f, 0.f, -S.proj.f};
        // For perspective we will compute view direction per-face (cam - faceCentroid)
        // set a sentinel: if using custom view vector, obey it instead
    } else {
        // for parallel (axonometric) projection the viewing direction is the rotated (0,0,1)
        Vec4 v{0.f, 0.f, 1.f, 0.f};
        Mat4 R = Mat4::Rx(S.proj.ax) * Mat4::Ry(S.proj.ay);
        Vec4 r = v * R;
        viewDirWorld = norm(Vec3{r.x, r.y, r.z});
    }

    Vec3 meshC_object = centroid(base);          // centroid(base) возвращает в object space
    Vec3 meshC = xform(meshC_object, model);
    const float EPS = 1e-6f;

    for (const auto &f: base.F) {
        if (f.idx.size() < 3) continue;

        // transformed vertices (world coordinates)
        Vec3 a = xform({base.V[f.idx[0]].x, base.V[f.idx[0]].y, base.V[f.idx[0]].z}, model);
        Vec3 b = xform({base.V[f.idx[1]].x, base.V[f.idx[1]].y, base.V[f.idx[1]].z}, model);
        Vec3 c = xform({base.V[f.idx[2]].x, base.V[f.idx[2]].y, base.V[f.idx[2]].z}, model);

        // face normal (using first three vertices)
        Vec3 n = norm(cross(b - a, c - a));

        // face centroid (world)
        Vec3 fc{0,0,0};
        for (int vidx : f.idx) {
            Vec3 v = xform({base.V[vidx].x, base.V[vidx].y, base.V[vidx].z}, model);
            fc.x += v.x; fc.y += v.y; fc.z += v.z;
        }
        fc = fc * (1.f / (float) f.idx.size());

        Vec3 faceV = fc - meshC;

        if (dot(n, faceV) < 0.f) {
            n = n * -1.f;
        }

        // determine view direction to use for culling
        Vec3 viewDir;
        if (S.useCustomView) {
            viewDir = norm(S.viewVec);
        } else if (S.proj.perspective) {
            Vec3 cam{0.f, 0.f, -S.proj.f};
            viewDir = norm(cam - fc);
        } else {
            viewDir = viewDirWorld;
        }

        // front-facing test: if dot(normal, viewDir) > 0 => face faces camera
        bool frontFacing = dot(n, viewDir) > EPS;

        if (!S.backfaceCull || frontFacing) {
            // draw polygon edges
            for (size_t i = 0; i < f.idx.size(); ++i) {
                int i0 = f.idx[i], i1 = f.idx[(i + 1) % f.idx.size()];
                Vec3 va = xform({base.V[i0].x, base.V[i0].y, base.V[i0].z}, model);
                Vec3 vb = xform({base.V[i1].x, base.V[i1].y, base.V[i1].z}, model);
                int x0, y0, x1, y1;
                if (S.proj.project(va, x0, y0) && S.proj.project(vb, x1, y1))
                    dl->AddLine(ImVec2((float) x0, (float) y0), ImVec2((float) x1, (float) y1), color, thick);
            }
        }

        // optionally draw face normal as a short line from centroid
        if (S.showFaceNormals) {
            Vec3 nstart = fc;
            Vec3 nend = fc + n * 30.f; // scale for visibility
            int xs, ys, xe, ye;
            if (S.proj.project(nstart, xs, ys) && S.proj.project(nend, xe, ye)) {
                dl->AddLine(ImVec2((float) xs, (float) ys), ImVec2((float) xe, (float) ye), IM_COL32(200, 30, 30, 255), 1.2f);
            }
        }
    }
}

static bool openObject(const string& filename, AppState& appState, Mesh& mesh) {
    mesh.V.clear();
    mesh.F.clear();

    ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: couldn`t open file  " << filename << " for reading" << std::endl;
        return false;
    }

    std::string line;
    int lineNumber = 0;
    int vertexCount = 0;
    int faceCount = 0;

    while (std::getline(file, line)) {
        lineNumber++;

        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::istringstream iss(line);
        std::string prefix;
        iss >> prefix;

        if (prefix == "v") {
            Vertex vertex;
            if (iss >> vertex.x >> vertex.y >> vertex.z) {
                mesh.V.push_back(vertex);
                vertexCount++;
            }
            else {
                std::cerr << "Warning: wrong vertex format " << lineNumber << std::endl;
            }
        }
        else if (prefix == "f") {
            Face face;
            std::string token;

            while (iss >> token) {
                std::istringstream tokenStream(token);
                std::string indexStr;

                if (std::getline(tokenStream, indexStr, '/')) {
                    try {
                        int index = std::stoi(indexStr) - 1;
                        if (index >= 0 && index < static_cast<int>(mesh.V.size())) {
                            face.idx.push_back(index);
                        }
                        else {
                            std::cerr << "Warning: wrong index vertexes " << (index + 1)
                                << " in line " << lineNumber << std::endl;
                        }
                    }
                    catch (const std::exception& e) {
                        std::cerr << "Error: wrong index format " << lineNumber << std::endl;
                    }
                }
            }

            if (face.idx.size() >= 3) {
                mesh.F.push_back(face);
                faceCount++;
            }
            else if (!face.idx.empty()) {
                std::cerr << "Warning: polygon have less then 3 vertexes " << lineNumber << std::endl;
            }
        }

        // Ignore other types. It will be realized later
        else if (prefix == "vt" || prefix == "vn" || prefix == "vp") {
            continue;
        }
    }
    file.close();
    filesystem::path filepath(filename);
    string objName = filepath.filename().string();
    appState.meshesNames.push_back(objName);
    appState.meshes.push_back({PolyKind::UserObj, mesh});
    return true;
}


static bool saveObject(const string& filename, AppState& appState, const Mesh& base) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: couldn`t open file  " << filename << " for writing" << std::endl;
        return false;
    }

    for (const auto& vertex : base.V) {
        file << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
    }

    file << "# Vertexes: " << base.V.size() << ", Faces: " << base.F.size() << "\n\n";
    for (const auto& vertex : base.V) {
        file << "v " << vertex.x << " " << vertex.y << " " << vertex.z << "\n";
    }
    file << "\n";
    for (const auto& face : base.F) {
        file << "f";
        for (int index : face.idx) {
            file << " " << (index + 1);
        }
        file << "\n";
    }

    file.close();
    cout << "File saved!" << endl;
    return true;
}

static void applyKeyOps(GLFWwindow *w, AppState &S) {
    auto down = [&](int k) { return glfwGetKey(w, k) == GLFW_PRESS; };
    float mv = 20.f, rot = 5.f, sm = 1.05f, lr = 5.f;
    if (ImGui::GetIO().WantCaptureKeyboard)
    {
        return;
    }
    if (down(GLFW_KEY_1)) {
        S.kind = PolyKind::Tetra;
        S.base = makeTetra(150.f);
        S.modelMat = Mat4::I();
    }
    if (down(GLFW_KEY_2)) {
        S.kind = PolyKind::Cube;
        S.base = makeCube(150.f);
        S.modelMat = Mat4::I();
    }
    if (down(GLFW_KEY_3)) {
        S.kind = PolyKind::Octa;
        S.base = makeOcta(150.f);
        S.modelMat = Mat4::I();
    }
    if (down(GLFW_KEY_4)) {
        S.kind = PolyKind::Ico;
        S.base = makeIcosa(150.f);
        S.modelMat = Mat4::I();
    }
    if (down(GLFW_KEY_5)) {
        S.kind = PolyKind::Dode;
        S.base = makeDodeca(150.f);
        S.modelMat = Mat4::I();
    }
    if (down(GLFW_KEY_P)) S.proj.perspective = true;
    if (down(GLFW_KEY_O)) S.proj.perspective = false;
    if (down(GLFW_KEY_LEFT)) worldTranslate(S, -mv, 0, 0);
    if (down(GLFW_KEY_RIGHT)) worldTranslate(S, +mv, 0, 0);
    if (down(GLFW_KEY_UP)) worldTranslate(S, 0, -mv, 0);
    if (down(GLFW_KEY_DOWN)) worldTranslate(S, 0, +mv, 0);
    if (down(GLFW_KEY_PAGE_UP)) worldTranslate(S, 0, 0, +mv);
    if (down(GLFW_KEY_PAGE_DOWN)) worldTranslate(S, 0, 0, -mv);
    Vec3 Cw = worldCenter(S.base, S.modelMat);
    if (down(GLFW_KEY_LEFT_BRACKET)) worldScaleAround(S, Cw, 1.f / sm, 1.f / sm, 1.f / sm);
    if (down(GLFW_KEY_RIGHT_BRACKET)) worldScaleAround(S, Cw, sm, sm, sm);
    if (down(GLFW_KEY_X)) worldRotateAroundAxisThrough(S, Cw, {1, 0, 0}, rot);
    if (down(GLFW_KEY_Y)) worldRotateAroundAxisThrough(S, Cw, {0, 1, 0}, rot);
    if (down(GLFW_KEY_Z)) worldRotateAroundAxisThrough(S, Cw, {0, 0, 1}, rot);
    if (down(GLFW_KEY_F1)) worldReflectPlane(S, Mat4::RefXY());
    if (down(GLFW_KEY_F2)) worldReflectPlane(S, Mat4::RefYZ());
    if (down(GLFW_KEY_F3)) worldReflectPlane(S, Mat4::RefXZ());
    static bool tl = false;
    if (down(GLFW_KEY_TAB)) {
        if (!tl) {
            S.axisSel = (S.axisSel == 'X' ? 'Y' : S.axisSel == 'Y' ? 'Z' : 'X');
            tl = true;
        }
    } else tl = false;
    if (down(GLFW_KEY_SEMICOLON)) {
        Vec3 a = (S.axisSel == 'X') ? Vec3{1, 0, 0} : (S.axisSel == 'Y') ? Vec3{0, 1, 0} : Vec3{0, 0, 1};
        worldRotateAroundAxisThrough(S, Cw, a, +rot);
    }
    if (down(GLFW_KEY_APOSTROPHE)) {
        Vec3 a = (S.axisSel == 'X') ? Vec3{1, 0, 0} : (S.axisSel == 'Y') ? Vec3{0, 1, 0} : Vec3{0, 0, 1};
        worldRotateAroundAxisThrough(S, Cw, a, -rot);
    }
    if (down(GLFW_KEY_MINUS)) worldRotateAroundLine(S, S.P0, S.P1, +lr);
    if (down(GLFW_KEY_EQUAL)) worldRotateAroundLine(S, S.P0, S.P1, -lr);
    if (down(GLFW_KEY_COMMA)) S.proj.scale *= 0.98f;
    if (down(GLFW_KEY_PERIOD)) S.proj.scale *= 1.02f;
    if (down(GLFW_KEY_C)) {
        /*switch (S.kind) {
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
        }*/
        S.modelMat = Mat4::I();
        //S.proj.perspective = true;
        S.proj.ax = 35.264f;
        S.proj.ay = 45.f;
        //S.proj.scale = 160.f;
    }
}

int run() {
    const int W = 1200, H = 800;
    if (!glfwInit())return 1;
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow *win = glfwCreateWindow(W, H, "GLFW+ImGui Wireframe", nullptr, nullptr);
    if (!win) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;
    ImGui::StyleColorsLight();
    ImGui_ImplGlfw_InitForOpenGL(win, true);
#ifdef __APPLE__
    ImGui_ImplOpenGL3_Init("#version 150");
#else
    ImGui_ImplOpenGL3_Init(nullptr);
#endif
    AppState S;
    int polyIdx = 1;
    bool persp = true, showAxes = true;

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

    while (!glfwWindowShouldClose(win)) {
        glfwPollEvents();
        applyKeyOps(win, S);
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Controls");
        //const char *items[] = {"Tetrahedron", "Cube", "Octahedron", "Icosahedron", "Dodecahedron"};
        auto meshNames = S.getMeshNamesForImGui();
        if (ImGui::Combo("Polyhedron", &S.polyIdx, meshNames.data(), meshNames.size())) {
            S.modelMat = Mat4::I();
            pair<PolyKind, Mesh> meshPair = S.meshes[S.polyIdx];
            S.kind = meshPair.first;
            S.base = meshPair.second;
        }
        ImGui::Checkbox("Perspective", &S.proj.perspective);
        ImGui::Checkbox("Show axes", &showAxes);
        ImGui::SliderFloat("Scale", &S.proj.scale, 20.f, 400.f);
        if (!S.proj.perspective) {
            ImGui::SliderFloat("Axon X", &S.proj.ax, 0.f, 90.f);
            ImGui::SliderFloat("Axon Y", &S.proj.ay, 0.f, 90.f);
        } else { ImGui::SliderFloat("Focus f", &S.proj.f, 100.f, 2000.f); }

        // New UI: back-face culling and view vector
        ImGui::SeparatorText("Back-face culling");
        ImGui::Checkbox("Back-face culling", &S.backfaceCull);
        ImGui::Checkbox("Show face normals", &S.showFaceNormals);
        ImGui::Checkbox("Use custom view vector", &S.useCustomView);
        if (S.useCustomView) {
            float vv[3] = {S.viewVec.x, S.viewVec.y, S.viewVec.z};
            if (ImGui::InputFloat3("View vector", vv)) {
                S.viewVec.x = vv[0]; S.viewVec.y = vv[1]; S.viewVec.z = vv[2];
            }
        }

        ImGui::SeparatorText("Rotate around center");
        if (ImGui::Button("X +5")) {
            Vec3 Cw = worldCenter(S.base, S.modelMat);
            worldRotateAroundAxisThrough(S, Cw, {1, 0, 0}, +5.f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Y +5")) {
            Vec3 Cw = worldCenter(S.base, S.modelMat);
            worldRotateAroundAxisThrough(S, Cw, {0, 1, 0}, +5.f);
        }
        ImGui::SameLine();
        if (ImGui::Button("Z +5")) {
            Vec3 Cw = worldCenter(S.base, S.modelMat);
            worldRotateAroundAxisThrough(S, Cw, {0, 0, 1}, +5.f);
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
            if (S.kind == PolyKind::Tetra)S.base = makeTetra(150.f);
            if (S.kind == PolyKind::Cube) S.base = makeCube(150.f);
            if (S.kind == PolyKind::Octa) S.base = makeOcta(150.f);
            if (S.kind == PolyKind::Ico) S.base = makeIcosa(150.f);
            if (S.kind == PolyKind::Dode) S.base = makeDodeca(150.f);
            S.modelMat = Mat4::I();
            S.proj.perspective = true;
            S.proj.ax = 35.264f;
            S.proj.ay = 45.f;
            S.proj.scale = 160.f;
            persp = true;
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
        ImGui::End();

        saveFileDialog.Display();
        if (saveFileDialog.HasSelected())
        {
            filesystem::path dir_path = saveFileDialog.GetDirectory();
            filesystem::path fullpath = dir_path / filesystem::path(newFilename);
            cout << fullpath.string() << endl;
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
        // changed: pass whole AppState so draw routine can cull using S settings
        drawWireImGui(S.base, S.modelMat, S, IM_COL32(20, 20, 20, 255), 1.8f);
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

#endif
