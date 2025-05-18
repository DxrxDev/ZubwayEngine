#include <cmath>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <limits>
//#include <memory> Currently unused
#include <queue>
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

GraphicsWindow *mainWnd;

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
    mainWnd = &wnd;

    Image cpI = cptoimg(cp_load_png("STB.png"));
    VulkanImage vi(&wnd, cpI.width, cpI.height, (Image::Pixel*)cpI.data);

    ZE::DataStructures::IDManager<1024> transformIDs; transformIDs.GenerateID(); // index 0 will be an identity matrix
    std::vector<MVP> mvps(1024);

    class Map{
        public:
            Map(uint32_t trsid, MVP *mvps) : trans(mvps){
                dq = {
                    std::vector<Vertex>(),
                    std::vector<uint16_t>()
                };
        
                ZE::DataStructures::Grid bgGrid( 25, 25, 1.0f );
                for (uint32_t y = 0; y < bgGrid.GetSize().y; ++y){
                    for (uint32_t x = 0; x < bgGrid.GetSize().x; ++x){
                        ZE::Visual::AddQuad(
                            bgGrid.ToBox2D(x, y),
                            ZE::Visual::TextureMapToBox2D({8, 8}, 0, 0),
                            0, trsid, MatrixRotateX( PI/2.0f ),
                            dq.verts, dq.inds
                        );
                    }
                }
                ZE::Visual::CreateDrawQueue(
                    mainWnd, dq,
                    0, 0
                );
            }
        
            MVP *trans;
            ZE::Visual::DrawQueue dq;
        };
        
    Map map(0, mvps.data());

    class Trees{
    public:
        Trees(MVP *mvps) : mvps(mvps){
            numtrees = 0;
            maxnumtrees = (0xff + 1);
            dq = { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
            ZE::Visual::CreateDrawQueue(
                mainWnd, dq,
                4 * 0xff, 6 * 0xff
            );
        }
        const char *AddTree(Vector3 pos){
            uint64_t treeid = trees.GenerateID();
            treestates.push((state){treeid, 0.0, 0.0, pos});
            ZE::Visual::AddQuad(
                (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({8, 8}, 5, 0),
                0, 0, MatrixTranslate(pos.x, pos.y, pos.z),
                dq.verts, dq.inds
            );
            dq.vb->UpdateMemory(dq.verts.data(), dq.verts.size(), 0);
            dq.ib->UpdateMemory(dq.inds.data(), dq.inds.size(), 0);

            return nullptr;
        }
        void Update(){
            state& s = treestates.front();
            while (true){
                break;
            }
        }

        struct state {
            uint64_t id;
            float age;
            float fruitcycle;
            Vector3 pos;
        };

        uint8_t maxnumtrees;
        uint8_t numtrees;
        ZE::DataStructures::IDManager<0xff + 1> trees;
        std::queue<state> treestates;

        ZE::Visual::DrawQueue dq;
        ZE::DataStructures::IDManager<1024> *idman;
        MVP *mvps;
    };
    Trees trees(mvps.data());
    trees.AddTree(Vector3{3, -0.5, -12.0});
    trees.AddTree(Vector3{2, -0.5, 10.0});
    trees.AddTree(Vector3{5, -0.5, -3.0});
    trees.AddTree(Vector3{-5, -0.5, 2.0});
    trees.AddTree(Vector3{2, -0.5, -5.0});
    trees.AddTree(Vector3{-7, -0.5, -10.0});
    trees.AddTree(Vector3{0, -0.5, 4.0});

    class Fella {
        public:
            Fella( const char *name, ZE::DataStructures::IDManager<1024> *idman, MVP *mvps ): name(name), idman(idman), mvps(mvps), energy(100.0){
                trsid = idman->GenerateID();
                dq = { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
                ZE::Visual::AddQuad(
                    (Box2D){0, 0, 2, 2}, (Box2D)ZE::Visual::TextureMapToBox2D({8, 8}, 5, 3),
                    0, trsid, dq.verts, dq.inds
                );
                ZE::Visual::CreateDrawQueue(
                    mainWnd, dq,
                    0, 0
                );
                pos = {0, -0.5, -10.0};
                targetPos = (Vector3){ ((float)GetRandom()/100.0f), -0.5, (float)GetRandom()/100.0f};
                foodsearching = false;
            }
            ~Fella(){
                idman->RemoveID(trsid);
                delete dq.vb;
                delete dq.ib;
            }
    
            void Update( const std::vector<Trees::state>& treestates ){
                if (energy < 50.0f && foodsearching == false){
                    foodsearching = true;

                    // Find closest tree
                    std::vector<Vector3> treepositions;
                    for (Trees::state s: treestates){
                        treepositions.push_back(s.pos);
                    }
                    int64_t indexOfClosestTree = -1;
                    float closestTreeDist = std::numeric_limits<float>::max();
                    for (int i = 0; i < treepositions.size(); ++i){
                        float cdist = sqrt(
                            pow(pos.x - treepositions[i].x, 2) + 
                            pow(pos.y - treepositions[i].y, 2) + 
                            pow(pos.z - treepositions[i].z, 2)
                        );
                    
                        if (cdist < closestTreeDist){
                            closestTreeDist = cdist;
                            indexOfClosestTree = i;
                        }
                    }
                    // move towards the tree
                    targetPos = treepositions[indexOfClosestTree];
                }
                
                float
                dx = targetPos.x - pos.x,
                dz = targetPos.z - pos.z
                ;
                float len = ((dx * dx) + (dz * dz));
                len = (float)((uint64_t)(len * 100)) / 100.0;

                printf("omg hiiii %d %f %f %f\n", trsid, targetPos.x, targetPos.z, len);
                
                if (len > 0){
                    len = sqrt(len);
                    dx /= len * 60;
                    dz /= len * 60;

                    pos = Vector3Add(pos, {dx, 0, dz});

                    energy -= 5.0 / 60.0;

                    mvps[trsid].model = MatrixTranslate( pos.x, pos.y, pos.z );
                }
                else {
                    targetPos =  (Vector3){ (((float)GetRandom()/50.0f)) - 10.0f, -0.5, ((float)GetRandom()/50.0f) - 10.0f};
                    if (foodsearching){
                        foodsearching = false;
                        energy = 100.0f;
                    }
                }
            }
    
            const char *name;
            Vector3 pos;
            Vector3 targetPos;
            float energy;
            bool foodsearching;
    
            ZE::Visual::DrawQueue dq;
            ZE::DataStructures::IDManager<1024> *idman;
            MVP *mvps;
            uint32_t trsid;
        };
        Fella philla( "philla", &transformIDs, mvps.data() );
        Fella merrin( "merrin", &transformIDs, mvps.data() );

    ZE::Visual::DrawQueue ApplesDQ {std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr}; 
    ZE::Visual::CreateDrawQueue(&wnd, ApplesDQ, 4, 6);

    uint32_t AppleTransformID = transformIDs.GenerateID();
    ZE::Visual::AddQuad(
        {0, 0, 3, 3}, ZE::Visual::TextureMapToBox2D({8, 8}, 4, 1),
        0, AppleTransformID, MatrixTranslate(-1, -0.5, -10),
        ApplesDQ.verts, ApplesDQ.inds
    );
    
    ZE::Visual::DrawQueue ProgressBar {std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr};
    uint32_t bartr1 = transformIDs.GenerateID();
    uint32_t bartr2 = transformIDs.GenerateID();
    ZE::Visual::AddQuad(
        (Box2D){0, 0, 0.6, 0.2},
        ZE::Visual::TextureMapToBox2D({128, 8}, 0, 7 ),
        0, bartr1, ProgressBar.verts, ProgressBar.inds
    );
    ZE::Visual::AddQuad(
        (Box2D){0, 0, 0.6, 0.2},
        ZE::Visual::TextureMapToBox2D({128, 8}, 1, 7 ),
        0, bartr2, ProgressBar.verts, ProgressBar.inds
    );
    ZE::Visual::CreateDrawQueue(&wnd, ProgressBar, 0, 0);

    ZE::Camera::ProjectionCamera cam(
        PI / 3.0,
        SCREEN_WIDTH / SCREEN_HEIGHT
    );
    cam.SetPos({0, -5.0, 0});
    
    mvps[0] = {MatrixIdentity()};
    mvps[philla.trsid] = {MatrixTranslate(0, -0.5, -10.0)};
    mvps[AppleTransformID] = {MatrixIdentity()};
    mvps[bartr1] = {MatrixTranslate(0, -0.5, -10.0)};
    mvps[bartr2] = {MatrixTranslate(0, -0.5, -10.0)};

    UniformBuffer ub1( &wnd, 1024 );

    ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );

    bool running = true;
    std::chrono::time_point<std::chrono::high_resolution_clock> t1, t2;
    float desiredMilliCount = 16.667;
    float deltaT = 0;
    millisecondsSinceLastFrame = 0;

    std::cout << "Entering main loop !!!" << std::endl;

    float cposx = 0, cposy = 0;
    
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

        cam.SetPos((Vector3){cposx, -5.0, cposy});

        mvps[bartr1].model = MatrixTranslate(philla.pos.x, philla.pos.y - 1.0, philla.pos.z);

        float bartr2offset = 
            (1.0 - (philla.energy / 100.0f)) * 0.3
        ;
        mvps[bartr2].model = MatrixScale(philla.energy / 100.0, 1, 1) * MatrixTranslate(philla.pos.x - bartr2offset, philla.pos.y - 1.0, philla.pos.z);

        std::vector<Trees::state> treestates;
        std::queue<Trees::state> treesCopy = trees.treestates;
        while (true){
            Trees::state s = treesCopy.front();
            treestates.push_back(s);
            treesCopy.pop();
            if (treesCopy.size() == 0){    
                break;
            }
        }

        philla.Update( treestates );
        merrin.Update( treestates );

        ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );
        
        //////////////////////////////////////
        /* =====( GRAPHICS PROCESSING )=====*/
        //////////////////////////////////////
        std::vector<std::pair<VertexBuffer *, IndexBuffer *>> vis = {
            {map.dq.vb, map.dq.ib},
            {trees.dq.vb, trees.dq.ib},
            {philla.dq.vb, philla.dq.ib},
            {merrin.dq.vb, merrin.dq.ib},
            {ApplesDQ.vb, ApplesDQ.ib},
            {ProgressBar.vb, ProgressBar.ib},
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