// ============================================================================
//
// Copyright (c) 1997-2000 The CGAL Consortium
//
// This software and related documentation is part of an INTERNAL release
// of the Computational Geometry Algorithms Library (CGAL). It is not
// intended for general use.
//
// ----------------------------------------------------------------------------
//
// file          : demo/Qt_widget/Triangulation_2/triangulation_2_layers.h
// package       : Qt_widget
// author(s)     : Radu Ursu
// release       : 
// release_date  : 
//
// coordinator   : Laurent Rineau <rineau@clipper.ens.fr>
//
//
// ============================================================================

#ifndef CGAL_REGULAR_TRIANGULATION_2_LAYERS_H
#define CGAL_REGULAR_TRIANGULATION_2_LAYERS_H

#include <CGAL/Cartesian.h>
#include <CGAL/Circle_2.h>
#include <CGAL/IO/Qt_widget_layer.h>
#include <CGAL/IO/Qt_widget_Regular_triangulation_2.h>
#include <qobject.h>


template <class T>
class Qt_layer_show_triangulation : public CGAL::Qt_widget_layer
{
public:
  Qt_layer_show_triangulation(T &t) : tr(t){};
  void draw()
  {
    *widget << CGAL::BLUE; 
    *widget << tr;
  };
private:
  T &tr;
};//end class 

template <class T>
class Qt_layer_show_voronoi : public CGAL::Qt_widget_layer
{
public:
  Qt_layer_show_voronoi(T &t1) : tr(t1){};
  void draw()
  {
    *widget << CGAL::RED ;
    tr.draw_dual(*widget);
  };
private:
  T	&tr;
};//end class 

template <class T>
class Qt_layer_show_points : public CGAL::Qt_widget_layer {
public:
  typedef typename T::Point           Point;  
  typedef typename T::Segment         Segment;
  typedef typename T::Vertex          Vertex;
  typedef typename T::Finite_vertices_iterator
                                      Finite_vertices_iterator;
  typedef typename T::Hidden_vertices_iterator
                                      Hidden_verticex_iterator;

  Qt_layer_show_points(T &t) : rt(t){};
  void draw()
  {  
    Finite_vertices_iterator it = rt.finite_vertices_begin(), 
                    beyond = rt.finite_vertices_end();
    *widget << CGAL::GREEN << CGAL::PointSize (3) 
		<< CGAL::PointStyle (CGAL::DISC);    
    while(it != beyond) {      
      *widget << (*it).point();
      *widget << Circle((*it).point().point(), (*it).point().weight());
      ++it;
    }

    Hidden_verticex_iterator hit=rt.hidden_vertices_begin(),
                       hbeyond = rt.hidden_vertices_end();
    *widget << CGAL::YELLOW << CGAL::PointSize (3) 
		<< CGAL::PointStyle (CGAL::DISC);  
    while(hit != hbeyond) {      
      *widget << (*hit).point();
      *widget << Circle((*hit).point().point(), (*hit).point().weight());
      ++hit;
    }
  };
private:
  T	&rt;
};//end class 


#endif
