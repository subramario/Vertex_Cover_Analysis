#include <iostream>
#include <list>
#include <vector>
#include <regex>
#include <iterator>
#include <algorithm> 
#include <string>
#include <stack>
#include <list>
#include <memory>
#include "minisat/core/SolverTypes.h"
#include "minisat/core/Solver.h"
#include <pthread.h>
#include <fstream>
#include <iomanip>
#include <time.h>

using namespace std;

/****GLOBAL VARIABLES + STRUCT****/

// Used to store Vertex Cover solutions
string cnf_sat_res = "CNF-SAT-VC: timeout\n";
string approx_vc1 = "";
string approx_vc2 = "" ;

// Used to calculate approximation ratios
double cnf_size = 0;
double approx_1_size = 0;
double approx_2_size = 0;

// Used to calculate runtimes
double cnf_sat_runtime = 0.0;
double approx_vc1_runtime = 0.0;
double approx_vc2_runtime = 0.0;

// Used to store arguments for algorithm functions
struct Thread_Args
{
    unsigned int V;
    vector<unsigned int> *adj_list;
};

/**** HELPER FUNCTIONS ****/

// Prints the adjacency list
void printGraph(vector<unsigned int> adj_list[], unsigned int V) { 
    for (unsigned int i = 0; i < V; i++) { 
        cout << i << ": ";

        for (unsigned int j = 0; j < adj_list[i].size(); j++) {

           cout << adj_list[i][j] << " ";
        }

        cout << endl;
    } 
} 

// Adds an edge to the adjacency list
void add_edge(vector<unsigned int> adj_list[], unsigned int u, unsigned int v){ 
    adj_list[u].push_back(v); //add v into list u
    adj_list[v].push_back(u); //add u into list v
}

// Removes an edge from Approx_VC_1 adjacency list
void remove_edge_vc1(vector<unsigned int> adj_list[], unsigned int u, unsigned int v) {
   // Remove edge from u to v
   vector<unsigned int>::iterator it  = std::find (adj_list[u].begin(), adj_list[u].end(), v);
   adj_list[u].erase(it);

   // Remove edge from v to u
   it = std::find(adj_list[v].begin(), adj_list[v].end(), u);
   adj_list[v].erase(it);
}

// Removes a vertex along with all its edges from Approx_VC_2 adjacency list
std::vector<unsigned int>* remove_edge_vc2(unsigned vertex, std::vector<unsigned int> adj_list[]){

    // Removes all occurences of the vertex from other edge lists 
    for (auto j = adj_list[vertex].begin(); j != adj_list[vertex].end(); j++){

        adj_list[*j].erase(std::remove(adj_list[*j].begin(), adj_list[*j].end(), vertex), adj_list[*j].end());
    }

    // Removes all edges from the vertex edge list
    adj_list[vertex].clear();

    // Return the altered edge list
    return adj_list;
}



/**** VERTEX COVER ALGORITHMS ****/

