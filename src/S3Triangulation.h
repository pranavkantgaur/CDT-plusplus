/// Causal Dynamical Triangulations in C++ using CGAL
///
/// Copyright (c) 2014 Adam Getchell
///
/// Creates a foliated 2-sphere triangulation
///
/// The number of desired timeslices is given, and
/// successive 3D spheres are created with increasing radii.
/// Each vertex at a given radius is assigned a timeslice so that the
/// entire triangulation will have a preferred foliation of time.
///
/// - DONE: Insert a 3-sphere into the triangulation data structure
/// - DONE: Assign each 3-sphere a unique timeslice
/// - DONE: Iterate over the number of desired timeslices
/// - DONE: Check/fix issues for large values of simplices and timeslices
/// - DONE: Iterate over cells and check timeslices of vertices don't differ
///       by more than 1.
/// - DONE: Gather ratio of cells with bad/good foliation.
///       Adjust value of radius to minimize.
///       Recheck the whole triangulation when finished.
/// - DONE: When a cell contains a bad foliation, delete it. Recheck.
/// - DONE: Fixup Delaunay triangulation after bad cells have been deleted
/// - DONE: Classify cells as (3,1), (2,2), or (1,3) based on their foliation.
/// The vectors **three_one**, **two_two**, and **one_three** contain cell
/// handles to the simplices of type (3,1), (2,2), and (1,3) respectively.
/// - DONE: Classify edges as timelike or spacelike so that action can be
/// calculated.

#ifndef SRC_S3TRIANGULATION_H_
#define SRC_S3TRIANGULATION_H_

// CGAL headers
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Delaunay_triangulation_3.h>
#include <CGAL/Triangulation_vertex_base_with_info_3.h>
#include <CGAL/Triangulation_cell_base_with_info_3.h>
#include <CGAL/point_generators_3.h>

// C headers
#include <assert.h>
#include <math.h>

// C++ headers
#include <boost/iterator/zip_iterator.hpp>
#include <vector>
#include <list>

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;
// Used so that each timeslice is assigned an integer
// typedef CGAL::Triangulation_3<K>  Triangulation;
// typedef CGAL::Triangulation_vertex_base_with_info_3<unsigned, K> Vb;
// typedef CGAL::Triangulation_cell_base_with_info_3<unsigned, K> Cb;
// typedef CGAL::Triangulation_data_structure_3<Vb, Cb> Tds;
// Parallel insertion capable with integer timeslices
typedef CGAL::Triangulation_data_structure_3<
  CGAL::Triangulation_vertex_base_with_info_3<unsigned, K>,
  CGAL::Triangulation_cell_base_with_info_3<unsigned, K>,
  CGAL::Parallel_tag>                           Tds;
typedef CGAL::Delaunay_triangulation_3<K, Tds>  Delaunay;
typedef Delaunay::Cell_handle Cell_handle;
typedef Delaunay::Vertex_handle Vertex_handle;
typedef Delaunay::Locate_type Locate_type;
typedef Delaunay::Point Point;

inline void insert_into_S3(Delaunay* D3, std::vector<Point> *vertices,
                    std::vector<unsigned> *timevalue) {
  // Zip together vertices and timeslice values
  D3->insert(boost::make_zip_iterator(boost::make_tuple(vertices->begin(),
                                      timevalue->begin() )),
             boost::make_zip_iterator(boost::make_tuple(vertices->end(),
                                      timevalue->end())));
} // insert_into_S3()

