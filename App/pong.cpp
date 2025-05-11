#include <cmath>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <limits>
//#include <memory> Currently unused
#include <vector>
#include <cstring>
#include <chrono>
#include <cstdlib>

#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_xcb.h>

#include "../ZubwayEngine/zubwayengine.hpp"

float millisecondsSinceLastFrame;
#define SCREEN_LEFT  -15.0f 
#define SCREEN_RIGHT  15.0f
#define SCREEN_TOP   -10.0f
#define SCREEN_BOTTOM 10.0f

#define SCREEN_WIDTH  30.0f
#define SCREEN_HEIGHT 20.0f

#define MAX_TRANSFORMS 1024

namespace ZE {
    namespace Visual {
        struct TextureMap{
            uint32_t numx, numy;

        };
        Box2D TextureMapToBox2D(TextureMap tm, uint32_t x, uint32_t y){
            float
                width = (1.0f / (float)tm.numx), 
                height = (1.0f / (float)tm.numy)
            ;
            return (Box2D){
                x * width, y * height,
                width, height
            };
        }



        struct DrawQueue {
            std::vector<Vertex>   verts;
            std::vector<uint16_t> inds;

        	VertexBuffer *vb;
            IndexBuffer *ib;
        };

        /**
         * @brief Create a Draw Queue object
         * 
         * @param wnd 
         * @param verts 
         * @param inds 
         * @param vr 
         * @param ir 
         * @return DrawQueue 
         */
        void CreateDrawQueue(GraphicsWindow *wnd, DrawQueue& dq, size_t vr, size_t ir){
            dq.vb = new VertexBuffer( wnd, dq.verts, vr );
            dq.ib = new IndexBuffer( wnd, dq.inds.data(), dq.inds.size(), ir );
        }
        void FreeDrawQueue( DrawQueue& dq ){
            if (dq.vb)
                delete dq.vb;
            if (dq.ib)
                delete dq.ib;
        }

        /**
         * @brief Add a Quad to your draw queue
         * @param rect 
         * @param texture 
         * @param texRot 
         * @param transformID 
         * @param verts 
         * @param inds 
         * @return const char* 
         */
        const char *AddQuad(
            Box2D rect, Box2D texture,
            uint32_t texRot, uint32_t transformID,
            std::vector<Vertex>& verts, std::vector<uint16_t>& inds
        ){
            if (transformID >= MAX_TRANSFORMS)
                return "ZE::Visual::AddQuad(): transformID out of bounds!";

            float hw = rect.w / 2.0f, hh = rect.h / 2.0f;
            uint32_t offset = verts.size();
        
            Vector2 texcorners[4] = {
                {texture.x, texture.y},
                {texture.x + texture.w, texture.y},
                {texture.x, texture.y + texture.h},
                {texture.x + texture.w, texture.y + texture.h}
            };

            uint64_t texrot[4];
            switch (texRot % 4){
                default: // FLOWING TO NEXT
                case 0: {
                    texrot[0] = 0;
                    texrot[1] = 1;
                    texrot[2] = 2;
                    texrot[3] = 3;
                } break;
                case 1: {
                    texrot[0] = 2;
                    texrot[1] = 0;
                    texrot[2] = 3;
                    texrot[3] = 1;
                } break;
                case 2: {
                    texrot[0] = 3;
                    texrot[1] = 2;
                    texrot[2] = 1;
                    texrot[3] = 0;
                } break;
                case 3: {
                    texrot[0] = 1;
                    texrot[1] = 3;
                    texrot[2] = 0;
                    texrot[3] = 2;
                } break;
            }
        
        
            verts.push_back({{rect.x - hw, rect.y - hh}, texcorners[texrot[0]], transformID}); //TL
            verts.push_back({{rect.x + hw, rect.y - hh}, texcorners[texrot[1]], transformID}); //TR
            verts.push_back({{rect.x - hw, rect.y + hh}, texcorners[texrot[2]], transformID}); //BL
            verts.push_back({{rect.x + hw, rect.y + hh}, texcorners[texrot[3]], transformID}); //BR


            inds.push_back( offset + 0 );
            inds.push_back( offset + 3 );
            inds.push_back( offset + 2 );

            inds.push_back( offset + 0 );
            inds.push_back( offset + 1 );
            inds.push_back( offset + 3 );

            return nullptr;
        };

