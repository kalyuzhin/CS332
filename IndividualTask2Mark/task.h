#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <future>
#include <mutex>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#if defined(__APPLE__)
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl3.h>
#else
#include <GL/gl.h>
#endif

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace cornell {

    static constexpr double PI  = 3.14159265358979323846;
    static constexpr double EPS = 1e-6;

    static inline double deg2rad(double deg) { return deg * PI / 180.0; }
    static inline double clampd(double v, double lo, double hi) { return std::max(lo, std::min(hi, v)); }

    struct Vec3 {
        double x = 0, y = 0, z = 0;
        Vec3() = default;
        Vec3(double xx, double yy, double zz) : x(xx), y(yy), z(zz) {}

        Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
        Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
        Vec3 operator*(double s) const { return {x * s, y * s, z * s}; }
        Vec3 operator/(double s) const { return {x / s, y / s, z / s}; }
    };

    static inline Vec3 operator*(double s, const Vec3& v) { return v * s; }

    static inline double dot(const Vec3& a, const Vec3& b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
    static inline Vec3 cross(const Vec3& a, const Vec3& b) {
        return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
    }
    static inline double length(const Vec3& v) { return std::sqrt(dot(v, v)); }
    static inline Vec3 normalize(const Vec3& v) {
        const double len = length(v);
        if (len < EPS) return {0,0,0};
        return v / len;
    }
    static inline Vec3 reflectDir(const Vec3& dir, const Vec3& n) {
        return dir - (2.0 * dot(dir, n)) * n;
    }

    struct Color8 { uint8_t r=0, g=0, b=0; };

    struct Material {
        double shininess    = 0;
        double kspecular    = 0;
        double kdiffuse     = 0;
        double kambient     = 0;
        double transparency = 0; // 0..1
        double reflectivity = 0; // 0..1
    };

    struct LightSource {
        Vec3   location{};
        double intensity = 1.0;
        Color8 color{255,255,255};
    };

    struct Hit {
        double t = 0;
        Vec3 p{};
        Vec3 n{};
    };

    class Figure {
    public:
        Vec3 center{};
        Color8 color{0,0,0};
        Material material{};

        virtual ~Figure() = default;
        virtual std::optional<Hit> intersect(const Vec3& rayOrigin, const Vec3& rayDir) const = 0;
        virtual bool blocksShadow() const { return true; } // в C# стены не участвуют в затенении
    };

    class Face final : public Figure {
    public:
        double width  = 1.0;
        double height = 1.0;
        Vec3 normal{};
        Vec3 heightVector{};
        Vec3 widthVector{};

        Face(const Vec3& c, const Vec3& n, const Vec3& h, double w, double hh)
                : width(w), height(hh) {
            center = c;
            normal = normalize(n);
            heightVector = normalize(h);
            widthVector  = normalize(cross(normal, heightVector));
        }

        Face(const Vec3& c, const Vec3& n, const Vec3& h, double w, double hh, const Color8& col, const Material& mat)
                : Face(c, n, h, w, hh) {
            color = col;
            material = mat;
        }

        bool blocksShadow() const override { return false; }

        Vec3 worldToFaceBasis(const Vec3& p) const {
            const Vec3 d = p - center;
            return { dot(widthVector, d), dot(heightVector, d), dot(normal, d) };
        }

        std::optional<Hit> intersect(const Vec3& rayOrigin, const Vec3& rayDir) const override {
            // Повторяет Face.getIntersection() из C#
            const double originInPlane = dot(normal, rayOrigin - center);
            if (std::abs(originInPlane) < 1e-5) return std::nullopt;

            const double tn = -dot(normal, center) + dot(normal, rayOrigin);
            if (std::abs(tn) < 1e-5) return std::nullopt;

            const double td = dot(normal, rayDir);
            if (std::abs(td) < 1e-5) return std::nullopt;

            const double t = -tn / td;
            if (t <= EPS) return std::nullopt;

            const Vec3 pointWorld = rayOrigin + rayDir * t;

            // В C# была доп.проверка на “противонаправленность” через z/direction.z
            if (std::abs(rayDir.z) > 1e-6) {
                const double k = (pointWorld.z - rayOrigin.z) / rayDir.z;
                if (k < 0) return std::nullopt;
            }

            const Vec3 local = worldToFaceBasis(pointWorld);
            if (std::abs(local.x) <= width/2.0 && std::abs(local.y) <= height/2.0) {
                return Hit{t, pointWorld, normal};
            }
            return std::nullopt;
        }
    };

    class Sphere final : public Figure {
    public:
        double radius = 1.0;

        Sphere(const Vec3& c, double r, const Color8& col, const Material& mat) {
            center = c;
            radius = r;
            color = col;
            material = mat;
        }

        std::optional<Hit> intersect(const Vec3& rayOrigin, const Vec3& rayDir) const override {
            // Классическая квадратика (устойчивее, чем ветвления из C#)
            const Vec3 oc = rayOrigin - center;
            const double a = dot(rayDir, rayDir);
            const double b = 2.0 * dot(oc, rayDir);
            const double c = dot(oc, oc) - radius*radius;
            const double disc = b*b - 4.0*a*c;
            if (disc < 0.0) return std::nullopt;

            const double s = std::sqrt(disc);
            double t = (-b - s) / (2.0*a);
            if (t <= EPS) t = (-b + s) / (2.0*a);
            if (t <= EPS) return std::nullopt;

            const Vec3 p = rayOrigin + rayDir * t;
            const Vec3 n = normalize(p - center);
            return Hit{t, p, n};
        }
    };

    class Cube final : public Figure {
    public:
        double side = 1.0;
        std::vector<Face> faces;

        Cube(const Vec3& c, double s, const Color8& col, const Material& mat) {
            center = c; side = s; color = col; material = mat;

            // как в C# Cube ctor
            faces.emplace_back(Vec3(center.x, center.y, center.z - side/2), Vec3(0,0,-1), Vec3(0,1,0), side, side);
            faces.emplace_back(Vec3(center.x, center.y, center.z + side/2), Vec3(0,0, 1), Vec3(0,1,0), side, side);
            faces.emplace_back(Vec3(center.x, center.y + side/2, center.z), Vec3(0, 1,0), Vec3(0,0,1), side, side);
            faces.emplace_back(Vec3(center.x, center.y - side/2, center.z), Vec3(0,-1,0), Vec3(0,0,1), side, side);
            faces.emplace_back(Vec3(center.x + side/2, center.y, center.z), Vec3(1,0,0), Vec3(0,1,0), side, side);
            faces.emplace_back(Vec3(center.x - side/2, center.y, center.z), Vec3(-1,0,0), Vec3(0,1,0), side, side);
        }

        std::optional<Hit> intersect(const Vec3& rayOrigin, const Vec3& rayDir) const override {
            double bestT = 1e100;
            std::optional<Hit> best{};
            for (const auto& f : faces) {
                auto h = f.intersect(rayOrigin, rayDir);
                if (!h) continue;
                if (h->t < bestT) { bestT = h->t; best = *h; }
            }
            return best;
        }
    };

    class RayTracing {
    public:
        static constexpr double fov = 80.0;

        Vec3 cameraPosition{0,0,0};
        std::vector<std::unique_ptr<Figure>> figures;
        std::vector<LightSource> lightSources;

        explicit RayTracing(const LightSource& mainLight) { lightSources.push_back(mainLight); }

        template <class T, class... Args>
        T* addFigure(Args&&... args) {
            auto u = std::make_unique<T>(std::forward<Args>(args)...);
            T* raw = u.get();
            figures.emplace_back(std::move(u));
            return raw;
        }

        void AddLightSource(double x, double y, double z) {
            if (lightSources.size() >= 2) return;
            lightSources.push_back(LightSource{Vec3(x,y,z), 0.5, Color8{181,255,201}});
            lightSources[0].intensity = 0.5;
        }

        void ChangeAddLightPos(double x, double y, double z) {
            if (lightSources.size() < 2) return;
            lightSources[1].location = Vec3(x,y,z);
        }

        void RemoveLightSource() {
            if (lightSources.size() < 2) return;
            lightSources.erase(lightSources.begin() + 1);
            lightSources[0].intensity = 1.0;
        }

        std::vector<uint8_t> Trace(int width, int height) const {
            std::vector<uint8_t> rgba(static_cast<size_t>(width) * static_cast<size_t>(height) * 4u);

            const double tanHalf = std::tan(deg2rad(fov / 2.0));
            const double aspect  = static_cast<double>(width) / static_cast<double>(height);

            for (int y = 0; y < height; ++y) {
                for (int x = 0; x < width; ++x) {
                    const double px = (2.0*(x + 0.5)/width  - 1.0) * tanHalf * aspect;
                    const double py = -(2.0*(y + 0.5)/height - 1.0) * tanHalf;
                    const Vec3 rayDir = normalize(Vec3(px, py, 1.0));

                    const Color8 c = shootRay(rayDir, cameraPosition, 0);

                    const size_t idx = (static_cast<size_t>(y)*static_cast<size_t>(width) + static_cast<size_t>(x)) * 4u;
                    rgba[idx+0] = c.r;
                    rgba[idx+1] = c.g;
                    rgba[idx+2] = c.b;
                    rgba[idx+3] = 255;
                }
            }
            return rgba;
        }

    private:
        static Color8 mixColors(const Color8& first, const Color8& second, double secondToFirstRatio) {
            secondToFirstRatio = clampd(secondToFirstRatio, 0.0, 1.0);
            const auto mix = [&](uint8_t a, uint8_t b) -> uint8_t {
                const double v = (static_cast<double>(b) * secondToFirstRatio) +
                                 (static_cast<double>(a) * (1.0 - secondToFirstRatio));
                return static_cast<uint8_t>(clampd(std::round(v), 0.0, 255.0));
            };
            return Color8{ mix(first.r, second.r), mix(first.g, second.g), mix(first.b, second.b) };
        }

        static Color8 CalcColor(const Color8& baseColor, double intensity, const std::vector<LightSource>& lightSources) {
            double finalR = 0, finalG = 0, finalB = 0;

            for (const auto& ls : lightSources) {
                if (ls.intensity < 0) continue;
                const double lightR = static_cast<double>(ls.color.r) / 255.0;
                const double lightG = static_cast<double>(ls.color.g) / 255.0;
                const double lightB = static_cast<double>(ls.color.b) / 255.0;

                finalR += static_cast<double>(baseColor.r) * lightR * ls.intensity;
                finalG += static_cast<double>(baseColor.g) * lightG * ls.intensity;
                finalB += static_cast<double>(baseColor.b) * lightB * ls.intensity;
            }

            const auto toU8 = [&](double v) -> uint8_t {
                return static_cast<uint8_t>(clampd(std::round(v * intensity), 0.0, 255.0));
            };
            return Color8{ toU8(finalR), toU8(finalG), toU8(finalB) };
        }

        bool doesRayIntersectSomething(const Vec3& direction, const Vec3& origin, double maxDist) const {
            const Vec3 o = origin + direction * (EPS * 50.0);
            for (const auto& fig : figures) {
                if (!fig->blocksShadow()) continue;
                auto hit = fig->intersect(o, direction);
                if (!hit) continue;
                if (hit->t > EPS && hit->t < maxDist) return true;
            }
            return false;
        }

        double CalcLightness(const Figure& figure, const Hit& hit, const Vec3& viewRay) const {
            double diffuseLightness  = 0.0;
            double specularLightness = 0.0;
            const double ambientLightness = 1.0;

            for (const auto& ls : lightSources) {
                const Vec3 toLight = ls.location - hit.p;
                const double distToLight = length(toLight);
                const Vec3 shadowRay = normalize(toLight);

                if (doesRayIntersectSomething(shadowRay, hit.p, distToLight)) continue;

                diffuseLightness += ls.intensity * clampd(dot(shadowRay, hit.n), 0.0, 1e100);

                const Vec3 reflectionRay = normalize(2.0 * dot(shadowRay, hit.n) * hit.n - shadowRay);
                const Vec3 negView = normalize(-1.0 * viewRay);
                specularLightness += ls.intensity *
                                     std::pow(clampd(dot(reflectionRay, negView), 0.0, 1e100), figure.material.shininess);
            }

            return ambientLightness * figure.material.kambient +
                   diffuseLightness  * figure.material.kdiffuse +
                   specularLightness * figure.material.kspecular;
        }

        Color8 shootRay(const Vec3& viewRay, const Vec3& origin, int depth) const {
            if (depth > 3) return Color8{128,128,128};

            double bestT = 1e100;
            const Figure* nearestFigure = nullptr;
            Hit nearestHit{};

            for (const auto& fig : figures) {
                auto h = fig->intersect(origin, viewRay);
                if (!h) continue;
                if (h->t < bestT) { bestT = h->t; nearestFigure = fig.get(); nearestHit = *h; }
            }

            if (!nearestFigure) return Color8{128,128,128};

            Color8 res = CalcColor(nearestFigure->color,
                                   CalcLightness(*nearestFigure, nearestHit, viewRay),
                                   lightSources);

            // отражение
            if (nearestFigure->material.reflectivity > 0.0) {
                const Vec3 reflDir = normalize(reflectDir(viewRay, nearestHit.n));
                const Vec3 newOrigin = nearestHit.p + nearestHit.n * (EPS * 80.0);
                const Color8 reflCol = shootRay(reflDir, newOrigin, depth + 1);
                res = mixColors(res, reflCol, nearestFigure->material.reflectivity);
            }

            // прозрачность (простое “просвечивание” по лучу дальше)
            if (nearestFigure->material.transparency > 0.0) {
                const Vec3 newOrigin = nearestHit.p + viewRay * (EPS * 80.0);
                const Color8 transCol = shootRay(viewRay, newOrigin, depth + 1);
                res = mixColors(res, transCol, nearestFigure->material.transparency);
            }

            return res;
        }
    };

    struct SceneHandles {
        Face* leftWall   = nullptr;
        Face* rightWall  = nullptr;
        Face* frontWall  = nullptr;
        Face* backWall   = nullptr;
        Face* floor      = nullptr;
        Face* ceiling    = nullptr;

        Sphere* sphereSmall = nullptr;
        Cube*   cube        = nullptr;
        Sphere* sphereBig   = nullptr;
    };

    static SceneHandles buildDefaultScene(RayTracing& rt) {
        // 1:1 как в C# Form1 ctor (цвета/материалы/координаты)
        const Vec3 center{0,0,14};
        const double roomSide = 30.0;

        const Material wallMat{0, 0, 0.9, 0.1, 0, 0};
        const Material objMat {40, 0.25, 0.7, 0.05, 0, 0};

        SceneHandles h{};

        h.leftWall = rt.addFigure<Face>(
                Vec3(center.x - roomSide/2, center.y, center.z),
                Vec3(1,0,0), Vec3(0,1,0),
                roomSide, roomSide,
                Color8{255,89,89}, wallMat
        );
        h.rightWall = rt.addFigure<Face>(
                Vec3(center.x + roomSide/2, center.y, center.z),
                Vec3(-1,0,0), Vec3(0,1,0),
                roomSide, roomSide,
                Color8{87,210,255}, wallMat
        );
        h.frontWall = rt.addFigure<Face>(
                Vec3(center.x, center.y, center.z + roomSide/2),
                Vec3(0,0,-1), Vec3(0,1,0),
                roomSide, roomSide,
                Color8{211,211,211}, wallMat
        );
        h.backWall = rt.addFigure<Face>(
                Vec3(center.x, center.y, center.z - roomSide/2),
                Vec3(0,0,1), Vec3(0,1,0),
                roomSide, roomSide,
                Color8{0,128,0}, wallMat
        );
        h.ceiling = rt.addFigure<Face>(
                Vec3(center.x, center.y + roomSide/2, center.z),
                Vec3(0,-1,0), Vec3(0,0,1),
                roomSide, roomSide,
                Color8{211,211,211}, wallMat
        );
        h.floor = rt.addFigure<Face>(
                Vec3(center.x, center.y - roomSide/2, center.z),
                Vec3(0,1,0), Vec3(0,0,1),
                roomSide, roomSide,
                Color8{211,211,211}, wallMat
        );

        h.sphereSmall = rt.addFigure<Sphere>(Vec3(6,-3,19), 2, Color8{255,165,0}, objMat);
        h.cube        = rt.addFigure<Cube>  (Vec3(6,-9,21), 7, Color8{255,255,255}, objMat);
        h.sphereBig   = rt.addFigure<Sphere>(Vec3(-5,-8,20), 5, Color8{255,228,196}, objMat);

        return h;
    }

    static void setSingleMirrorWall(SceneHandles& h, int idx) {
        // 0 none, 1 left, 2 right, 3 back, 4 front, 5 floor, 6 ceiling
        h.leftWall->material.reflectivity  = 0;
        h.rightWall->material.reflectivity = 0;
        h.backWall->material.reflectivity  = 0;
        h.frontWall->material.reflectivity = 0;
        h.floor->material.reflectivity     = 0;
        h.ceiling->material.reflectivity   = 0;

        Face* chosen = nullptr;
        switch (idx) {
            case 1: chosen = h.leftWall; break;
            case 2: chosen = h.rightWall; break;
            case 3: chosen = h.backWall; break;
            case 4: chosen = h.frontWall; break;
            case 5: chosen = h.floor; break;
            case 6: chosen = h.ceiling; break;
            default: break;
        }
        if (chosen) chosen->material.reflectivity = 1.0;
    }

} // namespace cornell

int run_indiv_2() {
    using namespace cornell;

    if (!glfwInit()) return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Cornell Box (CPU Ray Tracing) - GLFW + ImGui", nullptr, nullptr);
    if (!window) { glfwTerminate(); return 1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);

#if defined(__APPLE__)
    const char* glsl_version = "#version 150";
#else
    const char* glsl_version = "#version 130";
#endif
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Scene
    RayTracing rt(LightSource{Vec3(0,13,14), 1.0, Color8{255,255,240}});
    SceneHandles scene = buildDefaultScene(rt);

    // UI state
    int renderW = 800, renderH = 600;

    int  mirrorWallIdx = 0; // 0 none
    bool mirrorSmallSphere = false;
    bool mirrorCube = false;
    bool mirrorBigSphere = false;

    bool transparentSpheres = false;
    bool transparentCube = false;

    bool  addLightEnabled = false;
    float addLightX = 0.0f, addLightY = 0.0f, addLightZ = 0.0f;

    bool dirty = true;
    bool rendering = false;
    bool rerenderQueued = false;

    std::mutex rtMutex;
    std::future<std::vector<uint8_t>> renderFuture;
    std::chrono::steady_clock::time_point renderStart;
    double lastRenderMs = 0.0;

    GLuint tex = 0;
    int texW = 0, texH = 0;

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    auto kickRender = [&]() {
        if (rendering) { rerenderQueued = true; return; }
        rendering = true;
        rerenderQueued = false;
        dirty = false;
        renderStart = std::chrono::steady_clock::now();

        renderFuture = std::async(std::launch::async, [&]() {
            std::lock_guard<std::mutex> lk(rtMutex);
            return rt.Trace(renderW, renderH);
        });
    };

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        // finished render -> upload
        if (rendering) {
            if (renderFuture.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) {
                std::vector<uint8_t> rgba = renderFuture.get();
                const auto end = std::chrono::steady_clock::now();
                lastRenderMs = std::chrono::duration<double, std::milli>(end - renderStart).count();

                glBindTexture(GL_TEXTURE_2D, tex);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

                if (texW != renderW || texH != renderH) {
                    texW = renderW; texH = renderH;
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
                } else {
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texW, texH, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
                }

                glBindTexture(GL_TEXTURE_2D, 0);

                rendering = false;
                if (rerenderQueued) dirty = true;
            }
        }

        if (!rendering && dirty) kickRender();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Controls
        ImGui::Begin("Controls");

        ImGui::Text("Status: %s   (last: %.1f ms)", rendering ? "Rendering..." : "Ready", lastRenderMs);

        ImGui::Separator();

        int newW = renderW, newH = renderH;
        ImGui::PushItemWidth(120.0f);
        ImGui::InputInt("W", &newW);
        ImGui::InputInt("H", &newH);
        ImGui::PopItemWidth();
        newW = std::clamp(newW, 128, 1920);
        newH = std::clamp(newH, 128, 1080);
        if (newW != renderW || newH != renderH) { renderW = newW; renderH = newH; dirty = true; }

        if (ImGui::Button("Render now")) dirty = true;

        ImGui::Separator();

        static const char* wallItems[] = {
                "None", "Left (red)", "Right (blue)", "Back (green)", "Front", "Floor", "Ceiling"
        };
        int newMirrorIdx = mirrorWallIdx;
        if (ImGui::Combo("Mirror wall", &newMirrorIdx, wallItems, IM_ARRAYSIZE(wallItems))) {
            mirrorWallIdx = newMirrorIdx;
            std::lock_guard<std::mutex> lk(rtMutex);
            setSingleMirrorWall(scene, mirrorWallIdx);
            dirty = true;
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Mirror objects");

        if (ImGui::Checkbox("Small sphere", &mirrorSmallSphere)) {
            std::lock_guard<std::mutex> lk(rtMutex);
            scene.sphereSmall->material.reflectivity = mirrorSmallSphere ? 1.0 : 0.0;
            dirty = true;
        }
        if (ImGui::Checkbox("Cube", &mirrorCube)) {
            std::lock_guard<std::mutex> lk(rtMutex);
            scene.cube->material.reflectivity = mirrorCube ? 1.0 : 0.0;
            dirty = true;
        }
        if (ImGui::Checkbox("Big sphere", &mirrorBigSphere)) {
            std::lock_guard<std::mutex> lk(rtMutex);
            scene.sphereBig->material.reflectivity = mirrorBigSphere ? 1.0 : 0.0;
            dirty = true;
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Transparency objects");

        if (ImGui::Checkbox("Spheres transparent", &transparentSpheres)) {
            std::lock_guard<std::mutex> lk(rtMutex);
            const double t = transparentSpheres ? 0.65 : 0.0;
            scene.sphereSmall->material.transparency = t;
            scene.sphereBig->material.transparency   = t;
            dirty = true;
        }
        if (ImGui::Checkbox("Cube transparent", &transparentCube)) {
            std::lock_guard<std::mutex> lk(rtMutex);
            scene.cube->material.transparency = transparentCube ? 0.65 : 0.0;
            dirty = true;
        }

        ImGui::Separator();
        ImGui::TextUnformatted("Additional light (+1)");

        if (ImGui::Checkbox("Enable", &addLightEnabled)) {
            std::lock_guard<std::mutex> lk(rtMutex);
            if (addLightEnabled) rt.AddLightSource(addLightX, addLightY, addLightZ);
            else rt.RemoveLightSource();
            dirty = true;
        }

        if (addLightEnabled) {
            float x = addLightX, y = addLightY, z = addLightZ;
            const bool changed =
                    ImGui::SliderFloat("X", &x, -15.0f, 15.0f) ||
                    ImGui::SliderFloat("Y", &y, -15.0f, 15.0f) ||
                    ImGui::SliderFloat("Z", &z, -15.0f, 15.0f);

            if (changed) {
                addLightX = x; addLightY = y; addLightZ = z;
                std::lock_guard<std::mutex> lk(rtMutex);
                rt.ChangeAddLightPos(addLightX, addLightY, addLightZ);
                dirty = true;
            }
        }

        ImGui::End();

        // Render view
        ImGui::Begin("Render");
        const ImVec2 avail = ImGui::GetContentRegionAvail();
        if (tex != 0 && texW > 0 && texH > 0) {
            const float aspect = static_cast<float>(texW) / static_cast<float>(texH);
            ImVec2 size = avail;
            if (size.x <= 1) size.x = 1;
            if (size.y <= 1) size.y = 1;

            if (size.x / size.y > aspect) size.x = size.y * aspect;
            else size.y = size.x / aspect;

            ImGui::Image(reinterpret_cast<void*>(static_cast<intptr_t>(tex)), size);
        } else {
            ImGui::TextUnformatted("No image yet");
        }
        ImGui::End();

        ImGui::Render();
        int display_w = 0, display_h = 0;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }

    glDeleteTextures(1, &tex);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
