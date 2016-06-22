/*
    Creation Date: 08-04-2016
    Creator: Mike Tunnicliffe
*/

// TODO(fierydrake): Clean up this generic math code
// --BEGIN
#define Cosine cosf
#define Sine sinf
#define ArcTangent2 atan2f
#define ArcCosine acosf
#define Tau ((r32)(2.0f * HMM_PI))


// TODO(Wacko): So for now since this is a 2D Game and we are working with
// a grid lets just make the facing stuff easy, may not want to merge to master
// and want a rotation over time but for this simple game lets just make it snap!
#define X0 Vec2(1, 0)
#define X1 Vec2(-1, 0)
#define Y0 Vec2(0, 1)
#define Y1 Vec2(0, -1)
#define ZERO Vec2(0, 0)

internal r32
DirectionToAngle(u32 inDirection)
{
    return (r32)((90) * inDirection);
}

internal u32
PositionToDirection(vec2 inPosition)
{
    if(inPosition == X0)
    {
        return 3;
    }
    else if(inPosition == Y1)
    {
        return 2;
    }
    else if(inPosition == X1)
    {
        return 1;
    }
    else if(inPosition == Y0)
    {
        return 0;
    }

    return 0;
}

internal b32
NotZeroVec2(vec2 V)
{
    b32 Result = (V.X != 0.0f || V.Y != 0.0f);
    return Result;
}

internal b32
EqualsZeroVec2(vec2 V)
{
    b32 Result = (V.X == 0.0f && V.Y == 0.0f);
    return Result;
}

internal vec2
RotateVec2(r32 Angle, vec2 A)
{
    vec2 Result;

    r32 SineAngle   = Sine(Angle);
    r32 CosineAngle = Cosine(Angle);

    Result.X = A.X * CosineAngle - A.Y * SineAngle;
    Result.Y = A.X * SineAngle + A.Y * CosineAngle;

    return Result;
}
// --END

internal collision_area
RectCollisionArea(vec2 OffsetP, vec2 Dim, b32 StopOnCollision = true)
{
    collision_area Result  = {};
    Result.Shape           = CollisionShape_Rect;
    Result.StopOnCollision = StopOnCollision;
    Result.OffsetP         = OffsetP;
    Result.Dim             = Dim;
    return Result;
}

internal collision_area
CircleCollisionArea(vec2 OffsetP, r32 Radius, b32 StopOnCollision = true)
{
    collision_area Result  = {};
    Result.Shape           = CollisionShape_Circle;
    Result.StopOnCollision = StopOnCollision;
    Result.OffsetP         = OffsetP;
    Result.Radius          = Radius;
    return Result;
}

internal u32
GetNextFreeEntityIndex(game_state* GameState)
{
    u32 Result;
    if(GameState->FreeEntityCount == 0)
    {
        Result = GameState->EntityCount++;
        Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
    }
    else
    {
        Result = GameState->FirstFreeEntityIndex;
        GameState->FreeEntityCount--;
        if(GameState->FreeEntityCount)
        {
            // TODO(fierydrake): This is a little slow perhaps... could use linked list
            b32 Found = false;
            for(u32 EntityIndex = GameState->FirstFreeEntityIndex + 1; EntityIndex < GameState->EntityCount;
                ++EntityIndex)
            {
                if(GameState->Entities[EntityIndex].Inactive)
                {
                    Found                           = true;
                    GameState->FirstFreeEntityIndex = EntityIndex;
                    break;
                }
            }
            Assert(Found);
        }
    }

    return Result;
}

internal collision_area*
GetNextFreeCollisionArea(game_state* GameState, collision_area* Next = 0)
{
    collision_area* Result;
    if(GameState->FirstFreeCollisionArea)
    {
        Result                            = GameState->FirstFreeCollisionArea;
        GameState->FirstFreeCollisionArea = GameState->FirstFreeCollisionArea->Next;
    }
    else
    {
        Assert(GameState->OnePastLastCollisionArea < ArrayCount(GameState->CollisionAreas));
        Result = GameState->CollisionAreas + GameState->OnePastLastCollisionArea++;
    }
    Result->Next = Next;
    return Result;
}

internal void
FreeCollisionArea(game_state* GameState, collision_area* CollisionArea)
{
    CollisionArea->Next               = GameState->FirstFreeCollisionArea;
    GameState->FirstFreeCollisionArea = CollisionArea;
}

internal void
FreeEntity(game_state* GameState, u32 EntityIndex)
{
    GameState->Entities[EntityIndex].Inactive = true;
    collision_area* CollisionArea             = GameState->Entities[EntityIndex].FirstCollisionArea;
    while(CollisionArea)
    {
        collision_area* Free = CollisionArea;
        CollisionArea        = CollisionArea->Next;
        FreeCollisionArea(GameState, Free);
    }
    ++GameState->FreeEntityCount;
    if(GameState->FreeEntityCount == 1 || EntityIndex < GameState->FirstFreeEntityIndex)
    {
        GameState->FirstFreeEntityIndex = EntityIndex;
    }
}