        const char *AddQuad(
            Box2D rect, Box2D texture,
            uint32_t texRot, uint32_t transformID, Matrix mat,
            std::vector<Vertex>& verts, std::vector<uint16_t>& inds
        ){
            if (transformID >= MAX_TRANSFORMS)
                return "ZE::Visual::AddQuad(): transformID out of bounds!";
            float hw = rect.w / 2.0f, hh = rect.h / 2.0f;
            uint32_t offset = verts.size();

            Vector2 texcorners[4] = {
                {texture.x, texture.y},
                {texture.x + texture.w, texture.y},
                {texture.x, texture.y + texture.h},
                {texture.x + texture.w, texture.y + texture.h}
            };

            uint64_t texrot[4];
            switch (texRot % 4){
                default: // FLOWING TO NEXT
                case 0: {
                    texrot[0] = 0;
                    texrot[1] = 1;
                    texrot[2] = 2;
                    texrot[3] = 3;
                } break;
                case 1: {
                    texrot[0] = 2;
                    texrot[1] = 0;
                    texrot[2] = 3;
                    texrot[3] = 1;
                } break;
                case 2: {
                    texrot[0] = 3;
                    texrot[1] = 2;
                    texrot[2] = 1;
                    texrot[3] = 0;
                } break;
                case 3: {
                    texrot[0] = 1;
                    texrot[1] = 3;
                    texrot[2] = 0;
                    texrot[3] = 2;
                } break;
            }
        
            Vector3 v1 = (Vector3){rect.x - hw, rect.y - hh, 0} * mat;
            Vector3 v2 = (Vector3){rect.x + hw, rect.y - hh, 0} * mat;
            Vector3 v3 = (Vector3){rect.x - hw, rect.y + hh, 0} * mat;
            Vector3 v4 = (Vector3){rect.x + hw, rect.y + hh, 0} * mat;
            verts.push_back({v1, texcorners[texrot[0]], transformID}); //TL
            verts.push_back({v2, texcorners[texrot[1]], transformID}); //TR
            verts.push_back({v3, texcorners[texrot[2]], transformID}); //BL
            verts.push_back({v4, texcorners[texrot[3]], transformID}); //BR


            inds.push_back( offset + 0 );
            inds.push_back( offset + 3 );
            inds.push_back( offset + 2 );

            inds.push_back( offset + 0 );
            inds.push_back( offset + 1 );
            inds.push_back( offset + 3 );

            return nullptr;
        }

        /**
         * @brief Add a triangle to your draw queue
         * @param points 
         * @param texture 
         * @param trs 
         * @param verts 
         * @param inds 
         */
        void AddTriangle(
            Vector3 points[3], Vector2 texture[3],
            uint32_t trs,
            std::vector<Vertex>& verts, std::vector<uint16_t>& inds
        ){
            uint32_t offset = verts.size();
            for (uint32_t i = 0; i < 3; ++i){
                verts.push_back({ points[i], texture[i], trs });
            }
            inds.push_back( offset + 0 );
            inds.push_back( offset + 1 );
            inds.push_back( offset + 2 );
        }
    };
    namespace Camera {
        class Camera{
        public:
            void SetPos(Vector3 pos){
                this->pos = pos;
            }
        protected:
            Camera(){
                pos = (Vector3){0, 0, 0};
            }

            virtual Matrix GetProj() = 0;
            virtual Matrix GetView() = 0;

            Vector3 pos;
        };
        class ProjectionCamera: public Camera{
        public:
            ProjectionCamera(float fov, float aspect){
                this->fov = fov;
                this->aspect = aspect;
            }

            virtual Matrix GetProj() override{
                float 
                    near = 0.1,
                    far = 5000
                ;
                return MatrixPerspective(fov, aspect, near, far);
            }
            virtual Matrix GetView() override{
                return MatrixLookAt(
                    pos,
                    Vector3Add(pos, {0, 1, -1}),
                    {0, 1, 0}
                );
            }

        private:
            float fov, aspect;
        };
        class OrthroCamera: public Camera{
        public:
            OrthroCamera(Vector2 screenSize){
                this->screenSize = screenSize;
            }

            virtual Matrix GetMatrix(){
                return MatrixOrtho(
                    screenSize.x * -0.5,
                    screenSize.x * 0.5,
                    screenSize.y * -0.5,
                    screenSize.y * 0.5,
                    0.1, 5000.0
                );
            }
        private:
            Vector2 screenSize;
        };
    };
    namespace DataStructures{
        class Grid {
            public:
                Grid(uint32_t width, uint32_t height, float scalex, float scaley){
                    w = width;
                    h = height;
                    sx = scalex;
                    sy = scaley;
                }
                Grid(uint32_t width, uint32_t height, float scale){
                    w = width;
                    h = height;
                    sx = sy = scale;
                }
                Vector2 GetPos( uint32_t x, uint32_t y ){
                    float fieldPosOffsetX = (w * sx) / 2.0f;
                    float fieldPosOffsetY = (h * sy) / 2.0f;
                    return (Vector2){
                        x * sx + (sx / 2.0f) - fieldPosOffsetX,
                        y * sy + (sy / 2.0f) - fieldPosOffsetY
                    };
                }
                Vector2 GetSize(){
                    return (Vector2){(float)w, (float)h};
                }
            
