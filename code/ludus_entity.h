/*
    Creation Date: 08-04-2016
    Creator: Mike Tunnicliffe
*/

enum entity_type
{
    EntityType_Player,
    EntityType_Wall,
    EntityType_PowerUp,

    EntityType_Count,
};

enum collision_shape
{
    CollisionShape_Rect, // TODO(fierydrake): support polygons (in addition/instead of Rects)
    CollisionShape_Circle,

    CollisionShape_Count,
};

struct line_segment
{
    vec2 A;
    vec2 B;
};

// TODO(fierydrake): support polygons (in addition/instead of Rects)
struct collision_area
{
    collision_shape Shape;
    vec2 OffsetP;
    b32 StopOnCollision;

    union
    {
        struct
        {
            vec2 Dim;
        };

        struct
        {
            r32 Radius;
        };
    };

    collision_area* Next;
};

struct entity
{
    entity_type Type;
    vec2 P;
    vec2 dP;

    r32 Angle; // NOTE(fierydrake): Radians
    b32 Inactive;

    collision_area* FirstCollisionArea;

    union
    {
        struct
        {
            u8 ID;
            r32 Acceleration;
            vec2 FacingGoal;
            u32 Facing;
        } Player;

        struct
        {
            vec2 Dim;
        } Wall;
    };
};

struct entity_group
{
    u32 Count;
    entity* Entities; // TODO(fierydrake): Should be array or linked list?
};