// internal collision_area*
// GetNextFreeMapCollisionAreaIndex(game_state GameState)
//{
//}

internal void
UpdateMapCollision(game_state* GameState, transient_state* TranState, u32 MapEntityIndex)
{
    entity* MapEntity                = GameState->Entities + MapEntityIndex;
    GameState->MapCollisionAreaCount = 0;
    MapEntity->FirstCollisionArea    = 0;
    for(u32 MapX = 0; MapX < ArrayCount(TranState->Map); ++MapX)
    {
        for(u32 MapY = 0; MapY < ArrayCount(TranState->Map[0]); ++MapY)
        {
            if(TranState->Map[MapX][MapY].Tile == 1 && TranState->Map[MapX][MapY].Solid)
            {
                ++GameState->MapCollisionAreaCount;
                Assert(GameState->MapCollisionAreaCount < ArrayCount(GameState->MapCollisionAreas));
                collision_area* CollisionArea = GameState->MapCollisionAreas + GameState->MapCollisionAreaCount++;
                *CollisionArea =
                    RectCollisionArea(Vec2(32.0f + MapX * 64.0f, 32.0f + MapY * 64.0f), Vec2(64.0f, 64.0f));
                CollisionArea->Next           = MapEntity->FirstCollisionArea;
                MapEntity->FirstCollisionArea = CollisionArea;
            }
        }
    }
}

internal u32
AddMapCollision(game_state* GameState, transient_state* TranState)
{
    u32 EntityIndex = GetNextFreeEntityIndex(GameState);

    entity NewEntity = {};
    NewEntity.Type   = EntityType_Wall;

    for(u32 MapX = 0; MapX < ArrayCount(TranState->Map); ++MapX)
    {
        for(u32 MapY = 0; MapY < ArrayCount(TranState->Map[0]); ++MapY)
        {
            if(TranState->Map[MapX][MapY].Tile == 1 && TranState->Map[MapX][MapY].Solid)
            {
                ++GameState->MapCollisionAreaCount;
                Assert(GameState->MapCollisionAreaCount < ArrayCount(GameState->MapCollisionAreas));
                collision_area* CollisionArea = GameState->MapCollisionAreas + GameState->MapCollisionAreaCount++;
                *CollisionArea =
                    RectCollisionArea(Vec2(32.0f + MapX * 64.0f, 32.0f + MapY * 64.0f), Vec2(64.0f, 64.0f));
                CollisionArea->Next          = NewEntity.FirstCollisionArea;
                NewEntity.FirstCollisionArea = CollisionArea;
            }
        }
    }

    GameState->Entities[EntityIndex] = NewEntity;

    return EntityIndex;
}

internal u32
AddLDPlayer(game_state* GameState, vec2 P, vec2 Dim, r32 Acceleration, u8 ID)
{
    u32 EntityIndex = GetNextFreeEntityIndex(GameState);

    entity NewEntity              = {};
    NewEntity.Type                = EntityType_Player;
    NewEntity.P                   = P;
    NewEntity.FirstCollisionArea  = GetNextFreeCollisionArea(GameState);
    *NewEntity.FirstCollisionArea = RectCollisionArea(Vec2(0, 0), Dim);

    NewEntity.Player.ID           = ID;
    NewEntity.Player.Acceleration = Acceleration;
    NewEntity.Player.Facing       = PositionToDirection(Y1);

    GameState->Entities[EntityIndex] = NewEntity;

    return EntityIndex;
}

internal u32
AddPlayer(game_state* GameState, vec2 P, u8 ID)
{
    u32 EntityIndex = GetNextFreeEntityIndex(GameState);

    entity NewEntity              = {};
    NewEntity.Type                = EntityType_Player;
    NewEntity.P                   = P;
    NewEntity.FirstCollisionArea  = GetNextFreeCollisionArea(GameState);
    *NewEntity.FirstCollisionArea = RectCollisionArea(Vec2(0, 0), Vec2(100, 129));
    NewEntity.FirstCollisionArea  = GetNextFreeCollisionArea(GameState, NewEntity.FirstCollisionArea);
    *NewEntity.FirstCollisionArea = CircleCollisionArea(Vec2(0, 0), 50.0f);

    NewEntity.Player.ID     = ID;
    NewEntity.Player.Facing = PositionToDirection(X1);

    GameState->Entities[EntityIndex] = NewEntity;

    return EntityIndex;
}