                Box2D ToBox2D( uint32_t x, uint32_t y ){
                    Vector2 r = GetPos( x, y );
                    return (Box2D){
                        r.x, r.y, sx, sy
                    };
                }
            
                Vector2 GetScale(){
                    return (Vector2){sx, sy};
                }
            
            private:
                uint32_t w, h;
                float sx, sy;
        };

        template<size_t size>
        class IDManager{
        public:
            IDManager(){
                if (size % 8)
                    Error() << "IDManager: size must be a multiple of 8...";
                memset(ids, 0, size/8);
            }
            uint64_t GenerateID(){
                for (uint64_t i = 0; i < size; ++i){
                    if (!ids[i]){
                        ids[i] = 1;
                        return i;
                    }
                }
                Error() << "IDManager: No IDs left...";
                return std::numeric_limits<uint64_t>::max();
            }
            bool RemoveID(uint64_t id){
                if (!ids[id]){
                    return 0;
                }
                ids[id] = 0;
                return 1;
            }
            void Reset(){
                memset(ids, 0, size/8);
            }

        private:
            bool ids[size];
        };
    };
};

void ResetRandom( ){
    srand(time(nullptr));
}
double GetRandom(  ){
    return (double)rand() / ((double)RAND_MAX / 1000.0) ;
};


Image cptoimg(cp_image_t i){
    return (Image){
        (Image::Pixel *)i.pix,
        (uint32_t)i.w,
        (uint32_t)i.h
    };
}

