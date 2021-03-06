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
#include "removepoints.h"

#include "checksurfaceintegrity.h"

#include "geometrytools.h"
using namespace GeometryTools;

#include <cmath>
using namespace std;

#include <iostream>
using namespace std;

RemovePoints::RemovePoints() : SurfaceOperation()
{
  setQuickSave( true );
  getSet("surface meshing", "point removal threshold", 2, m_Threshold);
  m_ProtectFeatureEdges = false;
  m_PerformGeometricChecks = true;
}

void RemovePoints::markFeatureEdges()
{
  l2g_t  nodes = getPartNodes();
  g2l_t _nodes = getPartLocalNodes();
  l2g_t  cells = getPartCells();
  l2l_t  c2c   = getPartC2C();
  EG_VTKDCN(vtkDoubleArray, characteristic_length_desired, m_Grid, "node_meshdensity_desired" );
  m_IsFeatureNode.resize(nodes.size());
  for (int i = 0; i < m_IsFeatureNode.size(); ++i) {
    m_IsFeatureNode[i] = false;
  }
  if (m_ProtectFeatureEdges) {
    EG_BUG;// needs to be adapted to multiple volumes first! c2c is undefined!
    for (int i_cells = 0; i_cells < cells.size(); ++i_cells) {
      vtkIdType id_cell1 = cells[i_cells];
      vtkIdType N_pts, *pts;
      m_Part.getGrid()->GetCellPoints(id_cell1, N_pts, pts);
      for (int j = 0; j < c2c[i_cells].size(); ++j) {
        int j_cells = c2c[i_cells][j];
        vtkIdType id_cell2 = cells[j_cells];
        vtkIdType id_node1 = pts[j];
        vtkIdType id_node2 = pts[0];
        if (j < c2c[i_cells].size()-1) {
          id_node2 = pts[j+1];
        }
        vec3_t x1, x2;
        m_Grid->GetPoint(id_node1, x1.data());
        m_Grid->GetPoint(id_node2, x2.data());
        double L = (x1-x2).abs();
        //if (L > 0.5*characteristic_length_desired->GetValue(id_node1)) {
        {
          vec3_t n1 = GeometryTools::cellNormal(m_Part.getGrid(), id_cell1);
          vec3_t n2 = GeometryTools::cellNormal(m_Part.getGrid(), id_cell2);
          if (GeometryTools::angle(n1, n2) > m_FeatureAngle) {
            m_IsFeatureNode[_nodes[id_node1]] = true;
            m_IsFeatureNode[_nodes[id_node2]] = true;
          }
        }
      }
    }
    int N = 0;
    for (int i = 0; i < m_IsFeatureNode.size(); ++i) {
      if (m_IsFeatureNode[i]) {
        ++N;
      }
    }
    cout << N << " nodes on feature edges (angle >= " << GeometryTools::rad2deg(m_FeatureAngle) << "deg)" << endl;
  }
}