internal u32
AddWall(game_state* GameState, vec2 P, vec2 Dim, r32 Angle = 0.0f)
{
    u32 EntityIndex = GetNextFreeEntityIndex(GameState);

    entity NewEntity              = {};
    NewEntity.Type                = EntityType_Wall;
    NewEntity.P                   = P;
    NewEntity.Angle               = Angle;
    NewEntity.FirstCollisionArea  = GetNextFreeCollisionArea(GameState);
    *NewEntity.FirstCollisionArea = RectCollisionArea(Vec2(0, 0), Dim);

    NewEntity.Wall.Dim = Dim;

    GameState->Entities[EntityIndex] = NewEntity;

    return EntityIndex;
}

internal u32
AddPillar(game_state* GameState, vec2 P, r32 Radius)
{
    u32 EntityIndex = GetNextFreeEntityIndex(GameState);

    entity NewEntity              = {};
    NewEntity.Type                = EntityType_Wall;
    NewEntity.P                   = P;
    NewEntity.FirstCollisionArea  = GetNextFreeCollisionArea(GameState);
    *NewEntity.FirstCollisionArea = CircleCollisionArea(Vec2(0, 0), Radius);

    r32 Diameter       = 2.0f * Radius;
    NewEntity.Wall.Dim = Vec2(Diameter, Diameter);

    GameState->Entities[EntityIndex] = NewEntity;

    return EntityIndex;
}

internal u32
AddPowerUp(game_state* GameState, vec2 P, vec2 Dim)
{
    u32 EntityIndex = GetNextFreeEntityIndex(GameState);

    entity NewEntity              = {};
    NewEntity.Type                = EntityType_PowerUp;
    NewEntity.P                   = P;
    NewEntity.FirstCollisionArea  = GetNextFreeCollisionArea(GameState);
    *NewEntity.FirstCollisionArea = RectCollisionArea(Vec2(0, 0), Dim, false);

    GameState->Entities[EntityIndex] = NewEntity;

    return EntityIndex;
}

internal b32
CollisionBetween(entity** EntityA, entity** EntityB, entity_type TypeA, entity_type TypeB)
{
    b32 Result = ((*EntityA)->Type == TypeA && (*EntityB)->Type == TypeB);
    if(!Result)
    {
        Result = ((*EntityB)->Type == TypeA && (*EntityA)->Type == TypeB);
        if(Result)
        {
            // NOTE(fierydrake): Swap
            entity* Tmp = *EntityB;
            *EntityB    = *EntityA;
            *EntityA    = Tmp;
        }
    }
    return Result;
}

internal void
RemoveEntityDeferred(entity* Entity)
{
    Entity->Inactive = true;
}

// TODO(fierydrake): We may want more information here, like point of collision
//                   and CollisionArea or CollisionAreaIndex
internal void
HandleCollision(game_state* GameState, u32 EntityAIndex, u32 EntityBIndex)
{
    entity* EntityA = GameState->Entities + EntityAIndex;
    entity* EntityB = GameState->Entities + EntityBIndex;
    if(CollisionBetween(&EntityA, &EntityB, EntityType_Player, EntityType_PowerUp))
    {
        FreeEntity(GameState, EntityBIndex);
    }
}

internal entity_group
GetNearbyEntities(game_state* GameState, vec2 P)
{
    // TODO: Spatial partition
    entity_group Result;
    Result.Count    = GameState->EntityCount;
    Result.Entities = GameState->Entities;
    return Result;
}

internal b32
CanCollide(entity* A, entity* B)
{
    if(A == B)
        return false;
    return true;
}

internal vec2
LineBoundaryNormal(line_segment LineSegment, vec2 ConvexCenter)
{
    Assert(NotZeroVec2(LineSegment.A - ConvexCenter));

    vec2 Result;

    vec2 Line          = LineSegment.B - LineSegment.A;
    vec2 Orthogonals[] = {Vec2(-Line.Y, Line.X), Vec2(Line.Y, -Line.X)};

    vec2 Outwards = LineSegment.A - ConvexCenter;
    if(Dot(Orthogonals[0], Outwards) > 0)
    {
        Result = Orthogonals[0];
    }
    else
    {
        Assert(Dot(Orthogonals[1], Outwards) > 0);
        Result = Orthogonals[1];
    }

    return Result;
}

// NOTE(fierydrake): If multiple lowest vertices, it will choose the left-most.
internal u32
LowestVertexPreferLeftmost(vec2* Vertices, u32 VertexCount)
{
    Assert(VertexCount > 0);
    u32 Result      = 0;
    vec2 BestVertex = Vertices[0];
    for(u32 VertexIndex = 1; VertexIndex < VertexCount; ++VertexIndex)
    {
        vec2 V = Vertices[VertexIndex];
        if(V.Y < BestVertex.Y || (V.Y == BestVertex.Y && V.X < BestVertex.X))
        {
            Result     = VertexIndex;
            BestVertex = V;
        }
    }
    return Result;
}

// NOTE(fierydrake): Returns angle in range [0,Tau)
internal r32
AngleFromXAxis0Tau(vec2 V)
{
    r32 Result = ArcTangent2(V.Y, V.X);
    if(Result < 0)
    {
        Result += Tau;
    }
    return Result;
}

