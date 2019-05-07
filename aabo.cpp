#include "stdio.h"
#include <vector>
#include <time.h>
#include <math.h>

struct Clock
{
  const clock_t m_start;
  Clock() : m_start(clock())
  {
  }
  float seconds() const
  {
    const clock_t end = clock();
    const float seconds = ((float)(end - m_start)) / CLOCKS_PER_SEC;
    return seconds;
  }
};

struct float2
{
  float x,y;
};

struct float3
{
  float x,y,z;
};
  
float3 operator+(const float3 a, const float3 b)
{
  float3 c = {a.x+b.x, a.y+b.y, a.z+b.z};
  return c;
}

float dot(const float3 a, const float3 b)
{
  return a.x*b.x + a.y*b.y + a.z*b.z;
}

float length(const float3 a)
{
  return sqrtf(dot(a,a));
}

float3 min(const float3 a, const float3 b)
{
  float3 c = {std::min(a.x,b.x), std::min(a.y,b.y), std::min(a.z,b.z)};
  return c;
}

float3 max(const float3 a, const float3 b)
{
  float3 c = {std::max(a.x,b.x), std::max(a.y,b.y), std::max(a.z,b.z)};
  return c;
}

struct float4
{
  float a,b,c,d;
};

float4 min(const float4 a, const float4 b)
{
  float4 c = {std::min(a.a,b.a), std::min(a.b,b.b), std::min(a.c,b.c), std::min(a.d,b.d)};
  return c;
}

float4 max(const float4 a, const float4 b)
{
  float4 c = {std::max(a.a,b.a), std::max(a.b,b.b), std::max(a.c,b.c), std::max(a.d,b.d)};
  return c;
}

typedef float4 AABT;

float random(float lo, float hi)
{
  const int grain = 10000;
  const float t = (rand() % grain) * 1.f/(grain-1);
  return lo + (hi - lo) * t;
}

struct Mesh
{
  std::vector<float3> m_point;
  void Generate(int points, float radius)
  {
    m_point.resize(points);
    for(int p = 0; p < points; ++p)
    {
      do
      {
        m_point[p].x = random(-radius, radius);
        m_point[p].y = random(-radius, radius);
        m_point[p].z = random(-radius, radius);
      } while(length(m_point[p]) > radius);
    }
  }
};

struct Object
{
  Mesh *m_mesh;
  float3 m_position;
  void CalculateAABB(float3* mini, float3* maxi) const
  {
    const float3 xyz = m_position + m_mesh->m_point[0];
    *mini = *maxi = xyz;
    for(int p = 1; p < m_mesh->m_point.size(); ++p)
    {
      const float3 xyz = m_position + m_mesh->m_point[p];
      *mini = min(*mini, xyz);
      *maxi = max(*maxi, xyz);
    }
  }
  void CalculateAABT(AABT* mini, AABT* maxi) const
  { 
    const float3 xyz = m_position + m_mesh->m_point[0];
    const float4 abcd = {xyz.x, xyz.y, xyz.z, -(xyz.x + xyz.y + xyz.z)};
    *mini = *maxi = abcd;
    for(int p = 1; p < m_mesh->m_point.size(); ++p)
    {
      const float3 xyz = m_position + m_mesh->m_point[p];
      const float4 abcd = {xyz.x, xyz.y, xyz.z, -(xyz.x + xyz.y + xyz.z)};
      *mini = min(*mini, abcd);
      *maxi = max(*maxi, abcd);
    }
  };
};

int main(int argc, char* argv[])
{
  Mesh mesh;
  mesh.Generate(100, 1.0f);

  const int kTests = 100;
  
  const int kObjects = 10000000;
  std::vector<Object> objects(kObjects);
  for(int o = 0; o < kObjects; ++o)
  {
    objects[o].m_mesh = &mesh;
    objects[o].m_position.x = random(-50.f, 50.f);
    objects[o].m_position.y = random(-50.f, 50.f);
    objects[o].m_position.z = random(-50.f, 50.f);
  }
  
  std::vector<float3> aabbMin(kObjects);
  std::vector<float3> aabbMax(kObjects);
  for(int a = 0; a < kObjects; ++a)
    objects[a].CalculateAABB(&aabbMin[a], &aabbMax[a]);

  std::vector<float2> aabbX(kObjects);
  std::vector<float2> aabbY(kObjects);
  std::vector<float2> aabbZ(kObjects);
  for(int a = 0; a < kObjects; ++a)
  {
    aabbX[a].x = aabbMin[a].x;
    aabbX[a].y = aabbMax[a].x;
    aabbY[a].x = aabbMin[a].y;
    aabbY[a].y = aabbMax[a].y;
    aabbZ[a].x = aabbMin[a].z;
    aabbZ[a].y = aabbMax[a].z;
  }
  
  std::vector<AABT> aabtMin(kObjects);
  std::vector<AABT> aabtMax(kObjects);
  for(int a = 0; a < kObjects; ++a)
    objects[a].CalculateAABT(&aabtMin[a], &aabtMax[a]);
  
  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const float3 probeMin = aabbMin[test];
      const float3 probeMax = aabbMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const float3 targetMin = aabbMin[t];
        if(targetMin.x <= probeMax.x
        && targetMin.y <= probeMax.y
        && targetMin.z <= probeMax.z)
	{
	  const float3 targetMax = aabbMax[t];
	  if(targetMax.x >= probeMin.x
          && targetMax.y >= probeMin.y
          && targetMax.z >= probeMin.z)  
  	    ++intersections;
	}
      }
    }
    const float seconds = clock.seconds();
    
    printf("box min/max reported %d intersections in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const float2 probeX = aabbX[test];
      const float2 probeY = aabbY[test];
      const float2 probeZ = aabbZ[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const float2 targetX = aabbX[t];
        if(targetX.x <= probeX.y && targetX.y >= probeX.x)
	{
          const float2 targetY = aabbY[t];
          if(targetY.x <= probeY.y && targetY.y >= probeY.x)
    	  {
            const float2 targetZ = aabbZ[t];
	    if(targetZ.x <= probeZ.y && targetZ.y >= probeZ.x)
    	      ++intersections;
	  }
	}
      }
    }
    const float seconds = clock.seconds();
    
    printf("box x/y/z reported %d intersections in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const AABT probeMax = aabtMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const AABT targetMin = aabtMin[t];
        if(targetMin.a <= probeMax.a
        && targetMin.b <= probeMax.b
        && targetMin.c <= probeMax.c
        && targetMin.d <= probeMax.d)
        {
          ++intersections;
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("tetrahedron reported %d intersections in %f seconds\n", intersections, seconds);
  }

  {
    const Clock clock;
    int intersections = 0;
    for(int test = 0; test < kTests; ++test)
    {
      const AABT probeMin = aabtMin[test];
      const AABT probeMax = aabtMax[test];
      for(int t = 0; t < kObjects; ++t)
      {
        const AABT targetMin = aabtMin[t];
        if(targetMin.a <= probeMax.a
        && targetMin.b <= probeMax.b
        && targetMin.c <= probeMax.c
        && targetMin.d <= probeMax.d)
        {
	  const AABT targetMax = aabtMax[t];
	  if(targetMax.a >= probeMin.a
	  && targetMax.b >= probeMin.b
	  && targetMax.c >= probeMin.c
	  && targetMax.d >= probeMin.d)
	  {
	    ++intersections;
	  }
        }
      }
    }
    const float seconds = clock.seconds();
    
    printf("octahedron reported %d intersections in %f seconds\n", intersections, seconds);
  }

  return 0;
}
