#pragma once

#include "SVector.h"
#include "ObjectID.h"
#include "SCollisionInfo.h"

//=================================================================================
struct SSphere
{
    SSphere(SVector position, float radius, TMaterialID materialID)
        : m_position(position)
        , m_radius(radius)
        , m_objectID(GenerateObjectID())
        , m_materialID(materialID)
    {
    }

    SVector     m_position;
    float       m_radius;
    TObjectID   m_objectID;
    TMaterialID m_materialID;
};

//=================================================================================
inline bool RayIntersects (const SVector& rayPos, const SVector& rayDir, const SSphere& sphere, SCollisionInfo& info, TObjectID ignoreObjectId = c_invalidObjectID)
{
    if (ignoreObjectId == sphere.m_objectID)
        return false;

    //get the vector from the center of this circle to where the ray begins.
    SVector m = rayPos - sphere.m_position;

    //get the dot product of the above vector and the ray's vector
    float b = Dot(m, rayDir);

    float c = Dot(m, m) - sphere.m_radius * sphere.m_radius;

    //exit if r's origin outside s (c > 0) and r pointing away from s (b > 0)
    if (c > 0.0 && b > 0.0)
        return false;

    //calculate discriminant
    float discr = b * b - c;

    //a negative discriminant corresponds to ray missing sphere
    if (discr < 0.0)
        return false;

    //not inside til proven otherwise
    bool fromInside = false;

    //ray now found to intersect sphere, compute smallest t value of intersection
    float collisionTime = -b - sqrt(discr);

    //if t is negative, ray started inside sphere so clamp t to zero and remember that we hit from the inside
    if (collisionTime < 0.0)
    {
        collisionTime = -b + sqrt(discr);
        fromInside = true;
    }

    //enforce a max distance if we should
    if (info.m_maxCollisionTime >= 0.0 && collisionTime > info.m_maxCollisionTime)
        return false;

    SVector normal = (rayPos + rayDir * collisionTime) - sphere.m_position;
    Normalize(normal);

    // make sure normal is facing opposite of ray direction.
    // this is for if we are hitting the object from the inside / back side.
    if (Dot(normal, rayDir) > 0.0f)
        normal *= -1.0f;

    info.SuccessfulHit(
        sphere.m_objectID,
        sphere.m_materialID,
        rayPos + rayDir * collisionTime,
        normal,
        collisionTime
    );
    return true;
}