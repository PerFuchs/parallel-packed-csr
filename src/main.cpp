#include <iostream>
#include <vector>
#include <utility>
#include <set>
#include <fstream>
#include <string>
#include <ctime>
#include <chrono>
#include <assert.h>
#include "thread_pool/thread_pool.cpp"
#include "utils.h"

using namespace std;

struct BinaryEdge {
    uint32_t src;
    uint32_t dst;
};

/*
 * Reads an edgelist from a binary file which contains edges of the type of struct BinaryEdge.
 */
vector<pair<int, int>> read_binary(const string filename) {
    cout << "Reading binary file: " << filename << endl;

    FILE* file = fopen(filename.c_str(), "rb");
    assert(file);

    size_t fileSize = fsize(filename);
    size_t numberOfEdges = fileSize / sizeof(BinaryEdge);

    vector<pair<int, int>> edges;
    edges.reserve(numberOfEdges);

    BinaryEdge edge = {0, 0};
    while(fread(&edge, sizeof(edge), 1, file) > 0) {
        edges.emplace_back(make_pair(edge.src, edge.dst));
    }

    fclose(file);
    return edges;
}

// Reads edge list with space separator
vector<pair<int, int>> read_input(string filename) {
  ifstream f;
  string line;
  f.open(filename);
  vector<pair<int, int>> edges;
  while (getline(f, line)) {
    int src = stoi(line.substr(0, line.find(' ')));
    int target = stoi(line.substr(line.find(' ') + 1, line.size()));
    edges.push_back(make_pair(src, target));
  }
  return edges;
}

// Reads edge list with comma separator
vector<pair<int, int>> read_input2(string filename) {

  if (endsWith(filename, ".elog")) {
      return read_binary(filename);
  }

  ifstream f;
  string line;
  f.open(filename);
  vector<pair<int, int>> edges;
  while (getline(f, line)) {
    if (line.find(',') == std::string::npos) {
      return read_input(filename);
    }
    int src = stoi(line.substr(0, line.find(',')));
    int target = stoi(line.substr(line.find(',') + 1, line.size()));
    edges.push_back(make_pair(src, target));
  }
  return edges;
}

// Loads core graph
ThreadPool *insert_with_thread_pool(vector<pair<int, int>> *input, int threads, bool lock_search, uint vertex_count) {
  ThreadPool *thread_pool = new ThreadPool(threads, lock_search, vertex_count);
  cout << "Submitting in bulk" << endl;
  thread_pool->submit_bulk(input);
  cout << "Submitted edges to load to core graph" << endl;

  auto start = chrono::steady_clock::now();
  thread_pool->start(threads);
  thread_pool->stop();
  auto finish = chrono::steady_clock::now();
  cout << "Reading Core graph: " << chrono::duration_cast<chrono::milliseconds>(finish - start).count() << endl;
  return thread_pool;
}

// Does insertions
ThreadPool *update_existing_graph(vector<pair<int, int>> input, ThreadPool *thread_pool, int threads, int size) {
  for (int i = 0; i < size; i++) {
    thread_pool->submit_add(i % threads, input[i].first, input[i].second);
  }
  auto start = chrono::steady_clock::now();
  thread_pool->start(threads);
  thread_pool->stop();
  auto finish = chrono::steady_clock::now();
  cout << "Updating edges took (milliseconds): " << chrono::duration_cast<chrono::milliseconds>(finish - start).count() << endl;
  return thread_pool;
}

// Does deletions
void thread_pool_deletions(ThreadPool *thread_pool, vector<pair<int, int>> deletions, int threads, int size) {
  int NUM_OF_THREADS = threads;
  for (int i = 0; i < size; i++) {
    thread_pool->submit_delete(i % NUM_OF_THREADS, deletions[i].first, deletions[i].second);
  }
  auto start = chrono::steady_clock::now();
  thread_pool->start(threads);
  thread_pool->stop();
  auto finish = chrono::steady_clock::now();
  cout << "Deletions" << chrono::duration_cast<chrono::milliseconds>(finish - start).count() << endl;
}

int main(int argc, char *argv[]) {
  int threads = 8;
  int size = 1000000;
  bool lock_search = true;
  bool insert = true;
  uint vertex_count = 0;

  vector<pair<int, int>> core_graph;
  vector<pair<int, int>> updates;
  for (int i = 1; i < argc; i++) {
    string s = string(argv[i]);
    if (s.rfind("-threads=", 0) == 0) {
      threads = stoi(s.substr(9, s.length()));
    } else if (s.rfind("-size=", 0) == 0) {
      size = stoi(s.substr(6, s.length()));
    } else if (s.rfind("-lock_free", 0) == 0) {
      lock_search = false;
    } else if (s.rfind("-insert", 0) == 0) {
      insert = true;
    } else if (s.rfind("-delete", 0) == 0) {
      insert = false;
    } else if (s.rfind("-core_graph=", 0) == 0) {
      string core_graph_filename = s.substr(12, s.length());
      cout << "Core graph: " << core_graph_filename << endl;

      auto start = chrono::steady_clock::now();
      core_graph = read_input2(core_graph_filename);
      auto finish = chrono::steady_clock::now();
      cout << "Reading Core Graph from file: " << chrono::duration_cast<chrono::milliseconds>(finish - start).count() << endl;
    } else if (s.rfind("-update_file=", 0) == 0) {
      string update_filename = s.substr(13, s.length());
      cout << "Update file: " << update_filename << endl;
      updates = read_input2(update_filename);
    } else if (s.rfind("-vertex_count=", 0) == 0) {
        vertex_count = stoi(s.substr(14, s.length()));
        cout << "Vertex count: " << vertex_count << endl;
    }
  }
  assert(vertex_count);

  cout << "Threads used: " << threads << endl;
  cout << "Core graph size: " << core_graph.size() << endl;
//   sort(core_graph.begin(), core_graph.end());
  // Load core graph
  ThreadPool *thread_pool = insert_with_thread_pool(&core_graph, threads, lock_search, vertex_count);
  // Do updates
  if (insert) {
    update_existing_graph(updates, thread_pool, threads, size);
  } else {
    thread_pool_deletions(thread_pool, updates, threads, size);
  }

  // DEBUGGING CODE
  // Check that all edges are there and in sorted order
     for (int i = 0; i < core_graph.size(); i++) {
       if (!thread_pool->pcsr->edge_exists(core_graph[i].first, core_graph[i].second)) {
         cout << "Not there " << core_graph[i].first << " " << core_graph[i].second << endl;
       }
     }
        for (int i = 0; i < size; i++) {
          if (!thread_pool->pcsr->edge_exists(updates[i].first, updates[i].second)) {
            cout << "Not there" << endl;
          }
        }
//     if (!thread_pool->pcsr->is_sorted()) {
//       cout << "Not sorted" << endl;
//     }
  delete thread_pool;

  return 0;
}
