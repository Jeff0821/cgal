// Copyright (c) 2009 INRIA Sophia-Antipolis (France).
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
// $URL: $
// $Id: $
//
//
// Author(s)     : Pierre Alliez
//
//******************************************************************************
// File Description :
//
//******************************************************************************

#include <iostream>
#include <fstream>
#include <CGAL/Timer.h>

#include <CGAL/AABB_intersections.h>

#include <CGAL/AABB_tree.h>
#include <CGAL/AABB_traits.h>
#include <CGAL/Polyhedron_3.h>
#include <CGAL/IO/Polyhedron_iostream.h>
#include <CGAL/AABB_polyhedron_triangle_primitive.h>

#include "AABB_test_util.h"


template <class K>
void test(const char *filename)
{
        typedef typename K::FT FT;
        typedef typename K::Ray_3 Ray;
        typedef typename K::Point_3 Point;
        typedef typename K::Vector_3 Vector;
        typedef CGAL::Polyhedron_3<K> Polyhedron;
        typedef CGAL::AABB_polyhedron_triangle_primitive<K,Polyhedron> Primitive;
        typedef CGAL::AABB_traits<K, Primitive> Traits;
        typedef CGAL::AABB_tree<Traits> Tree;

        Polyhedron polyhedron;
        std::ifstream ifs(filename);
        ifs >> polyhedron;

        // constructs AABB tree and internal search KD-tree with 
        // the points of the polyhedron
        Tree tree(polyhedron.facets_begin(),polyhedron.facets_end());
        tree.accelerate_distance_queries(polyhedron.points_begin(),polyhedron.points_end());

        // call all tests
        test_distance_speed<Tree,K>(tree);
        test_all_distance_query_types<Tree,K>(tree);
}

void test_kernels(const char *filename)
{
    std::cout << std::endl;
    std::cout << "Polyhedron " << filename << std::endl;
    std::cout << "============================" << std::endl;

    std::cout << std::endl;
    std::cout << "Simple cartesian float kernel" << std::endl;
    test<CGAL::Simple_cartesian<float> >(filename);

    std::cout << std::endl;
    std::cout << "Cartesian float kernel" << std::endl;
    test<CGAL::Cartesian<float> >(filename);

    std::cout << std::endl;
    std::cout << "Simple cartesian double kernel" << std::endl;
    test<CGAL::Simple_cartesian<double> >(filename);

    std::cout << std::endl;
    std::cout << "Cartesian double kernel" << std::endl;
    test<CGAL::Cartesian<double> >(filename);

    std::cout << std::endl;
    std::cout << "Epic kernel" << std::endl;
    test<CGAL::Exact_predicates_inexact_constructions_kernel>(filename);
}

int main(void)
{
        std::cout << "AABB distance tests" << std::endl;
        test_kernels("./data/cube.off");
        test_kernels("./data/coverrear.off");
        test_kernels("./data/nested_spheres.off");
        test_kernels("./data/finger.off");
        return 0;
}