void RemovePoints::operate()
{
  int N1 = m_Grid->GetNumberOfPoints();
  
  markFeatureEdges();
  
  QVector<vtkIdType> selected_cells;
  getSurfaceCells(m_BoundaryCodes, selected_cells, m_Grid);
  QVector<vtkIdType> selected_nodes;
  getNodesFromCells(selected_cells, selected_nodes, m_Grid);

  setAllSurfaceCells();
  l2l_t  n2n   = getPartN2N();
  g2l_t _nodes = getPartLocalNodes();
  l2g_t  nodes = getPartNodes();
  
  UpdatePotentialSnapPoints(false);
  
  EG_VTKDCN(vtkCharArray,   node_type, m_Grid, "node_type" );
  EG_VTKDCC(vtkIntArray,    cell_code, m_Grid, "cell_code" );
  EG_VTKDCN(vtkDoubleArray, characteristic_length_desired,        m_Grid, "node_meshdensity_desired" );

  // global values
  QVector <vtkIdType> all_deadcells;
  QVector <vtkIdType> all_mutatedcells;
  int num_newpoints = 0;
  int num_newcells = 0;
  
  QVector <bool> marked_nodes(nodes.size(), false);
  
  QVector <vtkIdType> deadnode_vector;
  QVector <vtkIdType> snappoint_vector;
  
  //count
  for (int i_selected_nodes = 0; i_selected_nodes < selected_nodes.size(); ++i_selected_nodes) {
    vtkIdType id_node = selected_nodes[i_selected_nodes];
    int i_nodes = _nodes[id_node];
    if (node_type->GetValue(id_node) != VTK_FIXED_VERTEX) {
      if (!marked_nodes[i_nodes] && !m_IsFeatureNode[i_nodes]) {
        vec3_t xi;
        m_Grid->GetPoint(id_node, xi.data());
        double cl_node = characteristic_length_desired->GetValue(id_node);
        bool remove_node = false;
        for (int j = 0; j < n2n[i_nodes].size(); ++j) {
          vtkIdType id_neigh = nodes[n2n[i_nodes][j]];
          double cl_neigh = characteristic_length_desired->GetValue(id_neigh);
          vec3_t xj;
          m_Grid->GetPoint(id_neigh, xj.data());
          double L = (xi-xj).abs();
          if (L < 0.5*(cl_node+cl_neigh)/m_Threshold) {
            remove_node = true;
            break;
          }
        }
        if (remove_node) {
          // local values
          QVector <vtkIdType> dead_cells;
          QVector <vtkIdType> mutated_cells;
          int l_num_newpoints = 0;
          int l_num_newcells = 0;
          vtkIdType snap_point = FindSnapPoint(id_node, dead_cells, mutated_cells, l_num_newpoints, l_num_newcells, marked_nodes);
          if(snap_point >= 0) {
            // add deadnode/snappoint pair
            deadnode_vector.push_back(id_node);
            snappoint_vector.push_back(snap_point);
            // update global values
            num_newpoints += l_num_newpoints;
            num_newcells  += l_num_newcells;
            all_deadcells += dead_cells;
            all_mutatedcells += mutated_cells;
            // mark neighbour nodes
            foreach(int i_node_neighbour, n2n[_nodes[id_node]]) {
              marked_nodes[nodes[i_node_neighbour]] = true;
            }
          }
        }
      }
    }
  }

  //delete
  DeleteSetOfPoints(deadnode_vector, snappoint_vector, all_deadcells, all_mutatedcells, num_newpoints, num_newcells);

  int N2 = m_Grid->GetNumberOfPoints();
  m_NumRemoved = N1 - N2;
}

/// \todo finish this function and optimize it.
bool RemovePoints::checkForDestroyedVolumes( vtkIdType id_node1, vtkIdType id_node2, int& N_common_points )
{
  if(id_node1==id_node2) EG_BUG;
  
  l2l_t  n2n   = getPartN2N();
  l2l_t  n2c   = getPartN2C();
  g2l_t _nodes = getPartLocalNodes();
  l2g_t nodes  = getPartNodes();
  l2g_t cells = getPartCells();
  
  QVector<int> node1_neighbours = n2n[_nodes[id_node1]];
  QVector<int> node2_neighbours = n2n[_nodes[id_node2]];
  QVector<int> intersection;
  qcontIntersection( node1_neighbours, node2_neighbours, intersection );
  // set N_common_points
  N_common_points = intersection.size();
  
  // TEST 0: TOPOLOGICAL: DeadNode, PSP and any common point must belong to a cell.
  for(int i=0; i<intersection.size();i++) {
    int i_common_point_1 = intersection[i];
    vtkIdType id_common_point_1 = nodes[i_common_point_1];
    if(!isCell(id_node1, id_node2, id_common_point_1)) {
      if(DebugLevel>100) {
        qDebug()<<"test 0 failed";
        qDebug()<<"id_node1, id_node2, id_common_point_1="<< id_node1 << id_node2 << id_common_point_1;
      }
      return true;
    }
    // TEST 1: TOPOLOGICAL: Moving DeadNode to PSP must not lay any cell on another cell. => For any pair of common points (cp1,cp2), (cp1,cp2,DeadNode)+(cp1,cp2,PSP) must not be cells at the same time!
    for(int j=i+1; j<intersection.size();j++) {
      int i_common_point_2 = intersection[j];
      vtkIdType id_common_point_2 = nodes[i_common_point_2];
      if( isCell(id_common_point_1, id_common_point_2, id_node1) && isCell(id_common_point_1, id_common_point_2, id_node2) ) {
        if(DebugLevel>100) {
          qDebug()<<"test 1 failed";
          qDebug()<<"id_common_point_1, id_common_point_2, id_node1=" << id_common_point_1 << id_common_point_2 << id_node1;
          qDebug()<<"id_common_point_1, id_common_point_2, id_node2=" << id_common_point_1 << id_common_point_2 << id_node2;
        }
        return true;
      }
    }
  }
  
  /*
  // check if DeadNode, PSP and common points form a tetrahedron.
  if ( n2n[_nodes[intersection1]].contains( _nodes[intersection2] ) ) { //if there's an edge between intersection1 and intersection2
    //check if (node1,intersection1,intersection2) and (node2,intersection1,intersection2) are defined as cells!
    QVector<int> S1 = n2c[_nodes[intersection1]];
    QVector<int> S2 = n2c[_nodes[intersection2]];
    QVector<int> Si;
    qcontIntersection( S1, S2, Si );
    int counter = 0;
    foreach( int i_cell, Si ) {
      vtkIdType num_pts, *pts;
      m_Grid->GetCellPoints( cells[i_cell], num_pts, pts );
      for ( int i = 0; i < num_pts; ++i ) {
        if ( pts[i] == id_node1 || pts[i] == id_node2 ) counter++;
      }
    }
    if ( counter >= 2 ) {
      IsTetra = true;
    }
  }
  */
  return false;
}

