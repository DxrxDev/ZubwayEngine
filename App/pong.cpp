#include <algorithm>
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

    DepthStencil sd(&wnd);

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
                            ZE::Visual::TextureMapToBox2D({8, 8}, 0, 2),
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

    // struct Fruit{
    //     float ripeness; // 0 = seeds, 1.0 = flower, 2.0 = bulb, 3.0 = hard = 4.0 ripe, 5 = mushy, <5 = ew
    //     bool ontree;
    // };
    class Trees{
    public:
        Trees(MVP *mvps) : mvps(mvps){
            highestid = nullptr;
            maxnumtrees = 100;
            dq = { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
            ZE::Visual::CreateDrawQueue(
                mainWnd, dq,
                4 * maxnumtrees, 6 * maxnumtrees
            );
        }
        const char *AddTree(Vector3 pos, float age = 0.0f){
            uint64_t treeid = trees.GenerateID();

            if (treeid >= maxnumtrees){
                trees.RemoveID(treeid);
                return "tmt";
            }

            if (treeid == treestates.size()){
                treestates.push_back((state){treeid, age, 0.0, pos});
            }
            else {
                treestates[treeid] = (state){treeid, age, 0.0, pos};
            }

            Vertex verts[4];
            ZE::Visual::AddQuad(
                (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({8, 8}, 4, 0),
                (ZE::Visual::TextureModifications)0, 0, MatrixTranslate(pos.x, pos.y, pos.z),
                verts, treestates[treeid].inds, treeid * 4
            );
    
            if (nullptr != dq.vb->UpdateMemory(verts, 4, treeid * 4))
                Error() << "Couldn't update the trees vertex buffer!!";

            printf("created tree with id %lu, %lu\n", treeid, trees.GetNumActive());

            return nullptr;
        }
        void RemoveTree(uint64_t id){
            treestates[id] = (state){id, 0.0f, 0.0f, {0, -50.0f, 0.0f}};
            trees.RemoveID( id );
            std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
            ZE::Visual::AddQuad(
                (Box2D){0, 0, 0, 0}, ZE::Visual::TextureMapToBox2D({128, 128}, 0, 0),
                0, 0,
                updatedsprite, _
            );
            const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, id * 4);
        }
        void Update(){
            std::vector<Vector3> newtreespos;
            std::vector<uint32_t> toremove;
            for (state& s: treestates){
                if (!trees[s.id])
                    continue;

                s.age += 1;
                if (s.age == (60 * 2.f)){
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({8, 8}, 5, 0),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);
                    // printf("updated id num %lu, offsetval: %lu|| %s\n", s.id, 4 * s.id, a);
                }
                if (s.age == (60 * 4.f)){
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({8, 8}, 5, 1),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);
                }
                if (s.age == (60 * 6.0f)){
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 2}, ZE::Visual::TextureMapToBox2D({8, 4}, 6, 0),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y - 0.5f, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);
                }
                if (s.age == (60 * 40.0f)){
                    toremove.push_back(s.id);
                }
                if (s.age < (60 * 6.0)){
                    continue;
                }
                s.fruitcycle += 0.01;
                s.fruitcycle = std::round(s.fruitcycle * 100.0f) / 100.0f;

                if (s.fruitcycle == 2.f){
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 2}, ZE::Visual::TextureMapToBox2D({8, 4}, 7, 0),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y - 0.5f, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);   
                }
                if (s.fruitcycle == 5.f){
                    for (int i = 0; i < 3; ++i){
                        int32_t randomNum = (int32_t)GetRandom() % 6;
                        if (!randomNum){
                            printf("[RND %d]tree id:%lu can spawn a tree\n", randomNum, s.id);
                            newtreespos.push_back((Vector3){
                                s.pos.x + (((float)GetRandom() - 500.f)/800.f),
                                -0.5f,
                                s.pos.z + (((float)GetRandom() - 500.f)/800.f)
                            });
                        }
                    }
                    s.fruitcycle = 0.f;
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 2}, ZE::Visual::TextureMapToBox2D({8, 4}, 6, 0),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y - 0.5f, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);
                }

            }

            for (Vector3 v : newtreespos){
                AddTree( v );
            }
            for (uint32_t id: toremove){
                RemoveTree( id );
            }

            std::sort(
                treestates.begin(), treestates.end(),
                [](state a, state b) {
                    return a.pos.z < b.pos.z;
                }
            );
            std::vector<uint16_t> inds;
            for (const state& s: treestates){
                if (!trees[s.id])
                    continue;

                inds.push_back(s.inds[0]);
                inds.push_back(s.inds[1]);
                inds.push_back(s.inds[2]);
                inds.push_back(s.inds[3]);
                inds.push_back(s.inds[4]);
                inds.push_back(s.inds[5]);
            }
            dq.ib->UpdateMemory(inds.data(), inds.size(), 0);
        }
        // Fruit LookForFruit(){
            
        // }

        struct state {
            uint64_t id;
            float age;
            float fruitcycle;
            Vector3 pos;

            uint16_t inds[6];
        };

        size_t  maxnumtrees;
        size_t *highestid;
        ZE::DataStructures::IDManager<0xff + 1> trees;
        std::vector<state> treestates;

        ZE::Visual::DrawQueue dq;
        ZE::DataStructures::IDManager<1024> *idman;
        MVP *mvps;
    };
    Trees trees(mvps.data());
    uint32_t numtrees = 15;
    for (uint32_t i = 0; i < numtrees; ++i){
        trees.AddTree((Vector3){(((float)GetRandom()/50.0f)) - 10.0f, -0.5, ((float)GetRandom()/50.0f) - 10.0f}, 60 * 4.f);
    }

    class Tribe {
    public:
        struct Fella{
            const char *name;
            Vector3 pos;
            Vector3 targetPos;
            float energy;
            bool foodsearching;
            bool showingstamina;
        };
        Tribe( ZE::DataStructures::IDManager<1024> &idman, MVP *mvps ): idman(idman), mvps(mvps){
            // trsid = idman.GenerateID();
            // dq = { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
            // ZE::Visual::AddQuad(
            //     (Box2D){0, 0, 2, 2}, (Box2D)ZE::Visual::TextureMapToBox2D({8, 8}, 0, 0),
            //     0, trsid, dq.verts, dq.inds
            // );
            // ZE::Visual::CreateDrawQueue(
            //     mainWnd, dq,
            //     0, 0
            // );
            // pos = {0, -0.5, -10.0};
            // targetPos = (Vector3){ ((float)GetRandom()/100.0f), -0.5, (float)GetRandom()/100.0f};
            // foodsearching = false;
            // showingstamina = false;
        }
        ~Tribe() = default;

        void SpawnFella( const char *name ){

        }
    
        void Update( std::vector<WindowEvent> events, std::vector<Trees::state>& treestates ){
            // for (WindowEvent e: events){
            //     if (e.type != WindowEventType::Key) continue;
            // }
            // if (energy < 50.0f && foodsearching == false){
            //     foodsearching = true;
            //     // Find closest tree
            //     std::vector<Vector3> treepositions;
            //     for (Trees::state s: treestates){
            //         treepositions.push_back(s.pos);
            //     }
            //     int64_t indexOfClosestTree = -1;
            //     float closestTreeDist = std::numeric_limits<float>::max();
            //     for (int i = 0; i < treepositions.size(); ++i){
            //         float cdist = sqrt(
            //             pow(pos.x - treepositions[i].x, 2) + 
            //             pow(pos.y - treepositions[i].y, 2) + 
            //             pow(pos.z - treepositions[i].z, 2)
            //         );
                
            //         if (cdist < closestTreeDist){
            //             closestTreeDist = cdist;
            //             indexOfClosestTree = i;
            //         }
            //     }
            //     // move towards the tree
            //     targetPos = treepositions[indexOfClosestTree];
            // }
            
            // float
            //     dx = targetPos.x - pos.x,
            //     dz = targetPos.z - pos.z
            // ;
            // float len = ((dx * dx) + (dz * dz));
            // len = (float)((uint64_t)(len * 100)) / 100.0;
            // Vector2 v = {dx, dz};
            // v = Vector2Normalize(v) / 60.0f;
            // if (len > 0){
            //     len = sqrt(len);
            //     dx /= len * 30;
            //     dz /= len * 30;
            //     pos = Vector3Add(pos, (Vector3){v.x, 0, v.y});
            //     energy -= 1.0 / 60.0;
            //     mvps[trsid].model = MatrixTranslate( pos.x, pos.y, pos.z );
            // }
            // else {
            //     targetPos =  (Vector3){ (((float)GetRandom()/50.0f)) - 10.0f, -0.5, ((float)GetRandom()/50.0f) - 10.0f};
            //     if (foodsearching){
            //         foodsearching = false;
            //         energy = 100.0f;
            //     }
            // }
        }
        
        ZE::Visual::DrawQueue dq;
        ZE::DataStructures::IDManager<1024> &idman;
        MVP *mvps;
        uint32_t trsid;
    };

    Tribe theFirst(transformIDs, mvps.data());
    theFirst.SpawnFella("philla");
    theFirst.SpawnFella("merrin");
    theFirst.SpawnFella("cleet");
    theFirst.SpawnFella("fueller");
   
    // ZE::Visual::DrawQueue ProgressBar { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr};
    // uint32_t bartr1 = transformIDs.GenerateID();
    // uint32_t bartr2 = transformIDs.GenerateID();
    // ZE::Visual::AddQuad(
    //     (Box2D){0, 0, 0.6, 0.2},
    //     ZE::Visual::TextureMapToBox2D({128, 8}, 0, 7 ),
    //     0, bartr1, ProgressBar.verts, ProgressBar.inds
    // );
    // ZE::Visual::AddQuad(
    //     (Box2D){0, 0, 0.6, 0.2},
    //     ZE::Visual::TextureMapToBox2D({128, 8}, 1, 7 ),
    //     0, bartr2, ProgressBar.verts, ProgressBar.inds
    // );
    // ZE::Visual::CreateDrawQueue(&wnd, ProgressBar, 0, 0);

    ZE::Camera::ProjectionCamera cam(
        PI / 3.0,
        SCREEN_WIDTH / SCREEN_HEIGHT
    );
    cam.SetPos({0, -5.0, 0});
    
    mvps[0] = {MatrixIdentity()};

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

        //printf( "-(FRAME)-----------------------\n" );

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

        cam.SetPos((Vector3){cposx, -5.0, cposy});

        //mvps[bartr1].model = MatrixTranslate(philla.pos.x, philla.pos.y - 1.0, philla.pos.z);

        // float bartr2offset = 
        //     (1.0 - (philla.energy / 100.0f)) * 0.3
        // ;
        // mvps[bartr2].model = MatrixScale(philla.energy / 100.0, 1, 1) * MatrixTranslate(philla.pos.x - bartr2offset, philla.pos.y - 1.0, philla.pos.z);

        trees.Update();

        theFirst.Update( events, trees.treestates );

        ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );
        
        //////////////////////////////////////
        /* =====( GRAPHICS PROCESSING )=====*/
        //////////////////////////////////////
        std::vector<std::pair<VertexBuffer *, IndexBuffer *>> vis = {
            {map.dq.vb, map.dq.ib},
            {trees.dq.vb, trees.dq.ib},
            //{theFirst.dq.vb, theFirst.dq.ib}

            //{ProgressBar.vb, ProgressBar.ib},
        };
        wnd.DrawIndexed( vis, ub1, vi, cam.GetView() * cam.GetProj() );
        t2 = std::chrono::high_resolution_clock::now();

        millisecondsSinceLastFrame =
            std::chrono::duration< float, std::milli> (t2 - t1).count();
        deltaT = millisecondsSinceLastFrame / desiredMilliCount;
    }

    std::cout << "Quitting Game !!!" << std::endl;
    return 0;
}