// NOTE(fierydrake): A and B must be convex polygons with anti-clockwise winding
//                   (Edge angles with x-axis should be monotonic (non-strictly
//                   increasing) modulo 360 degrees / Tau radians)
// TODO(fierydrake): Add assert if polygon is concave or incorrectly wound
//                   (can be detected as a decrease in x-axis angle from
//                    one edge to next)
// TODO(fierydrake): PERF: Would it be more efficient if we construct the
//                         edges once in MoveEntity() and pass them here.
//                         I think we may be constructing them multiple
//                         times at the moment.
internal u32
FastMinkowskiSumNxM(vec2* MinkowskiVertices,
                    u32 MaxMinkowskiVertexCount,
                    vec2* VerticesInA,
                    u32 VertexCountInA,
                    vec2* VerticesInB,
                    u32 VertexCountInB)
{
    Assert(MaxMinkowskiVertexCount >= VertexCountInA + VertexCountInB);
    u32 MinkowskiVertexCount = 0;

    u32 BottomVertexInAIndex = LowestVertexPreferLeftmost(VerticesInA, VertexCountInA);
    u32 BottomVertexInBIndex = LowestVertexPreferLeftmost(VerticesInB, VertexCountInB);
    vec2 MinkowskiVertex     = VerticesInA[BottomVertexInAIndex] + VerticesInB[BottomVertexInBIndex];

    // NOTE(fierydrake): Merge edge vectors in A and B in order of their angle with the x-axis
    //                   Start at the bottom vertex of each
    u32 VerticesInAMerged = 0;
    u32 VerticesInBMerged = 0;
    u32 VerticesMerged    = 0;
    u32 VerticesToMerge   = VertexCountInA + VertexCountInB;
    while(VerticesMerged < VerticesToMerge)
    {
        MinkowskiVertices[MinkowskiVertexCount++] = MinkowskiVertex;

        if(VerticesInAMerged == VertexCountInA)
        {
            // NOTE(fierydrake): Only vertices in B left
            u32 CurrentVertexInBIndex = BottomVertexInBIndex + VerticesInBMerged;
            u32 NextVertexInBIndex    = CurrentVertexInBIndex + 1;
            vec2 CandidateEdgeInB =
                VerticesInB[NextVertexInBIndex % VertexCountInB] - VerticesInB[CurrentVertexInBIndex % VertexCountInB];
            Assert(NotZeroVec2(CandidateEdgeInB));

            MinkowskiVertex = MinkowskiVertex + CandidateEdgeInB;
            ++VerticesInBMerged;
            ++VerticesMerged;
        }
        else if(VerticesInBMerged == VertexCountInB)
        {
            // NOTE(fierydrake): Only vertices in A left
            u32 CurrentVertexInAIndex = BottomVertexInAIndex + VerticesInAMerged;
            u32 NextVertexInAIndex    = CurrentVertexInAIndex + 1;
            vec2 CandidateEdgeInA =
                VerticesInA[NextVertexInAIndex % VertexCountInA] - VerticesInA[CurrentVertexInAIndex % VertexCountInA];
            Assert(NotZeroVec2(CandidateEdgeInA));

            MinkowskiVertex = MinkowskiVertex + CandidateEdgeInA;
            ++VerticesInAMerged;
            ++VerticesMerged;
        }
        else
        {
            u32 CurrentVertexInAIndex = BottomVertexInAIndex + VerticesInAMerged;
            u32 NextVertexInAIndex    = CurrentVertexInAIndex + 1;
            vec2 CandidateEdgeInA =
                VerticesInA[NextVertexInAIndex % VertexCountInA] - VerticesInA[CurrentVertexInAIndex % VertexCountInA];
            Assert(NotZeroVec2(CandidateEdgeInA));

            u32 CurrentVertexInBIndex = BottomVertexInBIndex + VerticesInBMerged;
            u32 NextVertexInBIndex    = CurrentVertexInBIndex + 1;
            vec2 CandidateEdgeInB =
                VerticesInB[NextVertexInBIndex % VertexCountInB] - VerticesInB[CurrentVertexInBIndex % VertexCountInB];
            Assert(NotZeroVec2(CandidateEdgeInB));

            r32 CandidateEdgeInA_Angle = AngleFromXAxis0Tau(CandidateEdgeInA);
            r32 CandidateEdgeInB_Angle = AngleFromXAxis0Tau(CandidateEdgeInB);
            // NOTE(fierydrake): We need to deal with edges at the end of the
            //                   list that may have angle 0, since we do not
            //                   carefully select the starting index to ensure
            //                   the bottom vertex is the left-most if not unique.
            if(CandidateEdgeInA_Angle == CandidateEdgeInB_Angle)
            {
                // NOTE(fierydrake): Merge consecutive co-linear edges
                MinkowskiVertex = MinkowskiVertex + CandidateEdgeInA + CandidateEdgeInB;
                ++VerticesInAMerged;
                ++VerticesInBMerged;
                VerticesMerged += 2;
            }
            else if(CandidateEdgeInA_Angle < CandidateEdgeInB_Angle)
            {
                MinkowskiVertex = MinkowskiVertex + CandidateEdgeInA;
                ++VerticesInAMerged;
                ++VerticesMerged;
            }
            else
            {
                MinkowskiVertex = MinkowskiVertex + CandidateEdgeInB;
                ++VerticesInBMerged;
                ++VerticesMerged;
            }
        }
    }

    return MinkowskiVertexCount;
}

