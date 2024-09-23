#include "std_lib_facilities.h"
#include <queue>
#include <unordered_set>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

/*
 * Class Graph
 * Represents an undirected graph using an adjacency list.
 */
class Graph {
public:
    /*
     * Graph constructor
     * Initializes an empty graph.
     */
    Graph(){}

    /*
     * setGraphPath
     * Reads a graph from a file and builds an adjacency list.
     * 
     * Parameters:
     *  - path: A string representing the path to the file containing the graph data.
     * The file should contain pairs of integers representing edges between vertices.
     */
    void setGraphPath(string path){
        ifstream ifs(path);
        if (!ifs) {
            cout << path << "\n";
            cerr << "couldn't open" << endl;
            return;
        }
        int u, v;
        while (ifs >> u >> v) {
            vertex[u].push_back(v);
            vertex[v].push_back(u);
        }
    }

    /*
     * neighbors
     * Returns the neighbors of a given vertex.
     * 
     * Parameters:
     *  - u: An integer representing the vertex whose neighbors are to be returned.
     * 
     * Returns:
     *  - A vector of integers representing the neighbors of vertex u.
     */
    vector<int> neighbors(int u) {
        vector<int> v;
        if (vertex.find(u) != vertex.end())
            v = vertex[u];
        return v;
    }

    /*
     * bfs (Breadth-First Search)
     * Performs BFS to find the shortest path between two vertices.
     * 
     * Parameters:
     *  - source: The starting vertex.
     *  - dest: The destination vertex.
     * 
     * Returns:
     *  - A string representing the shortest path from source to dest, or 
     *    "No path between the vertex." if no path exists.
     */
    string bfs(int source, int dest) {
        queue<int> q;
        q.push(source);
        map<int, bool> visited_vertex;
        map<int, int> vertex_parents;
        visited_vertex[source] = true;
        while (!q.empty()) {
            int current = q.front();
            q.pop();
            if (current == dest) {
                string path = to_string(dest);
                int v = dest;
                while (vertex_parents.find(v) != vertex_parents.end()) {
                    v = vertex_parents[v];
                    path = to_string(v) + " " + path;
                }
                return path;
            }
            for (int neighbor : neighbors(current)) {
                if (!visited_vertex[neighbor]) {
                    q.push(neighbor);
                    visited_vertex[neighbor] = true;
                    vertex_parents[neighbor] = current;
                }
            }
        }
        return "No path between the vertex.";
    }

private:
    map<int, vector<int>> vertex; // Adjacency list representation of the graph
};

Graph g;
map<string, string> last_requests;
queue<string> order_of_last_request;

/*
 * shortest_distance (Thread Function)
 * Handles a client request to find the shortest path between two vertices.
 * The function checks if the request has been cached. If not, it calculates the path using BFS.
 * 
 * Parameters:
 *  - params: A pointer to the client socket file descriptor.
 * 
 * Returns:
 *  - NULL (void*): Thread exit with no return value.
 */
void* shortest_distance(void* params){
    int fd = *(int*)params;
    char bytes_read[256];
    memset(bytes_read, 0, sizeof(bytes_read));
    
    int n = read(fd, bytes_read, sizeof(bytes_read));
    if(n <= 0){
        close(fd);
        return NULL;
    }

    string line(bytes_read);
    size_t pos = line.find(' ');
    int v1 = stoi(line.substr(0, pos));
    int v2 = stoi(line.substr(pos + 1));

    // Check if the result is cached
    if(last_requests.find(line) != last_requests.end()){
        string result = last_requests[line];
        write(fd, result.c_str(), result.length());
        close(fd);
        return NULL;
    }

    // Perform BFS to calculate the shortest path
    string result = g.bfs(v1, v2);
    write(fd, result.c_str(), result.length());

    // Cache the result with synchronization
    pthread_mutex_lock(&m);
    if(last_requests.size() >= 10){
        string oldest_request = order_of_last_request.front();
        order_of_last_request.pop();
        last_requests.erase(oldest_request); 
    } 
    last_requests[line] = result;
    order_of_last_request.push(line);
    pthread_mutex_unlock(&m);
    
    close(fd);
    return NULL;
}

/*
 * main
 * Entry point of the program. Sets up the graph from a file and starts the server to handle client requests.
 * 
 * Parameters:
 *  - argc: The number of command-line arguments.
 *  - argv: Array of command-line arguments (path to the graph file and port number).
 * 
 * Returns:
 *  - int: Returns 0 upon successful execution.
 */
int main(int argc, char** argv){
    string path = argv[1];   // File path for the graph
    int portNum = stoi(argv[2]);  // Port number for the server
    
    g.setGraphPath(path);  // Initialize the graph with the file

    // Create a server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(portNum);
    bind(server_socket, (sockaddr*)&addr, sizeof(addr));
    listen(server_socket, 5);

    // Accept connections and handle them in new threads
    while(true){
        int fd2 = accept(server_socket, NULL, NULL);
        pthread_t thread;
        pthread_create(&thread, NULL, shortest_distance, &fd2);
    }

    return 0;
}