// APPROX-VC-1 APPROACH
void* approx_vc_1(void* args){

    struct timespec ts;
    clockid_t cid;

    // Start timer
    pthread_getcpuclockid(pthread_self(), &cid);

    struct Thread_Args *args_list = (struct Thread_Args *)args;
    
    vector<unsigned int>* adj_list = args_list->adj_list;
    unsigned int V = args_list->V;

    vector<unsigned int> vc;
    unsigned int count = 0;
    bool done = false;
  
    /*
        APPROX-VC-1 PROCESS:
        Step 1) Find vertex with maximum number of incident edges
        Step 2) Add vertex to vertex cover
        Step 3) Delete all edges incident to that vertex (from both vertice entries!)
        Step 4) Repeat till no edges remain
    */

    // Repeats this process until no vertices remain in edge list 
    while(count != V){

        // Goes through adjacency list and evaluates how many lists are empty
        for (unsigned int x = 0; x < V; x++){
            if (adj_list[x].size() == 0){
                count += 1;
            }

            if (count == V)
                done = true;
        }

        // If adjacency lis tis empty, break out of step 4 loop
        if (done == true)
            break;

        unsigned int incident_edge_location = 0; // Denotes location of highest degree vertice in list
        unsigned int incident_edge_size = 0; // Denotes size of highest degree vertice in list

        // Traverses entire adjacency list to find vertex of highest degree 
        for (unsigned int i = 0; i < V; i++){
            if (adj_list[i].size() > incident_edge_size){

                incident_edge_location = i;
                incident_edge_size = adj_list[i].size();
            } 
        }
        // Step 1 Completed --> vertice with highest degree is found!  

        vc.push_back(incident_edge_location);
        // Step 2 Completed --> highest degree vertex has been added to vertex cover!


        // Deleting all edges incident to highest degree vertex
        int size = adj_list[incident_edge_location].size();

        for (int j = 0; j < size; j++){ // The size changes everytime you remove an element! Must make sure loop conditions are constant
            int value = adj_list[incident_edge_location][0]; // After deleting, elements get pushed back into "value" index --> no need to use "j" for this!

            remove_edge_vc1(adj_list,incident_edge_location,value);
        }
        // Step 3 completed --> all edges incident to highest degree vertex are deleted!   

        count = 0; // Resets count so loop does not break prematurely when adjacency list is evaluated again for presence of vertices 
    }
    //Step 4 completed --> adjacency list is now empty, vertex cover can now be outputted! 

    // Sorts vertex cover in ascending order
    sort(vc.begin(),vc.end());
    approx_1_size = vc.size();

    // Output vertex cover
    string result = "APPROX-VC-1: ";
    unsigned int i = 0;
    
    for(i = 0; i < vc.size()-1; i++)
        result = result + to_string(vc[i]) + ",";
           
    result = result + to_string(vc[i]) + "\n";

    approx_vc1 = result;

    vc.clear();

    // Stop timer
    clock_gettime(cid, &ts);

    // Saves the runtime in microseconds
    approx_vc1_runtime = (((double)ts.tv_nsec)*0.001);

    // Free memory allocated for arguments
    free(args);

    pthread_exit(NULL);
}


// APPROX-VC-2 APPROACH
void* approx_vc_2(void* args){
    
    // Stores vertex cover
    std::vector<unsigned int> vc;

    struct timespec ts;
    clockid_t cid;

    // Start timer
    pthread_getcpuclockid(pthread_self(), &cid);

    struct Thread_Args *args_list = (struct Thread_Args *)args;

    std::vector<unsigned int>* adj_list = args_list->adj_list;
    unsigned int V = args_list->V;

    /*
        APPROX-VC-2 PROCESS:
        Step 1) Pick an edge <i,j>
        Step 2) Add both i and j to the vertex cover
        Step 3) Remove all edges attahed to i and j
        Step 4) Repeat till no edges remain 
    */

    for (unsigned int i = 0; i < V; i++){

            for (auto j = adj_list[i].begin(); j != adj_list[i].end(); j++){
                vc.push_back(i);
                vc.push_back(*j);
                // Step 1 and 2 completed --> picks the first available edge in adjacency list and adds vertices to vertex cover
                
                adj_list = remove_edge_vc2(i,adj_list);
                adj_list = remove_edge_vc2(*j,adj_list);
                // Step 3 completed --> all edges attached to i and j are removed
                
                break;
            }
            // Step 4 completed --> entire adjacency list is now empty

    }

    // Sorts vertex cover in ascending order
    sort(vc.begin(),vc.end());
    approx_2_size = vc.size();

    // Prints vertex cover
    string result = "APPROX-VC-2: ";
    unsigned int i = 0;
    
    for(i = 0; i < vc.size()-1; i++)
        result = result + to_string(vc[i]) + ",";
           
    result = result + to_string(vc[i]) + "\n";
    approx_vc2 = result;

    vc.clear();

    // Stop timer
    clock_gettime(cid, &ts);

    // Saves runtime in microsecs
    approx_vc2_runtime = (((double)ts.tv_nsec)*0.001);

    // Free allocated memory for arguments
    free(args);

    pthread_exit(NULL);
}

