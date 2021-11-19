#include "type.h"
#include "query_command.h"
#include <fstream>
#include <vector>
#include <set>
#include <cstring>
#include <chrono>

#define NANOSECTOSEC(elapsed_time) ((elapsed_time)/(double)1000000000)

void readLabelFile(std::string file, VertexID &label_num, VertexID* label_offset, char* labels_array, std::vector<std::string> &labels) {
	std::ifstream label_descriptor(file, std::ios::binary);

	if (!label_descriptor.is_open()) {
		std::cout << "wrong path! (label file)" << std::endl;
	}

	label_descriptor.read((char*)&label_num, sizeof(VertexID));

	VertexID size_vertex_num_offset = label_num + 1;
	label_offset = new VertexID[size_vertex_num_offset];
    label_descriptor.read((char*)label_offset, sizeof(VertexID) * size_vertex_num_offset);

    labels_array = new char[label_offset[label_num]];
    label_descriptor.read((char*)labels_array, sizeof(char) * label_offset[label_num]);
    std::string labels_string(labels_array);

    for (VertexID i = 0; i < label_num; i++) {
    	std::string label = labels_string.substr(label_offset[i], label_offset[i + 1] - label_offset[i]);

    	if (label.find("_") != std::string::npos) {
    		label = label.substr(label.find("_") + 1, label.length() - label.find("_") - 1);
    	}
    	labels.push_back(label);
    }
}


void readQueryFile(std::string file, ui &edge_num, std::vector<std::pair<std::string, std::string> > &edges, 
					std::set<std::string> &vertices) {
	std::ifstream query_descriptor(file);

	if (!query_descriptor.is_open()) {
		std::cout << "wrong path! (query file)" << std::endl;
	}

	query_descriptor >> edge_num;

	for (ui i = 0; i < edge_num; i++) {
		std::string src;
		std::string dest;

		query_descriptor >> src >> dest;

		edges.push_back(std::make_pair(src, dest));

		vertices.insert(src);
		vertices.insert(dest);
	}
}

