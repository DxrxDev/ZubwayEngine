#if !defined( __GFX_HPP )
#define       __GFX_HPP

#include "box2D.hpp"
#include "graphics.hpp"

namespace ZE {
    namespace Visual {
        struct TextureMap{
            uint32_t numx, numy;
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
};

#endif