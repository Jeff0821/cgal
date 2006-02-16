// Copyright (c) 2003,2004,2005  INRIA Sophia-Antipolis (France) and
// Notre Dame University (U.S.A.).  All rights reserved.
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
// Author(s)     : Menelaos Karavelas <mkaravel@cse.nd.edu>



#ifndef CGAL_SEGMENT_DELAUNAY_GRAPH_2_H
#define CGAL_SEGMENT_DELAUNAY_GRAPH_2_H

#include <iostream>
#include <vector>
#include <list>
#include <set>
#include <map>
#include <algorithm>
#include <boost/tuple/tuple.hpp>

#include <CGAL/Segment_Delaunay_graph_2/basic.h>

#include <CGAL/Triangulation_2.h>
#include <CGAL/Segment_Delaunay_graph_site_2.h>
#include <CGAL/Triangulation_data_structure_2.h>
#include <CGAL/Triangulation_face_base_2.h>
#include <CGAL/Segment_Delaunay_graph_vertex_base_2.h>

#include <CGAL/Segment_Delaunay_graph_2/Constructions_C2.h>

#include <CGAL/in_place_edge_list.h>
#include <CGAL/Segment_Delaunay_graph_2/edge_list.h>
#include <CGAL/Segment_Delaunay_graph_2/Traits_wrapper_2.h>

#include <CGAL/Iterator_project.h>
#include <CGAL/Segment_Delaunay_graph_2/Simple_container_wrapper.h>


/*
  Conventions:
  ------------
  1. we treat segments as open; the endpoints are separate objects
  2. a segment of length zero is treated as a point
  3. a point is deleted only if it has no segment adjacent to it
  4. when a segment is deleted it's endpoints are not deleted
  5. the user can force the deletion of endpoints; this is only done
     if condition 3 is met.
  6. when objects are written to a stream we distinguish between
     points and segments; points start by a 'p' and segments by an 's'.
*/


CGAL_BEGIN_NAMESPACE

namespace CGALi {

  template<typename Edge, typename LTag> struct SDG_which_list;

  // use the in-place edge list
  template<typename E>
  struct SDG_which_list<E,Tag_true>
  {
    typedef E                           Edge;
    typedef In_place_edge_list<Edge>    List;
  };

  // do not use the in-place edge list
  template<typename E>
  struct SDG_which_list<E,Tag_false>
  {
    typedef E                                 Edge;
    // change the following to Tag_false in order to use
    // CGAL's Unique_hash_map
    typedef Tag_true                          Use_stl_map_tag;
    typedef Edge_list<Edge,Use_stl_map_tag>   List;
  };


  template < class Node >
  struct Sdg_project_site_2 {
    typedef Node                   argument_type;
    typedef typename Node::Site_2  Site;
    typedef Site                   result_type;
    Site& operator()(const Node& x) const { 
      static Site s;
      s = x.site();
      return s;
    }
    //    const Site& operator()(const Node& x) const { return x.site(); }
  };

  template < class Node, class Site_t >
  struct Sdg_project_input_to_site_2 {
    typedef Node                   argument_type;
    typedef Site_t                 Site;
    typedef Site                   result_type;
    Site& operator()(const Node& x) const {
      static Site s;
      if ( boost::tuples::get<2>(x) /*x.third*/ ) { // it is a point
	//	s = Site::construct_site_2(*x.first);
	s = Site::construct_site_2( *boost::tuples::get<0>(x) );
      } else {
	//	s = Site::construct_site_2(*x.first, *x.second);
	s = Site::construct_site_2
	  (*boost::tuples::get<0>(x), *boost::tuples::get<1>(x));
      }
      return s;
    }
  };

} // namespace CGALi


template<class Gt, class STag, class DS, class LTag >
class Segment_Delaunay_graph_hierarchy_2;



template<class Gt,
	 class DS = Triangulation_data_structure_2 < 
                Segment_Delaunay_graph_vertex_base_2<Gt,
			    typename Gt::Intersections_tag>,
                Triangulation_face_base_2<Gt> >,
	 class LTag = Tag_false >