/// This function iterates over all edges in the triangulation
/// and classifies them as timelike or spacelike.
/// The integers **N1_TL** and **N1_SL** count the number of timelike and
/// spacelike edges respectively.
inline void classify_edges(Delaunay* D3,
                          unsigned* N1_TL,
                          unsigned* N1_SL) {
  Delaunay::Finite_edges_iterator eit;
  for (eit = D3->finite_edges_begin(); eit != D3->finite_edges_end(); ++eit) {
    // Get endpoints of edges and find their timevalues
    // If they differ, increment N1_TL, otherwise increment N1_SL
    // An edge is a triple; the first element is the cell handle, and the
    // second and third are the integers representing the i-th vertices of
    // the cell
    Cell_handle ch = eit->first;
    // Now we can get the values of the endpoints
    unsigned time1 = ch->vertex(eit->second)->info();
    unsigned time2 = ch->vertex(eit->third)->info();

    // Debugging
    // std::cout << "Edge: first timevalue is " << time1 << std::endl;
    // std::cout << "Edge: second timevalue is " << time2 << std::endl;
    // std::cout << std::endl;

    if (time1 == time2) {
      (*N1_SL)++;
    } else {
      (*N1_TL)++;
    }
  }
  // Debugging
  std::cout << "N1_SL = " << *N1_SL << std::endl;
  std::cout << "N1_TL = " << *N1_TL << std::endl;
}  // classify_edges()

/// This function iterates over all cells in the triangulation
/// and classifies them as:
/**
\f{eqnarray*}{
     31 &=& (3, 1) \\
     22 &=& (2, 2) \\
     13 &=& (1, 3)
\f}
The vectors **three_one**, **two_two**, and **one_three** contain cell handles
to all the simplices in the triangulation of that corresponding type.
*/
inline void classify_3_simplices(Delaunay* D3,
            std::vector<Cell_handle>* three_one,
            std::vector<Cell_handle>* two_two,
            std::vector<Cell_handle>* one_three) {
  std::cout << "Classifying simplices...." << std::endl;
  Delaunay::Finite_cells_iterator cit;

  for (cit = D3->finite_cells_begin(); cit != D3->finite_cells_end(); ++cit) {
    std::list<unsigned> timevalues;
    unsigned max_time{0};
    unsigned current_time{0};
    unsigned max_values{0};
    unsigned min_values{0};
    // Push every time value into a list
    for (size_t i = 0; i < 4; i++) {
      timevalues.push_back(cit->vertex(i)->info());
    }
    // Now sort the list
    timevalues.sort();
    // The maximum timevalue is at the end of the list
    max_time = timevalues.back();
    timevalues.pop_back();
    // std::cout << "The maximum time value is " << max_time << std::endl;
    max_values++;

    while (!timevalues.empty()) {
      current_time = timevalues.back();
      timevalues.pop_back();
      (current_time == max_time) ? max_values++ : min_values++;
    }

    if (max_values == 3) {
      cit->info() = 13;
      one_three->push_back(cit);
    } else if (max_values == 2) {
      cit->info() = 22;
      two_two->push_back(cit);
    } else {
      cit->info() = 31;
      three_one->push_back(cit);
    }
  }
}  // classify_3_simplices()


/// This function nulls out the **three_one**, **two_two**, and **one_three**
/// vectors and then calls **classify_3_simplices()**
inline void reclassify_3_simplices(Delaunay* D3,
            std::vector<Cell_handle>* three_one,
            std::vector<Cell_handle>* two_two,
            std::vector<Cell_handle>* one_three) {
  // Null out the vectors
  three_one->clear();
  two_two->clear();
  one_three->clear();
  // Now classify simplices
  classify_3_simplices(D3, three_one, two_two, one_three);
}  //  reclassify_3_simplices()

