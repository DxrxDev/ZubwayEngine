#include "raymath.h"
#include "thing.hpp"
#include <cmath>
#include <cstddef>
#if !defined( __GFX_HPP )
#define       __GFX_HPP

#include "box2D.hpp"
#include "graphics.hpp"

namespace ZE {
    namespace Visual {
        struct TextureMap{
            uint32_t numx, numy;
        };
        enum TextureModifications {
            FLIP_X   = 0b0000'00001,
            FLIP_Y   = 0b0000'00010,
            FLIP_Z   = 0b0000'00100,
            ROT_CW_1 = 0b0000'01000,
            ROT_CW_2 = 0b0000'10000,
            ROT_CW_3 = 0b0001'00000 
        };
        Box2D TextureMapToBox2D(TextureMap tm, uint32_t x, uint32_t y);

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
        void CreateDrawQueue(GraphicsWindow *wnd, DrawQueue& dq, size_t vr, size_t ir);
        
        /**
         * @brief Free GPU resources
         * 
         * @param dq 
         */
        void FreeDrawQueue( DrawQueue& dq );

        /**
         * @brief Add a Quad to your Draw Queue
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
        );

        /**
         * @brief Add a aQuad to your Draw Queue fith matrix transformation
         * 
         * @param rect 
         * @param texture 
         * @param texRot 
         * @param transformID 
         * @param mat 
         * @param verts 
         * @param inds 
         * @return const char* 
         */
        const char *AddQuad(
            Box2D rect, Box2D texture,
            uint32_t texRot, uint32_t transformID, Matrix mat,
            std::vector<Vertex>& verts, std::vector<uint16_t>& inds
        );

        const char *AddQuad(
            Box2D rect, Box2D texture,
            TextureModifications texmods, uint32_t transformID, Matrix mat,
            Vertex verts[], uint16_t inds[], size_t indsoffset
        );

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
        );
    };
    namespace Camera {
        class Camera{
        public:
            void SetPos(Vector3 pos);
        protected:
            Camera();

            virtual Matrix GetProj() = 0;
            virtual Matrix GetView() = 0;

            Vector3 pos;
        };
        class ProjectionCamera: public Camera{
        public:
            ProjectionCamera(float fov, float aspect);

            virtual Matrix GetProj() override;
            virtual Matrix GetView() override;

        private:
            float fov, aspect;
        };
        class OrthroCamera: public Camera{
        public:
            OrthroCamera(Vector2 screenSize);

            virtual Matrix GetMatrix();
        private:
            Vector2 screenSize;
        };
    };

    namespace UIll{
        void AddSquare(Vector2 pos, Vector2 size, Box2D tex, Vector4 col, std::vector<VertexUI>& verts);
    };
    class UI{
    public:
        struct Component {
            bool active;
            std::vector<uint32_t> path;
            
            struct Style{
                Box2D box;
                Vector4 col;
                Box2D tex;
            } style;

            bool splits;
            Component *children;
            uint32_t numchildren;

            uint32_t ind;
        };

        UI( GraphicsWindow *wnd, Vector2 screensize ){
            data = std::vector<VertexUI>();
            uib = new UIBuffer( wnd, data, 4000 );

            root = {
                true, {},
                {
                    { 0, 0, screensize.x, screensize.y},
                    {0, 0, 0.0, 0.0},
                    ZE::Visual::TextureMapToBox2D({16, 16}, 0, 0)

                },
                true, new Component, 1 };
        }
        ~UI(){
            delete uib;
        }

        bool ComparePaths( const Component& c1, const Component& c2 ){
            if (c1.path.size() != c2.path.size())
                return 0;

            for (uint32_t i = 0; i < c1.path.size(); ++i){
                if (c1.path[i] != c2.path[i])
                    return 0;
            }

            return 1;
        }
        uint32_t GetSizeOfTree( const Component& comp ){
            uint32_t total = 0;
            if (comp.active){
                total += 1;
                if (comp.splits){
                    for (uint32_t i = 0; i < comp.numchildren; ++i){
                        total += GetSizeOfTree( comp.children[i] );
                    }
                }
            };
            return total;
        }
        std::vector<VertexUI> GetVerts( Component& comp, bool updateoffsetandsize = false ){
            static uint32_t _ind = 0;
            bool cond = comp.path.size() == 0 && updateoffsetandsize;
            if (cond){
                printf("Recalculating UI...\n");
                _ind = 0;
            }
            else {
                //printf("\n");
            }

            if (!comp.active)
                return std::vector<VertexUI>(0);

            std::vector<VertexUI> verts;

            Vector2 posoffset = {0, 0};
            Component *offsetcomp = &root;
            printf("-----\nProcessing component, deep %lu\n", comp.path.size());
            for (uint32_t i = 0; i < comp.path.size(); ++i){
                if (i == comp.path.size()-1) continue; // This is kinda ugly but `for (uint32_t i = 0; i < comp.path.size()-1; ++i)` throws err
                
                offsetcomp = &offsetcomp->children[comp.path[i]];
                posoffset += {
                    offsetcomp->style.box.x,
                    offsetcomp->style.box.y
                };
                //printf("path step %u, index %u | x=%f, y=%f\n", i, comp.path[i], posoffset.x, posoffset.y );
            }
            Vector2 pos = {comp.style.box.x + posoffset.x, comp.style.box.y + posoffset.y};
            Vector2 size = {comp.style.box.w, comp.style.box.h};
            UIll::AddSquare(pos, size, comp.style.tex, comp.style.col, verts);    
            
            printf("number of children = %u\n", comp.numchildren);

            if (comp.splits){
                for (uint32_t i = 0; i < comp.numchildren; ++i){
                    for (VertexUI v: GetVerts(comp.children[i], updateoffsetandsize)){
                        verts.push_back(v);
                    }
                }
            }
            
            printf(
                "component level = %lu, (%f, %f), ind = %d\n"
                "-----\n",

                comp.path.size(), pos.x, pos.y, comp.ind
            );
            return verts;
        }
        void Redraw( uint32_t *path, size_t pathsize, bool updateoffsetandsize = false ){
            Component& c = root;

            for (uint32_t i = 0; i < pathsize; ++i){
                c = c.children[path[i]];
            }
            printf("fegtrhytrertgetr\n");
            data = GetVerts( c, updateoffsetandsize );
            uib->UpdateMemory(data.data(), data.size(), 0);

            printf("number of verts %lu\n", data.size());
        }
        void Redraw( ){
            Redraw( nullptr, 0, true );
        }
    
        UIBuffer *uib;
        Component root;
        private:
        std::vector<VertexUI> data;
    };
    // UI::Component::Style CreatePlane(){
        
    // }

    template <size_t size>
    class SpriteCluster {

    private:
        Window *wnd;
        Visual::DrawQueue dq;
        DataStructures::IDManager<size> idman;
        DataStructures::IDManager<1024> trsids;
    };
};

#endif