// CNF-SAT-VC Approach
void* minisat_reduction(void* args){

    struct timespec ts;
    clockid_t cid;

    // Start timer
    pthread_getcpuclockid(pthread_self(), &cid);

    struct Thread_Args *args_list = (struct Thread_Args *)args;

    std::vector<unsigned int>* adj_list = args_list->adj_list;
    unsigned int V = args_list->V;

    std::unique_ptr<Minisat::Solver> solver(new Minisat::Solver());
    bool res = false; // Initializing solver to be false
    unsigned int k = 1; // Initialize minimum vertex cover as one

    std::vector<unsigned int> vc;

    while(k < V){

        // Initialize atomic proposition matrix sized Vxk
        Minisat::Lit matrix[V][k];

        // Populate each element in the atomic proposition matrix as a literal
        for (unsigned int x = 0; x < V; x++)
            for (unsigned int y = 0; y < k; y++){
                Minisat::Lit literal;
                literal = Minisat::mkLit(solver->newVar());
                matrix[x][y] = literal; 
            }
        
        // Clause 1: At least one vertex is theith vertex in the vertex cover
        for (unsigned int i = 0; i < k; i++){ //for all i which exists from k=1 to k 
            
            // MiniSAT only accepts 4 arguments max for each addClause! If multiple literals exist, use one vector of literals
            Minisat::vec<Minisat::Lit> clause_1;
            
            for (unsigned int j = 0; j < V; j++){
                clause_1.push(matrix[j][i]);
            }

            solver->addClause(clause_1);
        }
            
        // Clause 2: No one vertex can appear twice in a vertex cover
        for (unsigned int m = 0; m < V; m++){
            for (unsigned int p = 0; p < k; p++){
                for (unsigned int q = 0; q < k; q++){
                    if (p<q){
                        solver->addClause(~matrix[m][p],~matrix[m][q]);
                    }
                }
            }
        }

        
        // Clause 3: No more than one vertex appears in themth position of the vertex cover
        for (unsigned int m = 0; m < k; m++){
            for (unsigned int p = 0; p < V; p++){
                for (unsigned int q = 0; q < V; q++){
                    if (p<q){
                        solver->addClause(~matrix[p][m],~matrix[q][m]);
                    }
                }
            }
        }
    
        // Clause 4: Every edge is incident to at least one vertex in the vertex cover
        for (unsigned int i = 0; i < V; i++){
            for (auto m: adj_list[i]){
                
                if (m < i)
                    continue;

                Minisat::vec<Minisat::Lit> clause_2;
                                
                for (unsigned int x = 0; x < k; x++){
                    clause_2.push(matrix[i][x]);
                    clause_2.push(matrix[m][x]);
                }

                solver->addClause(clause_2);
            }
        }

        // Use SAT solver for the current minimum vertex cover value, k
        res = solver->solve();

        if (res == true){
        

            for (unsigned int x = 0; x < V; x++){
                
                for (unsigned int y = 0; y < k; y++){
                   
                    bool lit_val = Minisat::toInt(solver->modelValue(matrix[x][y]));
                    
                    if(lit_val == 0){
                        vc.push_back(x);
                    }
                }
            }

            // Sorts vertex cover in ascending order
            std::sort(vc.begin(),vc.end());
            cnf_size = vc.size();

            // Print vertex cover
            string result = "CNF-SAT-VC: ";
            unsigned int i = 0;
            
            for(i = 0; i < vc.size()-1; i++)
                result = result + to_string(vc[i]) + ",";
                   
            result = result + to_string(vc[i]) + "\n";

            cnf_sat_res = result;

            vc.clear();

            // Stop timer
            clock_gettime(cid, &ts);


            // Saves the runtime in microsecs
            cnf_sat_runtime = (((double)ts.tv_nsec)*0.001);

            break;
        }

        else{
            solver.reset(new Minisat::Solver());
            k++;
        }
    }

    // Free memory allocated for arguments
    free(args);

    pthread_exit(NULL);

}

