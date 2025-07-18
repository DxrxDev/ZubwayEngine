#include "gfx.hpp"


Box2D ZE::Visual::TextureMapToBox2D(TextureMap tm, uint32_t x, uint32_t y){
    float
        width = (1.0f / (float)tm.numx), 
        height = (1.0f / (float)tm.numy)
    ;
    return (Box2D){
        x * width, y * height,
        width, height
    };
}

struct DrawQueue {
    std::vector<Vertex>   verts;
    std::vector<uint16_t> inds;
	VertexBuffer *vb;
    IndexBuffer *ib;
};

namespace ZE {
    namespace Visual {

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
        void CreateDrawQueue(GraphicsWindow *wnd, DrawQueue& dq, size_t vr, size_t ir){
            dq.vb = new VertexBuffer( wnd, dq.verts, vr );
            dq.ib = new IndexBuffer( wnd, dq.inds.data(), dq.inds.size(), ir );
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
            if (transformID >= 1024)
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

        const char *AddQuad(
            Box2D rect, Box2D texture,
            uint32_t texRot, uint32_t transformID, Matrix mat,
            std::vector<Vertex>& verts, std::vector<uint16_t>& inds
        ){
            if (transformID >= 1024)
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
        
            Vector3 v1 = (Vector3){rect.x - hw, rect.y - hh, 0} * mat;
            Vector3 v2 = (Vector3){rect.x + hw, rect.y - hh, 0} * mat;
            Vector3 v3 = (Vector3){rect.x - hw, rect.y + hh, 0} * mat;
            Vector3 v4 = (Vector3){rect.x + hw, rect.y + hh, 0} * mat;
            verts.push_back({v1, texcorners[texrot[0]], transformID}); //TL
            verts.push_back({v2, texcorners[texrot[1]], transformID}); //TR
            verts.push_back({v3, texcorners[texrot[2]], transformID}); //BL
            verts.push_back({v4, texcorners[texrot[3]], transformID}); //BR


            inds.push_back( offset + 0 );
            inds.push_back( offset + 3 );
            inds.push_back( offset + 2 );

            inds.push_back( offset + 0 );
            inds.push_back( offset + 1 );
            inds.push_back( offset + 3 );

            return nullptr;
        }

        const char *AddQuad(
            Box2D rect, Box2D texture,
            TextureModifications texmods, uint32_t transformID, Matrix mat,
            Vertex verts[], uint16_t inds[], size_t indsoffset
        ){
            if (transformID >= 1024)
                return "ZE::Visual::AddQuad(): transformID out of bounds!";
            float hw = rect.w / 2.0f, hh = rect.h / 2.0f;

            Vector2 texcorners[4] = {
                {texture.x, texture.y},
                {texture.x + texture.w, texture.y},
                {texture.x, texture.y + texture.h},
                {texture.x + texture.w, texture.y + texture.h}
            };

            uint64_t texrot[4];
            texrot[0] = 0;
            texrot[1] = 1;
            texrot[2] = 2;
            texrot[3] = 3;

            if (texmods & TextureModifications::ROT_CW_1){
                texrot[0] = 2;
                texrot[1] = 0;
                texrot[2] = 3;
                texrot[3] = 1;
            }
            
            if (texmods & TextureModifications::ROT_CW_2){
                texrot[0] = 3;
                texrot[1] = 2;
                texrot[2] = 1;
                texrot[3] = 0;
            }

            if (texmods & TextureModifications::ROT_CW_3){
                texrot[0] = 1;
                texrot[1] = 3;
                texrot[2] = 0;
                texrot[3] = 2;
                
            }
        
            Vector3 v1 = (Vector3){rect.x - hw, rect.y - hh, 0} * mat;
            Vector3 v2 = (Vector3){rect.x + hw, rect.y - hh, 0} * mat;
            Vector3 v3 = (Vector3){rect.x - hw, rect.y + hh, 0} * mat;
            Vector3 v4 = (Vector3){rect.x + hw, rect.y + hh, 0} * mat;
            verts[0] = {v1, texcorners[texrot[0]], transformID}; //TL
            verts[1] = {v2, texcorners[texrot[1]], transformID}; //TR
            verts[2] = {v3, texcorners[texrot[2]], transformID}; //BL
            verts[3] = {v4, texcorners[texrot[3]], transformID}; //BR


            inds[0] = indsoffset + 0;
            inds[1] = indsoffset + 3;
            inds[2] = indsoffset + 2;

            inds[3] = indsoffset + 0;
            inds[4] = indsoffset + 1;
            inds[5] = indsoffset + 3;

            return nullptr;
        }


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
    
            void Camera::SetPos(Vector3 pos){
                this->pos = pos;
            }
    
            Camera::Camera(){
                pos = (Vector3){0, 0, 0};
            }

            ProjectionCamera::ProjectionCamera(float fov, float aspect){
                this->fov = fov;
                this->aspect = aspect;
            }

            Matrix ProjectionCamera::GetProj(){
                float 
                    near = 0.1,
                    far = 5000
                ;
                return MatrixPerspective(fov, aspect, near, far);
            }
            Matrix ProjectionCamera::GetView(){
                return MatrixLookAt(
                    this->pos,
                    Vector3Add(pos, {0, 1, -1}),
                    {0, 1, 0}
                );
            }


        OrthroCamera::OrthroCamera(Vector2 screenSize){
                this->screenSize = screenSize;
            }

            Matrix OrthroCamera::GetMatrix(){
                return MatrixOrtho(
                    screenSize.x * -0.5,
                    screenSize.x * 0.5,
                    screenSize.y * -0.5,
                    screenSize.y * 0.5,
                    0.1, 5000.0
                );
            }
    };

    namespace UI {
        void AddSquare(Vector2 pos, Vector2 size, Vector4 col, std::vector<VertexUI>& verts){
            Vector2 texcorners[4] = {
                {0.0, 0.0},
                {1.0, 0.0},
                {0.0, 1.0},
                {1.0, 1.0}
            };
        
            verts.push_back({{pos.x, pos.y}, col, texcorners[0]}); //TL
            verts.push_back({{pos.x + size.x, pos.y}, col, texcorners[1]}); //TR
            verts.push_back({{pos.x, pos.y + size.y}, col, texcorners[2]}); //BL
            verts.push_back({{pos.x + size.x, pos.y + size.y}, col, texcorners[3]}); //BR

            return;
        };
    }
};