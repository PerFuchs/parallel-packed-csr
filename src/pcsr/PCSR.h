//
// Created by Eleni Alevra on 02/06/2020.
//
#include <shared_mutex>
#include <atomic>
using namespace std;
#ifndef PCSR2_PCSR_H
#define PCSR2_PCSR_H

/** Types */
typedef struct _node {
  // beginning and end of the associated region in the edge list
  uint32_t beginning;     // deleted = max int
  uint32_t end;           // end pointer is exclusive
  uint32_t num_neighbors; // number of edges with this node as source
} node_t;

// each node has an associated sentinel (max_int, offset) that gets back to its
// offset into the node array
// UINT32_MAX
//
// if value == UINT32_MAX, read it as null.
typedef struct _edge {
  uint32_t src;
  uint32_t dest; // destination of this edge in the graph, MAX_INT if this is a
  // sentinel
  uint32_t
      value; // edge value of zero means it a null since we don't store 0 edges
} edge_t;

typedef struct edge_list {
  int N;
  int H;
  int logN;
  shared_ptr<shared_timed_mutex> global_lock;
  atomic<int> *node_version_counters; // Keeps a version number for each leaf node.
  // The version number is incremented when a change happens
  edge_t *items;
} edge_list_t;

// When we acquire locks to insert we have to make checks to see up to which position we will redistribute
// during the insert
// To avoid repeating this check during the actual insertion we pass this struct to it so it can immediately know
// up to where it needs to redistribute
typedef struct insertion_info {
  bool slide_right; // false means slide_left
  uint32_t first_empty; // first empty spot for slide
  int max_len; // len to redistribute up to
  int node_index_final; // final node index for redistr
  bool double_list; // double_list during redistr
} insertion_info_t;

#define is_null(val) val == 0
#define NEED_GLOBAL_WRITE -1
#define NEED_RETRY -2
#define EDGE_NOT_FOUND -3

class PCSR {
public:
  // data members
  edge_list_t edges;

  PCSR(uint32_t init_n, uint32_t, bool lock_search);
  PCSR(uint32_t init_n, vector<condition_variable*> *cvs, bool search_lock);
  ~PCSR();
  /** Public API */
  bool edge_exists(uint32_t src, uint32_t dest);
  void add_node();
  void add_edge(uint32_t src, uint32_t dest, uint32_t value);
  void remove_edge(uint32_t src, uint32_t dest);
  void read_neighbourhood(int src);

private:
  // data members
  std::vector<node_t> nodes;
  shared_timed_mutex** node_locks; // locks for every PCSR leaf node
  bool lock_bsearch = false; // true if we lock during binary search

  // members used when parallel redistributing is enabled
  bool adding_sentinels = false; // true if we are in the middle of inserting a sentinel node
  mutex* redistr_mutex; // for synchronisation with the redistributing worker threads
  condition_variable* redistr_cv; // for synchronisation with the redistributing worker threads
  vector<mutex*> *redistr_locks; // for synchronisation with the redistributing worker threads
  vector<condition_variable*> *redistr_cvs; // for synchronisation with the redistributing worker threads

  void redistribute(int index, int len);
  bool got_correct_insertion_index(edge_t ins_edge, uint32_t src, uint32_t index, edge_t elem, int node_index,
                                   int node_id, uint32_t &max_node, uint32_t &min_node);
  pair<pair<int,int>, insertion_info_t*> acquire_insert_locks(uint32_t index, edge_t elem, uint32_t src, int ins_node_v,
                                                              uint32_t left_node_bound, int tries);
  pair<int,int> acquire_remove_locks(uint32_t index, edge_t elem, uint32_t src, int ins_node_v, uint32_t left_node_bound);
  void release_locks(pair<int,int> acquired_locks);
  void release_locks_no_inc(pair<int,int> acquired_locks);
  uint32_t find_value(uint32_t src, uint32_t dest);
  vector<uint32_t>
  sparse_matrix_vector_multiplication(std::vector<uint32_t> const &v);
  vector<float> pagerank(std::vector<float> const &node_values);
  vector<uint32_t> bfs(uint32_t start_node);
  void double_list();
  void half_list();
  int slide_right(int index, uint32_t src);
  void slide_left(int index, uint32_t src);
  void add_edge_parallel(uint32_t src, uint32_t dest, uint32_t value, int retries);
  void insert(uint32_t index, edge_t elem, uint32_t src, insertion_info_t *info);
  void remove(uint32_t index, edge_t elem, uint32_t src);
  uint32_t get_node_id(uint32_t node_index);
  void print_array();
  void print_graph(int);
  pair<double, int> redistr_store(edge_t *space, int index, int len);
  void fix_sentinel(int32_t node_index, int in);
  int count_elems(int index, int len);
  bool is_sorted();
  int count_total_edges();
  uint64_t get_size();
  uint64_t get_n();
  vector<tuple<uint32_t, uint32_t, uint32_t>> get_edges();
  void clear();
};

#endif //PCSR2_PCSR_H