int main( void ){
    ResetRandom();
    const char *appName = "Stones To Bridges"; 
    InitWindowSystem( );
    InitGraphicsSystem( appName );

    GraphicsWindow wnd(600, 400, appName, Window::CreationFlags::None);

    Image cpI = cptoimg(cp_load_png("STB.png"));
    VulkanImage vi(&wnd, cpI.width, cpI.height, (Image::Pixel*)cpI.data);

    ZE::DataStructures::IDManager<1024> transformIDs;
    uint64_t
        mapTrsID = transformIDs.GenerateID(),
        phillaTrsID = transformIDs.GenerateID()
    ;
    
    /* =====(Generate Map)===== */
    ZE::Visual::DrawQueue MapDQ = {
        std::vector<Vertex>(),
        std::vector<uint16_t>()
    };
    // Ground
    ZE::DataStructures::Grid bgGrid( 25, 25, 1.0f );
    for (uint32_t y = 0; y < bgGrid.GetSize().y; ++y){
        for (uint32_t x = 0; x < bgGrid.GetSize().x; ++x){
            ZE::Visual::AddQuad(
                bgGrid.ToBox2D(x, y),
                ZE::Visual::TextureMapToBox2D({4, 4}, 0, 0),
                0, mapTrsID, MatrixRotateX( PI/2.0f ),
                MapDQ.verts, MapDQ.inds
            );
        }
    }
    // Trees
    ZE::Visual::AddQuad(
        (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({4, 4}, 1, 0),
        0, mapTrsID, MatrixTranslate(3, -0.5, -12.0),
        MapDQ.verts, MapDQ.inds
    );
    ZE::Visual::AddQuad(
        (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({4, 4}, 1, 0),
        0, mapTrsID, MatrixTranslate(2, -0.5, -10.0),
        MapDQ.verts, MapDQ.inds
    );
    ZE::Visual::CreateDrawQueue(
        &wnd, MapDQ,
        0, 0
    );
    
    ZE::Visual::DrawQueue FellaDQ { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
    ZE::Visual::AddQuad(
        (Box2D){0, 0, 2, 2}, (Box2D)ZE::Visual::TextureMapToBox2D({4, 4}, 3, 0),
        0, phillaTrsID, FellaDQ.verts, FellaDQ.inds
    );
    ZE::Visual::CreateDrawQueue(
        &wnd, FellaDQ,
        0, 0
    );
    
    ZE::Visual::DrawQueue ApplesDQ {std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr}; 
    ZE::Visual::CreateDrawQueue(&wnd, ApplesDQ, 4, 6);

    uint32_t AppleTransformID = transformIDs.GenerateID();
    ZE::Visual::AddQuad(
        {0, 0, 3, 3}, ZE::Visual::TextureMapToBox2D({4, 4}, 0, 1),
        0, AppleTransformID, MatrixTranslate(-1, -0.5, -10),
        ApplesDQ.verts, ApplesDQ.inds
    );
    
    double randomNum = GetRandom(); // Random number between 0 .. 1000


    
    ZE::Camera::ProjectionCamera cam(
        PI / 3.0,
        SCREEN_WIDTH / SCREEN_HEIGHT
    );
    cam.SetPos({0, -5.0, 0});
    
    std::vector<MVP> mvps = {
        {
            MatrixIdentity()
        },
        {
            MatrixTranslate(0, -0.5, -10.0),
        },
        {
            MatrixIdentity()
        }
    };
    UniformBuffer ub1( &wnd, mvps.size() );

    ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );

    bool running = true;
    std::chrono::time_point<std::chrono::high_resolution_clock> t1, t2;
    float desiredMilliCount = 16.667;
    float deltaT = 0;
    millisecondsSinceLastFrame = 0;

    std::cout << "Entering main loop !!!" << std::endl;

    float cposx = 0, cposy = 0;

    std::vector<Vector3> TreePositions = {
        {3, -0.5, -12},
        {2, -0.5, -10},
    };

    Vector3 phillaPos = {
        0, -0.5, -10.0
    };


    while (running){ t1 = std::chrono::high_resolution_clock::now();
        std::vector<WindowEvent> events = wnd.GetEvents();
        running = wnd.IsRunning();

        ////////////////////////////
        /* =====(LOCIG STUFF)=====*/
        ////////////////////////////
        for (WindowEvent e : events){
            if (e.type == WindowEventType::Key) {
                if (e.key.pressed){
                    switch( e.key.key ){
                    case 'q':{
                        running = false;
                    } break;
                    }
                }
            }
        }

        if (wnd.IsPressed('w')){
            cposy -= 0.1;
        }
        if (wnd.IsPressed('s')){
            cposy += 0.1;
        }
        if (wnd.IsPressed('a')){
            cposx -= 0.1;
        }
        if (wnd.IsPressed('d')){
            cposx += 0.1;
        }

        if (wnd.IsPressed('1')){
            ApplesDQ.vb->UpdateMemory(ApplesDQ.verts.data(), 4, 0);
            ApplesDQ.ib->UpdateMemory(ApplesDQ.inds.data(), 6, 0);
        }

        // Find closest tree
        int64_t indexOfClosestTree = -1;
        float closestTreeDist = std::numeric_limits<float>::max();
        for (int i = 0; i < TreePositions.size(); ++i){
            float cdist = sqrt(
                pow(phillaPos.x - TreePositions[i].x, 2) + 
                pow(phillaPos.y - TreePositions[i].y, 2) + 
                pow(phillaPos.z - TreePositions[i].z, 2)
            );

            if (cdist < closestTreeDist){
                closestTreeDist = cdist;
                indexOfClosestTree = i;
            }
        }

        Vector3 tp = TreePositions[indexOfClosestTree];
        float
            dx = tp.x - phillaPos.x,
            dz = tp.z - phillaPos.z
        ;
        float len = (dx * dx) + (dz * dz);
        if (len > 0){
            len = sqrt(len);
            dx /= len * 100;
            dz /= len * 100;
        }
        phillaPos = Vector3Add(phillaPos, {dx, 0, dz});

        cam.SetPos((Vector3){cposx, -5.0, cposy});
        mvps[phillaTrsID].model = MatrixTranslate(phillaPos.x, phillaPos.y, phillaPos.z);

        ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );
        
        //////////////////////////////////////
        /* =====( GRAPHICS PROCESSING )=====*/
        //////////////////////////////////////
        std::vector<std::pair<VertexBuffer *, IndexBuffer *>> vis = {
            {MapDQ.vb, MapDQ.ib},
            {FellaDQ.vb, FellaDQ.ib},
            {ApplesDQ.vb, ApplesDQ.ib},
        };
        wnd.DrawIndexed( vis, ub1, vi, cam.GetView() * cam.GetProj() );
        t2 = std::chrono::high_resolution_clock::now();

        millisecondsSinceLastFrame =
            std::chrono::duration< float, std::milli> (t2 - t1).count();
        deltaT = millisecondsSinceLastFrame / desiredMilliCount;
    }

    std::cout << "Quitting Pong !!!" << std::endl;
    return 0;
}