/// This function iterates over all of the cells in the triangulation.
/// Within each cell, it iterates over all of the vertices and reads timeslices.
/// Validity of the cell is first checked by the **is_valid()** function.
/// The foliation validity is then checked by comparing timeslices in each
/// vertex and ensuring that the difference is exactly 1.
/// If a cell has a bad foliation, the vertex with the highest timeslice is
/// deleted. The Delaunay triangulation is then recomputed on the remaining
/// vertices.
/// This function is repeatedly called up to **MAX_FOLIATION_FIX_PASSES** times
/// as set in make_S3_triangulation().
inline void fix_timeslices(Delaunay* D3, bool output) {
  std::cout << "Fixing foliation...." << std::endl;
  Delaunay::Finite_cells_iterator cit;
  unsigned min_time, max_time;
  unsigned max_vertex{0};

  for (cit = D3->finite_cells_begin(); cit != D3->finite_cells_end(); ++cit) {
    if (cit->is_valid()) {
      // Set min_time and max_time to first vertex timeslice
      min_time = cit->vertex(0)->info();
      max_time = min_time;

      // Iterate over each vertex in a cell
      for (size_t i = 0; i < 4; i++) {
        unsigned current_time = cit->vertex(i)->info();
        if (current_time < min_time) min_time = current_time;
        if (current_time > max_time) {
          max_time = current_time;
          max_vertex = i;
        }
      }

      // If max_time - min_time != 1 delete max_vertex
      if (max_time - min_time != 1) {
        D3->remove(cit->vertex(max_vertex));
        if (output) {
          std::cout << "Vertex " << max_vertex;
          std::cout << " of cell removed." << std::endl;
        }
      }
    } else {
        // Do nothing for now
    }
  }
}  // fix_timeslices()

/// This function iterates over all of the cells in the triangulation.
/// Within each cell, it iterates over all of the vertices and reads timeslices.
/// Validity of the cell is first checked by the **is_valid()** function.
/// The foliation validity is then verified by comparing the maximum and
/// minimum timeslices in each cell and ensuring that the difference
/// is exactly 1.
/// The values of the unsigned variables **valid** and **invalid** give the
/// number of those types of cells respectively.
inline bool check_timeslices(Delaunay* D3, bool output) {
  Delaunay::Finite_cells_iterator cit;
  unsigned min_time, max_time;
  unsigned valid{0}, invalid{0};
  // Iterate over all cells in the Delaunay triangulation
  for (cit = D3->finite_cells_begin();  cit != D3->finite_cells_end(); ++cit) {
    if (cit->is_valid()) {
      // debugging
      if (output) std::cout << "The following cell is valid." << std::endl;
      min_time = cit->vertex(0)->info();
      max_time = min_time;
      for (size_t i = 0; i < 4; i++) {
        unsigned current_time = cit->vertex(i)->info();
        // Iterate over all vertices in the cell
        if (output) {  // debugging
          std::cout << "Vertex " << i << " is " << cit->vertex(i)->point();
          std::cout << " with timeslice " << current_time << std::endl;
        }

        if (current_time < min_time) min_time = current_time;
        if (current_time > max_time) max_time = current_time;
      }
        // There should be a difference of 1 between max and min
        if (max_time - min_time != 1) {
          if (output) {
            std::cout << "Foliation is invalid for this cell." << std::endl;
          }
          invalid++;
        } else {
            if (output) {  // debugging
              std::cout << "Foliation is valid for this cell." << std::endl;
            }
          valid++;
        }
    } else {
        // Remove all vertices in the invalid cell
        // for(size_t i = 0; i < 4; i++)
        // {
        //
        //   D3->remove(cit->vertex(i));
        // }

        // Or, just remove the cell directly!
        // D3->tds().delete_cell(cit);
        // This function does *not* preserve the Delaunay triangulation!
        // After this, D3->is_valid() is false!

        if (output) std::cout << "The following cell is invalid." << std::endl;
        invalid++;
    }
  }
  assert(D3->is_valid());
  if (output) {
    std::cout << "There are " << invalid << " invalid cells";
    std::cout << " and " << valid << " valid cells in this triangulation.";
    std::cout << std::endl;
  }

  return (invalid == 0) ? true : false;
}  // check_timeslices()

/// This function makes 2-spheres of varying radii.
/// The radius is used to denote the time value, so we can nest 2-spheres
/// such that our time foliation contains leaves of identical topology.
inline void make_2_sphere(std::vector<Point> *vertices,
            std::vector<unsigned> *timevalue,
            int number_of_points,
            double radius,
            bool output) {
  CGAL::Random_points_on_sphere_3<Point> gen(radius);

  for (size_t j = 0; j < number_of_points; j++) {
    vertices->push_back(*gen++);
    timevalue->push_back(static_cast<unsigned int>(radius));
  }

  if (output) {
    std::cout << "Generating " << number_of_points << " random points on "
    << "the surface of a sphere in 3D of center 0 and radius "
    << radius << "." << std::endl;
  }
}  // make_2_sphere()