/**** THREADING ****/
void threaded_vc(unsigned int num_vert, std::vector<unsigned int> adj_list_1[], vector<unsigned int> adj_list_2[], vector<unsigned int> adj_list_3[]){

    // Creating structs to store all arguments for algorithm functions
    struct Thread_Args *args_cnf_sat = (struct Thread_Args*) malloc(sizeof(*args_cnf_sat));
    struct Thread_Args *args_vc1 = (struct Thread_Args*) malloc(sizeof(*args_vc1));
    struct Thread_Args *args_vc2 = (struct Thread_Args*) malloc(sizeof(*args_vc2));



    // Setting values of the struct attributes with arguments
    if(args_cnf_sat!=NULL){
        args_cnf_sat->adj_list = adj_list_1;
        args_cnf_sat->V = num_vert;
    }


    if(args_vc1!=NULL){
        args_vc1->adj_list = adj_list_2;
        args_vc1->V = num_vert;
    }

    if(args_vc2!=NULL){
        args_vc2->adj_list = adj_list_3;
        args_vc2->V = num_vert;
    }


    // Creating threads to run each algorithm
    pthread_t thread1;
    pthread_t thread2;
    pthread_t thread3;

    // Concurrently run each vertex cover algorithm through multi-threading
    pthread_create(&thread1, NULL, minisat_reduction, args_cnf_sat);
    pthread_create(&thread2, NULL, approx_vc_1, args_vc1);
    pthread_create(&thread3, NULL, approx_vc_2, args_vc2);

    // Timeout function for CNF-SAT-VC approach (Check "Formal Report" pdf on Github repository for more information)
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 120;

    // Awaiting thread termination
    pthread_join(thread2, NULL);
    pthread_join(thread3, NULL);
    pthread_timedjoin_np(thread1, NULL, &ts);

    // Display results
    std::cout << "Vertex Cover for " << cnf_sat_res << std::flush;
    std::cout << "Vertex Cover for " << approx_vc1 << std::flush;
    std::cout << "Vertex Cover for " << approx_vc2 << std::flush;
    cout << endl;

    cout << "CNF-SAT-VC Runtime: " << cnf_sat_runtime << " microseconds" << endl;
    cout << "APPROX-VC-1 Runtime: " << approx_vc1_runtime << " microseconds" << endl;
    cout << "APPROX-VC-2 Runtime: " << approx_vc2_runtime << " microseconds" << endl;
    cout << endl;

    std::cout << "Approximation ratio for APPROX-VC-1:  " << fixed << setprecision(2) << approx_1_size/cnf_size << std::endl;
    std::cout << "Approximation ratio for APPROX-VC-2:  " << fixed << setprecision(2) << approx_2_size/cnf_size << std::endl;

    return;

}


