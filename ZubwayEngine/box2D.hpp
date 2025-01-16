#if !defined( __BOX2D_HPP )
#define       __BOX2D_HPP

#include "raymath.h"

struct Box2DCompare {
    float dx, dy;
    bool colliding, within;
    bool lwithin, rwithin, twithin, bwithin;
};
struct Box2D {
    float x, y,
          w, h;

    Box2DCompare CollidingWith(Box2D other);
    void GetEdges( float *l, float *r, float *t, float *d );
};
class HasBox2D {
public:
    HasBox2D( float x, float y, float w, float h );

    Box2DCompare CollidingWith(Box2D other);

    void Move( float dx, float dy );
    void MoveTo( float x, float y );

    Box2D GetBox( void );
    Box2D *GetPtr( void );

    void GetEdges( float *l, float *r, float *t, float *d );
    Matrix GetModel( void );
protected:
    Box2D box;
};

#endif /* __BOX2D_HPP */