/// This function creates a valid 2+1 foliation from a Delaunay triangulation.
/// First, the number of points per leaf in the foliation is estimated given
/// the desired number of simplices.
/// Next, make_2_sphere() is called per timeslice to generate nested spheres.
/// The radius of the sphere is assigned as the time value for each vertex
/// in that sphere, which comprises a leaf in the foliation.
/// All vertices in all spheres (along with their time values) are then
/// inserted into a Delaunay triangulation
/// (see http://en.wikipedia.org/wiki/Delaunay_triangulation for details).
/// Next, we use check_timeslices() to check every cell in the DT for valid
/// time values. Invalid time values in a cell are removed by fix_timeslices().
/// Finally, the cells (simplices) are sorted by classify_3_simplices() into
/// corresponding vectors which contain cell handles to that type of simplex.
/// The vector **three_one** contains handles to all the (3,1) simplices,
/// the vector **two_two** contains handles to the (2,2) simplices, and
/// the vector **one_three** contains handles to the (1,3) simplices.
/// A last check is performed to ensure a valid Delaunay triangulation.
inline void make_S3_triangulation(Delaunay* D3,
            unsigned simplices,
            unsigned timeslices,
            bool output,
            std::vector<Cell_handle>* three_one,
            std::vector<Cell_handle>* two_two,
            std::vector<Cell_handle>* one_three) {
  std::cout << "Generating universe ..." << std::endl;
  const unsigned simplices_per_timeslice = simplices / timeslices;
  const unsigned MAX_FOLIATION_FIX_PASSES = 20;

  assert(simplices_per_timeslice >= 1);

  const int points = simplices_per_timeslice * 4;
  const int total_points = points * timeslices;
  double radius;

  std::vector<Point> vertices;
  std::vector<unsigned> timevalue;

  // We know how many points we have in advance, so reserve memory
  vertices.reserve(total_points);
  timevalue.reserve(total_points);

  for (size_t i = 0; i < timeslices; i++) {
    // std::cout << "Loop " << i << std::endl;
    radius = 1.0 + static_cast<double>(i);
    make_2_sphere(&vertices, &timevalue, points, radius, output);
  }

  // D3->insert(vertices.begin(), vertices.end());
  // Insert vertices and timeslices
  insert_into_S3(D3, &vertices, &timevalue);

  // Remove cells that have invalid foliations
  unsigned pass = 0;
  while (!check_timeslices(D3, output)) {
    pass++;
    if (pass > MAX_FOLIATION_FIX_PASSES) break;
    std::cout << "Pass #" << pass << std::endl;
    fix_timeslices(D3, output);
  }

  // Classify simplices and put cell handles of each simplex type
  // into a corresponding vector
  classify_3_simplices(D3, three_one, two_two, one_three);

  // Print out results
  bool valid = check_timeslices(D3, false);
  std::cout << "Valid foliation: " << std::boolalpha << valid << std::endl;
  std::cout << "Delaunay triangulation has " << D3->number_of_finite_cells();
  std::cout << " cells." << std::endl;
  std::cout << "There are " << three_one->size() << " (3,1) simplices" <<
               " and " << two_two->size() << " (2,2) simplices and " <<
               one_three->size() << " (1,3) simplices." << std::endl;
  if (output) {
    Delaunay::Finite_vertices_iterator vit;
    for (vit = D3->finite_vertices_begin();
          vit != D3->finite_vertices_end();   ++vit) {
        std::cout << "Point " << vit->point() << " has timeslice ";
        std::cout << vit->info() << std::endl;
    }
  }
  assert(D3->is_valid());
}  // make_S3_triangulation()
#endif  // SRC_S3TRIANGULATION_H_