class Segment_Delaunay_graph_2
  : private Triangulation_2<
          Segment_Delaunay_graph_traits_wrapper_2<Gt>, DS >
{
  friend class Segment_Delaunay_graph_hierarchy_2<Gt,Tag_true,DS,LTag>;
  friend class Segment_Delaunay_graph_hierarchy_2<Gt,Tag_false,DS,LTag>;
protected:
  // LOCAL TYPES
  //------------
  typedef Segment_Delaunay_graph_2<Gt,DS,LTag>         Self;

  typedef Segment_Delaunay_graph_traits_wrapper_2<Gt>   Modified_traits;
  typedef Triangulation_2<Modified_traits,DS>           DG;
  typedef DG                                            Delaunay_graph;

  typedef LTag                                          List_tag;

public:
  // PUBLIC TYPES
  //-------------
  typedef DS                                     Data_structure;
  typedef DS                                     Triangulation_data_structure;
  typedef Gt                                     Geom_traits;
  typedef typename Gt::Site_2                    Site_2;
  typedef typename Gt::Point_2                   Point_2;

  typedef typename DS::Edge                      Edge;
  typedef typename DS::Vertex_handle             Vertex_handle;
  typedef typename DS::Face_handle               Face_handle;
  typedef typename DS::Vertex                    Vertex;
  typedef typename DS::Face                      Face;

  typedef typename DS::Vertex_circulator         Vertex_circulator;
  typedef typename DS::Edge_circulator           Edge_circulator;
  typedef typename DS::Face_circulator           Face_circulator;

  typedef typename DS::Face_iterator             All_faces_iterator;
  typedef typename DS::Vertex_iterator           All_vertices_iterator;
  typedef typename DS::Edge_iterator             All_edges_iterator;

  typedef typename DG::Finite_faces_iterator     Finite_faces_iterator;
  typedef typename DG::Finite_vertices_iterator  Finite_vertices_iterator;
  typedef typename DG::Finite_edges_iterator     Finite_edges_iterator;

protected:
  typedef typename Geom_traits::Arrangement_type_2  AT2;
  typedef typename AT2::Arrangement_type            Arrangement_type;

  typedef std::set<Point_2>                      PC;
  typedef typename PC::iterator                  PH;


  // these containers should have point handles and should replace the
  // point container...
  typedef boost::tuples::tuple<PH,PH,bool>       Site_rep_2;
  //  typedef Triple<PH,PH,bool>                     Site_rep_2;

  struct Site_rep_less_than {
    // less than for site reps
    bool operator()(const Site_rep_2& x, const Site_rep_2& y) const {
      PH x1 = boost::tuples::get<0>(x);
      PH y1 = boost::tuples::get<0>(y);

      if ( &(*x1) < &(*y1) ) { return true; }
      if ( &(*y1) < &(*x1) ) { return false; }

      PH x2 = boost::tuples::get<1>(x);
      PH y2 = boost::tuples::get<1>(y);

      return &(*x2) < &(*y2);
    }
  };

  typedef std::set<Site_rep_2,Site_rep_less_than>  Input_sites_container;
  typedef typename Input_sites_container::const_iterator
  All_inputs_iterator;

  typedef CGALi::Sdg_project_input_to_site_2<Site_rep_2, Site_2>
  Proj_input_to_site;

public:
  typedef Simple_container_wrapper<PC>           Point_container;
  typedef typename Point_container::iterator     Point_handle;

  typedef typename DS::size_type                 size_type;

protected:
  typedef CGALi::Sdg_project_site_2<Vertex>      Proj_site;

  typedef typename Point_container::const_iterator const_Point_handle;

  struct Point_handle_less_than {
    // less than
    bool operator()(const const_Point_handle& x,
		    const const_Point_handle& y) const {
      return &(*x) < &(*y);
    }
  };

  typedef std::pair<Point_handle,Point_handle>   Point_handle_pair;

  typedef std::map<Point_handle,Point_handle,Point_handle_less_than>
  Handle_map;

public:
  typedef Iterator_project<All_inputs_iterator, Proj_input_to_site>
  Input_sites_iterator;

  typedef Iterator_project<Finite_vertices_iterator, 
                           Proj_site>            Output_sites_iterator;
protected:
  // LOCAL VARIABLE(S)
  //------------------
  // the container of points
  Point_container pc_;
  Input_sites_container isc_;

protected:
  // MORE LOCAL TYPES
  //-----------------
  typedef typename Gt::Intersections_tag        Intersections_tag;

  typedef std::map<Face_handle,bool>            Face_map;
  typedef std::vector<Edge>                     Edge_vector;

  typedef std::list<Vertex_handle>              Vertex_list;
  typedef typename Vertex_list::iterator        Vertex_list_iterator;

  typedef Triple<Vertex_handle,Vertex_handle,Vertex_handle>
  Vertex_triple;

  typedef Vertex_handle                         Vh_triple[3];
  typedef std::map<Face_handle,Sign>            Sign_map;

  typedef std::pair<Face_handle,Face_handle>    Face_pair;

  typedef typename Data_structure::Vertex::Storage_site_2   Storage_site_2;

  // the edge list
  typedef typename CGALi::SDG_which_list<Edge,List_tag>::List  List;

public:
  // CREATION
  //---------
  Segment_Delaunay_graph_2(const Gt& gt=Gt()) : DG(gt) {}

  template< class Input_iterator >
  Segment_Delaunay_graph_2(Input_iterator first, Input_iterator beyond,
			   const Gt& gt=Gt()) : DG(gt)
  {
    insert(first, beyond);
  }

  Segment_Delaunay_graph_2(const Self& other);
  Self& operator=(const Self& other);

public:
  // ACCESS METHODS
  // --------------
  const Geom_traits&  geom_traits() const { return DG::geom_traits(); }

  const Data_structure&   data_structure() const { return this->_tds; }
  const Triangulation_data_structure& tds() const { return this->_tds; }
  const Point_container&  point_container() const { return pc_; }

  inline size_type number_of_input_sites() const {
    return isc_.size();
  }

  inline size_type number_of_output_sites() const {
    return number_of_vertices();
  }

  inline size_type number_of_vertices() const {
    return DG::number_of_vertices();
  }

  inline size_type number_of_faces() const {
    return DG::number_of_faces();
  }

  inline Vertex_handle infinite_vertex() const {
    return DG::infinite_vertex();
  }

  inline Face_handle infinite_face() const {
    return DG::infinite_face();
  }

  inline Vertex_handle finite_vertex() const {
    return DG::finite_vertex();
  }

  inline int dimension() const {
    return DG::dimension();
  }

  using Delaunay_graph::cw;
  using Delaunay_graph::ccw;

public:
  // TRAVERSAL OF THE DUAL GRAPH
  //----------------------------
  inline Finite_faces_iterator finite_faces_begin() const {
    return DG::finite_faces_begin();
  }

  inline Finite_faces_iterator finite_faces_end() const {
    return DG::finite_faces_end();
  }

  inline Finite_vertices_iterator finite_vertices_begin() const {
    return DG::finite_vertices_begin();
  }

  inline Finite_vertices_iterator finite_vertices_end() const {
    return DG::finite_vertices_end();
  }

  inline Finite_edges_iterator finite_edges_begin() const {
    return DG::finite_edges_begin();    
  }

  inline Finite_edges_iterator finite_edges_end() const {
    return DG::finite_edges_end();    
  }

  inline All_faces_iterator all_faces_begin() const {
    return DG::all_faces_begin();
  }

  inline All_faces_iterator all_faces_end() const {
    return DG::all_faces_end();
  }

  inline All_vertices_iterator all_vertices_begin() const {
    return DG::all_vertices_begin();
  }

  inline All_vertices_iterator all_vertices_end() const {
    return DG::all_vertices_end();
  }

  inline All_edges_iterator all_edges_begin() const {
    return DG::all_edges_begin();
  }

  inline All_edges_iterator all_edges_end() const {
    return DG::all_edges_end();
  }

  inline Input_sites_iterator input_sites_begin() const {
    return Input_sites_iterator(isc_.begin());
  }

  inline Input_sites_iterator input_sites_end() const {
    return Input_sites_iterator(isc_.end());
  }

  inline Output_sites_iterator output_sites_begin() const {
    return Output_sites_iterator(finite_vertices_begin());
  }

  inline Output_sites_iterator output_sites_end() const {
    return Output_sites_iterator(finite_vertices_end());    
  }

public:
  // CIRCULATORS
  //------------
  inline Face_circulator
  incident_faces(Vertex_handle v,
		 Face_handle f = Face_handle()) const {
    return DG::incident_faces(v, f);
  }

  inline Vertex_circulator
  incident_vertices(Vertex_handle v,
		    Face_handle f = Face_handle()) const { 
    return DG::incident_vertices(v, f);
  }

  inline Edge_circulator
  incident_edges(Vertex_handle v,
		 Face_handle f = Face_handle()) const {
    return DG::incident_edges(v, f);
  }
 
public:
  // PREDICATES
  //-----------
  inline bool is_infinite(const Vertex_handle& v) const {
    return DG::is_infinite(v);
  }

  inline bool is_infinite(const Face_handle& f) const {
    return DG::is_infinite(f);
  }

  inline bool is_infinite(const Face_handle& f, int i) const {
    return DG::is_infinite(f, i);
  }

  inline bool is_infinite(const Edge& e) const {
    return is_infinite(e.first, e.second);
  }

  inline bool is_infinite(const Edge_circulator& ec) const {
    return DG::is_infinite(ec);
  }

public:
  // INSERTION METHODS
  //------------------
  template<class Input_iterator>
  inline size_type insert(Input_iterator first, Input_iterator beyond) {
    return insert(first, beyond, Tag_false());
  }

  template<class Input_iterator>
  size_type insert(Input_iterator first, Input_iterator beyond, Tag_true)
  {
    std::vector<Site_2> site_vec;
    for (Input_iterator it = first; it != beyond; ++it) {
      site_vec.push_back(Site_2(*it));
    }
    std::random_shuffle(site_vec.begin(), site_vec.end());
    return insert(site_vec.begin(), site_vec.end(), Tag_false());
  }

  template<class Input_iterator>
  size_type insert(Input_iterator first, Input_iterator beyond,	Tag_false)
  {
    // do it the obvious way: insert them as they come;
    // one might think though that it might be better to first insert
    // all end points and then all segments, or a variation of that.
    size_type n_before = number_of_vertices();
    for (Input_iterator it = first; it != beyond; ++it) {
      insert(*it);
    }
    size_type n_after = number_of_vertices();
    return n_after - n_before;
  }

  // insert a point
  inline Vertex_handle insert(const Point_2& p) {
    // update input site container
    Point_handle ph = register_input_site(p);
    Storage_site_2 ss = Storage_site_2::construct_storage_site_2(ph);
    return insert_point(ss, p, Vertex_handle());
  }

  inline Vertex_handle insert(const Point_2& p, Vertex_handle vnear) {
    // update input site container
    Point_handle ph = register_input_site(p);
    Storage_site_2 ss = Storage_site_2::construct_storage_site_2(ph);
    return insert_point(ss, p, vnear);
  }

protected:
  // insert a point without registering it in the input sites
  // container: useful for the hierarchy
  inline Vertex_handle insert_no_register(const Storage_site_2& ss,
					  const Point_2& p,
					  Vertex_handle vnear) {
    return insert_point(ss, p, vnear);
  }

public:
  // insert a segment
  inline Vertex_handle insert(const Point_2& p0, const Point_2& p1) {
    // update input site container
    Point_handle_pair php = register_input_site(p0, p1);
    Storage_site_2 ss =
      Storage_site_2::construct_storage_site_2(php.first, php.second);
    Vertex_handle v = insert_segment(ss, Site_2::construct_site_2(p0, p1),
				     Vertex_handle());
    if ( v == Vertex_handle() ) {
      unregister_input_site(php.first, php.second);
    }
    return v;
  }

  // inserting a segment whose endpoints have already been inserted
  // update input site container
  inline Vertex_handle insert(const Vertex_handle& v0,
			      const Vertex_handle& v1) {
    CGAL_precondition( v0->storage_site().is_point() &&
		       v1->storage_site().is_point() );

    Storage_site_2 ss = Storage_site_2::construct_storage_site_2
      (v0->storage_site().point(), v1->storage_site().point());

    if ( number_of_vertices() == 2 ) {
      return insert_third(ss, v0, v1);
    }

    return insert_segment_interior(ss.site(), ss, v0);
  }

  inline Vertex_handle insert(const Point_2& p0, const Point_2& p1, 
			      Vertex_handle vnear) {
    // update input site container
    Point_handle_pair php = register_input_site(p0, p1);
    Storage_site_2 ss =
      Storage_site_2::construct_storage_site_2(php.first, php.second);
    Vertex_handle v =
      insert_segment(ss, Site_2::construct_site_2(p0, p1), vnear);
    if ( v == Vertex_handle() ) {
      unregister_input_site(php.first, php.second);
    }
    return v;
  }

  inline Vertex_handle  insert(const Site_2& t) {
    return insert(t, Vertex_handle());
  }

  Vertex_handle  insert(const Site_2& t, Vertex_handle vnear)
  {
    // the intended use is to unify the calls to insert(...);
    // thus the site must be an exact one; 
    CGAL_precondition( t.is_input() );

    // update input site container

    if ( t.is_segment() ) {
      Point_handle_pair php =
	register_input_site( t.source_of_supporting_site(),
			     t.target_of_supporting_site() );
      Storage_site_2 ss =
	Storage_site_2::construct_storage_site_2(php.first, php.second);
      Vertex_handle v = insert_segment(ss, t, vnear);
      if ( v == Vertex_handle() ) {
	unregister_input_site( php.first, php.second );
      }
      return v;
    } else if ( t.is_point() ) {
      Point_handle ph = register_input_site( t.point() );
      Storage_site_2 ss = Storage_site_2::construct_storage_site_2(ph);
      return insert_point(ss, t.point(), vnear);
    } else {
      CGAL_precondition ( t.is_defined() );
      return Vertex_handle(); // to avoid compiler error
    }
  }

  // REMOVAL METHODS
  //----------------
protected:
  bool is_star(const Vertex_handle& v) const;
  bool is_linear_chain(const Vertex_handle& v0, const Vertex_handle& v1,
		       const Vertex_handle& v2) const;
  bool is_flippable(const Face_handle& f, int i) const;

  void minimize_degree(const Vertex_handle& v);

  // this method does not really do the job as intended, i.e., for removal
  void equalize_degrees(const Vertex_handle& v, Self& small,
			std::map<Vertex_handle,Vertex_handle>& vmap,
			List& l) const;

  void expand_conflict_region_remove(const Face_handle& f,
				     const Site_2& t,
				     const Storage_site_2& ss,
				     List& l, Face_map& fm,
				     Sign_map& sign_map);

  void find_conflict_region_remove(const Vertex_handle& v,
				   const Vertex_handle& vnearest,
				   List& l, Face_map& fm, Sign_map& vm);

  template<class OutputItFaces>
  OutputItFaces get_faces(const List& l, OutputItFaces fit) const
  {
    // map that determines if a face has been visited
    std::map<Face_handle,bool> fmap;

    // compute the initial face
    Edge e_front = l.front();
    Face_handle fstart = e_front.first->neighbor(e_front.second);

    // do the recursion
    return get_faces(l, fstart, fmap, fit);
  }

  template<class OutputItFaces>
  OutputItFaces get_faces(const List& l, Face_handle f,
			  std::map<Face_handle,bool>& fmap,
			  OutputItFaces fit) const
  {
    // if the face has been visited return
    if ( fmap.find(f) != fmap.end() ) { return fit; }

    // mark the face as visited
    fmap[f] = true;

    // output the face
    *fit++ = f;

    // recursively go to neighbors
    for (int i = 0; i < 3; i++) {
      Face_handle n = f->neighbor(i);
      Edge ee(n, n->index( this->_tds.mirror_vertex(f,i) ));
      if ( !l.is_in_list(ee) ) {
	fit = get_faces(l, n, fmap, fit);
      }
    }
    return fit;
  }

  size_type count_faces(const List& l) const;

  void fill_hole(const Self& small, const Vertex_handle& v, const List& l,
		 std::map<Vertex_handle,Vertex_handle>& vmap);

  bool remove_first(const Vertex_handle& v);
  bool remove_second(const Vertex_handle& v);
  bool remove_third(const Vertex_handle& v);

  void compute_small_diagram(const Vertex_handle& v, Self& small) const;
  void compute_vertex_map(const Vertex_handle& v, const Self& small,
			  std::map<Vertex_handle,Vertex_handle>& vmap) const;
  void remove_degree_d_vertex(const Vertex_handle& v);

  bool remove_base(const Vertex_handle& v);

public:
  bool remove(const Vertex_handle& v);

protected:
  inline void unregister_input_site(const Point_handle& h)
  {
    Site_rep_2 rep(h, h, true);
    typename Input_sites_container::iterator it = isc_.find(rep);
    CGAL_assertion( it != isc_.end() );

    pc_.remove(h);
    isc_.erase(it);
  }

  inline void unregister_input_site(const Point_handle& h1,
				    const Point_handle& h2)
  {   
    Site_rep_2 rep(h1, h2, false);
    typename Input_sites_container::iterator it = isc_.find(rep);
    
    Site_rep_2 sym_rep(h2, h1, false);
    typename Input_sites_container::iterator sym_it = isc_.find(sym_rep);

    CGAL_assertion( it != isc_.end() || sym_it != isc_.end() );

    if ( it != isc_.end() ) { isc_.erase(it); }
    if ( sym_it != isc_.end() ) { isc_.erase(sym_it); }

    Site_rep_2 r1(h1, h1, true);
    if ( isc_.find(r1) == isc_.end() ) { isc_.insert(r1); }

    Site_rep_2 r2(h2, h2, true);
    if ( isc_.find(r2) == isc_.end() ) { isc_.insert(r2); }
  }

  inline Point_handle register_input_site(const Point_2& p)
  {
    std::pair<PH,bool> it = pc_.insert(p);
    Site_rep_2 rep(it.first, it.first, true);
    isc_.insert( rep );
    return it.first;
  }

  inline
  Point_handle_pair register_input_site(const Point_2& p0, const Point_2& p1)
  {
    std::pair<PH,bool> it1 = pc_.insert(p0);
    std::pair<PH,bool> it2 = pc_.insert(p1);
    Site_rep_2 rep(it1.first, it2.first, false);
    isc_.insert( rep );
    return Point_handle_pair(it1.first, it2.first);
  }

  Vertex_handle  insert_first(const Storage_site_2& ss, const Point_2& p);
  Vertex_handle  insert_second(const Storage_site_2& ss, const Point_2& p);
  Vertex_handle  insert_third(const Storage_site_2& ss, const Point_2& p);
  Vertex_handle  insert_third(const Site_2& t, const Storage_site_2& ss);
  Vertex_handle  insert_third(const Storage_site_2& ss, Vertex_handle v0,
			      Vertex_handle v1);

  Vertex_handle insert_point(const Storage_site_2& ss, const Point_2& p,
			     Vertex_handle vnear);
  Vertex_handle insert_point(const Storage_site_2& ss,
			     const Site_2& t, Vertex_handle vnear);
  Vertex_handle insert_point2(const Storage_site_2& ss,
			      const Site_2& t, Vertex_handle vnear);

  Triple<Vertex_handle,Vertex_handle,Vertex_handle>
  insert_point_on_segment(const Storage_site_2& ss, const Site_2& t,
			  Vertex_handle v, const Tag_true&);

  Triple<Vertex_handle,Vertex_handle,Vertex_handle>
  insert_exact_point_on_segment(const Storage_site_2& ss, const Site_2& t,
				Vertex_handle v);

  Vertex_handle insert_segment(const Storage_site_2& ss, const Site_2& t,
			       Vertex_handle vnear);

  Vertex_handle insert_segment_interior(const Site_2& t,
					const Storage_site_2& ss,
					Vertex_handle vnear);

  template<class ITag>
  inline
  Vertex_handle insert_intersecting_segment(const Storage_site_2& ss,
					    const Site_2& t,
					    Vertex_handle v,
					    ITag tag) {
    return insert_intersecting_segment_with_tag(ss, t, v, tag);
  }

  Vertex_handle
  insert_intersecting_segment_with_tag(const Storage_site_2& ss,
				       const Site_2& t,
				       Vertex_handle v, Tag_false);

  Vertex_handle
  insert_intersecting_segment_with_tag(const Storage_site_2& ss,
				       const Site_2& t,
				       Vertex_handle v, Tag_true);



public:
  // NEAREST NEIGHBOR LOCATION
  //--------------------------
  inline Vertex_handle nearest_neighbor(const Point_2& p) const {
    return nearest_neighbor(Site_2::construct_site_2(p), Vertex_handle());
  }

  inline Vertex_handle nearest_neighbor(const Point_2& p,
					Vertex_handle vnear) const {
    return nearest_neighbor(Site_2::construct_site_2(p), vnear);
  }

protected:
  Vertex_handle nearest_neighbor(const Site_2& p,
				 Vertex_handle vnear) const;


protected:
  // I/O METHODS
  //------------
  typedef std::map<const_Point_handle,size_type,Point_handle_less_than>
  Point_handle_mapper;

  typedef std::vector<Point_handle> Point_handle_vector;

  void file_output(std::ostream&, const Storage_site_2&,
		   Point_handle_mapper&) const;

  void file_output(std::ostream&, Point_handle_mapper&,
		   bool print_point_container) const;

  void file_input(std::istream&, Storage_site_2&,
		  const Point_handle_vector&, const Tag_true&) const;
  void file_input(std::istream&, Storage_site_2&,
		  const Point_handle_vector&, const Tag_false&) const;
  void file_input(std::istream&, bool read_handle_vector,
		  Point_handle_vector&);

public:
  void file_input(std::istream& is) {
    Point_handle_vector P;
    file_input(is, true, P);
  }

  void file_output(std::ostream& os) const {
    Point_handle_mapper P;
    size_type inum = 0;
    for (const_Point_handle ph = pc_.begin(); ph != pc_.end(); ++ph) {
      P[ph] = inum++;
    }
    file_output(os, P, true);
  }

  template< class Stream >
  Stream& draw_dual(Stream& str) const
  {
    Finite_edges_iterator eit = finite_edges_begin();
    for (; eit != finite_edges_end(); ++eit) {
      draw_dual_edge(*eit, str);
    }
    return str;
  }

  template < class Stream > 
  Stream& draw_skeleton(Stream& str) const
  {
    Finite_edges_iterator eit = finite_edges_begin();
    for (; eit != finite_edges_end(); ++eit) {
      Site_2 p = eit->first->vertex(  cw(eit->second) )->site();
      Site_2 q = eit->first->vertex( ccw(eit->second) )->site();

      bool is_endpoint_of_seg =
	( p.is_segment() && q.is_point() &&
	  is_endpoint_of_segment(q, p) ) ||
	( p.is_point() && q.is_segment() &&
	  is_endpoint_of_segment(p, q) );

      if ( !is_endpoint_of_seg ) {
	draw_dual_edge(*eit, str);
      }
    }
    return str;
  }

  // MK: this has to be rewritten. all the checking must be done in
  // the geometric traits class.
  template< class Stream >
  Stream& draw_dual_edge(Edge e, Stream& str) const
  {
    CGAL_precondition( !is_infinite(e) );

    typename Geom_traits::Line_2              l;
    typename Geom_traits::Segment_2           s;
    typename Geom_traits::Ray_2               r;
    CGAL::Parabola_segment_2<Gt>              ps;

    Object o = primal(e);

    if (CGAL::assign(l, o))   str << l;
    if (CGAL::assign(s, o))   str << s; 
    if (CGAL::assign(r, o))   str << r;
    if (CGAL::assign(ps, o))  ps.draw(str);

    return str;
  }

  template< class Stream >
  inline
  Stream& draw_dual_edge(Edge_circulator ec, Stream& str) const {
    return draw_dual_edge(*ec, str);
  }

  template< class Stream >
  inline
  Stream& draw_dual_edge(Finite_edges_iterator eit, Stream& str) const {
    return draw_dual_edge(*eit, str);
  }

public:
  // VALIDITY CHECK
  //---------------
  bool is_valid(bool verbose = false, int level = 1) const;

public:
  // MISCELLANEOUS
  //--------------
  void clear() {
    DG::clear();
    pc_.clear();
    isc_.clear();
  }

  void swap(Segment_Delaunay_graph_2& sdg) {
    DG::swap(sdg);
    pc_.swap(sdg.pc_);
    isc_.swap(sdg.isc_);
  }

  //////////////////////////////////////////////////////////////////////
  // THE METHODS BELOW ARE LOCAL
  //////////////////////////////////////////////////////////////////////

protected:
  // THE COPY METHOD
  //------------------------------------------------------------------
  // used in the copy constructor and assignment operator

  Storage_site_2
  copy_storage_site(const Storage_site_2& ss_other,
		    Handle_map& hm, const Tag_false&);

  Storage_site_2
  copy_storage_site(const Storage_site_2& ss_other,
		    Handle_map& hm, const Tag_true&);

  void copy(Segment_Delaunay_graph_2& other);
  void copy(Segment_Delaunay_graph_2& other, Handle_map& hm);

protected:
  // HELPER METHODS FOR COMBINATORIAL OPERATIONS ON THE DATA STRUCTURE
  //------------------------------------------------------------------

  // getting the degree of a vertex
  inline
  typename Data_structure::size_type degree(const Vertex_handle& v) const {
    return this->_tds.degree(v);
  }

  // getting the symmetric edge
  inline Edge sym_edge(const Edge e) const {
    return sym_edge(e.first, e.second);
  }

  inline Edge sym_edge(const Face_handle& f, int i) const {
    Face_handle f_sym = f->neighbor(i);
    return Edge(  f_sym, f_sym->index( this->_tds.mirror_vertex(f, i) )  );
  }

  Edge flip(Face_handle& f, int i) {
    CGAL_precondition ( f != Face_handle() );
    CGAL_precondition (i == 0 || i == 1 || i == 2);
    CGAL_precondition( this->dimension()==2 ); 

    CGAL_precondition( f->vertex(i) != this->_tds.mirror_vertex(f, i) );

    this->_tds.flip(f, i);

    return Edge(f, ccw(i));
  }

  inline Edge flip(Edge e) {
    return flip(e.first, e.second);
  }

  inline bool is_degree_2(const Vertex_handle& v) const {
    Face_circulator fc = incident_faces(v);
    Face_circulator fc1 = fc;
    ++(++fc1);
    return ( fc == fc1 );
  }

  inline Vertex_handle insert_degree_2(Edge e) {
    return this->_tds.insert_degree_2(e.first,e.second);
  }

  inline Vertex_handle insert_degree_2(Edge e, const Storage_site_2& ss) {
    Vertex_handle v = insert_degree_2(e);
    v->set_site(ss);
    return v;
  }

  inline void remove_degree_2(Vertex_handle v) {
    CGAL_precondition( is_degree_2(v) );
    this->_tds.remove_degree_2(v);
  }

  inline void remove_degree_3(Vertex_handle v) {
    CGAL_precondition( degree(v) == 3 );
    this->_tds.remove_degree_3(v, Face_handle());
  }

  inline Vertex_handle create_vertex(const Storage_site_2& ss) {
    Vertex_handle v = this->_tds.create_vertex();
    v->set_site(ss);
    return v;
  }

  inline Vertex_handle create_vertex_dim_up(const Storage_site_2& ss) {
    Vertex_handle v = this->_tds.insert_dim_up(infinite_vertex());
    v->set_site(ss);
    return v;
  }

protected:
  // HELPER METHODS FOR CREATING STORAGE SITES
  //------------------------------------------
#if 0
  inline Storage_site_2 create_storage_site(const Point_2& p) {
    Point_handle ph = pc_.insert(p).first;
    return Storage_site_2::construct_storage_site_2(ph);
  }

  inline Storage_site_2 create_storage_site(Vertex_handle v0,
					    Vertex_handle v1) {
    return Storage_site_2::construct_storage_site_2
      ( v0->storage_site().point(), v1->storage_site().point() );
  }
#endif

  inline
  Storage_site_2 split_storage_site(const Storage_site_2& ss0,
				    const Storage_site_2& ss1,
				    unsigned int i, const Tag_false&)
  {
    // Split the first storage site which is a segment using the
    // second storage site which is an exact point
    // i denotes whether the first or second half is to be created
    CGAL_precondition( ss0.is_segment() && ss1.is_point() );
    CGAL_precondition( ss1.is_input() );
    CGAL_precondition( i < 2 );

    if ( i == 0 ) {
      return Storage_site_2::construct_storage_site_2
	(ss0.source_of_supporting_site(), ss1.point());
    } else {
      return Storage_site_2::construct_storage_site_2
	(ss1.point(), ss0.target_of_supporting_site());
    }
  }

  Storage_site_2 split_storage_site(const Storage_site_2& ss0,
				    const Storage_site_2& ss1,
				    unsigned int i, const Tag_true&);

  inline
  Storage_site_2 create_storage_site(const Storage_site_2& ss0,
				     const Storage_site_2& ss1) {
    return Storage_site_2::construct_storage_site_2
      ( ss0.source_of_supporting_site(),
	ss0.target_of_supporting_site(),
	ss1.source_of_supporting_site(),
	ss1.target_of_supporting_site() );
  }

  inline
  Storage_site_2 create_storage_site(const Storage_site_2& ss0,
				     const Storage_site_2& ss1,
				     bool is_first_exact) {
    return Storage_site_2::construct_storage_site_2
      ( ss0.source_of_supporting_site(),
	ss0.target_of_supporting_site(),
	ss1.source_of_supporting_site(),
	ss1.target_of_supporting_site(), is_first_exact );
  }

  inline
  Storage_site_2 create_storage_site_type1(const Storage_site_2& ss0,
					   const Storage_site_2& ss1,
					   const Storage_site_2& ss2) {
    return Storage_site_2::construct_storage_site_2
      ( ss0.source_of_supporting_site(),
	ss0.target_of_supporting_site(),
	ss1.source_of_crossing_site(0),
	ss1.target_of_crossing_site(0),
	ss2.source_of_supporting_site(),
	ss2.target_of_supporting_site() );
  }

  inline
  Storage_site_2 create_storage_site_type2(const Storage_site_2& ss0,
					   const Storage_site_2& ss1,
					   const Storage_site_2& ss2) {
    return Storage_site_2::construct_storage_site_2
      ( ss0.source_of_supporting_site(),
	ss0.target_of_supporting_site(),
	ss1.source_of_supporting_site(),
	ss1.target_of_supporting_site(),
	ss2.source_of_crossing_site(1),
	ss2.target_of_crossing_site(1) );
  }

public:
  // METHODS FOR ACCESSING THE PRIMAL GRAPH
  //---------------------------------------
  // used primarily for visualization
  inline Point_2 primal(const Face_handle& f) const {
    return circumcenter(f);
  }

  Object primal(const Edge e) const;

  inline Object primal(const Edge_circulator& ec) const {
    return primal(*ec); 
  }

  inline Object primal(const Finite_edges_iterator& ei) const {
    return primal(*ei);
  }

protected:
  void print_error_message() const;

  void print_error_message(const Tag_false&) const
  {
    static int i = 0;

    if ( i == 0 ) {
      i++;
      std::cerr << "SDG::Insert aborted: intersecting segments found"
		<< std::endl;
    }
  }

  void print_error_message(const Tag_true&) const {}

  //protected:
public:
  // wrappers for constructions
  inline Point_2 circumcenter(const Face_handle& f) const {
    CGAL_precondition( this->dimension()==2 || !is_infinite(f) );
    return circumcenter(f->vertex(0)->site(),
			f->vertex(1)->site(),
			f->vertex(2)->site());
  }

  inline Point_2 circumcenter(const Site_2& t0, const Site_2& t1, 
			      const Site_2& t2) const {
    return
    geom_traits().construct_svd_vertex_2_object()(t0, t1, t2);
  }

protected:
  // HELPER METHODS FOR INSERTION
  //-----------------------------
  void initialize_conflict_region(const Face_handle& f, List& l);

  std::pair<Face_handle,Face_handle>
  find_faces_to_split(const Vertex_handle& v, const Site_2& t) const;

  void expand_conflict_region(const Face_handle& f, const Site_2& t,
			      const Storage_site_2& ss,
			      List& l, Face_map& fm,
			      std::map<Face_handle,Sign>& sign_map,
			      Triple<bool, Vertex_handle,
			      Arrangement_type>& vcross);

  Vertex_handle add_bogus_vertex(Edge e, List& l);
  Vertex_list   add_bogus_vertices(List& l);
  void          remove_bogus_vertices(Vertex_list& vl);

  void retriangulate_conflict_region(Vertex_handle v, List& l,
				     Face_map& fm);


protected:
  // TYPES AND ACCESS METHODS FOR VISUALIZATION
  //-------------------------------------------

  // types
  typedef CGAL::Construct_sdg_circle_2<Gt,Ring_tag>
  Construct_sdg_circle_2;

  typedef CGAL::Construct_sdg_bisector_2<Gt,Ring_tag>
  Construct_sdg_bisector_2;

  typedef CGAL::Construct_sdg_bisector_ray_2<Gt,Ring_tag>
  Construct_sdg_bisector_ray_2;

  typedef CGAL::Construct_sdg_bisector_segment_2<Gt,Ring_tag>
  Construct_sdg_bisector_segment_2;

  // access
  inline Construct_sdg_circle_2
  construct_sdg_circle_2_object() const{
    return Construct_sdg_circle_2();
  }

  inline Construct_sdg_bisector_2
  construct_sdg_bisector_2_object() const {
    return Construct_sdg_bisector_2();
  }

  inline Construct_sdg_bisector_ray_2
  construct_sdg_bisector_ray_2_object() const {
    return Construct_sdg_bisector_ray_2();
  }

  inline Construct_sdg_bisector_segment_2
  construct_sdg_bisector_segment_2_object() const { 
    return Construct_sdg_bisector_segment_2(); 
  }

protected:
  // WRAPPERS FOR GEOMETRIC PREDICATES
  //----------------------------------
  inline
  bool same_points(const Storage_site_2& p, const Storage_site_2& q) const {
    return geom_traits().equal_2_object()(p.site(), q.site());
  }

  inline
  bool same_segments(const Storage_site_2& t, Vertex_handle v) const {
    if ( is_infinite(v) ) { return false; }
    if ( t.is_point() || v->storage_site().is_point() ) { return false; }
    return same_segments(t.site(), v->site());
  }

  inline
  bool is_endpoint_of_segment(const Storage_site_2& p,
			      const Storage_site_2& s) const
  {
    CGAL_precondition( p.is_point() && s.is_segment() );
    return ( same_points(p, s.source_site()) ||
	     same_points(p, s.target_site()) );
  }

  inline
  bool is_degenerate_segment(const Storage_site_2& s) const {
    CGAL_precondition( s.is_segment() );
    return same_points(s.source_site(), s.target_site());
  }

  // returns:
  //   ON_POSITIVE_SIDE if q is closer to t1
  //   ON_NEGATIVE_SIDE if q is closer to t2
  //   ON_ORIENTED_BOUNDARY if q is on the bisector of t1 and t2
  inline
  Oriented_side side_of_bisector(const Storage_site_2 &t1,
				 const Storage_site_2 &t2,
				 const Storage_site_2 &q) const {
    return
      geom_traits().oriented_side_of_bisector_2_object()(t1.site(),
							 t2.site(),
							 q.site());
  }

  inline
  Sign incircle(const Storage_site_2 &t1, const Storage_site_2 &t2,
		const Storage_site_2 &t3, const Storage_site_2 &q) const {
    return geom_traits().vertex_conflict_2_object()(t1.site(),
						    t2.site(),
						    t3.site(),
						    q.site());
  }

  inline
  Sign incircle(const Storage_site_2 &t1, const Storage_site_2 &t2,
		const Storage_site_2 &q) const {
    return geom_traits().vertex_conflict_2_object()(t1.site(),
						    t2.site(),
						    q.site());
  }

  inline
  Sign incircle(const Face_handle& f, const Storage_site_2& q) const {
    return incircle(f, q.site());
  }

  inline
  bool finite_edge_interior(const Storage_site_2& t1,
			    const Storage_site_2& t2,
			    const Storage_site_2& t3,
			    const Storage_site_2& t4,
			    const Storage_site_2& q, Sign sgn) const {
    return
      geom_traits().finite_edge_interior_conflict_2_object()
      (t1.site(), t2.site(), t3.site(), t4.site(), q.site(), sgn);
  }

  inline
  bool finite_edge_interior(const Face_handle& f, int i,
			    const Storage_site_2& q, Sign sgn) const {
    CGAL_precondition( !is_infinite(f) &&
		       !is_infinite(f->neighbor(i)) );
    return finite_edge_interior( f->vertex( ccw(i) )->site(),
				 f->vertex(  cw(i) )->site(),
				 f->vertex(     i  )->site(),
				 this->_tds.mirror_vertex(f, i)->site(),
				 q.site(), sgn);
  }

  inline
  bool finite_edge_interior(const Storage_site_2& t1,
			    const Storage_site_2& t2,
			    const Storage_site_2& t3,
			    const Storage_site_2& q,
			    Sign sgn) const {
    return geom_traits().finite_edge_interior_conflict_2_object()(t1.site(),
								  t2.site(),
								  t3.site(),
								  q.site(),
								  sgn);
  }

  inline
  bool finite_edge_interior(const Storage_site_2& t1,
			    const Storage_site_2& t2,
			    const Storage_site_2& q,
			    Sign sgn) const {
    return
      geom_traits().finite_edge_interior_conflict_2_object()(t1.site(),
							     t2.site(),
							     q.site(),
							     sgn);
  }

  bool finite_edge_interior(const Face_handle& f, int i,
			    const Storage_site_2& p, Sign sgn,
			    int j) const {
    return finite_edge_interior(f, i, p.site(), sgn, j);
  }

  inline
  bool infinite_edge_interior(const Storage_site_2& t2,
			      const Storage_site_2& t3,
			      const Storage_site_2& t4,
			      const Storage_site_2& q, Sign sgn) const {
    return
      geom_traits().infinite_edge_interior_conflict_2_object()
      (t2.site(), t3.site(), t4.site(), q.site(), sgn);
  }

  inline
  bool infinite_edge_interior(const Face_handle& f, int i,
			      const Storage_site_2& q, Sign sgn) const
  {
    return infinite_edge_interior(f, i, q, sgn);
  }

  inline
  bool edge_interior(const Face_handle& f, int i,
		     const Storage_site_2& t, Sign sgn) const {
    return edge_interior(f, i, t.site(), sgn);
  }

  inline
  bool edge_interior(const Edge& e,
		     const Storage_site_2& t, Sign sgn) const {
    return edge_interior(e.first, e.second, t, sgn);
  }

  inline Arrangement_type
  arrangement_type(const Storage_site_2& t, const Vertex_handle& v) const {
    if ( is_infinite(v) ) { return AT2::DISJOINT; }
    return arrangement_type(t, v->storage_site());
  }

  inline
  Arrangement_type arrangement_type(const Storage_site_2& p,
				    const Storage_site_2& q) const {
    return arrangement_type(p.site(), q.site());
  }

  inline
  bool are_parallel(const Storage_site_2& p, const Storage_site_2& q) const {
    return geom_traits().are_parallel_2_object()(p.site(), q.site());
  }

  inline Oriented_side
  oriented_side(const Storage_site_2& q, const Storage_site_2& supp,
		const Storage_site_2& p) const
  {
    CGAL_precondition( q.is_point() && supp.is_segment() && p.is_point() );
    return
      geom_traits().oriented_side_2_object()(q.site(), supp.site(), p.site());
  }

  inline Oriented_side
  oriented_side(const Storage_site_2& s1, const Storage_site_2& s2,
		const Storage_site_2& s3, const Storage_site_2& supp,
		const Storage_site_2& p) const {
    CGAL_precondition( supp.is_segment() && p.is_point() );
    return geom_traits().oriented_side_2_object()(s1.site(),
						  s2.site(),
						  s3.site(),
						  supp.site(), p.site());
  }


  //-------

  inline
  bool same_points(const Site_2& p, const Site_2& q) const {
    return geom_traits().equal_2_object()(p, q);
  }

  inline
  bool same_segments(const Site_2& t, Vertex_handle v) const {
    if ( is_infinite(v) ) { return false; }
    if ( t.is_point() || v->site().is_point() ) { return false; }
    return same_segments(t, v->site());
  }

  inline
  bool same_segments(const Site_2& p, const Site_2& q) const {
    CGAL_precondition( p.is_segment() && q.is_segment() );

    return
      (same_points(p.source_site(), q.source_site()) &&
       same_points(p.target_site(), q.target_site())) ||
      (same_points(p.source_site(), q.target_site()) &&
       same_points(p.target_site(), q.source_site()));
  }

  inline
  bool is_endpoint_of_segment(const Site_2& p, const Site_2& s) const
  {
    CGAL_precondition( p.is_point() && s.is_segment() );
    return ( same_points(p, s.source_site()) ||
	     same_points(p, s.target_site()) );
  }

  inline
  bool is_degenerate_segment(const Site_2& s) const {
    CGAL_precondition( s.is_segment() );
    return same_points(s.source_site(), s.target_site());
  }

  // returns:
  //   ON_POSITIVE_SIDE if q is closer to t1
  //   ON_NEGATIVE_SIDE if q is closer to t2
  //   ON_ORIENTED_BOUNDARY if q is on the bisector of t1 and t2
  inline
  Oriented_side side_of_bisector(const Site_2 &t1, const Site_2 &t2,
				 const Site_2 &q) const {
    return geom_traits().oriented_side_of_bisector_2_object()(t1, t2, q);
  }

  inline
  Sign incircle(const Site_2 &t1, const Site_2 &t2,
		const Site_2 &t3, const Site_2 &q) const {
    return geom_traits().vertex_conflict_2_object()(t1, t2, t3, q);
  }

  inline
  Sign incircle(const Site_2 &t1, const Site_2 &t2,
		const Site_2 &q) const {
    return geom_traits().vertex_conflict_2_object()(t1, t2, q);
  }

  inline
  Sign incircle(const Face_handle& f, const Site_2& q) const;

  inline
  Sign incircle(const Vertex_handle& v0, const Vertex_handle& v1,
		const Vertex_handle& v) const {
    CGAL_precondition( !is_infinite(v0) && !is_infinite(v1)
		       && !is_infinite(v) );

    return incircle( v0->site(), v1->site(), v->site());
  }

  Sign incircle(const Vertex_handle& v0, const Vertex_handle& v1,
		const Vertex_handle& v2, const Vertex_handle& v) const;

  inline
  bool finite_edge_interior(const Site_2& t1, const Site_2& t2,
			    const Site_2& t3, const Site_2& t4,
			    const Site_2& q,  Sign sgn) const {
    return
      geom_traits().finite_edge_interior_conflict_2_object()
      (t1,t2,t3,t4,q,sgn);
  }

  inline
  bool finite_edge_interior(const Face_handle& f, int i,
			    const Site_2& q, Sign sgn) const {
    CGAL_precondition( !is_infinite(f) &&
		       !is_infinite(f->neighbor(i)) );
    return finite_edge_interior( f->vertex( ccw(i) )->site(),
				 f->vertex(  cw(i) )->site(),
				 f->vertex(     i  )->site(),
				 this->_tds.mirror_vertex(f, i)->site(),
				 q, sgn);
  }

  inline
  bool finite_edge_interior(const Vertex_handle& v1, const Vertex_handle& v2,
			    const Vertex_handle& v3, const Vertex_handle& v4,
			    const Vertex_handle& v, Sign sgn) const {
    CGAL_precondition( !is_infinite(v1) && !is_infinite(v2) &&
		       !is_infinite(v3) && !is_infinite(v4) &&
		       !is_infinite(v) );
    return finite_edge_interior( v1->site(), v2->site(),
				 v3->site(), v4->site(),
				 v->site(), sgn);
  }

  inline
  bool finite_edge_interior(const Site_2& t1, const Site_2& t2,
			    const Site_2& t3, const Site_2& q,
			    Sign sgn) const {
    return
    geom_traits().finite_edge_interior_conflict_2_object()(t1,t2,t3,q,sgn);
  }

  inline
  bool finite_edge_interior(const Site_2& t1, const Site_2& t2,
			    const Site_2& q,  Sign sgn) const {
    return
    geom_traits().finite_edge_interior_conflict_2_object()(t1,t2,q,sgn);
  }

  bool finite_edge_interior(const Face_handle& f, int i,
			    const Site_2& p, Sign sgn, int) const;

  bool finite_edge_interior(const Vertex_handle& v1, const Vertex_handle& v2,
			    const Vertex_handle& v3, const Vertex_handle& v4,
			    const Vertex_handle& v, Sign, int) const;

  inline
  bool infinite_edge_interior(const Site_2& t2, const Site_2& t3,
			      const Site_2& t4, const Site_2& q,
			      Sign sgn) const {
    return
      geom_traits().infinite_edge_interior_conflict_2_object()
      (t2,t3,t4,q,sgn);
  }


  bool infinite_edge_interior(const Face_handle& f, int i,
			      const Site_2& q, Sign sgn) const;

  bool infinite_edge_interior(const Vertex_handle& v1,
			      const Vertex_handle& v2,
			      const Vertex_handle& v3,
			      const Vertex_handle& v4,
			      const Vertex_handle& v,
			      Sign sgn) const;

  bool edge_interior(const Face_handle& f, int i,
		     const Site_2& t, Sign sgn) const;

  bool edge_interior(const Edge& e,
		     const Site_2& t, Sign sgn) const {
    return edge_interior(e.first, e.second, t, sgn);
  }

  bool edge_interior(const Vertex_handle& v1,
		     const Vertex_handle& v2,
		     const Vertex_handle& v3,
		     const Vertex_handle& v4,
		     const Vertex_handle& v,
		     Sign sgn) const;

  inline Arrangement_type
  arrangement_type(const Site_2& t, const Vertex_handle& v) const {
    if ( is_infinite(v) ) { return AT2::DISJOINT; }
    return arrangement_type(t, v->site());
  }

  Arrangement_type arrangement_type(const Site_2& p, const Site_2& q) const;

  inline
  bool are_parallel(const Site_2& p, const Site_2& q) const {
    return geom_traits().are_parallel_2_object()(p, q);
  }

  inline Oriented_side
  oriented_side(const Site_2& q, const Site_2& supp, const Site_2& p) const
  {
    CGAL_precondition( q.is_point() && supp.is_segment() && p.is_point() );
    return geom_traits().oriented_side_2_object()(q, supp, p);
  }

  inline Oriented_side
  oriented_side(const Site_2& s1, const Site_2& s2, const Site_2& s3,
		const Site_2& supp, const Site_2& p) const {
    CGAL_precondition( supp.is_segment() && p.is_point() );
    return geom_traits().oriented_side_2_object()(s1, s2, s3, supp, p);
  }

  bool is_degenerate_edge(const Site_2& p1,
			  const Site_2& p2,
			  const Site_2& p3,
			  const Site_2& p4) const {
    return geom_traits().is_degenerate_edge_2_object()
      (p1, p2, p3, p4);
  }

  bool is_degenerate_edge(const Vertex_handle& v1,
			  const Vertex_handle& v2,
			  const Vertex_handle& v3,
			  const Vertex_handle& v4) const {
    CGAL_precondition( !is_infinite(v1) && !is_infinite(v2) &&
		       !is_infinite(v3) && !is_infinite(v4) );

    return is_degenerate_edge(v1->site(), v2->site(),
			      v3->site(), v4->site());
  }

  bool is_degenerate_edge(const Face_handle& f, int i) const {
    Vertex_handle v1 = f->vertex( ccw(i) );
    Vertex_handle v2 = f->vertex(  cw(i) );
    Vertex_handle v3 = f->vertex(     i  );
    Vertex_handle v4 = this->_tds.mirror_vertex(f, i);

    return is_degenerate_edge(v1, v2, v3, v4);
  }

  bool is_degenerate_edge(const Edge& e) const {
    return is_degenerate_edge(e.first, e.second);
  }

  Vertex_handle first_endpoint_of_segment(const Vertex_handle& v) const;
  Vertex_handle second_endpoint_of_segment(const Vertex_handle& v) const;

}; // Segment_Delaunay_graph_2


template<class Gt, class DS, class LTag>
std::istream& operator>>(std::istream& is,
			 Segment_Delaunay_graph_2<Gt,DS,LTag>& sdg)
{
  sdg.file_input(is);
  return is;
}

template<class Gt, class DS, class LTag>
std::ostream& operator<<(std::ostream& os,
			 const Segment_Delaunay_graph_2<Gt,DS,LTag>& sdg)
{
  sdg.file_output(os);
  return os;
}

CGAL_END_NAMESPACE


#ifdef CGAL_CFG_NO_AUTOMATIC_TEMPLATE_INCLUSION
#  include <CGAL/Segment_Delaunay_graph_2.C>
#endif



#endif // CGAL_SEGMENT_DELAUNAY_GRAPH_2_H
