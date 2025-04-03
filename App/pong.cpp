#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <limits>
#include <fstream>
#include <cstring>
#include <chrono>
#include <thread>

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

void AddRectangle( Box2D rect, uint32_t trs, std::vector<Vertex>& verts, std::vector<uint16_t>& inds ){
    float hw = rect.w / 2.0f, hh = rect.h / 2.0f;
    uint32_t offset = verts.size();
    verts.push_back({{rect.x - hw, rect.y - hh}, {0.0, 0.0}, trs}); //TL
    verts.push_back({{rect.x + hw, rect.y - hh}, {1.0, 0.0}, trs}); //TR
    verts.push_back({{rect.x - hw, rect.y + hh}, {0.0, 1.0}, trs}); //BL
    verts.push_back({{rect.x + hw, rect.y + hh}, {1.0, 1.0}, trs}); //BR

    inds.push_back( offset + 0 );
    inds.push_back( offset + 3 );
    inds.push_back( offset + 2 );

    inds.push_back( offset + 0 );
    inds.push_back( offset + 1 );
    inds.push_back( offset + 3 );
}

void AddRectangle2( Box2D rect, Box2D tex, uint32_t trs, std::vector<Vertex>& verts, std::vector<uint16_t>& inds ){
    float hw = rect.w / 2.0f, hh = rect.h / 2.0f;
    uint32_t offset = verts.size();
    verts.push_back({{rect.x - hw, rect.y - hh}, {tex.x, tex.y}, trs}); //TL
    verts.push_back({{rect.x + hw, rect.y - hh}, {tex.x + tex.w, tex.y}, trs}); //TR
    verts.push_back({{rect.x - hw, rect.y + hh}, {tex.x, tex.y + tex.h}, trs}); //BL
    verts.push_back({{rect.x + hw, rect.y + hh}, {tex.x + tex.w, tex.y + tex.h}, trs}); //BR

    inds.push_back( offset + 0 );
    inds.push_back( offset + 3 );
    inds.push_back( offset + 2 );

    inds.push_back( offset + 0 );
    inds.push_back( offset + 1 );
    inds.push_back( offset + 3 );
}

/**
 * @brief 
 * 
 * @param rect 
 * @param tex 
 * @param texRot 
 * @param trs 
 * @param verts 
 * @param inds 
 */
void AddRectangle3( Box2D rect, Box2D tex, uint32_t texRot, uint32_t trs, std::vector<Vertex>& verts, std::vector<uint16_t>& inds ){
    float hw = rect.w / 2.0f, hh = rect.h / 2.0f;
    uint32_t offset = verts.size();

    Vector2 texcorners[4] = {
        {tex.x, tex.y},
        {tex.x + tex.w, tex.y},
        {tex.x, tex.y + tex.h},
        {tex.x + tex.w, tex.y + tex.h}
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


    verts.push_back({{rect.x - hw, rect.y - hh}, texcorners[texrot[0]], trs}); //TL
    verts.push_back({{rect.x + hw, rect.y - hh}, texcorners[texrot[1]], trs}); //TR
    verts.push_back({{rect.x - hw, rect.y + hh}, texcorners[texrot[2]], trs}); //BL
    verts.push_back({{rect.x + hw, rect.y + hh}, texcorners[texrot[3]], trs}); //BR

    inds.push_back( offset + 0 );
    inds.push_back( offset + 3 );
    inds.push_back( offset + 2 );

    inds.push_back( offset + 0 );
    inds.push_back( offset + 1 );
    inds.push_back( offset + 3 );
}

namespace ZE {
    namespace visual {
        /**
         * @brief 
         * Add a Quad to your draw queue
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
    };
};


Image cptoimg(cp_image_t i){
    return (Image){
        (Image::Pixel *)i.pix,
        (uint32_t)i.w,
        (uint32_t)i.h
    };
}

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
        sx = scale;
    }
    Vector2 GetPos( uint32_t x, uint32_t y ){
        float fieldPosOffsetX = (w * sx) / 2.0f;
        float fieldPosOffsetY = (h * sy) / 2.0f;
        return (Vector2){
            x * sx + (sx / 2.0f) - fieldPosOffsetX,
            y * sy + (sy / 2.0f) - fieldPosOffsetY
        };
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

int main( void ){
    std::cout << "Hello World" << std::endl;

    const char *appName = "Pong"; 

    InitWindowSystem( );
    InitGraphicsSystem( appName );

    GraphicsWindow wnd(600, 400, appName, Window::CreationFlags::None);

    Image cpI = cptoimg(cp_load_png("STB.png"));
    
    VulkanImage vi(&wnd, cpI.width, cpI.height, (Image::Pixel*)cpI.data);

	std::vector<Vertex> verts;
    std::vector<uint16_t> inds;

    /* Generate background 
    Grid bgGrid( FEILD_WIDTH, FEILD_HEIGHT, 1.0f );
    for (uint32_t y = 0; y < FEILD_HEIGHT; ++y){
        for (uint32_t x = 0; x < FEILD_WIDTH; ++x){
            AddRectangle2(
                bgGrid.ToBox2D(x, y),
                {0.0f, 0.0, 1.0f / 4.0f, 1.0f/ 4.0f },
                0, verts, inds
            );
        }
    }
    */

    ZE::visual::AddQuad(
        (Box2D){0, 0, 5, 5},
        (Box2D){0, 0, 1, 1},
        0, 0,
        verts, inds
    );

	VertexBuffer vb( &wnd, verts, 0 );
    IndexBuffer ib( &wnd, inds.data(), inds.size(), 0 );

    std::cout << "balls shit fuck: " << verts.size();

    Matrix view = MatrixLookAt(
        {0, 0, 1},
        {0, 0, 0 },
        {0, 1, 0}
    );
    Matrix proj = MatrixOrtho(SCREEN_LEFT, SCREEN_RIGHT, SCREEN_TOP, SCREEN_BOTTOM, 0.1, 10.0);
    
    std::vector<MVP> mvps = {
        {
            MatrixTranslate(0, 0, 0),
            view, proj
        }
    };
    UniformBuffer2 ub1( &wnd, 1 );

    ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );
    

    bool running = true;
    std::chrono::time_point<std::chrono::high_resolution_clock> t1, t2;
    float desiredMilliCount = 16.667;
    float deltaT = 0;
    millisecondsSinceLastFrame = 0;

    std::cout << "Entering main loop !!!" << std::endl;

    while (running){ t1 = std::chrono::high_resolution_clock::now();
        std::vector<WindowEvent> events = wnd.GetEvents();
        running = wnd.IsRunning();
        
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

            /*
            else if (e.type == WindowEventType::MouseMove){
                mvps[1].model = MatrixTranslate(
                    (e.mm.x * (SCREEN_WIDTH / 600.0f)) + SCREEN_LEFT,
                    (e.mm.y * (SCREEN_HEIGHT / 400.0f)) + SCREEN_TOP,
                    0
                );
            }*/
        }
        ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );
        
        std::vector<std::pair<VertexBuffer *, IndexBuffer *>> vis = {
            {&vb, &ib}
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