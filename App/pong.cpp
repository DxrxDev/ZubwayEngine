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

#define SCREEN_WIDTH  1280.0f
#define SCREEN_HEIGHT 720.0f

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

bool gamepaused;

int main( void ){
    gamepaused = false;
    ResetRandom();
    const char *appName = "Stones To Bridges"; 
    InitWindowSystem( );
    InitGraphicsSystem( appName );

    GraphicsWindow wnd(SCREEN_WIDTH, SCREEN_HEIGHT, appName, Window::CreationFlags::None);
    mainWnd = &wnd;

    Image worldtexmap = cptoimg(cp_load_png("STB.png"));
    Image uitexmap = cptoimg(cp_load_png("STB_ui.png"));
    VulkanImage vi(&wnd, worldtexmap, uitexmap);

    // Image uispritesheeddata = cptoimg(cp_load_png("STB_ui.png"));
    // VulkanImage viui(&wnd, uispritesheeddata.width, uispritesheeddata.height, (Image::Pixel*)uispritesheeddata.data);

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
        const char *AddTree( Vector3 pos, uint64_t age = 0 ){
            uint64_t treeid = trees.GenerateID();

            if (treeid >= maxnumtrees){
                trees.RemoveID(treeid);
                return "tmt";
            }

            if (treeid == treestates.size()){
                treestates.push_back((state){treeid, age, 0, pos});
            }
            else {
                treestates[treeid] = (state){treeid, age, 0, pos};
            }

            Vertex verts[4];
            ZE::Visual::AddQuad(
                (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({8, 8}, 4, 0),
                (ZE::Visual::TextureModifications)0, 0, MatrixTranslate(pos.x, pos.y, pos.z),
                verts, treestates[treeid].inds, treeid * 4
            );
    
            if (nullptr != dq.vb->UpdateMemory(verts, 4, treeid * 4))
                Error() << "Couldn't update the trees vertex buffer!!";

            return nullptr;
        }
        void RemoveTree(uint64_t id){
            treestates[id] = (state){id, 0, 0, {0, -50.0f, 0.0f}};
            if (trees.RemoveID( id )){
                printf("tree with id wasnt active %lu, %lu\n", id, trees.GetNumActive());
                return;
            }

            std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
            ZE::Visual::AddQuad(
                (Box2D){0, 0, 0, 0}, ZE::Visual::TextureMapToBox2D({128, 128}, 0, 0),
                0, 0,
                updatedsprite, _
            );
            uint16_t zeroinds[6] = {0, 0, 0, 0, 0 ,0};
            const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, id * 4);
            a = dq.ib->UpdateMemory( zeroinds, 6, id * 6 );

            printf("removing tree with id %lu, %lu\n", id, trees.GetNumActive());
        }
        void Update(){
            if (gamepaused)
                return;
            std::vector<Vector3> newtreespos;
            std::vector<uint32_t> toremove;
            for (state& s: treestates){
                if (!trees[s.id])
                    continue;

                s.age += 1;
                if (s.age == (60 * 2)){
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({8, 8}, 5, 0),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);
                    // printf("updated id num %lu, offsetval: %lu|| %s\n", s.id, 4 * s.id, a);
                }
                if (s.age == (60 * 4)){
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({8, 8}, 5, 1),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);
                }
                if (s.age == (60 * 6)){
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 2}, ZE::Visual::TextureMapToBox2D({8, 4}, 6, 0),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y - 0.5f, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);
                }
                if (s.age == (60 * 120)){
                    toremove.push_back(s.id);
                }
                if (s.age < (60 * 6)){
                    continue;
                }
                s.fruitcycle += 1;

                if (s.fruitcycle == (60 * 2)){
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 2}, ZE::Visual::TextureMapToBox2D({8, 4}, 7, 0),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y - 0.5f, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);   
                }
                if (s.fruitcycle == (60 * 20)){
                    int32_t randomNum = (int32_t)GetRandom() % 10;
                    if (!randomNum){
                        printf("tree id:%lu can spawn a tree\n", s.id);
                        newtreespos.push_back((Vector3){
                            s.pos.x + (((float)GetRandom() - 500.f)/800.f),
                            -0.5f,
                            s.pos.z + (((float)GetRandom() - 500.f)/800.f)
                        });
                    }
                    s.fruitcycle = 0;
                    std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
                    ZE::Visual::AddQuad(
                        (Box2D){0, 0, 1, 2}, ZE::Visual::TextureMapToBox2D({8, 4}, 6, 0),
                        0, 0, MatrixTranslate(s.pos.x, s.pos.y - 0.5f, s.pos.z),
                        updatedsprite, _
                    );
                    const char *a = dq.vb->UpdateMemory(updatedsprite.data(), 4, s.id * 4);
                }

            }

            for (uint32_t id: toremove){
                RemoveTree( id );
            }
            for (Vector3 v : newtreespos){
                AddTree( v );
            }

            std::vector<state> draworder = treestates;
            std::sort(
                draworder.begin(), draworder.end(),
                [](state a, state b) {
                    return a.pos.z < b.pos.z;
                }
            );
            std::vector<uint16_t> inds;
            for (const state& s: draworder){
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

            if (newtreespos.size() || toremove.size())
                printf(
                    "trees created: %lu\n"
                    "trees removed: %lu\n",
                    newtreespos.size(), toremove.size()
                );
        }
        // Fruit LookForFruit(){
            
        // }

        struct state {
            uint64_t id;
            uint64_t age;
            uint64_t fruitcycle;
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

            uint32_t trs;
        };
        Tribe( ZE::DataStructures::IDManager<1024> &idman, MVP *mvps ): idman(idman), mvps(mvps){
            // trsid = idman.GenerateID();
            dq = { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
            ZE::Visual::CreateDrawQueue(
                mainWnd, dq,
                1000, 1000
            );
        }
        ~Tribe() = default;

        void SpawnFella( const char *name ){
            uint32_t id = (uint32_t)idman.GenerateID();
            fellas.push_back(
                (Fella){
                    name,
                    {0, 0, 0}, {0, 0, 0},
                    100, 0, 0,
                    id
                }
            );

            ZE::Visual::AddQuad(
                {0, 0, 0.5, 1.0},
                ZE::Visual::TextureMapToBox2D({16, 8}, 0, 0),
                0, id,
                MatrixTranslate(0, -0.5, 0),
                dq.verts, dq.inds
            );

            dq.vb->UpdateMemory(dq.verts.data(), dq.verts.size(), 0);
            dq.ib->UpdateMemory(dq.inds.data(), dq.inds.size(), 0);

            mvps[id] = {MatrixIdentity()};
        }
    
        void Update( std::vector<WindowEvent> events, std::vector<Trees::state>& treestates ){
            for (Fella& f: fellas){
                Trees::state closesttree = treestates[0];

                float closesttreedist = INFINITY;/*sqrt(
                    (f.pos.x - closesttree.pos.x) * (f.pos.x - closesttree.pos.x) +
                    (f.pos.x - closesttree.pos.z) * (f.pos.x - closesttree.pos.z)
                );*/

                for (uint32_t i = 0; i < treestates.size(); ++i){
                    float nexttreedist = sqrt(
                        (f.pos.x - treestates[i].pos.x) * (f.pos.x - treestates[i].pos.x) +
                        (f.pos.z - treestates[i].pos.z) * (f.pos.z - treestates[i].pos.z)
                    );
                    if (nexttreedist < closesttreedist){
                        closesttreedist = nexttreedist;
                        closesttree = treestates[i];
                    }
                }

                f.energy -= 0.5;
                if (f.energy <= 0){
                    f.foodsearching = true;

                    if (closesttreedist < 0.1){
                        f.pos = closesttree.pos;
                        closesttreedist = 0.f;
                        closesttree.age =  60 * 240;
                        f.energy = 100.f;
                        f.foodsearching = false;
                    }
                    float xdist = f.pos.x - closesttree.pos.x;
                    float zdist = f.pos.z - closesttree.pos.z;

                    if (xdist < 0.)
                        f.pos.x += std::min<float>(0.05, (xdist * -1));
                    else if (xdist > 0.)
                        f.pos.x -= std::max<float>(0.05, (xdist * -1));

                    if (zdist < 0.)
                        f.pos.z += std::min<float>(0.05, (zdist * -1));
                    else if (zdist > 0.)
                        f.pos.z -= std::max<float>(0.05, (zdist * -1));

                }
                else{
                    f.pos.x += (GetRandom() - 500) / (100.f * 60.f);
                    f.pos.z += (GetRandom() - 500) / (100.f * 60.f);
                }
                mvps[f.trs] = {MatrixTranslate(f.pos.x, 0, f.pos.z)};
            }
        }
        
        std::vector<Fella> fellas;
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

    ZE::UI ui( mainWnd, {SCREEN_WIDTH, SCREEN_HEIGHT} );

    Vector4 bgcol = { 1, 0.95, 0.9, 1.0 };
    Vector4 energycol1 = { 0.6, 0.4, 0.9, 1.0 };
    Vector4 energycol2 = { 0.8, 0.0, 0.0, 1.0 };

    ZE::UI::Component& menu1 =
    ui.root.children[0] = {
        true, {0},
        {
            { 10, 10, 450, 600 },
            bgcol,
            ZE::Visual::TextureMapToBox2D({16, 16}, 0, 0)
        },
        true, new ZE::UI::Component, 1
    };
    ZE::UI::Component& energybarbg =
    ui.root.children[0].children[0] = {
        true, {0, 0},
        {
            { 25, 10, 400, 100 },
            energycol2,
            ZE::Visual::TextureMapToBox2D({16, 16}, 0, 0)
        },
        true, new ZE::UI::Component, 1
    };
    ZE::UI::Component& energybarfg =
    ui.root.children[0].children[0].children[0] = {
        true, {0, 0, 0},
        {
            { 0, 0, 200, 90 },
            energycol1,
            ZE::Visual::TextureMapToBox2D({16, 16}, 0, 0)
        }
    };
    ui.Redraw();

    energybarfg.style.col = {0.0, 0.5, 0.3,1 };

    uint32_t path[] = {
        0, 0
    };
    ui.Redraw( path, 2, false );

    printf("number of children %d\n", ui.GetSizeOfTree(ui.root));

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
                    case 'p':{
                        gamepaused = !gamepaused;
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
            {theFirst.dq.vb, theFirst.dq.ib},
            {trees.dq.vb, trees.dq.ib},
            //{ProgressBar.vb, ProgressBar.ib},
        };

        wnd.DrawIndexed( vis, *ui.uib, ub1, vi, cam.GetView() * cam.GetProj() );

        t2 = std::chrono::high_resolution_clock::now();

        millisecondsSinceLastFrame =
            std::chrono::duration< float, std::milli> (t2 - t1).count();
        deltaT = millisecondsSinceLastFrame / desiredMilliCount;
    }

    std::cout << "Quitting Game !!!" << std::endl;
    return 0;
}