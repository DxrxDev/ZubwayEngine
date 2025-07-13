#if !defined( __THING_HPP )
#define       __THING_HPP

#include <cstdint>
#include "raymath.h"
#include "box2D.hpp"
#include "error.hpp"

namespace ZE {
    namespace DataStructures{
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
                    sx = sy = scale;
                }
                Vector2 GetPos( uint32_t x, uint32_t y ){
                    float fieldPosOffsetX = (w * sx) / 2.0f;
                    float fieldPosOffsetY = (h * sy) / 2.0f;
                    return (Vector2){
                        x * sx + (sx / 2.0f) - fieldPosOffsetX,
                        y * sy + (sy / 2.0f) - fieldPosOffsetY
                    };
                }
                Vector2 GetSize(){
                    return (Vector2){(float)w, (float)h};
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

        template<size_t size>
        class IDManager{
        public:
            IDManager(){
                if (size % 8)
                    Error() << "IDManager: size must be a multiple of 8...";
                memset(ids, 0, size/8);
            }
            uint64_t GenerateID(){
                for (uint64_t i = 0; i < size; ++i){
                    if (!ids[i]){
                        ids[i] = 1; numactive++;
                        return i;
                    }
                }
                Error() << "IDManager: No IDs left...";
                return std::numeric_limits<uint64_t>::max();
            }
            bool RemoveID(uint64_t id){
                if (!ids[id]){
                    return 1;
                }
                ids[id] = 0; numactive--;
                return 0;
            }
            void Reset(){
                memset(ids, 0, size/8);
            }
            size_t GetNumActive( void ){
                return numactive;
            }
            const constexpr size_t GetSize(){
                return size;
            }
            const bool operator[](size_t id){
                return ids[id];
            };

        private:
            bool ids[size];
            size_t numactive;
        };
    };
};

#endif