int main(int argc, char **argv)
{
	QueryCommand command(argc, argv);

    std::string input_query_graph_file = command.getIQueryGraphFile();
    std::string input_label_file = command.getLabelFilePath();
    std::string output_query_graph_file = command.getOQueryGraphFile();

    std::cout << "Command Line:" << std::endl;
    std::cout << "\tInput Query Graph: " << input_query_graph_file << std::endl;
    std::cout << "\tLabel: " << input_label_file << std::endl;
    std::cout << "\tOutput Query Graph: " << output_query_graph_file << std::endl;
    std::cout << "--------------------------------------------------------------------" << std::endl;

    std::cout << "Reading label profile..." << std::endl;

auto start = std::chrono::high_resolution_clock::now();

    VertexID label_num;
    VertexID* label_offset = NULL;
    char* labels_array = NULL;
    std::vector<std::string> labels;
    readLabelFile(input_label_file, label_num, label_offset, labels_array, labels);
    for (ui i = 0; i < label_num; i++) {
    	std::cout << "\t" << labels[i] << std::endl;
    }
    std::cout << "|\u03A3|: " << label_num << std::endl;

auto end = std::chrono::high_resolution_clock::now();
double load_label_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "--------------------------------------------------------------------" << std::endl;
    std::cout << "Reading input query graph..." << std::endl;

start = std::chrono::high_resolution_clock::now();

    ui edge_num;
    std::vector<std::pair<std::string, std::string> > edges;
    std::set<std::string> vertices;
    readQueryFile(input_query_graph_file, edge_num, edges, vertices);

    ui vertex_num = vertices.size();
    ui size_vertex_num_offset = label_num + 1;
    ui* vertex_num_offset = new ui[size_vertex_num_offset];
    std::memset(vertex_num_offset, 0, size_vertex_num_offset * sizeof(ui));

    for (auto vertex : vertices) {
    	for (ui i = 0; i < label_num; i++) {
    		if (vertex.find(labels[i]) != std::string::npos){
    			vertex_num_offset[i + 1]++;
    			break;
    		}
    	}
    }

    for (ui i = 0; i < label_num; i++) {
    	vertex_num_offset[i + 1] = vertex_num_offset[i + 1] + vertex_num_offset[i];
    }

    std::map<std::string, VertexID> vertices_newid;
    for (auto vertex : vertices) {
    	VertexID newid = 0;
    	for (ui i = 0; i < label_num; i++) {
    		if (vertex.find(labels[i]) != std::string::npos){
    			newid = i;
    			break;
    		}
    	}
    	newid = vertex_num_offset[newid];

    	std::string offset_string = vertex.substr(vertex.find("_") + 1, vertex.length() - vertex.find("_") - 1);
    	ui offset_inlabel = std::stoul(offset_string) - 1;
    	newid += offset_inlabel;
    	vertices_newid.insert(std::make_pair(vertex, newid));
    }

    std::vector<std::set<VertexID> > in_neighbors;
    std::vector<std::set<VertexID> > out_neighbors;
    in_neighbors.resize(vertex_num);
    out_neighbors.resize(vertex_num);
    for (auto edge : edges) {
    	VertexID src_newid = vertices_newid[edge.first];
    	VertexID dest_newid = vertices_newid[edge.second];

    	out_neighbors[src_newid].insert(dest_newid);
        in_neighbors[dest_newid].insert(src_newid);
    }

    std::cout << "|V|: " << vertex_num << std::endl;
    std::cout << "|E|: " << edge_num << std::endl;

end = std::chrono::high_resolution_clock::now();
double load_query_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "--------------------------------------------------------------------" << std::endl;
    std::cout << "Storing graph..." << std::endl;

start = std::chrono::high_resolution_clock::now();

    std::ofstream query_descriptor(output_query_graph_file, std::ios::binary);

    query_descriptor.write((char*)&vertex_num, sizeof(ui));
    query_descriptor.write((char*)&size_vertex_num_offset, sizeof(ui));
    query_descriptor.write((char*)vertex_num_offset, sizeof(ui) * size_vertex_num_offset);

    ui* in_degree = new ui[vertex_num];
    ui* out_degree = new ui[vertex_num];
    ui sum_in_edge = 0;
    ui sum_out_edge = 0;
    for (ui i = 0; i < vertex_num; i++) {
        in_degree[i] = in_neighbors[i].size();
        out_degree[i] = out_neighbors[i].size();
        sum_in_edge += in_degree[i];
        sum_out_edge += out_degree[i];
    }
    std::cout << "|E-|: " << sum_in_edge << std::endl;
    std::cout << "|E+|: " << sum_out_edge << std::endl;

    VertexID** in_neighbors_array = new VertexID*[vertex_num];
    VertexID** out_neighbors_array = new VertexID*[vertex_num];
    for (ui i = 0; i < vertex_num; i++) {
        in_neighbors_array[i] = new VertexID[in_degree[i]];
        std::copy(in_neighbors[i].begin(), in_neighbors[i].end(), in_neighbors_array[i]);

        out_neighbors_array[i] = new VertexID[out_degree[i]];
        std::copy(out_neighbors[i].begin(), out_neighbors[i].end(), out_neighbors_array[i]);
    }
    
    query_descriptor.write((char*)in_degree, sizeof(ui) * vertex_num);
    query_descriptor.write((char*)out_degree, sizeof(ui) * vertex_num);
    for (ui i = 0; i < vertex_num; i++) {
        query_descriptor.write((char*)in_neighbors_array[i], sizeof(VertexID) * in_degree[i]);
    }
    for (ui i = 0; i < vertex_num; i++) {
        query_descriptor.write((char*)out_neighbors_array[i], sizeof(VertexID) * out_degree[i]);
    }

    query_descriptor.close();

end = std::chrono::high_resolution_clock::now();
double store_query_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
	
	std::cout << "--------------------------------------------------------------------" << std::endl;
    delete[] vertex_num_offset;
    delete[] in_degree;
    delete[] out_degree;
    for (VertexID i = 0; i < vertex_num; i++) {
        delete[] in_neighbors_array[i];
        delete[] out_neighbors_array[i];
    }
    delete[] label_offset;
    delete[] labels_array;

    // print time info
    printf("Load label profile time (seconds): %.4lf\n", NANOSECTOSEC(load_label_time_in_ns));
    printf("Load query graph time (seconds): %.4lf\n", NANOSECTOSEC(load_query_time_in_ns));
    printf("Store query graph time (seconds): %.4lf\n", NANOSECTOSEC(store_query_time_in_ns));
    std::cout << "End." << std::endl;

	return 0;
}