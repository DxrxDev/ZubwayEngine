#include "thing.hpp"
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