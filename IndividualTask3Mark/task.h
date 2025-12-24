#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstring>
#include <vector>
#include <array>
#include <string>
#include <cmath>
#include <algorithm>
#include <random>

#define GL_SILENCE_DEPRECATION
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/gl3.h>
#include <OpenGL/gl3ext.h>
#else
#include <GL/gl.h>
#endif

struct Vec2 { float x{}, y{}; };
struct Vec3 { float x{}, y{}, z{}; };

static inline Vec3 operator+(Vec3 a, Vec3 b){ return {a.x+b.x,a.y+b.y,a.z+b.z}; }
static inline Vec3 operator-(Vec3 a, Vec3 b){ return {a.x-b.x,a.y-b.y,a.z-b.z}; }
static inline Vec3 operator*(Vec3 a, float s){ return {a.x*s,a.y*s,a.z*s}; }
static inline Vec3 operator/(Vec3 a, float s){ return {a.x/s,a.y/s,a.z/s}; }

static inline float dot(Vec3 a, Vec3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; }
static inline Vec3 cross(Vec3 a, Vec3 b){
    return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
static inline float len(Vec3 a){ return std::sqrt(dot(a,a)); }
static inline Vec3 norm(Vec3 a){ float l = len(a); return (l>1e-6f)? a/l : Vec3{0,0,0}; }
static inline Vec3 lerp(Vec3 a, Vec3 b, float t){ return a*(1.0f-t) + b*t; }
static inline float clampf(float v, float a, float b){ return std::max(a, std::min(b, v)); }
static inline float smoothstep(float a, float b, float x){
    float t = clampf((x-a)/(b-a), 0.0f, 1.0f);
    return t*t*(3.0f - 2.0f*t);
}

struct Mat4 {
    float m[16]{};
    static Mat4 I(){
        Mat4 r{};
        r.m[0]=r.m[5]=r.m[10]=r.m[15]=1.0f;
        return r;
    }
};

static inline Mat4 mul(const Mat4& A, const Mat4& B){
    Mat4 R{};
    for(int c=0;c<4;c++){
        for(int r=0;r<4;r++){
            R.m[c*4 + r] =
                    A.m[0*4 + r]*B.m[c*4 + 0] +
                    A.m[1*4 + r]*B.m[c*4 + 1] +
                    A.m[2*4 + r]*B.m[c*4 + 2] +
                    A.m[3*4 + r]*B.m[c*4 + 3];
        }
    }
    return R;
}

static inline Vec3 mulDir(const Mat4& M, Vec3 v){
    return {
            M.m[0]*v.x + M.m[4]*v.y + M.m[8]*v.z,
            M.m[1]*v.x + M.m[5]*v.y + M.m[9]*v.z,
            M.m[2]*v.x + M.m[6]*v.y + M.m[10]*v.z
    };
}

static inline Mat4 translate(Vec3 t){
    Mat4 r = Mat4::I();
    r.m[12]=t.x; r.m[13]=t.y; r.m[14]=t.z;
    return r;
}
static inline Mat4 scale(Vec3 s){
    Mat4 r{};
    r.m[0]=s.x; r.m[5]=s.y; r.m[10]=s.z; r.m[15]=1.0f;
    return r;
}
static inline Mat4 rotY(float a){
    Mat4 r = Mat4::I();
    float c=std::cos(a), s=std::sin(a);

    r.m[0]= c;  r.m[2]= s;
    r.m[8]=-s;  r.m[10]=c;
    return r;
}
static inline Mat4 rotX(float a){
    Mat4 r = Mat4::I();
    float c=std::cos(a), s=std::sin(a);

    r.m[5]= c;  r.m[6]= s;
    r.m[9]=-s;  r.m[10]=c;
    return r;
}
static inline Mat4 perspective(float fovy, float aspect, float znear, float zfar){
    float f = 1.0f / std::tan(fovy*0.5f);
    Mat4 r{};
    r.m[0] = f/aspect;
    r.m[5] = f;
    r.m[10]= (zfar+znear)/(znear-zfar);
    r.m[11]= -1.0f;
    r.m[14]= (2.0f*zfar*znear)/(znear-zfar);
    return r;
}
static inline Mat4 lookAt(Vec3 eye, Vec3 center, Vec3 up){
    Vec3 f = norm(center - eye);
    Vec3 upv = up;

    if(len(cross(f, upv)) < 1e-3f){
        upv = Vec3{0,0,1};
        if(len(cross(f, upv)) < 1e-3f) upv = Vec3{1,0,0};
    }
    Vec3 s = norm(cross(f, upv));
    Vec3 u = cross(s, f);
    Mat4 r = Mat4::I();

    r.m[0]=s.x;  r.m[1]=s.y;  r.m[2]=s.z;
    r.m[4]=u.x;  r.m[5]=u.y;  r.m[6]=u.z;
    r.m[8]=-f.x; r.m[9]=-f.y; r.m[10]=-f.z;
    r.m[12]= -dot(s, eye);
    r.m[13]= -dot(u, eye);
    r.m[14]=  dot(f, eye);
    return r;
}

static GLuint compileShader(GLenum type, const char* src){
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok=0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if(!ok){
        GLint len=0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log; log.resize((size_t)len+1);
        glGetShaderInfoLog(s, len, nullptr, log.data());
        std::fprintf(stderr, "Shader compile error:\n%s\n", log.c_str());
        std::exit(1);
    }
    return s;
}
static GLuint linkProgram(GLuint vs, GLuint fs){
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok=0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    if(!ok){
        GLint len=0;
        glGetProgramiv(p, GL_INFO_LOG_LENGTH, &len);
        std::string log; log.resize((size_t)len+1);
        glGetProgramInfoLog(p, len, nullptr, log.data());
        std::fprintf(stderr, "Program link error:\n%s\n", log.c_str());
        std::exit(1);
    }
    glDetachShader(p, vs);
    glDetachShader(p, fs);
    return p;
}

static GLuint makeTextureRGBA(int w, int h, const std::vector<uint8_t>& rgba, bool mip=true){
    GLuint tex=0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mip?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());
    if(mip) glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static GLuint makeTextureR8(int w, int h, const std::vector<uint8_t>& r8){
    GLuint tex=0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, r8.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

using PFNGLVERTEXATTRIBDIVISORPROC_ = void (*)(GLuint, GLuint);
using PFNGLDRAWELEMENTSINSTANCEDPROC_ = void (*)(GLenum, GLsizei, GLenum, const void*, GLsizei);
static PFNGLVERTEXATTRIBDIVISORPROC_ glVertexAttribDivisor_ = nullptr;
static PFNGLDRAWELEMENTSINSTANCEDPROC_ glDrawElementsInstanced_ = nullptr;

static void loadInstancingFunctions(){
    glVertexAttribDivisor_ = (PFNGLVERTEXATTRIBDIVISORPROC_)glfwGetProcAddress("glVertexAttribDivisor");
    glDrawElementsInstanced_ = (PFNGLDRAWELEMENTSINSTANCEDPROC_)glfwGetProcAddress("glDrawElementsInstanced");
}

struct Vertex{
    float px,py,pz;
    float nx,ny,nz;
    float u,v;
};

struct Mesh{
    GLuint vao=0, vbo=0, ebo=0;
    GLsizei indexCount=0;
};

struct InstanceData{
    float m[16];
    float tint[4];
};

struct InstancedMesh{
    GLuint vao=0;
    GLuint instanceVBO=0;
    GLsizei indexCount=0;
    GLsizei capacity=0;
};

static InstancedMesh makeInstancedMesh(const Mesh& base, GLsizei capacity){
    InstancedMesh im{};
    im.indexCount = base.indexCount;
    im.capacity = capacity;

    glGenVertexArrays(1, &im.vao);
    glBindVertexArray(im.vao);


    glBindBuffer(GL_ARRAY_BUFFER, base.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, base.ebo);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, px));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));


    glGenBuffers(1, &im.instanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, im.instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(capacity * (GLsizei)sizeof(InstanceData)), nullptr, GL_DYNAMIC_DRAW);


    GLsizei stride = (GLsizei)sizeof(InstanceData);
    std::size_t off0 = 0;
    std::size_t off1 = sizeof(float)*4;
    std::size_t off2 = sizeof(float)*8;
    std::size_t off3 = sizeof(float)*12;
    std::size_t offT = sizeof(float)*16;

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)off0);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void*)off1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void*)off2);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void*)off3);

    glEnableVertexAttribArray(7);
    glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, stride, (void*)offT);

    if(glVertexAttribDivisor_){
        glVertexAttribDivisor_(3, 1);
        glVertexAttribDivisor_(4, 1);
        glVertexAttribDivisor_(5, 1);
        glVertexAttribDivisor_(6, 1);
        glVertexAttribDivisor_(7, 1);
    }

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return im;
}

