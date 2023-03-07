#ifndef FDE20_BONUSPROJECT_3_KNN_HPP
#define FDE20_BONUSPROJECT_3_KNN_HPP

#include "Matrix.hpp"
#include <set>
#include <map>
#include <iterator>


using namespace std;
#define INF 0x3f3f3f3f

//---------------------------------------------------------------------------
/// Find the top k neighbors for the node start. The directed graph is stored in
/// matrix m and you can use getNeighbors(node) to get all neighbors of a node.
/// A more detailed description is provided in Matrix.hpp.
/// The function should return the k nearest neighbors in sorted order as vector
/// of Matrix::Entry, where Entry->column is the neighbor node and Entry->weight
/// is the cost to reach the node from the start node.
std::vector<Matrix::Entry> getKNN(const Matrix &m, unsigned start, unsigned k) {

  using Entry = Matrix::Entry;
  std::vector<Entry> result;
  result.reserve(k);

  // ToDo implement your solution here
  set<pair<double,unsigned>> nearestpath;
  set<pair<double,unsigned>> temp_queue;
  vector<double> weight_vec(m.width + 1, INF);
  //initialize temp_queue
  temp_queue.insert(make_pair(0.0,start));
  weight_vec[start] = 0.0;
  
  while(!temp_queue.empty())
  {
    pair<int,double> vertex = *(temp_queue.begin());
    int vertex_name = vertex.second;
    double vertex_weight = weight_vec[vertex_name];
    temp_queue.erase(temp_queue.begin());

    for (auto &neighbour : m.getNeighbors(vertex_name))
    {
        int neighbour_name = neighbour.column;

        double neighbour_weight = neighbour.weight;

        if (weight_vec[neighbour_name] > vertex_weight + neighbour_weight)
        {
            

            if (weight_vec[neighbour_name] != INF)
            {
                temp_queue.erase(temp_queue.find(make_pair(weight_vec[neighbour_name],neighbour_name)));
                nearestpath.erase(nearestpath.find(make_pair(weight_vec[neighbour_name],neighbour_name)));
            }

            
            weight_vec[neighbour_name] = vertex_weight + neighbour_weight;
            temp_queue.insert(make_pair(weight_vec[neighbour_name],neighbour_name));
            nearestpath.insert(make_pair(weight_vec[neighbour_name],neighbour_name));

        }
        
    }
  }
    if (nearestpath.size() < k)
    {
        
        for (auto iter = nearestpath.begin(); iter != nearestpath.end(); iter++)
        {
            if (iter->first == INF)
                break;
            result.push_back({iter->second,iter->first});
        }
    }
    else
    {
        
        auto iter_end = nearestpath.begin();
        std::advance(iter_end, k);
        for (auto iter = nearestpath.begin(); iter != iter_end; iter++)
        {
            if (iter->first == INF)
                break;
            result.push_back({iter->second,iter->first});
        }
    }

return result;
}

//---------------------------------------------------------------------------

#endif // FDE20_BONUSPROJECT_3_KNN_HPP
