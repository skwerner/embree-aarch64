// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "primitive.h"

namespace embree
{
  template <int M>
  struct LineMi
  {
    typedef Vec3<vfloat<M>> Vec3vfM;

    /* Virtual interface to query information about the line segment type */
    struct Type : public PrimitiveType
    {
      Type();
      size_t size(const char* This) const;
    };
    static Type type;

  public:

    /* Returns maximal number of stored line segments */
    static __forceinline size_t max_size() { return M; }

    /* Returns required number of primitive blocks for N line segments */
    static __forceinline size_t blocks(size_t N) { return (N+max_size()-1)/max_size(); }

  public:

    /* Default constructor */
    __forceinline LineMi() {  }

    /* Construction from vertices and IDs */
    __forceinline LineMi(Vec3fa* base[M], const vint<M>& geomIDs, const vint<M>& primIDs)
      : geomIDs(geomIDs), primIDs(primIDs)
    {
      for (size_t i=0; i<M; i++)
        v0[i] = base[i];
    }

    /* Returns a mask that tells which line segments are valid */
    __forceinline vbool<M> valid() const { return primIDs != vint<M>(-1); }

    /* Returns if the specified line segment is valid */
    __forceinline bool valid(const size_t i) const { assert(i<M); return geomIDs[i] != -1; }

    /* Returns the number of stored line segments */
    __forceinline size_t size() const { return __bsf(~movemask(valid())); }

    /* Returns the geometry IDs */
    __forceinline vint<M> geomID() const { return geomIDs; }
    __forceinline int geomID(const size_t i) const { assert(i<M); return geomIDs[i]; }

    /* Returns the primitive IDs */
    __forceinline vint<M> primID() const { return primIDs; }
    __forceinline int primID(const size_t i) const { assert(i<M); return primIDs[i]; }

    /* gather the line segments */
    __forceinline void gather(Vec4<vfloat<M>>& p0, Vec4<vfloat<M>>& p1) const;

    /* Fill line segment from line segment list */
    __forceinline void fill(atomic_set<PrimRefBlock>::block_iterator_unsafe& prims, Scene* scene, const bool list)
    {
      vint<M> geomID = -1, primID = -1;
      Vec3fa* v0[M];
      PrimRef& prim = *prims;

      for (size_t i=0; i<M; i++)
      {
        const LineSegments* in = scene->getLineSegments(prim.geomID());
        const unsigned vertexID = in->segment(primID);
        if (prims) {
          geomID[i] = prim.geomID();
          primID[i] = prim.primID();
          v0[i] = (Vec3fa*)in->vertexPtr(vertexID);
          prims++;
        } else {
          assert(i);
          geomID[i] = -1;
          primID[i] = -1;
          v0[i] = v0[i-1];
        }
        if (prims) prim = *prims;
      }

      new (this) LineMi(v0,geomID,primID); // FIXME: use non temporal store
    }

    /* Fill line segment from line segment list */
    __forceinline void fill(const PrimRef* prims, size_t& begin, size_t end, Scene* scene, const bool list)
    {
      vint<M> geomID = -1, primID = -1;
      Vec3fa* v0[M];
      const PrimRef* prim = &prims[begin];

      for (size_t i=0; i<M; i++)
      {
        const LineSegments* in = scene->getLineSegments(prim->geomID());
        const unsigned vertexID = in->segment(prim->primID());
        if (begin<end) {
          geomID[i] = prim->geomID();
          primID[i] = prim->primID();
          v0[i] = (Vec3fa*)in->vertexPtr(vertexID);
          begin++;
        } else {
          assert(i);
          geomID[i] = -1;
          primID[i] = -1;
          v0[i] = v0[i-1];
        }
        if (begin<end) prim = &prims[begin];
      }

      new (this) LineMi(v0,geomID,primID); // FIXME: use non temporal store
    }

  public:
    const Vec3fa* v0[M]; // pointer to 1st vertex
    vint<M> geomIDs;     // geometry ID of mesh
    vint<M> primIDs;     // primitive ID of primitive inside mesh
  };

  template<>
  __forceinline void LineMi<4>::gather(Vec4vf4& p0, Vec4vf4& p1) const
  {
    const vfloat4 a0 = vfloat4::loadu(v0[0]  ), a1 = vfloat4::loadu(v0[1]  ), a2 = vfloat4::loadu(v0[2]  ), a3 = vfloat4::loadu(v0[3]  );
    const vfloat4 b0 = vfloat4::loadu(v0[0]+1), b1 = vfloat4::loadu(v0[1]+1), b2 = vfloat4::loadu(v0[2]+1), b3 = vfloat4::loadu(v0[3]+1);
    transpose(a0,a1,a2,a3,p0.x,p0.y,p0.z,p0.w);
    transpose(b0,b1,b2,b3,p1.x,p1.y,p1.z,p1.w);
  }

  template<int M>
  typename LineMi<M>::Type LineMi<M>::type;

  typedef LineMi<4> Line4i;
}