static void destroyInstancedMesh(InstancedMesh& im){
    if(im.instanceVBO) glDeleteBuffers(1, &im.instanceVBO);
    if(im.vao) glDeleteVertexArrays(1, &im.vao);
    im = {};
}

static GLsizei uploadInstances(const InstancedMesh& im, const std::vector<InstanceData>& data){
    GLsizei n = (GLsizei)std::min<std::size_t>(data.size(), (std::size_t)im.capacity);
    glBindBuffer(GL_ARRAY_BUFFER, im.instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, (GLsizeiptr)(n * (GLsizei)sizeof(InstanceData)), data.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    return n;
}

static Mesh uploadMesh(const std::vector<Vertex>& v, const std::vector<uint32_t>& idx){
    Mesh m{};
    glGenVertexArrays(1, &m.vao);
    glGenBuffers(1, &m.vbo);
    glGenBuffers(1, &m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(v.size()*sizeof(Vertex)), v.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(idx.size()*sizeof(uint32_t)), idx.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, px));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glBindVertexArray(0);
    m.indexCount = (GLsizei)idx.size();
    return m;
}

static void destroyMesh(Mesh& m){
    if(m.ebo) glDeleteBuffers(1,&m.ebo);
    if(m.vbo) glDeleteBuffers(1,&m.vbo);
    if(m.vao) glDeleteVertexArrays(1,&m.vao);
    m = {};
}

static void addTri(std::vector<uint32_t>& idx, uint32_t a, uint32_t b, uint32_t c){
    idx.push_back(a); idx.push_back(b); idx.push_back(c);
}

static Mesh makeCube(){

    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    v.reserve(24); idx.reserve(36);

    auto pushFace = [&](Vec3 n, Vec3 p0, Vec3 p1, Vec3 p2, Vec3 p3){
        uint32_t base = (uint32_t)v.size();
        v.push_back({p0.x,p0.y,p0.z,n.x,n.y,n.z, 0,0});
        v.push_back({p1.x,p1.y,p1.z,n.x,n.y,n.z, 1,0});
        v.push_back({p2.x,p2.y,p2.z,n.x,n.y,n.z, 1,1});
        v.push_back({p3.x,p3.y,p3.z,n.x,n.y,n.z, 0,1});
        addTri(idx, base+0, base+1, base+2);
        addTri(idx, base+0, base+2, base+3);
    };

    float s=0.5f;

    pushFace({0,0,1},  {-s,-s, s},{ s,-s, s},{ s, s, s},{-s, s, s});
    pushFace({0,0,-1}, { s,-s,-s},{-s,-s,-s},{-s, s,-s},{ s, s,-s});
    pushFace({1,0,0},  { s,-s, s},{ s,-s,-s},{ s, s,-s},{ s, s, s});
    pushFace({-1,0,0}, {-s,-s,-s},{-s,-s, s},{-s, s, s},{-s, s,-s});
    pushFace({0,1,0},  {-s, s, s},{ s, s, s},{ s, s,-s},{-s, s,-s});
    pushFace({0,-1,0}, {-s,-s,-s},{ s,-s,-s},{ s,-s, s},{-s,-s, s});

    return uploadMesh(v, idx);
}

static Mesh makeSphere(int segU=32, int segV=16){
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    v.reserve((segU+1)*(segV+1));
    for(int y=0;y<=segV;y++){
        float vv = (float)y/(float)segV;
        float phi = vv * (float)M_PI;
        for(int x=0;x<=segU;x++){
            float uu = (float)x/(float)segU;
            float theta = uu * 2.0f*(float)M_PI;
            float sx = std::sin(phi)*std::cos(theta);
            float sy = std::cos(phi);
            float sz = std::sin(phi)*std::sin(theta);
            Vec3 n{sx,sy,sz};
            v.push_back({sx,sy,sz, n.x,n.y,n.z, uu, 1.0f-vv});
        }
    }
    for(int y=0;y<segV;y++){
        for(int x=0;x<segU;x++){
            uint32_t i0 = (uint32_t)(y*(segU+1) + x);
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (segU+1);
            uint32_t i3 = i2 + 1;

            addTri(idx, i0,i1,i2);
            addTri(idx, i1,i3,i2);
        }
    }
    return uploadMesh(v, idx);
}

