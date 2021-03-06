//
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
// +                                                                      +
// + This file is part of enGrid.                                         +
// +                                                                      +
// + Copyright 2008,2009 Oliver Gloth                                     +
// +                                                                      +
// + enGrid is free software: you can redistribute it and/or modify       +
// + it under the terms of the GNU General Public License as published by +
// + the Free Software Foundation, either version 3 of the License, or    +
// + (at your option) any later version.                                  +
// +                                                                      +
// + enGrid is distributed in the hope that it will be useful,            +
// + but WITHOUT ANY WARRANTY; without even the implied warranty of       +
// + MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        +
// + GNU General Public License for more details.                         +
// +                                                                      +
// + You should have received a copy of the GNU General Public License    +
// + along with enGrid. If not, see <http://www.gnu.org/licenses/>.       +
// +                                                                      +
// ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <QVector>

#include "vtkIdList.h"
#include "vtkUnstructuredGrid.h"
#include "math/mathvector.h"
#include "math/smallsquarematrix.h"
#include "egvtkobject.h"

class Triangle : public EgVtkObject {
  public:
    vtkIdType m_id_a, m_id_b, m_id_c;
    vec3_t m_a, m_b, m_c;
    vec3_t m_g1, m_g2, m_g3;
    mat3_t m_G, m_GI;
    double m_A;
    double m_smallest_length;
    bool m_Valid;
    vec3_t m_Normal_a, m_Normal_b, m_Normal_c;

  public:
    QVector <bool> m_has_neighbour; ///< True if edge i has a neighbour in the grid

  public:
    Triangle();
    Triangle(vec3_t a_a, vec3_t a_b, vec3_t a_c);
    Triangle(vtkUnstructuredGrid* a_grid, vtkIdType a_id_a, vtkIdType a_id_b, vtkIdType a_id_c);
    Triangle(vtkUnstructuredGrid* a_grid, vtkIdType a_id_cell);
    void setupTriangle();
    void setDefaults();
  
  public:
    /**
     * Calculates the closest (NOT the projection!) point (xi,ri) of point xp on the triangle.
     * @param xp Point to "project"
     * @param xi Global 3D coordinates of the closest point on the triangle.
     * @param ri Local 3D triangle coordinates of the closest point on the triangle. (0<=ri[0]<=1 and 0<=ri[1]<=1 and ri[2]=0)
     * @param d Distance of xp to (xi,ri)
     * @return True if (xi,ri) is the result of a direct projection on the triangle, else false.
    */
    bool projectOnTriangle(vec3_t xp, vec3_t &xi, vec3_t &ri, double &d, int& side, bool restrict_to_triangle);

    vec3_t local3DToGlobal3D(vec3_t l_M);
    vec3_t global3DToLocal3D(vec3_t g_M);
    vec3_t local2DToGlobal3D(vec2_t l_M);
    vec2_t global3DToLocal2D(vec3_t g_M);

    void saveTriangle(QString filename);
};

#endif