int RemovePoints::NumberOfCommonPoints( vtkIdType id_node1, vtkIdType id_node2, bool& IsTetra )
{
  l2l_t  n2n   = getPartN2N();
  l2l_t  n2c   = getPartN2C();
  g2l_t _nodes = getPartLocalNodes();
  l2g_t nodes  = getPartNodes();
  l2g_t cells = getPartCells();
  
  QVector<int> node1_neighbours = n2n[_nodes[id_node1]];
  QVector<int> node2_neighbours = n2n[_nodes[id_node2]];
  QVector<int> intersection;
  qcontIntersection( node1_neighbours, node2_neighbours, intersection );
  int N = intersection.size();
  IsTetra = false;
  if ( N == 2 ) {
    vtkIdType intersection1 = nodes[intersection[0]];
    vtkIdType intersection2 = nodes[intersection[1]];
    
    // test if id_node1, id_node2 and intersection* form a cell
    QVector <vtkIdType> EdgeCells_1i;
    QVector <vtkIdType> EdgeCells_2i;
    QVector <vtkIdType> inter;
    int Ncells;
    
    // intersection1
    Ncells = getEdgeCells( id_node1, intersection1, EdgeCells_1i );
    if ( N != 2 ) {
      qWarning()<<"Ncells="<<Ncells;
      EG_BUG;
    }
    Ncells = getEdgeCells( id_node2, intersection1, EdgeCells_2i );
    if ( Ncells != 2 ) {
      qWarning()<<"Ncells="<<Ncells;
      EG_BUG;
    }
    qcontIntersection( EdgeCells_1i, EdgeCells_2i, inter );
    if ( inter.size() <= 0 ) EG_BUG;// (id_node1, id_node2, intersection1) is not a cell
    
    // intersection2
    Ncells = getEdgeCells( id_node1, intersection2, EdgeCells_1i );
    if ( Ncells != 2 ) {
      qWarning()<<"Ncells="<<Ncells;
      EG_BUG;
    }
    Ncells = getEdgeCells( id_node2, intersection2, EdgeCells_2i );
    if ( Ncells != 2 ) {
      qWarning()<<"Ncells="<<Ncells;
      EG_BUG;
    }
    qcontIntersection( EdgeCells_1i, EdgeCells_2i, inter );
    if ( inter.size() <= 0 ) EG_BUG;// (id_node1, id_node2, intersection2) is not a cell
    
    // check if DeadNode, PSP and common points form a tetrahedron.
    if ( n2n[_nodes[intersection1]].contains( _nodes[intersection2] ) ) { //if there's an edge between intersection1 and intersection2
      //check if (node1,intersection1,intersection2) and (node2,intersection1,intersection2) are defined as cells!
      QVector<int> S1 = n2c[_nodes[intersection1]];
      QVector<int> S2 = n2c[_nodes[intersection2]];
      QVector<int> Si;
      qcontIntersection( S1, S2, Si );
      int counter = 0;
      foreach( int i_cell, Si ) {
        vtkIdType num_pts, *pts;
        m_Grid->GetCellPoints( cells[i_cell], num_pts, pts );
        for ( int i = 0; i < num_pts; ++i ) {
          if ( pts[i] == id_node1 || pts[i] == id_node2 ) counter++;
        }
      }
      if ( counter >= 2 ) {
        IsTetra = true;
      }
    }
  }
  return( N );
}