static Mesh makeCylinder(int seg=24, float height=1.0f, float radius=0.5f, bool caps=true){
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    float h0=-height*0.5f, h1=height*0.5f;


    for(int i=0;i<=seg;i++){
        float u=(float)i/(float)seg;
        float a = u*2.0f*(float)M_PI;
        float c=std::cos(a), s=std::sin(a);
        Vec3 n{c,0,s};
        v.push_back({radius*c,h0,radius*s, n.x,n.y,n.z, u,0});
        v.push_back({radius*c,h1,radius*s, n.x,n.y,n.z, u,1});
    }
    for(int i=0;i<seg;i++){
        uint32_t b = (uint32_t)(i*2);
        addTri(idx, b+0, b+1, b+2);
        addTri(idx, b+1, b+3, b+2);
    }

    if(caps){
        uint32_t base = (uint32_t)v.size();

        v.push_back({0,h0,0, 0,-1,0, 0.5f,0.5f});

        for(int i=0;i<=seg;i++){
            float u=(float)i/(float)seg;
            float a=u*2.0f*(float)M_PI;
            float c=std::cos(a), s=std::sin(a);
            v.push_back({radius*c,h0,radius*s, 0,-1,0, 0.5f+0.5f*c,0.5f+0.5f*s});
        }
        for(int i=0;i<seg;i++){
            addTri(idx, base, base+1+i, base+1+i+1);
        }

        uint32_t topBase = (uint32_t)v.size();
        v.push_back({0,h1,0, 0,1,0, 0.5f,0.5f});
        for(int i=0;i<=seg;i++){
            float u=(float)i/(float)seg;
            float a=u*2.0f*(float)M_PI;
            float c=std::cos(a), s=std::sin(a);
            v.push_back({radius*c,h1,radius*s, 0,1,0, 0.5f+0.5f*c,0.5f+0.5f*s});
        }
        for(int i=0;i<seg;i++){
            addTri(idx, topBase, topBase+1+i+1, topBase+1+i);
        }
    }
    return uploadMesh(v, idx);
}

static Mesh makeCone(int seg=24, float height=1.2f, float radius=0.6f){
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;

    float h0=-height*0.5f, h1=height*0.5f;
    Vec3 tip{0,h1,0};


    for(int i=0;i<=seg;i++){
        float u=(float)i/(float)seg;
        float a=u*2.0f*(float)M_PI;
        float c=std::cos(a), s=std::sin(a);
        Vec3 p{radius*c,h0,radius*s};

        Vec3 n = norm(Vec3{c, radius/height, s});
        v.push_back({p.x,p.y,p.z, n.x,n.y,n.z, u,0});
        v.push_back({tip.x,tip.y,tip.z, n.x,n.y,n.z, u,1});
    }
    for(int i=0;i<seg;i++){
        uint32_t b = (uint32_t)(i*2);
        addTri(idx, b+0, b+1, b+2);
    }


    uint32_t base = (uint32_t)v.size();
    v.push_back({0,h0,0, 0,-1,0, 0.5f,0.5f});
    for(int i=0;i<=seg;i++){
        float u=(float)i/(float)seg;
        float a=u*2.0f*(float)M_PI;
        float c=std::cos(a), s=std::sin(a);
        v.push_back({radius*c,h0,radius*s, 0,-1,0, 0.5f+0.5f*c,0.5f+0.5f*s});
    }
    for(int i=0;i<seg;i++){
        addTri(idx, base, base+1+i, base+1+i+1);
    }

    return uploadMesh(v, idx);
}

static Mesh makeGrid(int n=128, float size=80.0f){
    std::vector<Vertex> v;
    std::vector<uint32_t> idx;
    v.reserve((n+1)*(n+1));
    idx.reserve(n*n*6);

    float half=size*0.5f;
    for(int z=0;z<=n;z++){
        for(int x=0;x<=n;x++){
            float u=(float)x/(float)n;
            float w=(float)z/(float)n;
            float px = -half + u*size;
            float pz = -half + w*size;
            v.push_back({px,0.0f,pz, 0,1,0, u, w});
        }
    }

    for(int z=0;z<n;z++){
        for(int x=0;x<n;x++){
            uint32_t i0 = (uint32_t)(z*(n+1) + x);
            uint32_t i1 = i0 + 1;
            uint32_t i2 = i0 + (n+1);
            uint32_t i3 = i2 + 1;

            addTri(idx, i0,i1,i2);
            addTri(idx, i1,i3,i2);
        }
    }

    return uploadMesh(v, idx);
}

struct Heightmap{
    int w=0,h=0;
    std::vector<uint8_t> data;
    float worldSize=80.0f;
    float heightScale=6.0f;
    float sample01(float u, float v) const {
        u = u - std::floor(u);
        v = v - std::floor(v);
        float x = u*(w-1);
        float y = v*(h-1);
        int x0=(int)std::floor(x), y0=(int)std::floor(y);
        int x1=std::min(x0+1, w-1), y1=std::min(y0+1, h-1);
        float tx = x - x0, ty = y - y0;
        auto at=[&](int ix,int iy)->float{
            return data[iy*w + ix]/255.0f;
        };
        float a=at(x0,y0), b=at(x1,y0), c=at(x0,y1), d=at(x1,y1);
        float ab = a*(1-tx)+b*tx;
        float cd = c*(1-tx)+d*tx;
        return ab*(1-ty)+cd*ty;
    }
    float heightAtXZ(float x, float z) const {
        float half=worldSize*0.5f;
        float u = (x + half)/worldSize;
        float v = (z + half)/worldSize;
        float h01 = sample01(u, v);
        return h01*heightScale;
    }
};