struct collision
{
    u32 EntityIndex;
    entity* Entity;
    collision_area* CollisionArea;
    vec2 P;
    vec2 Normal;
    r32 Distance;
    r32 DistanceAlongNormal;
    r32 ConservativeDistance;
};

internal void
MoveEntity(game_state* GameState,
           entity* Entity,
           u32 EntityIndex,
           r32 dt,
           vec2 ddP,
           render_buffer* RenderBuffer,
           transient_state* TranState)
{
    entity_group NearbyEntities = GetNearbyEntities(GameState, Entity->P);

    vec2 DeltaPThisFrame = Entity->dP * dt + ddP * .5f * Square(dt); // s = ut + (1/2)at^2
    vec2 DeltaVThisFrame = ddP * 10.0f * dt - Entity->dP * 0.2f;     // TODO: Super hack
    Entity->dP           = Entity->dP + DeltaVThisFrame;

    vec2 DeltaPRemaining  = DeltaPThisFrame;
    vec2 Direction        = Normalize2(DeltaPThisFrame);
    r32 DistanceRemaining = Length(DeltaPThisFrame);

    if(DistanceRemaining > 0)
    {
// TODO(fierydrake): Collisions with triggers should probably be done with
// overlap tests so they don't consume collision steps unecessarily
#define MaxCollisionSteps 4
#define MaxTriggersPerStep 10
        for(u32 CollisionStep = 0; CollisionStep < MaxCollisionSteps; ++CollisionStep)
        {
            collision ClosestCollision = {};
            collision TriggersHit[MaxTriggersPerStep];
            u32 TriggerCount = 0;

            for(u32 OtherEntityIndex = 0; OtherEntityIndex < NearbyEntities.Count; ++OtherEntityIndex)
            {
                entity* OtherEntity = NearbyEntities.Entities + OtherEntityIndex;
                if(OtherEntity->Inactive)
                    continue;

                if(CanCollide(Entity, OtherEntity))
                {
                    for(collision_area* CollisionArea = Entity->FirstCollisionArea; CollisionArea;
                        CollisionArea                 = CollisionArea->Next)
                    {
                        for(collision_area* OtherCollisionArea = OtherEntity->FirstCollisionArea; OtherCollisionArea;
                            OtherCollisionArea                 = OtherCollisionArea->Next)
                        {
                            // TODO(fierydrake): Collisions between two circles
                            if(CollisionArea->Shape == CollisionShape_Circle &&
                               OtherCollisionArea->Shape == CollisionShape_Circle)
                            {
                                // NOTE(fierydrake): Construct the minkowski area
                                r32 Radius          = CollisionArea->Radius;
                                r32 OtherRadius     = OtherCollisionArea->Radius;
                                r32 MinkowskiRadius = Radius + OtherRadius;
                                vec2 MinkowskiP     = OtherEntity->P + OtherCollisionArea->OffsetP;
                                vec2 CollisionAreaP = Entity->P + CollisionArea->OffsetP;

                                // NOTE(fierydrake): Check collision for each boundary of the minkowski area
                                r32 CoefficientA = Square(Direction.X) + Square(Direction.Y);
                                r32 CoefficientB = 2.0f * (Direction.X * (CollisionAreaP.X - MinkowskiP.X) +
                                                           Direction.Y * (CollisionAreaP.Y - MinkowskiP.Y));
                                r32 CoefficientC = Square(CollisionAreaP.X - MinkowskiP.X) +
                                                   Square(CollisionAreaP.Y - MinkowskiP.Y) - Square(MinkowskiRadius);

                                r32 Determinant = Square(CoefficientB) - 4.0f * CoefficientA * CoefficientC;

                                if(Determinant >= 0)
                                {
                                    // NOTE(fierydrake): Full collision (2 roots) or glancing collision (1 root)
                                    r32 CollisionDistance =
                                        (-CoefficientB - SquareRoot(Determinant)) / (2.0f * CoefficientA);
                                    if(CollisionDistance >= 0.0f)
                                    {
                                        vec2 CollisionDelta  = Direction * CollisionDistance;
                                        vec2 CollisionP      = Entity->P + CollisionDelta;
                                        vec2 Normal          = Normalize2(CollisionP - MinkowskiP);
                                        r32 LookAsideEpsilon = 0.001f;
                                        r32 LookAheadEpsilon = LookAsideEpsilon / -Dot(Direction, Normal);
                                        r32 MoveBackEpsilon  = LookAheadEpsilon;

                                        if(CollisionDistance <= DistanceRemaining + LookAheadEpsilon)
                                        {
                                            r32 ConservativeCollisionDistance =
                                                AtLeast(0.0f, CollisionDistance - MoveBackEpsilon);
                                            r32 CollisionDistanceAlongNormal = -Dot(CollisionDelta, Normal);
                                            if(OtherCollisionArea->StopOnCollision)
                                            {
                                                if(!ClosestCollision.Entity ||
                                                   ConservativeCollisionDistance <
                                                       ClosestCollision.ConservativeDistance ||
                                                   (ConservativeCollisionDistance ==
                                                        ClosestCollision.ConservativeDistance &&
                                                    CollisionDistanceAlongNormal <
                                                        ClosestCollision.DistanceAlongNormal))
                                                {
                                                    ClosestCollision.Entity              = OtherEntity;
                                                    ClosestCollision.EntityIndex         = OtherEntityIndex;
                                                    ClosestCollision.CollisionArea       = OtherCollisionArea;
                                                    ClosestCollision.P                   = CollisionP;
                                                    ClosestCollision.Normal              = Normal;
                                                    ClosestCollision.Distance            = CollisionDistance;
                                                    ClosestCollision.DistanceAlongNormal = CollisionDistanceAlongNormal;

                                                    ClosestCollision.ConservativeDistance =
                                                        ConservativeCollisionDistance;
                                                }
                                            }
                                            else
                                            {
                                                // NOTE(fierydrake): We hit a trigger, so don't consume a collision step
                                                // or perform physics collision response. Handle collision event
                                                // handlers
                                                // specially in collision resolution phase.
                                                // TODO(fierydrake): Collapse these assignments with other similar
                                                // areas?
                                                Assert(TriggerCount < ArrayCount(TriggersHit));
                                                collision* TriggerCollision           = TriggersHit + TriggerCount++;
                                                TriggerCollision->Entity              = OtherEntity;
                                                TriggerCollision->EntityIndex         = OtherEntityIndex;
                                                TriggerCollision->CollisionArea       = OtherCollisionArea;
                                                TriggerCollision->P                   = CollisionP;
                                                TriggerCollision->Normal              = Normal;
                                                TriggerCollision->Distance            = CollisionDistance;
                                                TriggerCollision->DistanceAlongNormal = CollisionDistanceAlongNormal;

                                                TriggerCollision->ConservativeDistance = ConservativeCollisionDistance;
                                            }
                                        }
                                    }
                                }
                            }

                            // NOTE(fierydrake): Collisions between two arbitrarily rotated rectangles
                            if(CollisionArea->Shape == CollisionShape_Rect &&
                               OtherCollisionArea->Shape == CollisionShape_Rect)
                            {
                                // NOTE(fierydrake): Construct the minkowski area
                                vec2 HalfDim   = CollisionArea->Dim * 0.5f;
                                vec2 Corners[] = {
                                    RotateVec2(Entity->Angle, Vec2(-HalfDim.X, -HalfDim.Y)),
                                    RotateVec2(Entity->Angle, Vec2(HalfDim.X, -HalfDim.Y)),
                                    RotateVec2(Entity->Angle, Vec2(HalfDim.X, HalfDim.Y)),
                                    RotateVec2(Entity->Angle, Vec2(-HalfDim.X, HalfDim.Y)),
                                };

                                vec2 OtherHalfDim   = OtherCollisionArea->Dim * 0.5f;
                                vec2 OtherCorners[] = {
                                    RotateVec2(OtherEntity->Angle, Vec2(-OtherHalfDim.X, -OtherHalfDim.Y)),
                                    RotateVec2(OtherEntity->Angle, Vec2(OtherHalfDim.X, -OtherHalfDim.Y)),
                                    RotateVec2(OtherEntity->Angle, Vec2(OtherHalfDim.X, OtherHalfDim.Y)),
                                    RotateVec2(OtherEntity->Angle, Vec2(-OtherHalfDim.X, OtherHalfDim.Y)),
                                };

                                vec2 MinkowskiPoints[ArrayCount(Corners) + ArrayCount(OtherCorners)];
                                // NOTE(fierydrake): This path can handle pairs of arbitrary convex polygons (vertices
                                //                   must be wound anti-clockwise)
                                u32 MinkowskiPointCount = FastMinkowskiSumNxM(MinkowskiPoints,
                                                                              ArrayCount(MinkowskiPoints),
                                                                              Corners,
                                                                              ArrayCount(Corners),
                                                                              OtherCorners,
                                                                              ArrayCount(OtherCorners));

                                vec2 MinkowskiP     = OtherEntity->P + OtherCollisionArea->OffsetP;
                                vec2 CollisionAreaP = Entity->P + CollisionArea->OffsetP;

                                // NOTE(fierydrake): Check collision for each boundary of the minkowski area
                                for(u32 MinkowskiIndex = 0; MinkowskiIndex < MinkowskiPointCount; ++MinkowskiIndex)
                                {
                                    line_segment Boundary = {
                                        MinkowskiP + MinkowskiPoints[MinkowskiIndex],
                                        MinkowskiP + MinkowskiPoints[(MinkowskiIndex + 1) % MinkowskiPointCount],
                                    };

                                    // TODO(fierydrake): PERF: Infer normal direction from polygon winding, instead of
                                    // center
                                    //                         point?
                                    vec2 Normal          = Normalize2(LineBoundaryNormal(Boundary, MinkowskiP));
                                    r32 LookAsideEpsilon = 0.001f;
                                    r32 LookAheadEpsilon = LookAsideEpsilon / -Dot(Direction, Normal);
                                    r32 MoveBackEpsilon  = LookAheadEpsilon;

                                    r32 CollisionDistanceAlongNormal = Dot(CollisionAreaP - Boundary.A, Normal);
                                    r32 DeltaPRemainingTowardsLine   = -Dot(DeltaPRemaining, Normal);
                                    if(CollisionDistanceAlongNormal >= 0.0f && DeltaPRemainingTowardsLine >= 0.0f &&
                                       CollisionDistanceAlongNormal - DeltaPRemainingTowardsLine < LookAsideEpsilon)
                                    {
                                        // NOTE(fierydrake): Collision this frame (+Epsilon)
                                        vec2 BoundaryAToB            = Boundary.B - Boundary.A;
                                        vec2 Line                    = Normalize2(BoundaryAToB);
                                        r32 DeltaPRemainingAlongLine = ABS(Dot(DeltaPRemaining, Line));

                                        r32 SimilarityFactor = CollisionDistanceAlongNormal /
                                                               (DeltaPRemainingTowardsLine + LookAsideEpsilon);
                                        vec2 CollisionDelta = DeltaPRemaining * SimilarityFactor;
                                        vec2 CollisionP     = Entity->P + CollisionDelta;

                                        // NOTE(fierydrake): Is the collision within the line segment extents?
                                        vec2 BoundaryAToCollision = CollisionP - Boundary.A;
                                        if(Dot(BoundaryAToCollision, Line) > 0 &&
                                           LengthSq(BoundaryAToCollision) < LengthSq(BoundaryAToB))
                                        {
                                            r32 CollisionDistance = Length(CollisionDelta);
                                            r32 ConservativeCollisionDistance =
                                                AtLeast(0, CollisionDistance - MoveBackEpsilon);

                                            if(ConservativeCollisionDistance <= DistanceRemaining)
                                            {
                                                if(OtherCollisionArea->StopOnCollision)
                                                {
                                                    if(!ClosestCollision.Entity ||
                                                       ConservativeCollisionDistance <
                                                           ClosestCollision.ConservativeDistance ||
                                                       (ConservativeCollisionDistance ==
                                                            ClosestCollision.ConservativeDistance &&
                                                        CollisionDistanceAlongNormal <
                                                            ClosestCollision.DistanceAlongNormal))
                                                    {
                                                        ClosestCollision.Entity        = OtherEntity;
                                                        ClosestCollision.EntityIndex   = OtherEntityIndex;
                                                        ClosestCollision.CollisionArea = OtherCollisionArea;
                                                        ClosestCollision.P             = CollisionP;
                                                        ClosestCollision.Normal        = Normal;
                                                        ClosestCollision.Distance      = CollisionDistance;
                                                        ClosestCollision.DistanceAlongNormal =
                                                            CollisionDistanceAlongNormal;

                                                        ClosestCollision.ConservativeDistance =
                                                            ConservativeCollisionDistance;
                                                    }
                                                }
                                                else
                                                {
                                                    // NOTE(fierydrake): We hit a trigger, so don't consume a collision
                                                    // step
                                                    // or perform physics collision response. Handle collision event
                                                    // handlers
                                                    // specially in collision resolution phase.
                                                    // TODO(fierydrake): Collapse these assignments with other similar
                                                    // areas?
                                                    Assert(TriggerCount < ArrayCount(TriggersHit));
                                                    collision* TriggerCollision     = TriggersHit + TriggerCount++;
                                                    TriggerCollision->Entity        = OtherEntity;
                                                    TriggerCollision->EntityIndex   = OtherEntityIndex;
                                                    TriggerCollision->CollisionArea = OtherCollisionArea;
                                                    TriggerCollision->P             = CollisionP;
                                                    TriggerCollision->Normal        = Normal;
                                                    TriggerCollision->Distance      = CollisionDistance;
                                                    TriggerCollision->DistanceAlongNormal =
                                                        CollisionDistanceAlongNormal;

                                                    TriggerCollision->ConservativeDistance =
                                                        ConservativeCollisionDistance;
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // NOTE(fierydrake): Deal with triggers
            if(TriggerCount > 0)
            {
                for(u32 TriggerIndex = 0; TriggerIndex < TriggerCount; ++TriggerIndex)
                {
                    collision* TriggerCollision = TriggersHit + TriggerIndex;
                    HandleCollision(GameState, EntityIndex, TriggerCollision->EntityIndex);
                }
            }

            // NOTE(fierydrake): Process results of this iteration's collision finding
            if(ClosestCollision.Entity)
            {
                Assert(NotZeroVec2(ClosestCollision.Normal));
                Assert(ClosestCollision.ConservativeDistance >= 0);

                Entity->P         = Entity->P + Direction * ClosestCollision.ConservativeDistance;
                DistanceRemaining = AtLeast(
                    0,
                    DistanceRemaining - ClosestCollision.ConservativeDistance); // TODO: Accuracy of DistanceRemaining

                HandleCollision(GameState, EntityIndex, ClosestCollision.EntityIndex);

                if(ClosestCollision.CollisionArea->StopOnCollision)
                {
                    // DistanceRemaining = ...; // TODO: Cancel some amount of remaining movement?
                    // NOTE(fierydrake): Hacky collision robustness
                    // Point normal slightly away from surface so the direction
                    // adjustment doesn't push the entity too close to the collision
                    // line with small incremental drifts
                    r32 Epsilon            = 0.0001f;
                    vec2 AdjustedNormals[] = {
                        RotateVec2(Epsilon, ClosestCollision.Normal), RotateVec2(-Epsilon, ClosestCollision.Normal),
                    };
                    r32 AdjustedProducts[] = {
                        Dot(Direction, AdjustedNormals[0]), Dot(Direction, AdjustedNormals[1]),
                    };
                    u32 AdjustmentToUse = (AdjustedProducts[0] < AdjustedProducts[1]) ? 0 : 1;
                    Direction  = Direction - AdjustedNormals[AdjustmentToUse] * AdjustedProducts[AdjustmentToUse];
                    Entity->dP = Entity->dP -
                                 AdjustedNormals[AdjustmentToUse] * Dot(Entity->dP, AdjustedNormals[AdjustmentToUse]);
                }

                DeltaPRemaining = Direction * DistanceRemaining;
                if(DistanceRemaining == 0)
                    break;
            }
            else
            {
                Entity->P = Entity->P + DeltaPRemaining;
                break;
            }
        }
    }
}

internal void
UpdateEntities(game_state* GameState, game_input* Input, render_buffer* RenderBuffer, transient_state* TranState)
{
#if 0
    // NOTE(fierydrake): Fixed time step
    r32 PhysicsDeltaTime = 1.0f / 15.0f;
    r32 RemainingTime;
    for (RemainingTime = Input->dtForFrame + GameState->PhysicsTimeToCarry;
         RemainingTime > PhysicsDeltaTime;
         RemainingTime -= PhysicsDeltaTime)
    {
#else
    r32 PhysicsDeltaTime = Input->dtForFrame;
#endif
    for(u32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex)
    {
        entity* Entity = GameState->Entities + EntityIndex;
        if(Entity->Inactive)
            continue;
        switch(Entity->Type)
        {
            case EntityType_Player:
            {
                if(Entity->Player.ID == 1)
                {
                    // TODO(fierydrake): Where does this control code really belong?
                    // NOTE(zak): Probably the game correct?
                    vec2 InputDirection = Vec2i(0, 0);
                    if(Input->Controllers[0].MoveLeft.EndedDown)
                    {
                        InputDirection.X = -1;
                    }
                    if(Input->Controllers[0].MoveRight.EndedDown)
                    {
                        InputDirection.X = 1;
                    }
                    if(Input->Controllers[0].MoveBackward.EndedDown)
                    {
                        InputDirection.Y = 1;
                    }
                    if(Input->Controllers[0].MoveForward.EndedDown)
                    {
                        InputDirection.Y = -1;
                    }
                    if(NotZeroVec2(InputDirection))
                    {
                        InputDirection        = Normalize2(InputDirection);
                        Entity->Player.Facing = PositionToDirection(InputDirection);
                    }
                    vec2 ddP = InputDirection * Entity->Player.Acceleration;
                    MoveEntity(GameState, Entity, EntityIndex, PhysicsDeltaTime, ddP, RenderBuffer, TranState);
                }
            }
            break;

            case EntityType_Wall:
            case EntityType_PowerUp:
            {
            }
            break;

            default:
            {
                // TODO: Clean up assert
                Assert(!"Invalid code path");
            }
            break;
        }
    }
#if 0
    }
    GameState->PhysicsTimeToCarry = RemainingTime;
#endif
}