int main(int argc, char* argv[]){

    //local variables declared here
    smatch m;
    string input;
    unsigned int a,b,c,V = 1;
    vector <unsigned int> *adj_list_1 = NULL; // Initializing adjacency list for CNF-SAT-VC Approach
    vector <unsigned int> *adj_list_2 = NULL; // Initializing adjacency list for APPROX-VC-1 Approach
    vector <unsigned int> *adj_list_3 = NULL; // Initializing adjacency list for APPROX-VC-2 Approach

    // Matches command input to appropriate loop
    regex vert_rx("^[V]"); 
    regex edge_rx("^[E]");

    // Extract relevant info from input
    const regex numbers("([0-9]+)+");
    const regex coordinates("([0-9]+[,][0-9]+)+");

    while (!cin.eof()){ //eof allows program to exit gracefully 
        
        bool duplication = false;
        bool inverse_duplication = false;
        bool self_edge = false;
        bool exceeded_graph_size = false;

        getline(cin, input);

        if (cin.eof())
            break;

        // Executes this block of code if the number of vertices is specifed through the "V" command
        if (regex_search(input,m,vert_rx)){
            regex_search(input, m, numbers);

            string temp = m[0]; // Stores the match object in string "temp"
            a = stoi(temp); // Converts the string to an integer

            // Error case --> user specifies a graph with zero vertices
            if (a == 0){ 
                cout << "Error: Graph cannot have zero vertices!" << endl;
                continue;
            }

            else
                V = a;
        }

        getline(cin, input);

        // Executes this block of code if the edge list is specifed through the "E" command
        if (regex_search(input,m,edge_rx)){
            adj_list_1 = new vector<unsigned int>[V];
            adj_list_2 = new vector<unsigned int>[V];
            adj_list_3 = new vector<unsigned int>[V];

            // Creates a match object, "m", with all edges parsed from user input using regex "coordinates"
            while (regex_search(input,m,coordinates)){
                string s = m.str(0); // Parses one edge in the match object at a time
                
                // Extracts vertices from the edge coordinate
                string delimiter_1 = ",";
                string delimiter_2 = ">";
                string token_1 = s.substr(0, s.find(delimiter_1));
                string token_2 = s.substr(s.find(",")+1, s.find(delimiter_2));
                b = stoi(token_1);
                c = stoi(token_2);
                
                // Error case --> edge specification includes the same vertice twice
                if (b == c){ 
                    cout << "Error: Cannot have an edge between a vertice and itself!" << endl;
                    self_edge = true;
                    break;
                }

                if (self_edge == true)
                    break;

                // Error case --> edge specification includes a vertex higher than the total number of vertices in the graph
                if (b >= V || c >= V){
                    cout << "Error: Cannot have an edge between non-existant vertices." << endl;
                    exceeded_graph_size = true;
                    break;
                }

                if (exceeded_graph_size == true)
                    break;

                // Error case --> duplicate edges
                for (unsigned int i = 0; i < adj_list_1[b].size(); i++){ 
                    if (adj_list_1[b][i] == c){
                        cout << "Error: System does not allow duplicate edges." << endl;
                        duplication = true;
                        break;
                    }
                }

                if (duplication == true)
                    break;

                // Error case --> inverse duplicate edges
                for (unsigned int i = 0; i < adj_list_1[c].size(); i++){ 
                    if (adj_list_1[c][i] == b){
                        cout << "Error: System does not allow duplicate edges." << endl;
                        inverse_duplication = true;
                        break;
                    }
                }

                if (inverse_duplication == true)
                    break;
                
                if (duplication == false && inverse_duplication == false){ //If the edge spec is valid, add it to the adjacency list
                    add_edge(adj_list_1, b ,c);
                    add_edge(adj_list_2, b, c);
                    add_edge(adj_list_3, b, c);

                }

                input = m.suffix(); //Used to iterate through remaining edges
            }

        }

        // Output case for graph sized 0 vertices
        if (adj_list_1->size() == 0){
                std::cout << "CNF-SAT-VC: " << std::endl;
                std::cout << "APPROX-VC-1: " << std::endl;
                std::cout << "APPROX-VC-2: " << std::endl;
            }

        else{
            cnf_sat_res = "CNF-SAT-VC: timeout\n";
            approx_vc1 = "";
            string approx_vc2 = "" ;

            cnf_sat_runtime = 0.0;
            approx_vc1_runtime = 0.0;
            approx_vc2_runtime = 0.0;

            cnf_size = 0;
            approx_1_size = 0;
            approx_2_size = 0;

            // Inititate multi-threading
            threaded_vc(V,adj_list_1,adj_list_2,adj_list_3);
            
        }

        
    }

   return 0;
} 