static Heightmap generateHeightmap(int w=256, int h=256){
    Heightmap hm{};
    hm.w=w; hm.h=h;
    hm.data.resize((size_t)w*h);

    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> uf(0.0f, 1.0f);

    auto noise = [&](int x,int y)->float{

        rng.seed((uint32_t)(x*73856093u ^ y*19349663u ^ 0x9E3779B9u));
        return uf(rng);
    };

    auto smoothNoise = [&](float x,float y)->float{
        int x0=(int)std::floor(x), y0=(int)std::floor(y);
        float tx=x-x0, ty=y-y0;
        float a=noise(x0,y0), b=noise(x0+1,y0), c=noise(x0,y0+1), d=noise(x0+1,y0+1);
        float ab=a*(1-tx)+b*tx;
        float cd=c*(1-tx)+d*tx;
        return ab*(1-ty)+cd*ty;
    };

    auto fbm = [&](float x,float y)->float{
        float f=0.0f, amp=0.55f, freq=1.0f;
        for(int o=0;o<5;o++){
            f += amp*smoothNoise(x*freq, y*freq);
            freq *= 2.0f;
            amp *= 0.5f;
        }
        return f;
    };

    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            float u=(float)x/(float)(w-1);
            float v=(float)y/(float)(h-1);

            float n = fbm(u*6.0f, v*6.0f);
            float dx=u-0.5f, dz=v-0.5f;
            float d = std::sqrt(dx*dx+dz*dz);
            float centerFlatten = smoothstep(0.08f, 0.18f, d);
            float h01 = 0.25f + 0.75f*(n*centerFlatten + 0.15f*(1.0f-centerFlatten));
            h01 = clampf(h01, 0.0f, 1.0f);
            hm.data[y*w + x] = (uint8_t)std::lround(h01*255.0f);
        }
    }
    return hm;
}

static std::vector<uint8_t> makeTerrainAlbedo(int w=512,int h=512){
    std::vector<uint8_t> rgba((size_t)w*h*4);
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> ui(-12, 12);
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            float u=(float)x/(float)(w-1);
            float v=(float)y/(float)(h-1);

            float stripes = 0.5f + 0.5f*std::sin((u*18.0f + v*23.0f)*2.0f*(float)M_PI);
            float g = 0.35f + 0.10f*stripes;
            float r = 0.20f + 0.08f*stripes;
            float b = 0.18f + 0.06f*stripes;

            float snow = 0.18f * (0.5f + 0.5f*std::sin((u*5.0f - v*7.0f)*2.0f*(float)M_PI));
            r = r*(1.0f-snow) + 0.92f*snow;
            g = g*(1.0f-snow) + 0.92f*snow;
            b = b*(1.0f-snow) + 0.95f*snow;

            int R = (int)std::lround(r*255.0f) + ui(rng);
            int G = (int)std::lround(g*255.0f) + ui(rng);
            int B = (int)std::lround(b*255.0f) + ui(rng);
            R = std::clamp(R, 0, 255);
            G = std::clamp(G, 0, 255);
            B = std::clamp(B, 0, 255);
            size_t i=((size_t)y*w + x)*4;
            rgba[i+0]=(uint8_t)R;
            rgba[i+1]=(uint8_t)G;
            rgba[i+2]=(uint8_t)B;
            rgba[i+3]=255;
        }
    }
    return rgba;
}

static std::vector<uint8_t> makeStripedTexture(int w,int h, uint8_t r1,uint8_t g1,uint8_t b1, uint8_t r2,uint8_t g2,uint8_t b2){
    std::vector<uint8_t> rgba((size_t)w*h*4);
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            float u=(float)x/(float)(w-1);
            int stripe = ((int)std::floor(u*12.0f)) & 1;
            uint8_t r = stripe? r1:r2;
            uint8_t g = stripe? g1:g2;
            uint8_t b = stripe? b1:b2;
            size_t i=((size_t)y*w + x)*4;
            rgba[i+0]=r; rgba[i+1]=g; rgba[i+2]=b; rgba[i+3]=255;
        }
    }
    return rgba;
}

static std::vector<uint8_t> makeTreeTexture(int w,int h){
    std::vector<uint8_t> rgba((size_t)w*h*4);
    for(int y=0;y<h;y++){
        for(int x=0;x<w;x++){
            float v=(float)y/(float)(h-1);
            float u=(float)x/(float)(w-1);
            float bark = 0.5f + 0.5f*std::sin((u*24.0f)*2.0f*(float)M_PI);
            float r = 0.25f + 0.10f*bark;
            float g = 0.16f + 0.07f*bark;
            float b = 0.10f + 0.05f*bark;

            float leaf = smoothstep(0.65f, 1.0f, v);
            r = r*(1.0f-leaf) + 0.12f*leaf;
            g = g*(1.0f-leaf) + 0.55f*leaf;
            b = b*(1.0f-leaf) + 0.15f*leaf;

            size_t i=((size_t)y*w + x)*4;
            rgba[i+0]=(uint8_t)std::lround(clampf(r,0,1)*255.0f);
            rgba[i+1]=(uint8_t)std::lround(clampf(g,0,1)*255.0f);
            rgba[i+2]=(uint8_t)std::lround(clampf(b,0,1)*255.0f);
            rgba[i+3]=255;
        }
    }
    return rgba;
}

struct Package{
    Vec3 pos;
    Vec3 vel;
    bool alive=true;
};

struct House{
    Vec3 pos;
    Vec3 half;
    float yaw=0.0f;
    bool delivered=false;
};

static const char* kModelVS = R"GLSL(
#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aUV;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

uniform bool uWobble;
uniform float uTime;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main(){
  vec3 pos = aPos;
  vec3 nrm = aNormal;

  if(uWobble){
    float k = clamp(pos.y*0.7 + 0.5, 0.0, 1.0);
    float w = sin(uTime*1.7 + pos.x*3.0 + pos.z*2.0) * 0.12 * k;
    pos.x += w;
    pos.z += w*0.3;
  }

  vec4 world = uModel * vec4(pos, 1.0);
  vWorldPos = world.xyz;
  vNormal = mat3(uModel) * nrm;
  vUV = aUV;
  gl_Position = uProj * uView * world;
}
)GLSL";

static const char* kInstVS = R"GLSL(
#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aUV;

layout (location=3) in vec4 iM0;
layout (location=4) in vec4 iM1;
layout (location=5) in vec4 iM2;
layout (location=6) in vec4 iM3;
layout (location=7) in vec4 iTint;

uniform mat4 uView;
uniform mat4 uProj;

uniform bool uWobble;
uniform float uTime;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;
out vec3 vTint;

void main(){
  mat4 M = mat4(iM0, iM1, iM2, iM3);

  vec3 pos = aPos;
  vec3 nrm = aNormal;

  if(uWobble){
    float k = clamp(pos.y*0.7 + 0.5, 0.0, 1.0);
    float w = sin(uTime*1.7 + pos.x*3.0 + pos.z*2.0) * 0.12 * k;
    pos.x += w;
    pos.z += w*0.3;
  }

  vec4 world = M * vec4(pos, 1.0);
  vWorldPos = world.xyz;
  vNormal = mat3(M) * nrm;
  vUV = aUV;
  vTint = iTint.rgb;
  gl_Position = uProj * uView * world;
}
)GLSL";