bool RemovePoints::flippedCell2(vtkIdType id_node, vec3_t x_new)
{
  /*
  for (int i = 0; i < m_Part.n2cGSize(id_node); ++i) {
    
    vtkIdType id_cell = m_Part.n2cGG(id_node, i);
    
    vtkIdType N_pts, *pts;
    m_Grid->GetCellPoints(id_cell, N_pts, pts);
    if(N_pts!=3) EG_BUG;
    
    int i_pts=0;
    for(i_pts=0; i_pts<N_pts; i_pts++) {
      if(pts[i_pts]==id_node) break;
    }
    if(pts[i_pts]!=id_node) EG_BUG;
    
    vec3_t x1, x2, x_old;
    m_Grid->GetPoint(pts[(i_pts+1)%N_pts],x1.data());
    m_Grid->GetPoint(pts[(i_pts+2)%N_pts],x2.data());
    
    vec3_t old_cell_normal = GeometryTools::triNormal(x_old, x1, x2);
    vec3_t new_cell_normal = GeometryTools::triNormal(x_new, x1, x2);
    
    if(old_cell_normal.abs2()==0) EG_BUG;
    if(old_cell_normal.abs2()==0) EG_BUG;
    
    GeometryTools::cellNormal(m_Grid, );
    cell_normals.normalise();
    
    vtkIdType *pts;
    vtkIdType npts;
    vec3_t n(0,0,0);
    grid->GetCellPoints(i, npts, pts);
    if (npts == 3) {
      return triNormal(grid,pts[0],pts[1],pts[2]);
    } else if (npts == 4) {
      return quadNormal(grid,pts[0],pts[1],pts[2],pts[3]);
    } else {
      EG_BUG;
    }
    return n;
    
  }
  */
  return true;
}

/// \todo adapt for multiple volumes
bool RemovePoints::flippedCell(vtkIdType id_node, vec3_t x_new, vtkIdType id_cell)
{
  vec3_t x_old;
  m_Grid->GetPoint(id_node, x_old.data());
  
  vec3_t n(0,0,0);
  bool move = true;
  QVector<vec3_t> cell_normals(m_Part.n2cGSize(id_node));
  double A_max = 0;
  for (int i = 0; i < m_Part.n2cGSize(id_node); ++i) {
    double A = fabs(GeometryTools::cellVA(m_Grid, m_Part.n2cGG(id_node, i)));
    A_max = max(A, A_max);
    cell_normals[i] = GeometryTools::cellNormal(m_Grid, m_Part.n2cGG(id_node, i));
    cell_normals[i].normalise();
  }
  int N = 0;
  for (int i = 0; i < m_Part.n2cGSize(id_node); ++i) {
    double A = fabs(GeometryTools::cellVA(m_Grid, m_Part.n2cGG(id_node, i)));
    if (A > 0.01*A_max) {
      n += cell_normals[i];
      ++N;
    }
  }
  if (N == 0) {
    move = false;
  } else {
    n.normalise();
    double L_max = 0;
    for (int i = 0; i < m_Part.n2nGSize(id_node); ++i) {
      vec3_t xn;
      m_Grid->GetPoint(m_Part.n2nGG(id_node, i), xn.data());
      double L = (xn - x_old).abs();
      L_max = max(L, L_max);
    }
    vec3_t x_summit = x_old + L_max*n;
    vec3_t x[3];
    vtkIdType N_pts, *pts;
    m_Grid->GetCellPoints(id_cell, N_pts, pts);
    if (N_pts != 3) {
      EG_BUG;
    }
    for (int j = 0; j < N_pts; ++j) {
      m_Grid->GetPoint(pts[j], x[j].data());
    }
    if (GeometryTools::tetraVol(x[0], x[1], x[2], x_summit, false) <= 0) {
      move = false;
    }
  }

  return !move;
}

