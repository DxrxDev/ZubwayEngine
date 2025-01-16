#include "box2D.hpp"


Box2DCompare Box2D::CollidingWith(Box2D other){
    float l1, r1, t1, b1;
    float l2, r2, t2, b2;
        
    GetEdges( &l1, &r1, &t1, &b1 );
    other.GetEdges( &l2, &r2, &t2, &b2 );

    float dx = x - other.x;
    float dy = y - other.y;

    bool colliding = !(
        (r1 < l2) || 
        (r2 < l1) ||
        (b1 < t2) ||
        (b2 < t1)
    );

    bool within = 
        (l2 < l1) && (r1 < r2) &&
        (t2 < t1) && (b1 < b2)
    ;

    Box2DCompare ret = {
        dx, dy,
        colliding,
        within,
        (l2 < l1), (r1 < r2),
        (t2 < t1), (b1 < b2)
    };

    return ret;
}
void Box2D::GetEdges( float *l, float *r, float *t, float *d ){
    *l = x - (w * 0.5f);
    *r = x + (w * 0.5f);
    *t = y - (h * 0.5f);
    *d = y + (h * 0.5f);
}



HasBox2D::HasBox2D( float x, float y, float w, float h ){
    box = {x, y, w, h};
}

Box2DCompare HasBox2D::CollidingWith(Box2D other){
    return box.CollidingWith( other );
}

void HasBox2D::Move( float dx, float dy ){
    box.x += dx;
    box.y += dy;
}
void HasBox2D::MoveTo( float x, float y ){
    box.x = x;
    box.y = y;
}

Box2D HasBox2D::GetBox( void ){
    return box;
}
Box2D *HasBox2D::GetPtr( void ){
    return &box;
}

void HasBox2D::GetEdges( float *l, float *r, float *t, float *d ){
    box.GetEdges( l, r, t, d );
}
Matrix HasBox2D::GetModel( void ){
    return MatrixTranslate(box.x, box.y, 0) * MatrixScale(box.w, box.h, 1);
}