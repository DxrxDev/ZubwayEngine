#include "raymath.h"
#include "thing.hpp"
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
#if !defined( __GFX_HPP )
#define       __GFX_HPP

#define UI_RED   { 1.0, 0.0, 0.0, 1.0 }
#define UI_GREEN { 0.0, 1.0, 0.0, 1.0 }
#define UI_BLUE  { 0.0, 0.0, 1.0, 1.0 }
#define UI_BLACK { 0.0, 0.0, 0.0, 1.0 }
#define UI_WHITE { 1.0, 1.0, 1.0, 1.0 }

#define UI_CLEAR     { 0, 0, 0, 0 }
#define UI_REDa(x)   { 1.0, 0.0, 0.0, x }
#define UI_GREENa(x) { 0.0, 1.0, 0.0, x }
#define UI_BLUEa(x)  { 0.0, 0.0, 1.0, x }
#define UI_BLACKa(x) { 0.0, 0.0, 0.0, x }
#define UI_WHITEa(x) { 1.0, 1.0, 1.0, x }

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

        uint32_t AddSqPyramid( Vector3 pos, Vector3 scl, Box2D tex, uint32_t trs, std::vector<Vertex>& verts, std::vector<uint16_t>& inds);
    };
    namespace Camera {
        class Camera{
        public:
            void SetPos(Vector3 pos);
            Vector3 GetPos(void);
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
        };

        UI( GraphicsWindow *wnd, Vector2 screensize, uint32_t numberchild ){
            data = std::vector<VertexUI>();
            uib = new UIBuffer( wnd, data, 4000 );

            root = {
                true, {},
                {
                    { 0, 0, screensize.x, screensize.y},
                    {0, 0, 0.0, 0.0},
                    ZE::Visual::TextureMapToBox2D({16, 16}, 0, 0)

                },
                true, new Component[numberchild], numberchild };
        }
        ~UI(){
            delete uib;
        }

        bool ComparePaths( const Component& c1, const Component& c2 ){
            if (c1.path.size() != c2.path.size())
                return 0;

            for (uint32_t i = 0; i < c1.path.size(); ++i)
                if (c1.path[i] != c2.path[i])
                    return 0;

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
        uint32_t GetSizeOfTree( const Component& comp, const Component& until ){
            uint32_t total = 0;
            if (comp.active){
                total += 1;
                if (ComparePaths(comp, until))
                    return total;

                if (comp.splits)
                    for (uint32_t i = 0; i < comp.numchildren; ++i)
                        total += GetSizeOfTree( comp.children[i], until );
            };
            return total;
        }
        std::vector<VertexUI> GetVerts( Component& comp, Vector2 offset ){
            if (!comp.active)
                return std::vector<VertexUI>(0);

            std::vector<VertexUI> verts;

            Vector2 pos = {comp.style.box.x + offset.x, comp.style.box.y + offset.y};
            Vector2 size = {comp.style.box.w, comp.style.box.h};
            UIll::AddSquare(pos, size, comp.style.tex, comp.style.col, verts);    
            
            if (comp.splits){
                for (uint32_t i = 0; i < comp.numchildren; ++i){
                    for (VertexUI v: GetVerts(comp.children[i], pos)){
                        verts.push_back(v);
                    }
                }
            }
            
            return verts;
        }
        void Redraw( uint32_t *path, size_t pathsize ){
            Component c = root;
            Vector2 offset = {0, 0};
            uint32_t dataoffset = 0;

            //printf("im krilling \n");

            if (pathsize > 0){
                //printf("im guh \n");
                for (uint32_t i = 0; i < pathsize; ++i){
                    offset += {
                        c.style.box.x,
                        c.style.box.y
                    };
                    
                    c = c.children[path[i]];
                }
                dataoffset = GetSizeOfTree(root, c ) * 6;
            }
            else {

            }
            //printf("im krilling \n");
            data = GetVerts( c, offset );
            //printf("im krilling \n");
            uib->UpdateMemory( data.data(), data.size(), dataoffset );
            //printf("im krilling \n");
        }
        void Redraw( ){
            Redraw( nullptr, 0 );
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