// DEFINITIONS:
// Normal cell: nothing has changed
// Dead cell: the cell does not exist anymore
// Mutated cell: the cell's form has changed

///\todo Clean up this function
vtkIdType RemovePoints::FindSnapPoint(vtkIdType DeadNode,
                                      QVector<vtkIdType>& DeadCells,
                                      QVector<vtkIdType>& MutatedCells,
                                      int& num_newpoints,
                                      int& num_newcells,
                                      const QVector<bool>& marked_nodes)
{
  // preparations
  l2l_t n2c = getPartN2C();
  g2l_t _nodes = getPartLocalNodes();
  l2g_t cells = getPartCells();
  
  EG_VTKDCN( vtkCharArray, node_type, m_Grid, "node_type" );
  if ( node_type->GetValue( DeadNode ) == VTK_FIXED_VERTEX ) {
    cout << "ERROR: unable to remove fixed vertex." << endl;
    EG_BUG;
    return( -1 );
  }
  
  vtkIdType SnapPoint = -1;
  
  QVector <vtkIdType> PSP_vector = getPotentialSnapPoints( DeadNode );
  foreach( vtkIdType PSP, PSP_vector ) { // loop through potential snappoints
    
    bool IsValidSnapPoint = true;
    
    // TEST -1 : TOPOLOGICAL : Is the node already marked?
    if(marked_nodes[PSP]) {
      IsValidSnapPoint = false; continue;
    }
    
    // TEST 0: TOPOLOGICAL: DeadNode, PSP and any common point must belong to a cell.
    // TEST 1: TOPOLOGICAL: Moving DeadNode to PSP must not lay any cell on another cell.
    int N_common_points = 0;
    if(checkForDestroyedVolumes(DeadNode, PSP, N_common_points)) {
      if ( DebugLevel > 10 ) cout << "Sorry, but you are not allowed to move point " << DeadNode << " to point " << PSP << " because it would destroy volume." << endl;
      IsValidSnapPoint = false; continue;
    }
    
    //count number of points and cells to remove + analyse cell transformations
    num_newpoints = -1;
    num_newcells = 0;
    DeadCells.clear();
    MutatedCells.clear();
    foreach( int i_cell, n2c[_nodes[DeadNode]] ) { //loop through potentially dead cells
      vtkIdType id_cell = cells[i_cell];
      
      //get points around cell
      vtkIdType num_pts, *pts;
      m_Grid->GetCellPoints( id_cell, num_pts, pts );
      
      if ( num_pts != 3 ) {
        cout << "ERROR: Non-triangle detected!" << endl;
        EG_BUG;
      }
      
      bool ContainsSnapPoint = false;
      bool invincible = false; // a point with only one cell is declared invincible.
      int index_PSP, index_DeadNode;
      for ( int i = 0; i < num_pts; ++i ) {
        if ( pts[i] == PSP ) {
          ContainsSnapPoint = true;
          index_PSP = i;
        }
        if ( pts[i] == DeadNode ) {
          index_DeadNode = i;
        }
        if ( pts[i] != DeadNode && pts[i] != PSP &&  n2c[_nodes[pts[i]]].size() <= 1 ) {
          invincible = true;
        }
      }
      
      if ( ContainsSnapPoint ) { // potential dead cell
        if ( invincible ) {
          // TEST 3: TOPOLOGICAL: Check that empty lines aren't left behind when a cell is killed
          if ( DebugLevel > 10 ) cout << "Sorry, but you are not allowed to move point " << DeadNode << " to point " << PSP << " because it would leave behind empty lines." << endl;
          IsValidSnapPoint = false; continue;
        }
        else {
          if(IsValidSnapPoint) {
            DeadCells.push_back( id_cell );
            num_newcells -= 1;
          }
        }
      }
      else { // if the cell does not contain the SnapPoint (potential mutated cell)
        
        vtkIdType OldTriangle[3];
        vtkIdType NewTriangle[3];
        
        for ( int i = 0; i < num_pts; ++i ) {
          OldTriangle[i] = pts[i];
          NewTriangle[i] = (( pts[i] == DeadNode ) ? PSP : pts[i] );
        }
        vec3_t Old_N = triNormal( m_Grid, OldTriangle[0], OldTriangle[1], OldTriangle[2] );
        vec3_t New_N = triNormal( m_Grid, NewTriangle[0], NewTriangle[1], NewTriangle[2] );
        
        // TEST 4: GEOMETRICAL: area + inversion check
        if (m_PerformGeometricChecks) {
          if ( Old_N*New_N < 0 || New_N*New_N < Old_N*Old_N*1. / 100. ) {
            if ( DebugLevel > 10 ) cout << "Sorry, but you are not allowed to move point " << DeadNode << " to point " << PSP << " because area+inversion check failed." << endl;
            IsValidSnapPoint = false; continue;
          }

          // TEST 5: GEOMETRICAL: flipped cell test
/*          vec3_t P;
          m_Grid->GetPoint( PSP, P.data() );
          if (flippedCell(DeadNode, P, id_cell)) {
            if ( DebugLevel > 10 ) cout << "Sorry, but you are not allowed to move point " << DeadNode << " to point " << PSP << "." << endl;
            IsValidSnapPoint = false;
          }*/
          
          // id_cell
          // DeadNode -> PSP
          // index_DeadNode -> index_PSP
          /*
          vec3_t x_old, x_new;
          m_Grid->GetPoint( DeadNode, x_old.data() );
          m_Grid->GetPoint( PSP, x_new.data() );
          m_Grid->GetPoint( pts[], x_new.data() );
          m_Grid->GetPoint( PSP, x_new.data() );
          
          if (flippedCell2(DeadNode, P, id_cell)) {
            if ( DebugLevel > 10 ) cout << "Sorry, but you are not allowed to move point " << DeadNode << " to point " << PSP << " because flipped cell test failed." << endl;
            IsValidSnapPoint = false; continue;
          }
          */
        }
        
        //mutated cell
        if(IsValidSnapPoint) MutatedCells.push_back( id_cell );
      }
    }
    
    // TEST 6: TOPOLOGICAL: survivor check
    if ( m_Grid->GetNumberOfCells() + num_newcells <= 0 ) {
      if ( DebugLevel > 10 ) cout << "Sorry, but you are not allowed to move point " << DeadNode << " to point " << PSP << " because survivor test failed." << endl;
      IsValidSnapPoint = false; continue;
    }
    
    if ( IsValidSnapPoint ) {
      SnapPoint = PSP;
      break;
    }
  } //end of loop through potential SnapPoints
  
  /*
  if(SnapPoint>=0 && DeadCells.size()!=2) {
    qWarning() << "SnapPoint>=0 && DeadCells.size()!=2";
    qWarning() << "SnapPoint==" << SnapPoint << " && DeadCells.size()=" << DeadCells.size();
    EG_BUG;
  }
  */
  
  return ( SnapPoint );
}
//End of FindSnapPoint