static const char* kInstFS = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;
in vec3 vTint;

uniform sampler2D uTex;

uniform vec3 uCamPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;

out vec4 FragColor;

void main(){
  vec3 albedo = texture(uTex, vUV).rgb * vTint;
  vec3 N = normalize(vNormal);
  vec3 L = normalize(-uLightDir);
  float ndl = max(dot(N, L), 0.0);
  vec3 diffuse = albedo * uLightColor * ndl;

  vec3 V = normalize(uCamPos - vWorldPos);
  vec3 H = normalize(L + V);
  float spec = pow(max(dot(N, H), 0.0), 48.0) * 0.35;
  vec3 specular = uLightColor * spec;

  vec3 color = uAmbient * albedo + diffuse + specular;
  FragColor = vec4(color, 1.0);
}
)GLSL";

static const char* kModelFS = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform sampler2D uTex;

uniform vec3 uCamPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;

uniform vec3 uTint;

out vec4 FragColor;

void main(){
  vec3 albedo = texture(uTex, vUV).rgb * uTint;
  vec3 N = normalize(vNormal);
  vec3 L = normalize(-uLightDir);
  float ndl = max(dot(N, L), 0.0);
  vec3 diffuse = albedo * uLightColor * ndl;

  vec3 V = normalize(uCamPos - vWorldPos);
  vec3 H = normalize(L + V);
  float spec = pow(max(dot(N, H), 0.0), 48.0) * 0.35;
  vec3 specular = uLightColor * spec;

  vec3 color = uAmbient * albedo + diffuse + specular;
  FragColor = vec4(color, 1.0);
}
)GLSL";

static const char* kTerrainVS = R"GLSL(
#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;
layout (location=2) in vec2 aUV;

uniform mat4 uView;
uniform mat4 uProj;

uniform sampler2D uHeightmap;
uniform float uHeightScale;
uniform float uWorldSize;

out vec3 vWorldPos;
out vec3 vNormal;
out vec2 vUV;

void main(){

  float h = texture(uHeightmap, aUV).r;
  vec3 pos = aPos;
  pos.y = h * uHeightScale;


  float du = 1.0 / 256.0;
  float dv = 1.0 / 256.0;
  float hL = texture(uHeightmap, aUV + vec2(-du, 0)).r * uHeightScale;
  float hR = texture(uHeightmap, aUV + vec2( du, 0)).r * uHeightScale;
  float hD = texture(uHeightmap, aUV + vec2(0, -dv)).r * uHeightScale;
  float hU = texture(uHeightmap, aUV + vec2(0,  dv)).r * uHeightScale;

  vec3 dx = vec3( (2.0*du)*uWorldSize, hR-hL, 0.0 );
  vec3 dz = vec3( 0.0, hU-hD, (2.0*dv)*uWorldSize );
  vec3 n = normalize(cross(dz, dx));

  vWorldPos = pos;
  vNormal = n;
  vUV = aUV * 10.0;
  gl_Position = uProj * uView * vec4(pos, 1.0);
}
)GLSL";

static const char* kTerrainFS = R"GLSL(
#version 330 core
in vec3 vWorldPos;
in vec3 vNormal;
in vec2 vUV;

uniform sampler2D uTex;

uniform vec3 uCamPos;
uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbient;

out vec4 FragColor;

void main(){
  vec3 albedo = texture(uTex, vUV).rgb;
  vec3 N = normalize(vNormal);
  vec3 L = normalize(-uLightDir);
  float ndl = max(dot(N, L), 0.0);
  vec3 diffuse = albedo * uLightColor * ndl;


  vec3 V = normalize(uCamPos - vWorldPos);
  vec3 H = normalize(L + V);
  float spec = pow(max(dot(N, H), 0.0), 24.0) * 0.10;
  vec3 specular = uLightColor * spec;

  vec3 color = uAmbient * albedo + diffuse + specular;
  FragColor = vec4(color, 1.0);
}
)GLSL";

struct InputState{
    bool keys[512]{};
    bool toggleCam=false;
    bool toggleWobble=false;
    bool drop=false;
};

static void keyCb(GLFWwindow*, int key, int, int action, int){


}

