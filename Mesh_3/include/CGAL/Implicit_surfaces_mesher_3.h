// Copyright (c) 2004-2005  INRIA Sophia-Antipolis (France).
// All rights reserved.
//
// This file is part of CGAL (www.cgal.org); you may redistribute it under
// the terms of the Q Public License version 1.0.
// See the file LICENSE.QPL distributed with CGAL.
//
// Licensees holding a valid commercial license may use this file in
// accordance with the commercial license agreement provided with the software.
//
// This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
// WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
//
// $URL$
// $Id$
// 
//
// Author(s)     : Laurent RINEAU

#ifndef CGAL_IMPLICIT_SURFACES_MESHER_3_H
#define CGAL_IMPLICIT_SURFACES_MESHER_3_H

#include <CGAL/Mesh_3/Refine_tets.h>
#include <CGAL/Surface_mesher/Surface_mesher_manifold.h>
#include <CGAL/Surface_mesher/Surface_mesher.h>
#include <CGAL/Surface_mesher/Surface_mesher_visitor.h>
#include <CGAL/Mesh_3/Implicit_surface_mesher_visitor.h>
#include <CGAL/Mesh_3/Refine_tets_visitor.h>

namespace CGAL {

  namespace Mesh_3 
  {
  }

template <
  typename Tr,
  typename Oracle,
  typename Facets_criteria,
  typename Tets_criteria
  >
class Implicit_surfaces_mesher_3
{
public:
  typedef typename Tr::Point Point;
  

  // ** two mesher levels **/

  typedef typename
  Surface_mesher::Surface_mesher<Tr,
				 Oracle,
				 Facets_criteria> Facets_level;

  typedef typename Mesh_3::Refine_tets<Tr,
                                       Tets_criteria,
                                       Oracle,
                                       Mesh_3::Refine_tets_with_oracle_base<Tr,
                                        Tets_criteria, Oracle>, Facets_level>
                                                     Tets_level;

  // ** visitors **
  typedef typename Mesh_3::tets::Refine_facets_visitor<Tr,
     Tets_level, Null_mesh_visitor> Tets_facets_visitor;
  typedef typename Mesh_3::Visitor_for_surface <
    Tr,
    Null_mesh_visitor
    > Surface_facets_visitor;

  typedef Combine_mesh_visitor<Surface_facets_visitor,
    Tets_facets_visitor> Facets_visitor;

  typedef Surface_mesher::Visitor<Tr, Facets_level, 
    Facets_visitor> Tets_visitor;

  // ** C2T3 **
  typedef Complex_2_in_triangulation_3_surface_mesh<Tr> C2t3;

private:
  Null_mesher_level null_mesher_level;
  Null_mesh_visitor null_visitor;

  C2t3 c2t3;
  Oracle& oracle;
  Facets_level facets;
  Tets_level tets;

  Surface_facets_visitor surface_facets_visitor;
  Tets_facets_visitor tets_facets_visitor;
  Facets_visitor facets_visitor;
  Tets_visitor tets_visitor;

  bool initialized;

public:
  Implicit_surfaces_mesher_3(Tr& t, Oracle& o,
                             Facets_criteria& c,
                             Tets_criteria tets_crit)
    : c2t3(t), oracle(o), 
      facets(t, c2t3, oracle, c), tets(t, tets_crit, oracle, facets),
      surface_facets_visitor(&null_visitor),
      tets_facets_visitor(&tets, &null_visitor),
      facets_visitor(Facets_visitor(&surface_facets_visitor,
				    &tets_facets_visitor)),
      tets_visitor(&facets, &facets_visitor),
      initialized(false)
  {}

  void init()
  {
    Tr& tr = tets.triangulation_ref_impl();

    typedef typename Tr::Geom_traits Geom_traits;
    typename Geom_traits::Construct_circumcenter_3 circumcenter = 
      tr.geom_traits().construct_circumcenter_3_object();

    for(typename Tr::Finite_cells_iterator cit = tr.finite_cells_begin();
	cit != tr.finite_cells_end();
	++cit)
      {
	const Point& p = cit->vertex(0)->point();
	const Point& q = cit->vertex(1)->point();
	const Point& r = cit->vertex(2)->point();
	const Point& s = cit->vertex(3)->point();

	cit->set_in_domain(oracle.is_in_volume(circumcenter(p,q,r,s)));
      }
    
    for(typename Tr::Finite_vertices_iterator vit = 
      tr.finite_vertices_begin();
      vit != tr.finite_vertices_end();
      ++vit)
      vit->info()=true;
    std::cerr << "Restore infos.\n";

    facets.scan_triangulation();
    tets.scan_triangulation();
    initialized = true;
  }

  C2t3& complex_2_in_triangulation_3()
  {
    return c2t3;
  }

  void refine_mesh()
  {
    if(!initialized)
      init();
    tets.refine(tets_visitor);
  }

  void refine_surface()
  {
    if(!initialized)
      init();
    std::cerr << "Starting refine_surface()\n";
    
    while( ! facets.is_algorithm_done() )
      {
	Tr& tr = tets.triangulation_ref_impl();
	for(typename Tr::Finite_vertices_iterator vit = 
	      tr.finite_vertices_begin();
	    vit != tr.finite_vertices_end();
	    ++vit)
	  CGAL_assertion(vit->info()==true);
	
	facets.one_step(tets_visitor.previous_level());
      }
  }

  void step_by_step()
  {
    if(!initialized)
      init();
    tets.try_to_insert_one_point(tets_visitor);
  }

  bool done()
  {
    return tets.no_longer_element_to_refine();
    
  }
}; // end Implicit_surfaces_mesher_3

} // end namespace CGAL

#endif // CGAL_IMPLICIT_SURFACES_MESHER_3_H
