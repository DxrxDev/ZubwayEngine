#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <cstring>
#include <chrono>

using std::string;

#include <stdlib.h>

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

/*
void AddTriangle( Vector3 points[3], uint32_t trs, std::vector<Vertex>& verts, std::vector<uint16_t>& inds ){
    uint32_t offset = verts.size();
    for (uint32_t i = 0; i < 3; ++i){
        verts.push_back({ points[i], trs });
    }
    inds.push_back( offset + 0 );
    inds.push_back( offset + 1 );
    inds.push_back( offset + 2 );
}
void AddTriangleFancy( float x, float y, float angle1, float angle2, float side1, uint32_t trs, std::vector<Vertex>& verts, std::vector<uint16_t>& inds ){
    float angle3 = 180.f - (angle1 + angle2);
    float h = sinf( angle2 * DEG2RAD ) * side1 / sinf( angle3 * DEG2RAD );

    float u = h * cosf(angle1 * DEG2RAD);
    float v = h * sinf(angle1 * DEG2RAD);
    uint32_t offset = verts.size();

    float centreX = (side1/2.0);
    float centreY = v / 2.0f;

    verts.push_back({{x         - centreX, y + centreY,     0.0f}, trs});
    verts.push_back({{x + u     - centreX, y + centreY - v, 0.0f}, trs});
    verts.push_back({{x + side1 - centreX, y + centreY,     0.0f}, trs});

    inds.push_back( offset + 0 );
    inds.push_back( offset + 1 );
    inds.push_back( offset + 2 );
}
*/

namespace ZE {
    namespace Visual {
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
        DrawQueue CreateDrawQueue(GraphicsWindow *wnd, std::vector<Vertex> verts, std::vector<uint16_t> inds, size_t vr, size_t ir){
            return (DrawQueue){verts, inds,
                new VertexBuffer( wnd, verts, vr ),
            new IndexBuffer( wnd, inds.data(), inds.size(), ir )};
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
    };
};


Image cptoimg(cp_image_t i){
    return (Image){
        (Image::Pixel *)i.pix,
        (uint32_t)i.w,
        (uint32_t)i.h
    };
}

int main( void ){
    std::cout << "Hello World" << std::endl;

    const char *appName = "Stones To Bridges"; 

    InitWindowSystem( );
    InitGraphicsSystem( appName );

    GraphicsWindow wnd(600, 400, appName, Window::CreationFlags::None);

    Image cpI = cptoimg(cp_load_png("STB.png"));
    
    VulkanImage vi(&wnd, cpI.width, cpI.height, (Image::Pixel*)cpI.data);

	std::vector<Vertex> verts;
    std::vector<uint16_t> inds;

    /* Generate background */
    ZE::DataStructures::Grid bgGrid( 25, 25, 1.0f );
    for (uint32_t y = 0; y < bgGrid.GetSize().y; ++y){
        for (uint32_t x = 0; x < bgGrid.GetSize().x; ++x){
            ZE::Visual::AddQuad(
                bgGrid.ToBox2D(x, y),
                {0.0f, 0.0, 0.25f, 0.25f },
                0, 0, verts, inds
            );
        }
    }

    ZE::Visual::DrawQueue dq = ZE::Visual::CreateDrawQueue(&wnd, verts, inds, 0, 0);

    ZE::Visual::DrawQueue FellaDQ { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
    ZE::Visual::AddQuad(
        (Box2D){0, 0, 2, 2}, (Box2D){0.75, 0.0, 0.25, 0.25},
        0, 1, FellaDQ.verts, FellaDQ.inds
    );
    FellaDQ = ZE::Visual::CreateDrawQueue(
        &wnd, FellaDQ.verts, FellaDQ.inds,
        0, 0
    );

    ZE::Visual::DrawQueue TreeDQ { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
    ZE::Visual::AddQuad(
        (Box2D){0, 0, 1, 1}, (Box2D){0.25, 0.0, 0.25, 0.25},
        0, 2, TreeDQ.verts, TreeDQ.inds
    );
    ZE::Visual::AddQuad(
        (Box2D){0, 0, 1, 1}, (Box2D){0.25, 0.0, 0.25, 0.25},
        0, 3, TreeDQ.verts, TreeDQ.inds
    );
    TreeDQ = ZE::Visual::CreateDrawQueue(
        &wnd, TreeDQ.verts, TreeDQ.inds,
        0, 0
    );
    
    ZE::Camera::ProjectionCamera cam(
        PI / 3.0,
        SCREEN_WIDTH / SCREEN_HEIGHT
    );
    cam.SetPos({0, -5.0, 0});
    
    std::vector<MVP> mvps = {
        {
            MatrixRotateX( PI/2.0f ),
            cam.GetView(), cam.GetProj()
        },
        {
            MatrixTranslate(0, -0.5, -10.0),
            cam.GetView(), cam.GetProj()
        },
        {
            MatrixTranslate(3, -0.5, -12.0),
            cam.GetView(), cam.GetProj()
        },
        {
            MatrixTranslate(2, -0.5, -10.0),
            cam.GetView(), cam.GetProj()
        }
    };
    UniformBuffer2 ub1( &wnd, mvps.size() );

    ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );
    
    bool running = true;
    std::chrono::time_point<std::chrono::high_resolution_clock> t1, t2;
    float desiredMilliCount = 16.667;
    float deltaT = 0;
    millisecondsSinceLastFrame = 0;

    std::cout << "Entering main loop !!!" << std::endl;

    float groundRot = 0.0f;
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

        
        cam.SetPos((Vector3){cposx, -5.0, cposy});
        for (auto& mvp : mvps){
            mvp.view = cam.GetView();
        };
        mvps[1].model = MatrixTranslate(phillaPos.x, phillaPos.y, phillaPos.z);

        ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );
        
        //////////////////////////////////////
        /* =====( GRAPHICS PROCESSING )=====*/
        //////////////////////////////////////
        std::vector<std::pair<VertexBuffer *, IndexBuffer *>> vis = {
            {dq.vb, dq.ib},
            {FellaDQ.vb, FellaDQ.ib},
            {TreeDQ.vb, TreeDQ.ib}
        };
        wnd.DrawIndexed5( vis, ub1, vi );
        t2 = std::chrono::high_resolution_clock::now();

        millisecondsSinceLastFrame =
            std::chrono::duration< float, std::milli> (t2 - t1).count();
        deltaT = millisecondsSinceLastFrame / desiredMilliCount;
    }

    std::cout << "Quitting Pong !!!" << std::endl;
    return 0;
}