bool RemovePoints::DeleteSetOfPoints(const QVector<vtkIdType>& deadnode_vector,
                                     const QVector<vtkIdType>& snappoint_vector,
                                     const QVector<vtkIdType>& all_deadcells,
                                     const QVector<vtkIdType>& all_mutatedcells,
                                     int& num_newpoints, int& num_newcells)
{
  int initial_num_points = m_Grid->GetNumberOfPoints();
  
  //src grid info
  int num_points = m_Grid->GetNumberOfPoints();
  int num_cells = m_Grid->GetNumberOfCells();
  
/*  if ( num_newcells != 2*num_newpoints ) {
    EG_BUG;
  }*/
  
  //allocate
  EG_VTKSP( vtkUnstructuredGrid, dst );
  allocateGrid( dst, num_cells + num_newcells, num_points + num_newpoints );
  
  //vector used to redefine the new point IDs
  QVector <vtkIdType> OffSet( num_points );
  
  //copy undead points
  QVector<bool> is_deadnode(m_Grid->GetNumberOfPoints(), false);
  QVector<int> glob2dead(m_Grid->GetNumberOfPoints(), -1);
  for (int i_deadnodes = 0; i_deadnodes < deadnode_vector.size(); ++i_deadnodes) {
    vtkIdType id_node = deadnode_vector[i_deadnodes];
    if (id_node > m_Grid->GetNumberOfPoints()) {
      EG_BUG;
    }
    is_deadnode[id_node] = true;
    glob2dead[id_node] = i_deadnodes;
  }
  vtkIdType dst_id_node = 0;
  for (vtkIdType src_id_node = 0; src_id_node < num_points; ++src_id_node) {//loop through src points
    OffSet[src_id_node] = src_id_node - dst_id_node;
    if (!is_deadnode[src_id_node]) { //if the node isn't dead, copy it
      vec3_t x;
      m_Grid->GetPoints()->GetPoint( src_id_node, x.data() );
      dst->GetPoints()->SetPoint( dst_id_node, x.data() );
      copyNodeData( m_Grid, src_id_node, dst, dst_id_node );
      dst_id_node++;
    } else {
      if ( DebugLevel > 0 ) {
        cout << "dead node encountered: src_id_node=" << src_id_node << " dst_id_node=" << dst_id_node << endl;
      }
    }
  }

  //Copy undead cells
  QVector<bool> is_alldeadcell(m_Grid->GetNumberOfCells(), false);
  foreach (vtkIdType id_cell, all_deadcells) {
    is_alldeadcell[id_cell] = true;
  }
  QVector<bool> is_allmutatedcell(m_Grid->GetNumberOfCells(), false);
  foreach (vtkIdType id_cell, all_mutatedcells) {
    is_allmutatedcell[id_cell] = true;
  }
  for (vtkIdType id_cell = 0; id_cell < m_Grid->GetNumberOfCells(); ++id_cell) {//loop through src cells
    if (!is_alldeadcell[id_cell]) { //if the cell isn't dead
      vtkIdType src_num_pts, *src_pts;
      vtkIdType dst_num_pts, dst_pts[3];
      m_Grid->GetCellPoints( id_cell, src_num_pts, src_pts );
      vtkIdType type_cell = m_Grid->GetCellType( id_cell );
      
      dst_num_pts = 3;//src_num_pts;
      
      if (is_allmutatedcell[id_cell]) { //mutated cell
        int num_deadnode = 0;
        for ( int i = 0; i < src_num_pts; i++ ) {
          int DeadIndex = glob2dead[src_pts[i]];
          if (DeadIndex != -1) {
            dst_pts[i] = snappoint_vector[DeadIndex] - OffSet[snappoint_vector[DeadIndex]]; // dead node
            num_deadnode++;
          } else {
            dst_pts[i] = src_pts[i] - OffSet[src_pts[i]]; // not a dead node
          }
        }
        if ( num_deadnode != 1 ) {
          qWarning() << "FATAL ERROR: Mutated cell has more than one dead node!";
          EG_BUG;
        }
      } else { //normal cell
        if ( DebugLevel > 10 ) {
          cout << "processing normal cell " << id_cell << endl;
        }
        for ( int i = 0; i < src_num_pts; i++ ) {
          if (is_deadnode[src_pts[i]]) {
            qWarning() << "FATAL ERROR: Normal cell contains a dead node!";
            EG_BUG;
          }
          dst_pts[i] = src_pts[i] - OffSet[src_pts[i]];
        }
      }
      // copy the cell
      vtkIdType id_new_cell = dst->InsertNextCell( type_cell, dst_num_pts, dst_pts );
      copyCellData( m_Grid, id_cell, dst, id_new_cell );
    }
  }
  
  makeCopy( dst, m_Grid );
  
  if ( -num_newpoints != deadnode_vector.size() ) {
    EG_BUG;
  }
  
  int final_num_points = m_Grid->GetNumberOfPoints();
  if ( initial_num_points - final_num_points != deadnode_vector.size() ) {
    EG_BUG;
  }
  
  return( true );
}
//End of DeleteSetOfPoints