int run_indiv_3(){
    if(!glfwInit()){
        std::fprintf(stderr, "glfwInit failed\n");
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow* win = glfwCreateWindow(1280, 720, "Airship Delivery (Lab15)", nullptr, nullptr);
    if(!win){
        std::fprintf(stderr, "glfwCreateWindow failed\n");
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(win);
    glfwSwapInterval(1);

    InputState input{};
    glfwSetWindowUserPointer(win, &input);
    glfwSetKeyCallback(win, [](GLFWwindow* w, int key, int sc, int action, int mods){
        (void)sc; (void)mods;
        auto* in = (InputState*)glfwGetWindowUserPointer(w);
        if(key >= 0 && key < 512){
            if(action == GLFW_PRESS) in->keys[key] = true;
            if(action == GLFW_RELEASE) in->keys[key] = false;
        }
        if(action == GLFW_PRESS){
            if(key == GLFW_KEY_C) in->toggleCam = true;
            if(key == GLFW_KEY_V) in->toggleWobble = true;
            if(key == GLFW_KEY_SPACE) in->drop = true;
        }
    });

    loadInstancingFunctions();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


    GLuint vsModel = compileShader(GL_VERTEX_SHADER, kModelVS);
    GLuint fsModel = compileShader(GL_FRAGMENT_SHADER, kModelFS);
    GLuint progModel = linkProgram(vsModel, fsModel);
    glDeleteShader(vsModel);
    glDeleteShader(fsModel);

    GLuint vsInst = compileShader(GL_VERTEX_SHADER, kInstVS);
    GLuint fsInst = compileShader(GL_FRAGMENT_SHADER, kInstFS);
    GLuint progInst = linkProgram(vsInst, fsInst);
    glDeleteShader(vsInst);
    glDeleteShader(fsInst);

    GLuint vsTer = compileShader(GL_VERTEX_SHADER, kTerrainVS);
    GLuint fsTer = compileShader(GL_FRAGMENT_SHADER, kTerrainFS);
    GLuint progTer = linkProgram(vsTer, fsTer);
    glDeleteShader(vsTer);
    glDeleteShader(fsTer);


    Mesh cube = makeCube();
    Mesh sphere = makeSphere(36, 18);
    Mesh cone = makeCone(28, 2.6f, 1.2f);
    Mesh cyl = makeCylinder(28, 2.2f, 0.35f, true);
    Mesh grid = makeGrid(160, 80.0f);

    InstancedMesh cubeInst = makeInstancedMesh(cube, 256);


    Heightmap hm = generateHeightmap(256,256);
    GLuint texHeight = makeTextureR8(hm.w, hm.h, hm.data);
    GLuint texTerrain = makeTextureRGBA(512,512, makeTerrainAlbedo(512,512), true);
    GLuint texBlimp = makeTextureRGBA(512,128, makeStripedTexture(512,128, 220,40,40, 245,235,210), true);
    GLuint texTree  = makeTextureRGBA(256,256, makeTreeTexture(256,256), true);
    GLuint texHouse = makeTextureRGBA(256,256, makeStripedTexture(256,256, 80,60,40, 120,90,60), true);
    GLuint texDeco  = makeTextureRGBA(256,256, makeStripedTexture(256,256, 180,180,190, 120,120,140), true);
    GLuint texPack  = makeTextureRGBA(128,128, makeStripedTexture(128,128, 240,200,60, 200,80,30), true);


    std::mt19937 rng(7);
    std::uniform_real_distribution<float> posd(-35.0f, 35.0f);
    std::uniform_real_distribution<float> yawd(0.0f, 2.0f*(float)M_PI);

    std::vector<House> houses;
    houses.reserve(10);

    auto okPos=[&](Vec3 p){
        if(len(Vec3{p.x,0,p.z}) < 8.0f) return false;
        for(auto& h: houses){
            if(len(Vec3{p.x-h.pos.x,0,p.z-h.pos.z}) < 6.0f) return false;
        }
        return true;
    };

    while((int)houses.size() < 7){
        Vec3 p{posd(rng), 0.0f, posd(rng)};
        if(!okPos(p)) continue;
        p.y = hm.heightAtXZ(p.x, p.z);
        houses.push_back({p, {1.2f,1.4f,1.2f}, yawd(rng), false});
    }


    std::vector<Vec3> rocks;
    std::vector<Vec3> flags;
    for(int i=0;i<28;i++){
        Vec3 p{posd(rng),0,posd(rng)};
        if(len(Vec3{p.x,0,p.z}) < 7.0f) { i--; continue; }
        p.y = hm.heightAtXZ(p.x,p.z);
        rocks.push_back(p);
    }
    for(int i=0;i<12;i++){
        Vec3 p{posd(rng),0,posd(rng)};
        if(len(Vec3{p.x,0,p.z}) < 7.0f) { i--; continue; }
        p.y = hm.heightAtXZ(p.x,p.z);
        flags.push_back(p);
    }


    Vec3 shipPos{0.0f, 12.0f, 22.0f};
    Vec3 shipVel{0,0,0};
    float shipYaw = 0.0f;

    float shipPitch = 0.0f;

    bool aimCam = false;
    bool wobble = true;

    Vec3 camPos = shipPos + Vec3{0,6,12};

    std::vector<Package> packages;


    Vec3 lightDir = norm(Vec3{0.5f, -1.0f, 0.25f});
    Vec3 lightColor{1.0f, 0.98f, 0.92f};
    Vec3 ambient{0.22f, 0.22f, 0.25f};

    double lastT = glfwGetTime();

    while(!glfwWindowShouldClose(win)){
        double now = glfwGetTime();
        float dt = (float)std::min(0.033, now - lastT);
        lastT = now;


        if(input.toggleCam){ aimCam = !aimCam; input.toggleCam=false; }
        if(input.toggleWobble){ wobble = !wobble; input.toggleWobble=false; }


        float yawSpeed = 1.35f;
        float pitchSpeed = 1.1f;
        if(input.keys[GLFW_KEY_LEFT])  shipYaw += yawSpeed*dt;
        if(input.keys[GLFW_KEY_RIGHT]) shipYaw -= yawSpeed*dt;
        if(input.keys[GLFW_KEY_UP])    shipPitch += pitchSpeed*dt;
        if(input.keys[GLFW_KEY_DOWN])  shipPitch -= pitchSpeed*dt;
        shipPitch = clampf(shipPitch, -0.7f, 0.7f);

        Mat4 shipR = mul(rotY(shipYaw), rotX(shipPitch));

        Vec3 fwd = norm(mulDir(shipR, Vec3{0,0,-1}));
        Vec3 right = norm(mulDir(shipR, Vec3{1,0,0}));
        Vec3 up = norm(mulDir(shipR, Vec3{0,1,0}));


        Vec3 acc{0,0,0};
        float accel = 18.0f;
        if(input.keys[GLFW_KEY_W]) acc = acc + fwd*accel;
        if(input.keys[GLFW_KEY_S]) acc = acc - fwd*accel;
        if(input.keys[GLFW_KEY_D]) acc = acc + right*accel;
        if(input.keys[GLFW_KEY_A]) acc = acc - right*accel;
        if(input.keys[GLFW_KEY_R]) acc = acc + Vec3{0,1,0}*accel;
        if(input.keys[GLFW_KEY_F]) acc = acc - Vec3{0,1,0}*accel;

        shipVel = shipVel + acc*dt;
        shipVel = shipVel * std::pow(0.86f, dt*60.0f);
        shipPos = shipPos + shipVel*dt;


        shipPos.x = clampf(shipPos.x, -38.0f, 38.0f);
        shipPos.z = clampf(shipPos.z, -38.0f, 38.0f);
        shipPos.y = clampf(shipPos.y, 6.0f, 24.0f);

        float shipGround = hm.heightAtXZ(shipPos.x, shipPos.z);
        shipPos.y = std::max(shipPos.y, shipGround + 8.0f);
        shipPos.y = std::min(shipPos.y, 24.0f);


        if(input.drop){
            input.drop=false;
            Package p{};
            p.pos = shipPos - up*2.2f;

            float pg = hm.heightAtXZ(p.pos.x, p.pos.z);
            if(p.pos.y < pg + 0.6f) p.pos.y = pg + 0.6f;
            p.vel = shipVel - up*1.2f;
            packages.push_back(p);
        }


        Vec3 g{0,-18.0f,0};
        for(auto& p: packages){
            if(!p.alive) continue;
            p.vel = p.vel + g*dt;
            p.pos = p.pos + p.vel*dt;


            for(auto& h: houses){
                if(h.delivered) continue;
                Vec3 d = p.pos - h.pos;
                if(std::fabs(d.x) < h.half.x && std::fabs(d.z) < h.half.z && p.pos.y < (h.pos.y + h.half.y*2.2f)){
                    h.delivered = true;
                    p.alive = false;
                    break;
                }
            }
            if(!p.alive) continue;

            float ground = hm.heightAtXZ(p.pos.x, p.pos.z);
            if(p.pos.y <= ground + 0.05f){
                p.alive = false;
            }
        }
        packages.erase(std::remove_if(packages.begin(), packages.end(), [](const Package& p){ return !p.alive; }), packages.end());


        int wW=0,hH=0;
        glfwGetFramebufferSize(win, &wW, &hH);
        float aspect = (hH>0)? (float)wW/(float)hH : 1.0f;

        Vec3 camTarget = shipPos + fwd*4.0f + Vec3{0,1.5f,0};
        Vec3 desiredCamPos;
        Vec3 viewUp{0,1,0};
        float fov = 60.0f*(float)M_PI/180.0f;
        if(!aimCam){
            desiredCamPos = shipPos - fwd*12.0f + Vec3{0,7.0f,0};
            camPos = lerp(camPos, desiredCamPos, 1.0f - std::exp(-dt*6.0f));
        }else{

            desiredCamPos = shipPos - up*1.35f - fwd*0.15f;
            camPos = lerp(camPos, desiredCamPos, 1.0f - std::exp(-dt*10.0f));


            camTarget = camPos - up*80.0f + fwd*2.0f;


            viewUp = fwd;

            fov = 38.0f*(float)M_PI/180.0f;
        }

        Mat4 V = lookAt(camPos, camTarget, viewUp);
        Mat4 P = perspective(fov, aspect, 0.1f, 220.0f);


        glViewport(0,0,wW,hH);
        glClearColor(0.55f, 0.72f, 0.92f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        glDisable(GL_CULL_FACE);
        glUseProgram(progTer);
        glUniformMatrix4fv(glGetUniformLocation(progTer,"uView"), 1, GL_FALSE, V.m);
        glUniformMatrix4fv(glGetUniformLocation(progTer,"uProj"), 1, GL_FALSE, P.m);
        glUniform3f(glGetUniformLocation(progTer,"uCamPos"), camPos.x,camPos.y,camPos.z);
        glUniform3f(glGetUniformLocation(progTer,"uLightDir"), lightDir.x,lightDir.y,lightDir.z);
        glUniform3f(glGetUniformLocation(progTer,"uLightColor"), lightColor.x,lightColor.y,lightColor.z);
        glUniform3f(glGetUniformLocation(progTer,"uAmbient"), ambient.x,ambient.y,ambient.z);
        glUniform1f(glGetUniformLocation(progTer,"uHeightScale"), hm.heightScale);
        glUniform1f(glGetUniformLocation(progTer,"uWorldSize"), hm.worldSize);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texHeight);
        glUniform1i(glGetUniformLocation(progTer,"uHeightmap"), 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, texTerrain);
        glUniform1i(glGetUniformLocation(progTer,"uTex"), 1);

        glBindVertexArray(grid.vao);
        glDrawElements(GL_TRIANGLES, grid.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
        glUseProgram(0);
        glEnable(GL_CULL_FACE);


        auto drawMesh = [&](const Mesh& m, GLuint tex, Mat4 M, Vec3 tint, bool wob){
            glUseProgram(progModel);
            glUniformMatrix4fv(glGetUniformLocation(progModel,"uModel"), 1, GL_FALSE, M.m);
            glUniformMatrix4fv(glGetUniformLocation(progModel,"uView"), 1, GL_FALSE, V.m);
            glUniformMatrix4fv(glGetUniformLocation(progModel,"uProj"), 1, GL_FALSE, P.m);
            glUniform1i(glGetUniformLocation(progModel,"uWobble"), wob?1:0);
            glUniform1f(glGetUniformLocation(progModel,"uTime"), (float)now);
            glUniform3f(glGetUniformLocation(progModel,"uCamPos"), camPos.x,camPos.y,camPos.z);
            glUniform3f(glGetUniformLocation(progModel,"uLightDir"), lightDir.x,lightDir.y,lightDir.z);
            glUniform3f(glGetUniformLocation(progModel,"uLightColor"), lightColor.x,lightColor.y,lightColor.z);
            glUniform3f(glGetUniformLocation(progModel,"uAmbient"), ambient.x,ambient.y,ambient.z);
            glUniform3f(glGetUniformLocation(progModel,"uTint"), tint.x,tint.y,tint.z);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glUniform1i(glGetUniformLocation(progModel,"uTex"), 0);

            glBindVertexArray(m.vao);
            glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);
            glBindVertexArray(0);
            glUseProgram(0);
        };

        auto drawInstancedBatch = [&](GLuint tex, const std::vector<InstanceData>& instances, bool wob){
            if(!glDrawElementsInstanced_ || !glVertexAttribDivisor_){

                for(const auto& it : instances){
                    Mat4 M{}; std::memcpy(M.m, it.m, sizeof(it.m));
                    drawMesh(cube, tex, M, {it.tint[0], it.tint[1], it.tint[2]}, wob);
                }
                return;
            }

            GLsizei count = uploadInstances(cubeInst, instances);
            if(count <= 0) return;

            glUseProgram(progInst);
            glUniformMatrix4fv(glGetUniformLocation(progInst,"uView"), 1, GL_FALSE, V.m);
            glUniformMatrix4fv(glGetUniformLocation(progInst,"uProj"), 1, GL_FALSE, P.m);
            glUniform1i(glGetUniformLocation(progInst,"uWobble"), wob?1:0);
            glUniform1f(glGetUniformLocation(progInst,"uTime"), (float)now);
            glUniform3f(glGetUniformLocation(progInst,"uCamPos"), camPos.x,camPos.y,camPos.z);
            glUniform3f(glGetUniformLocation(progInst,"uLightDir"), lightDir.x,lightDir.y,lightDir.z);
            glUniform3f(glGetUniformLocation(progInst,"uLightColor"), lightColor.x,lightColor.y,lightColor.z);
            glUniform3f(glGetUniformLocation(progInst,"uAmbient"), ambient.x,ambient.y,ambient.z);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glUniform1i(glGetUniformLocation(progInst,"uTex"), 0);

            glBindVertexArray(cubeInst.vao);
            glDrawElementsInstanced_(GL_TRIANGLES, cubeInst.indexCount, GL_UNSIGNED_INT, 0, count);
            glBindVertexArray(0);
            glUseProgram(0);
        };


        {
            float gy = hm.heightAtXZ(0,0);
            Mat4 T = translate({0, gy+1.1f, 0});
            Mat4 S = scale({0.9f, 1.6f, 0.9f});
            drawMesh(cyl, texTree, mul(T,S), {1,1,1}, false);
            Mat4 T2 = translate({0, gy+3.4f, 0});
            Mat4 S2 = scale({2.2f, 2.8f, 2.2f});
            drawMesh(cone, texTree, mul(T2,S2), {1,1,1}, wobble);
        }


        std::vector<InstanceData> instHouses;
        std::vector<InstanceData> instRoofs;
        instHouses.reserve(houses.size());
        instRoofs.reserve(houses.size());

        for(const auto& h: houses){
            Vec3 tint = h.delivered ? Vec3{0.55f, 1.05f, 0.55f} : Vec3{1,1,1};
            Mat4 T = translate({h.pos.x, h.pos.y + h.half.y, h.pos.z});
            Mat4 R = rotY(h.yaw);
            Mat4 S = scale({h.half.x*2.0f, h.half.y*2.0f, h.half.z*2.0f});
            Mat4 M = mul(T, mul(R,S));

            InstanceData id{};
            std::memcpy(id.m, M.m, sizeof(id.m));
            id.tint[0]=tint.x; id.tint[1]=tint.y; id.tint[2]=tint.z; id.tint[3]=1.0f;
            instHouses.push_back(id);

            Mat4 T2 = translate({h.pos.x, h.pos.y + h.half.y*2.2f, h.pos.z});
            Mat4 S2 = scale({h.half.x*2.3f, h.half.y*0.9f, h.half.z*2.3f});
            Mat4 M2 = mul(T2, mul(R,S2));

            InstanceData roof{};
            std::memcpy(roof.m, M2.m, sizeof(roof.m));
            roof.tint[0]=1.05f; roof.tint[1]=0.75f; roof.tint[2]=0.75f; roof.tint[3]=1.0f;
            instRoofs.push_back(roof);
        }

        drawInstancedBatch(texHouse, instHouses, false);
        drawInstancedBatch(texHouse, instRoofs, false);


        std::vector<InstanceData> instRocks;
        instRocks.reserve(rocks.size());
        for(auto p: rocks){
            float s = 0.6f;
            Mat4 T = translate({p.x, p.y + 0.25f, p.z});
            Mat4 S = scale({s, 0.5f, s});
            Mat4 M = mul(T,S);

            InstanceData id{};
            std::memcpy(id.m, M.m, sizeof(id.m));
            id.tint[0]=0.9f; id.tint[1]=0.9f; id.tint[2]=1.0f; id.tint[3]=1.0f;
            instRocks.push_back(id);
        }
        drawInstancedBatch(texDeco, instRocks, false);


        std::vector<InstanceData> instPoles;
        std::vector<InstanceData> instCloth;
        instPoles.reserve(flags.size());
        instCloth.reserve(flags.size());
        for(auto p: flags){
            Mat4 Tp = translate({p.x, p.y + 1.2f, p.z});
            Mat4 Sp = scale({0.12f, 2.4f, 0.12f});
            Mat4 Mp = mul(Tp,Sp);

            InstanceData pole{};
            std::memcpy(pole.m, Mp.m, sizeof(pole.m));
            pole.tint[0]=1.0f; pole.tint[1]=1.0f; pole.tint[2]=1.0f; pole.tint[3]=1.0f;
            instPoles.push_back(pole);

            Mat4 Tc = translate({p.x+0.65f, p.y + 2.0f, p.z});
            Mat4 Sc = scale({1.3f, 0.55f, 0.06f});
            Mat4 Mc = mul(Tc,Sc);

            InstanceData cloth{};
            std::memcpy(cloth.m, Mc.m, sizeof(cloth.m));
            cloth.tint[0]=1.0f; cloth.tint[1]=1.0f; cloth.tint[2]=1.0f; cloth.tint[3]=1.0f;
            instCloth.push_back(cloth);
        }
        drawInstancedBatch(texDeco, instPoles, false);
        drawInstancedBatch(texBlimp, instCloth, wobble);

        {
            Mat4 T = translate(shipPos);
            Mat4 R = shipR;
            Mat4 Body = mul(T, mul(R, scale({3.2f, 1.25f, 1.25f})));
            drawMesh(sphere, texBlimp, Body, Vec3{1,1,1}, false);

            Mat4 Gond = mul(T, mul(R, mul(translate({0,-1.3f,0}), scale({1.4f,0.45f,0.55f}))));
            drawMesh(cube, texBlimp, Gond, Vec3{1.0f,1.0f,1.0f}, false);

            Mat4 Fin1 = mul(T, mul(R, mul(translate({-2.2f, 0.0f, 0}), scale({0.12f,0.7f,0.9f}))));
            Mat4 Fin2 = mul(T, mul(R, mul(translate({ 2.2f, 0.0f, 0}), scale({0.12f,0.7f,0.9f}))));
            Mat4 Fin3 = mul(T, mul(R, mul(translate({ 0.0f, 0.4f, 1.05f}), scale({0.8f,0.15f,0.5f}))));
            drawMesh(cube, texBlimp, Fin1, Vec3{1,1,1}, false);
            drawMesh(cube, texBlimp, Fin2, Vec3{1,1,1}, false);
            drawMesh(cube, texBlimp, Fin3, Vec3{1,1,1}, false);
        }


        glDisable(GL_CULL_FACE);
        for(const auto& p: packages){
            Mat4 T = translate({p.pos.x, p.pos.y, p.pos.z});
            Mat4 S = scale({0.35f,0.35f,0.35f});
            drawMesh(cube, texPack, mul(T,S), Vec3{1,1,1}, false);
        }

        glEnable(GL_CULL_FACE);
        glfwSwapBuffers(win);
        glfwPollEvents();
    }


    destroyMesh(cube);
    destroyMesh(sphere);
    destroyMesh(cone);
    destroyMesh(cyl);
    destroyMesh(grid);
    destroyInstancedMesh(cubeInst);

    GLuint texs[] = {texHeight, texTerrain, texBlimp, texTree, texHouse, texDeco, texPack};
    glDeleteTextures(7, texs);
    glDeleteProgram(progModel);
    glDeleteProgram(progInst);
    glDeleteProgram(progTer);

    glfwDestroyWindow(win);
    glfwTerminate();
    return 0;
}
