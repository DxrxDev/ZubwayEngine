#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <ctime>
#include <iostream>
#include <queue>
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

float deltaT = 0;
float desiredfps = 60.0;

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
            Map( ZE::DataStructures::IDManager<1024>& _idman, MVP *mvps ) : idman(_idman), trans(mvps){
                dq = {
                    std::vector<Vertex>(),
                    std::vector<uint16_t>()
                };

                groundID = 0;
                bonfireID = idman.GenerateID();

        
                ZE::DataStructures::Grid bgGrid( 25, 25, 1.0f );
                for (uint32_t y = 0; y < bgGrid.GetSize().y; ++y){
                    for (uint32_t x = 0; x < bgGrid.GetSize().x; ++x){
                        ZE::Visual::AddQuad(
                            bgGrid.ToBox2D(x, y),
                            ZE::Visual::TextureMapToBox2D({8, 8}, 0, 2),
                            0, groundID, MatrixRotateX( PI/2.0f ),
                            dq.verts, dq.inds
                        );
                    }
                }
                ZE::Visual::AddSqPyramid(
                    {0, 0,0 }, {.1, 2.0, .1},
                    ZE::Visual::TextureMapToBox2D({8, 8}, 2, 1),
                    bonfireID, dq.verts, dq.inds
                );

                ZE::Visual::CreateDrawQueue(
                    mainWnd, dq,
                    0, 0
                );
            }
        
            ZE::DataStructures::IDManager<1024>& idman;

            uint32_t groundID;
            uint32_t bonfireID;
            MVP *trans;
            ZE::Visual::DrawQueue dq;
    };
    Map map(transformIDs, mvps.data());

    // struct Fruit{
    //     float ripeness; // 0 = seeds, 1.0 = flower, 2.0 = bulb, 3.0 = hard = 4.0 ripe, 5 = mushy, <5 = ew
    //     bool ontree;
    // };
    class Trees{
    public:
        enum class Sprite {
            SEED, SAPLING, BUSH, TREE, FLOWERING_TREE 
        };
        struct spriteupdate{
            uint32_t index;
            Sprite sprite;
            float x, y, z;
        };

        Trees(MVP *mvps) : mvps(mvps){
            highestid = nullptr;
            maxnumtrees = 100;
            dq = { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
            ZE::Visual::CreateDrawQueue(
                mainWnd, dq,
                4 * maxnumtrees, 6 * maxnumtrees
            );
        }
        const char *AddTree( Vector3 pos, float age = 0 ){
            uint64_t treeid = trees.GenerateID();

            if (treeid >= maxnumtrees){
                trees.RemoveID(treeid);
                return "tmt";
            }

            if (treeid == treestates.size()){
                treestates.push_back((state){treeid, age, 0, 0, pos});
            }
            else {
                treestates[treeid] = (state){treeid, age, 0, 0, pos};
            }

            Vertex verts[4];
            ZE::Visual::AddQuad(
                (Box2D){0, 0, 1, 1}, ZE::Visual::TextureMapToBox2D({8, 8}, 4, 0),
                (ZE::Visual::TextureModifications)0, 0, MatrixTranslate(pos.x, pos.y, pos.z),
                verts, treestates[treeid].inds, treeid * 4
            );
    
            if (nullptr != dq.vb->UpdateMemory(verts, 4, treeid * 4))
                Error() << "Couldn't update the trees vertex buffer!!";

            if (age > 6.0)
                treestates[treeid].sprite = Sprite::TREE;
            else if (age > 4.0)
                treestates[treeid].sprite = Sprite::BUSH;
            else if (age > 2.0)
                treestates[treeid].sprite = Sprite::SAPLING;
            else
                treestates[treeid].sprite = Sprite::SEED;
            return nullptr;
        }
        void RemoveTree(uint64_t id){
            treestates[id] = (state){id, 0, 0, 0, {0, -50.0f, 0.0f}, Sprite::SEED};
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
            std::vector<spriteupdate> spriteupdates;
            for (state& s: treestates){
                if (!trees[s.id])
                    continue;

                s.age += deltaT;
                if (s.age > 2.0 && s.sprite == Sprite::SEED){
                    spriteupdates.push_back({
                        (uint32_t)s.id, Sprite::SAPLING, s.pos.x, s.pos.y, s.pos.z
                    });
                    s.sprite = Sprite::SAPLING;
                }
                else if (s.age > 4.0 && s.sprite == Sprite::SAPLING){
                    spriteupdates.push_back({
                        (uint32_t)s.id, Sprite::BUSH, s.pos.x, s.pos.y, s.pos.z
                    });
                    s.sprite = Sprite::BUSH;
                }
                else if (s.age > 6.0 && s.sprite == Sprite::BUSH){
                    spriteupdates.push_back({
                        (uint32_t)s.id, Sprite::TREE, s.pos.x, s.pos.y - 0.5f, s.pos.z
                    });
                    s.sprite = Sprite::TREE;
                }
                if (s.age > 120.0){
                    toremove.push_back(s.id);
                }
                if (s.age < 6.0){
                    continue;
                }

                s.fruitcycle += deltaT;
                if (s.fruitcycle > 10.0 && s.sprite == Sprite::TREE){
                    spriteupdates.push_back({
                        (uint32_t)s.id, Sprite::FLOWERING_TREE, s.pos.x, s.pos.y - 0.5f, s.pos.z
                    });  
                    s.sprite = Sprite::FLOWERING_TREE;
                    s.fruit = 4;
                }
                if (s.fruitcycle > 20.0 && s.sprite == Sprite::FLOWERING_TREE){
                    int32_t numseeds = (int32_t)GetRandom() % 2;
                    numseeds *= (int32_t)((float)s.fruit * 0.25f);
                    for (uint32_t i = 0; i < numseeds; ++i){
                        newtreespos.push_back((Vector3){
                            s.pos.x + (((float)GetRandom() - 500.f)/500.f),
                            -0.5f,
                            s.pos.z + (((float)GetRandom() - 500.f)/500.f)
                        });
                    }
                    s.fruitcycle = 0;
                    spriteupdates.push_back({
                        (uint32_t)s.id, Sprite::TREE, s.pos.x, s.pos.y - 0.5f, s.pos.z
                    });  
                    s.sprite = Sprite::TREE;
                }

            }

            for (uint32_t id: toremove){
                RemoveTree( id );
            }
            for (Vector3 v : newtreespos){
                AddTree( v );
            }
            SpriteUpdates(spriteupdates);

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

            // if (newtreespos.size() || toremove.size())
            //     printf(
            //         "trees created: %lu\n"
            //         "trees removed: %lu\n",
            //         newtreespos.size(), toremove.size()
            //     );
        }

        void SpriteUpdates(std::vector<spriteupdate> list){
            if (list.size()==0) return;

            std::vector<uint32_t> chains {1};
            uint32_t lastindex = list[0].index;
            for (uint32_t i = 1; i < list.size(); ++i){
                spriteupdate su = list[i];
                if (lastindex < (su.index-1)){
                    chains.push_back(1);
                }
                else {
                    chains[chains.size()-1] += 1;
                }
                lastindex = su.index;
            }

            // printf("{");
            // for (uint32_t i: chains){
            //     printf("%d, ", i);
            // }
            // printf("}\n");

            uint32_t chainoffset = 0;
            std::vector<Vertex> updatedsprite; std::vector<uint16_t> _;
            for (uint32_t i = 0; i < chains.size(); ++i){
                for (uint32_t j = 0; j < chains[i]; ++j){
                    spriteupdate su = list[(chainoffset)+j];
                    Box2D spritebox;
                    Box2D treeshape;
                    switch (su.sprite){
                        case Sprite::SEED: spritebox = ZE::Visual::TextureMapToBox2D({8, 8}, 4, 0); break;
                        case Sprite::SAPLING: spritebox = ZE::Visual::TextureMapToBox2D({8, 8}, 5, 0); break;
                        case Sprite::BUSH: spritebox = ZE::Visual::TextureMapToBox2D({8, 8}, 5, 1); break;
                        case Sprite::TREE: spritebox = ZE::Visual::TextureMapToBox2D({8, 4}, 6, 0); break;
                        case Sprite::FLOWERING_TREE: spritebox = ZE::Visual::TextureMapToBox2D({8, 4}, 7, 0); break;
                    }
                    if (su.sprite == Sprite::TREE || su.sprite == Sprite::FLOWERING_TREE)
                        treeshape = (Box2D){0, 0, 1, 2};
                    else
                        treeshape = (Box2D){0, 0, 1, 1};
                    ZE::Visual::AddQuad(
                        treeshape, spritebox,
                        0, 0, MatrixTranslate(su.x, su.y, su.z),
                        updatedsprite, _
                    );
                }
                
                if (chains[i] != updatedsprite.size()/4){
                    printf("SOMETHINGS TERRIBLY WRONG");
                    exit(-1);
                }

                //printf("sprite update round %lu, %d + %d\n", updatedsprite.size()/4, firstindexoffset, chainoffset);
                dq.vb->UpdateMemory(updatedsprite.data(), updatedsprite.size(), (list[chainoffset].index)*4);
                updatedsprite = std::vector<Vertex>();
                chainoffset += chains[i];
            }
        }

        uint32_t RemoveFruit( uint32_t id, uint32_t num = 1 ){
            state& s = treestates[id];
            if (s.sprite != Sprite::FLOWERING_TREE) return 1;
            if (trees[id]){
                if (s.fruit > 0){
                    s.fruit -= num;
                }
                else {
                    s.fruitcycle = 0.0;
                    SpriteUpdates({
                        (spriteupdate){id, Sprite::TREE, s.pos.x, s.pos.y, s.pos.z }
                    });
                    s.sprite = Sprite::TREE;
                }
            }
            
            return 0;
        }
        
        struct state {
            uint64_t id;
            float age;
            float fruitcycle;
            uint8_t fruit;
            Vector3 pos;

            Sprite sprite;

            uint16_t inds[6];
        };

        size_t  maxnumtrees;
        size_t *highestid;
        ZE::DataStructures::IDManager<0xff + 1> trees;
        std::vector<state> treestates;

        ZE::Visual::DrawQueue dq;
        MVP *mvps;
    };
    Trees trees(mvps.data());
    uint32_t numtrees = 15;
    for (uint32_t i = 0; i < numtrees; ++i){
        trees.AddTree((Vector3){(((float)GetRandom()/50.0f)) - 10.0f, -0.5, ((float)GetRandom()/50.0f) - 10.0f}, 6.0);
    }

    struct FellaInfoUI{
        FellaInfoUI(ZE::UI *ui, std::vector<uint32_t> parentpath, Vector2 pos){
            setupworked = false;
            path = parentpath;
            root = &ui->root;
            for (uint32_t step: parentpath){
                root = &root->children[step];
            }

            
            for (uint32_t i = 0; i < root->numchildren; ++i){
                if (root->children[i].active == false){
                    path.push_back(i);
                    root = &root->children[i];

                    root->active = true;
                    root->path = path;
                    root->splits = true;
                    root->children = new ZE::UI::Component[4];
                    root->numchildren = 4;

                    root->style = (ZE::UI::Component::Style){
                        {pos.x, pos.y, 40, (10 * 4) + (5*5)}, UI_WHITE, 
                        ZE::Visual::TextureMapToBox2D({16, 16}, 0, 0)
                    };

                    for (uint32_t i = 0; i < 4; ++i){
                        Vector4 colour;
                        switch (i){
                        case 0:{
                            colour = {0.8, 0.6, 0.1, 1.0};
                        } break;
                        case 1:{
                            colour = {0.3, 0.2, 0.05, 1.0};
                        } break;
                        case 2:{
                            colour = {0.2, 0.5, 0.7, 1.0};
                        } break;
                        case 3:{
                            colour = {0.05, 0.15, 0.25, 1.0};
                        } break;
                        }

                        root->children[i].active = true;
                        root->children[i].path = root->path; root->children[i].path.push_back(i);
                        root->children[i].splits = true;
                        root->children[i].children = new ZE::UI::Component;
                        root->children[i].numchildren = 1;

                        root->children[i].style = {
                            {5, 5.f + (i * 15.f), 30, 10 },
                            {0.5, 0.5, 0.5, 1.0f},
                            ZE::Visual::TextureMapToBox2D({16, 16}, 0, 0)
                        };

                        root->children[i].children[0].active = true;
                        root->children[i].children[0].path = root->children[i].path; root->children[i].children[0].path.push_back(0);

                        root->children[i].children[0].style = {
                            {0, 0, 30, 10 },
                            colour,
                            ZE::Visual::TextureMapToBox2D({16, 16}, 0, 0)
                        };
                    }

                    printf("");

                    break;
                }
            }

            printf("isrgbirgb %lu %lu, %u\n", parentpath.size(), path.size(), path[path.size()-1]);

            setupworked = true;
        }

        void UpdateBars( float vals[4] ){
            for (uint32_t i = 0; i < 4; ++i){
                root->children[i].children[0].style.box = {0, 0, 30 * vals[i], 10 };
            }
        }

        bool setupworked;
        ZE::UI::Component *root;
        std::vector<uint32_t> path;
    };

    struct Fella {
        // About fella
        const char *name;
        float age;
        float health;
        float respect;

        Vector3 position;

        // Needs
        float fatigue;
        float hungry, thirsty;
        float gottago;
        float boredom;

        // Attributes
        float fit;

        // Personality
        float selfless;
        float artistic;

        struct Task {
            enum class Type{
                NONE, RESTING, SEEKING, SOCIALISING
            } type;
            float priority;
            union {
                struct {
                } none;
                struct {
                } rest;
                struct {
                    // Some spicification for items, idk yet
                } seek;
                struct {

                } socialise;
            };
        } doing;

        // Engine
        struct {
            float
                energy, hunger, thirsty, gottago, boredom;
        } brain;

        uint64_t trsid;

        FellaInfoUI* eb;
    };

    class Tribe {
    public:
        Tribe( ZE::DataStructures::IDManager<1024> &idman, MVP *mvps ): idman(idman), mvps(mvps){
            // trsid = idman.GenerateID();
            dq = { std::vector<Vertex>(), std::vector<uint16_t>(), nullptr, nullptr };
            ZE::Visual::CreateDrawQueue(
                mainWnd, dq,
                1000, 1000
            );
        }
        ~Tribe() = default;

        void SpawnFella( const char *name, FellaInfoUI *comp ){
            uint32_t id = (uint32_t)idman.GenerateID();
            fellas.push_back(
                (Fella){
                    name, 0, 100., 100., {0, 0},
                    0., 0., 0., 0., 0.,
                    1, 1, 1, {},
                    { 1, 1, 1, 1, 1 }, id, comp
                }
            );

            ZE::Visual::AddQuad(
                {0, 0, 0.5, 1.0},
                ZE::Visual::TextureMapToBox2D({16, 8}, 1, 0),
                0, id,
                MatrixTranslate(0, -0.5, 0),
                dq.verts, dq.inds
            );

            dq.vb->UpdateMemory(dq.verts.data(), dq.verts.size(), 0);
            dq.ib->UpdateMemory(dq.inds.data(), dq.inds.size(), 0);

            mvps[id] = {MatrixIdentity()};
        }
    
        void Update( std::vector<WindowEvent> events, Trees& trees ){
            std::vector<Trees::state>& treestates = trees.treestates;
            for (uint32_t fi = 0; fi < fellas.size(); ++fi ){
                Fella& f = fellas[fi];

                Trees::state closesttree = treestates[0];
                float closesttreedist = INFINITY;/*sqrt(
                    (f.pos.x - closesttree.pos.x) * (f.pos.x - closesttree.pos.x) +
                    (f.pos.x - closesttree.pos.z) * (f.pos.x - closesttree.pos.z)
                );*/

                for (uint32_t i = 0; i < treestates.size(); ++i){
                    float nexttreedist = sqrt(
                        (f.position.x - treestates[i].pos.x) * (f.position.x - treestates[i].pos.x) +
                        (f.position.z - treestates[i].pos.z) * (f.position.z - treestates[i].pos.z)
                    );
                    if (nexttreedist < closesttreedist){
                        closesttreedist = nexttreedist;
                        closesttree = treestates[i];
                    }
                }

                f.fatigue += deltaT*0.5;
                f.hungry += deltaT*2.0;

                float x = f.fatigue;
                float restdesire =
                    (x >= 90.0) ? 100. * f.brain.energy :
                    (x >= 75.0) ? 50.0 * f.brain.energy :
                    (x >= 50.0) ? 10.0 * f.brain.energy:
                                  0.0;
                ;

                x = f.hungry;
                float hungerdesire =  
                    (x >= 90.0) ? 100. * f.brain.hunger :
                    (x >= 75.0) ? 50.0 * f.brain.hunger :
                    (x >= 10.0) ? 10.0 * f.brain.hunger  :
                                  0.0
                ;
                
                x = f.thirsty;
                float thirstdesire =  
                    (x >= 90.0) ? 100. * f.brain.thirsty :
                    (x >= 75.0) ? 50.0 * f.brain.thirsty :
                    (x >= 50.0) ? 10.0 * f.brain.thirsty :
                                  0.0
                ;

                x = f.gottago;
                float gottagodesire =  
                    (x >= 90.0) ? 100. * f.brain.gottago :
                    (x >= 75.0) ? 50.0 * f.brain.gottago :
                    (x >= 50.0) ? 10.0 * f.brain.gottago  :
                                  0.0
                ;

                float needs[4] = {
                    restdesire, hungerdesire, thirstdesire, gottagodesire
                };
                uint32_t desireind[4] = {0, 1, 2, 3};
                bool sorted = false;
                while (!sorted){
                    sorted = true;
                    for (uint32_t i = 0; i < 3; ++i){
                        if (needs[i] < needs[i+1]){
                            float t = needs[i];
                            needs[i] = needs[i+1];
                            needs[i+1] = t;

                            float t2 = desireind[i];
                            desireind[i] = desireind[i+1];
                            desireind[i+1] = t;

                            sorted = false;
                        }
                    }
                }
                
                // printf("{");
                // for (uint32_t i = 0; i < 4; ++i){
                //     printf("(%f, %d), ", needs[i], desireind[i]);
                // }
                // printf("}\n");

                if (desireind[0] == 1){
                    float
                        xdist = f.position.x - closesttree.pos.x,
                        zdist = f.position.z - closesttree.pos.z,
                        vdist = sqrt((xdist*xdist) + (zdist * zdist));

                    if (vdist < 0.02){
                        f.position.x = closesttree.pos.x;
                        f.position.z = closesttree.pos.z;
                        
                        if (closesttree.sprite == Trees::Sprite::FLOWERING_TREE){
                            trees.RemoveFruit(closesttree.id);
                            f.hungry = 0;
                        }
                    }
                    else {
                        xdist = xdist * deltaT / (vdist);
                        zdist = zdist * deltaT / (vdist);
                        f.position -= {xdist, 0.0, zdist};
                    }

                       
                }
                else {
                    f.position.x += (GetRandom() - 500) / (100.f * 60.f);
                    f.position.z += (GetRandom() - 500) / (100.f * 60.f);
                }

                float uivals[4] = {
                    f.fatigue / 100.f,
                    f.hungry / 100.f,
                    f.thirsty / 100.f,
                    f.gottago / 100.f,
                };
                f.eb->UpdateBars(uivals);
                mvps[f.trsid] = {MatrixTranslate(f.position.x, 0, f.position.z)};
            }
        }
        
        std::vector<Fella> fellas;
        ZE::Visual::DrawQueue dq;
        ZE::DataStructures::IDManager<1024> &idman;
        MVP *mvps;
        uint32_t trsid;
    };

    ZE::UI ui( mainWnd, {SCREEN_WIDTH, SCREEN_HEIGHT}, 3 );

    ZE::UI::Component& energybars =
    ui.root.children[0] = {
        true, {0},
        {},
        true, new ZE::UI::Component[30], 30
    };
    FellaInfoUI f0 = FellaInfoUI(&ui, {0}, {0, 0});
    FellaInfoUI f1 = FellaInfoUI(&ui, {0}, {0, 50});
    FellaInfoUI f2 = FellaInfoUI(&ui, {0}, {0, 100});
    FellaInfoUI f3 = FellaInfoUI(&ui, {0}, {0, 150});

    dodebug;

    Tribe theFirst(transformIDs, mvps.data());

    theFirst.SpawnFella("philla",  &f0);
    theFirst.SpawnFella("merrin",  &f1);
    theFirst.SpawnFella("cleet",   &f2);
    theFirst.SpawnFella("fueller", &f3);

    dodebug;

    ui.Redraw();

    dodebug;

    uint32_t path[] = {
        0
    };

    dodebug;

    ui.Redraw( path, 1 );

    dodebug;

    printf("number of children %d\n", ui.GetSizeOfTree(ui.root));

    ZE::Camera::ProjectionCamera cam(
        PI / 3.0,
        SCREEN_WIDTH / SCREEN_HEIGHT
    );
    cam.SetPos({0, -5.0, 0});
    
    mvps[0] = {MatrixIdentity()};
    UniformBuffer ub1( &wnd, 1024 );
    ub1.UpdateMVP( mvps.data(), mvps.size(), 0 );

    dodebug;

    bool running = true;
    std::chrono::time_point<std::chrono::high_resolution_clock> t1, t2;
    float desiredMilliCount = 16.667;
    millisecondsSinceLastFrame = 0;

    dodebug;

    std::cout << "Entering main loop !!!" << std::endl;

    float cposx = 0, cposy = 0;
    Vector2 mp = {0, 0};
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
                        case 'p':{
                            gamepaused = !gamepaused;
                        } break;
                    }
                }
            }
            else if (e.type == WindowEventType::MouseMove){
                mp = {
                    e.mm.x - (SCREEN_WIDTH/2.0f),
                    e.mm.y -(SCREEN_HEIGHT/2.0f)
                };
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
        
        float x = mp.x / SCREEN_WIDTH * 2.0;
        float y = mp.y / SCREEN_HEIGHT * 2.0;

        Vector4 eye = (Vector4){x, y, 1, 1} * MatrixInvert(cam.GetProj()); 
        Vector4 gee = eye * MatrixInvert(cam.GetView());
        gee = gee * 0.001;
        Vector4 travel = {cam.GetPos().x, cam.GetPos().y, cam.GetPos().z, 1.0};
        while (travel.y < 0.00000001){
            travel += gee;
        }
        mvps[map.bonfireID].model = MatrixTranslate(travel.x, travel.y, travel.z);

        printf("eye %f %f %f\n", travel.x, travel.y, travel.z);

        trees.Update();
        theFirst.Update( events, trees );
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
           
        uint32_t path[] = {
            0
        };
        ui.Redraw( path, 1);
           
        wnd.DrawIndexed( vis, *ui.uib, ub1, vi, cam.GetView() * cam.GetProj() );
           
        t2 = std::chrono::high_resolution_clock::now();
           
        millisecondsSinceLastFrame =
        std::chrono::duration< float, std::milli> (t2 - t1).count();
        deltaT = (millisecondsSinceLastFrame / desiredMilliCount) / desiredfps;
    }
    
    std::cout << "Quitting Game !!!" << std::endl;
    return 0;
}