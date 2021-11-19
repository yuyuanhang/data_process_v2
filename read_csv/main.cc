#include "csv.hpp"
#include "csv_command.h"
#include "type.h"
#include <dirent.h>
#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <fstream>
#include <chrono>

#define NANOSECTOSEC(elapsed_time) ((elapsed_time)/(double)1000000000)

using namespace csv;

std::vector<std::string> getFileList(const char* file_path, std::string format) {
    std::vector<std::string> files;
    DIR* dir = opendir(file_path);
    if (dir != nullptr) {
        struct dirent* diread = readdir(dir);
        while (diread != nullptr) {
            std::string file_name(diread->d_name);
            if (file_name.find(format) != std::string::npos) {
                files.push_back(file_name);
            }
            diread = readdir(dir);
        }
        closedir(dir);
    } else {
        std::cout << "wrong directory!" << std::endl;
    }
    return files;
}

int main(int argc, char **argv)
{
    // parse parameters
    CSVCommand command(argc, argv);

    std::string input_csv_file_path = command.getCSVFilePath();
    std::string output_data_graph_file = command.getGraphFilePath();
    std::string output_label_file = command.getLabelFilePath();

    std::cout << "Command Line:" << std::endl;
    std::cout << "\tCSV Files: " << input_csv_file_path << std::endl;
    std::cout << "\tData Graph: " << output_data_graph_file << std::endl;
    std::cout << "\tLabel: " << output_label_file << std::endl;
    std::cout << "--------------------------------------------------------------------" << std::endl;

    // get and classify .csv files
    std::vector<std::string> files = getFileList(const_cast<char*>(input_csv_file_path.c_str()), ".csv");
    std::vector<std::string> vertices_files;
    std::vector<std::string> edges_files;
    
    for (auto const& file : files) {
        int char_num = std::count(file.begin(), file.end(), '_');
        if (char_num == 2) {
            vertices_files.push_back(file);
        } else {
            edges_files.push_back(file);
        }
    }

    std::cout << "Reading vertices..." << std::endl;

auto start = std::chrono::high_resolution_clock::now();

    std::vector<std::map<std::string, VertexID> > vertices_with_newid;
    std::vector<std::set<std::string> > vertices_with_oldid;
    std::vector<std::string> labels;
    std::vector<std::string> specific_labels;
    std::vector<int> specific_labels_index;

    for (auto const& file : vertices_files) {
        std::string csv_file_absolute_path = input_csv_file_path + file;
        CSVReader reader(csv_file_absolute_path);
        std::vector<std::string> col_names = reader.get_col_names();

        // record available columns and filter out these columns including attributes
        int id_col = -1;
        int type_col = -1;
        for (long unsigned i = 0; i < col_names.size(); i++) {
            if (col_names[i].find("id") != std::string::npos) {
                id_col = i;
            } else if (col_names[i].find("type") != std::string::npos) {
                type_col = i;
            }
        }
        
        std::vector<std::set<std::string> > vertices_in_this_file;
        std::vector<std::string> labels_in_this_file;
        std::string overall_label;

        if (type_col == -1) {
            std::string label = file.substr(0, file.find("_"));
            std::transform(label.begin(), label.end(), label.begin(), ::tolower);
            labels_in_this_file.push_back(label);

            std::set<std::string> cur_vertex_set;
            vertices_in_this_file.push_back(cur_vertex_set);
        } else {
            overall_label = file.substr(0, file.find("_"));
            std::transform(overall_label.begin(), overall_label.end(), overall_label.begin(), ::tolower);

            specific_labels.push_back(overall_label);
            specific_labels_index.push_back(labels.size());
        }

        CSVRow row;
        while (reader.read_row(row)) {
            std::string id = row[id_col].get();
            
            if (type_col == -1) {
                vertices_in_this_file[0].insert(id);
            } else {
                std::string label = row[type_col].get();
                std::transform(label.begin(), label.end(), label.begin(), ::tolower);
                label = overall_label + "_" + label;

                if (std::find(labels_in_this_file.begin(), labels_in_this_file.end(), label) != labels_in_this_file.end()) {
                    int label_index = std::find(labels_in_this_file.begin(), labels_in_this_file.end(), label) - labels_in_this_file.begin();
                    vertices_in_this_file[label_index].insert(id);
                } else {
                    labels_in_this_file.push_back(label);

                    std::set<std::string> cur_vertex_set;
                    cur_vertex_set.insert(id);
                    vertices_in_this_file.push_back(cur_vertex_set);
                }
            }
        }

        for (auto const& vertex_set : vertices_in_this_file) {
            vertices_with_oldid.push_back(vertex_set);
        }

        for (auto const& label : labels_in_this_file) {
            labels.push_back(label);
        }

        // print statistics during counting
        for (long unsigned i = 0; i < labels_in_this_file.size(); i++) {
            std::cout << "\t|" << labels_in_this_file[i] << "|: " << vertices_in_this_file[i].size() << std::endl;
        }
    }
    
    ui vertex_num = 0;
    ui label_num = labels.size();
    for (auto const& vertex_set : vertices_with_oldid) {
        std::map<std::string, VertexID> vertex_set_with_newid;
        for (auto const& vertex : vertex_set) {
            vertex_set_with_newid.insert(std::make_pair(vertex, vertex_num));
            vertex_num++;
        }
        vertices_with_newid.push_back(vertex_set_with_newid);
    }

    // count statistics
    ui size_vertex_num_offset = labels.size() + 1;
    ui* vertex_num_offset = new ui[labels.size() + 1];
    vertex_num_offset[0] = 0;
    for (long unsigned i = 0; i < labels.size(); i++) {
        vertex_num_offset[i + 1] = vertex_num_offset[i] + vertices_with_oldid[i].size();
    }
    std::cout << "|V|: " << vertex_num << " |\u03A3|: " << label_num << std::endl;

auto end = std::chrono::high_resolution_clock::now();
double load_vertices_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "--------------------------------------------------------------------" << std::endl;
    std::cout << "Reading edges..." << std::endl;

start = std::chrono::high_resolution_clock::now();

    std::vector<std::set<VertexID> > in_neighbors;
    std::vector<std::set<VertexID> > out_neighbors;
    in_neighbors.resize(vertex_num);
    out_neighbors.resize(vertex_num);
    // count statistics
    std::vector<std::vector<VertexID> > edge_num;
    VertexID sum_edge = 0;

    edge_num.resize(labels.size());
    for (long unsigned i = 0; i < edge_num.size(); i++) {
        edge_num[i].resize(labels.size());
        for (long unsigned j = 0; j < edge_num[i].size(); j++) {
            edge_num[i][j] = 0;
        }
    }

    for (auto const& file : edges_files) {
        std::string csv_file_absolute_path = input_csv_file_path + file;
        CSVReader reader(csv_file_absolute_path);
        std::vector<std::string> col_names = reader.get_col_names();

        // record available columns and filter out these columns including attributes
        int src_col = -1;
        int dest_col = -1;
        std::string src_label;
        std::string dest_label;
        int src_label_index = -1;
        int dest_label_index = -1;
        int src_label_spe_index = -1;
        int dest_label_spe_index = -1;

        for (long unsigned i = 0; i < col_names.size(); i++) {
            if (col_names[i].find(".id") != std::string::npos) {
                if (src_col == -1) {
                    src_col = i;
                    src_label = col_names[i].substr(0, col_names[i].find(".id"));
                    std::transform(src_label.begin(), src_label.end(), src_label.begin(), ::tolower);
                } else {
                    dest_col = i;
                    dest_label = col_names[i].substr(0, col_names[i].find(".id"));
                    std::transform(dest_label.begin(), dest_label.end(), dest_label.begin(), ::tolower);
                }
            }
        }

        if (std::find(specific_labels.begin(), specific_labels.end(), src_label) != specific_labels.end()) {
            src_label_index = -1;
            src_label_spe_index = std::find(specific_labels.begin(), specific_labels.end(), src_label) - specific_labels.begin();
            src_label_spe_index = specific_labels_index[src_label_spe_index];
        } else {
            src_label_index = std::find(labels.begin(), labels.end(), src_label) - labels.begin();
        }

        if (std::find(specific_labels.begin(), specific_labels.end(), dest_label) != specific_labels.end()) {
            dest_label_index = -1;
            dest_label_spe_index = std::find(specific_labels.begin(), specific_labels.end(), dest_label) - specific_labels.begin();
            dest_label_spe_index = specific_labels_index[dest_label_spe_index];
        } else {
            dest_label_index = std::find(labels.begin(), labels.end(), dest_label) - labels.begin();
        }

        CSVRow row;
        while (reader.read_row(row)) {
            std::string src_id = row[src_col].get();
            std::string dest_id = row[dest_col].get();

            VertexID src_newid;
            VertexID dest_newid;

            int cur_src_label_index = -1;
            int cur_dest_label_index = -1;

            if (src_label_index != -1) {
                src_newid = vertices_with_newid[src_label_index][src_id];
                cur_src_label_index = src_label_index;
            } else {
                int temp_counter = src_label_spe_index;
                while (labels[temp_counter].find(src_label) != std::string::npos) {
                    if (vertices_with_newid[temp_counter].find(src_id) != vertices_with_newid[temp_counter].end()) {
                        src_newid = vertices_with_newid[temp_counter][src_id];
                        cur_src_label_index = temp_counter;
                        break;
                    }
                    temp_counter++;
                }
            }

            if (dest_label_index != -1) {
                dest_newid = vertices_with_newid[dest_label_index][dest_id];
                cur_dest_label_index = dest_label_index;
            } else {
                int temp_counter = dest_label_spe_index;
                while (labels[temp_counter].find(dest_label) != std::string::npos) {
                    if (vertices_with_newid[temp_counter].find(dest_id) != vertices_with_newid[temp_counter].end()) {
                        dest_newid = vertices_with_newid[temp_counter][dest_id];
                        cur_dest_label_index = temp_counter;
                        break;
                    }
                    temp_counter++;
                }
            }

            if (out_neighbors[src_newid].insert(dest_newid).second) {
                in_neighbors[dest_newid].insert(src_newid);
                edge_num[cur_src_label_index][cur_dest_label_index]++;
            }
        }

        // print statistics during counting.
        if (src_label_index != -1) {
            if (dest_label_index != -1) {
                std::cout << "\t|" << src_label << "->" << dest_label << "|: " << edge_num[src_label_index][dest_label_index] << std::endl;
            } else {
                int temp_counter = dest_label_spe_index;
                while (labels[temp_counter].find(dest_label) != std::string::npos) {
                    if (edge_num[src_label_index][temp_counter] > 0) {
                        std::cout << "\t|" << src_label << "->" << labels[temp_counter] << "|: " << edge_num[src_label_index][temp_counter] << std::endl;
                    }
                    temp_counter++;
                }
            }
        } else {
            if (dest_label_index != -1) {
                int temp_counter = src_label_spe_index;
                while (labels[temp_counter].find(src_label) != std::string::npos) {
                    std::cout << "\t|" << labels[temp_counter] << "->" << dest_label << "|: " << edge_num[temp_counter][dest_label_index] << std::endl;
                    temp_counter++;
                }
            } else {
                int temp_counter1 = src_label_spe_index;

                while (labels[temp_counter1].find(src_label) != std::string::npos) {
                    int temp_counter2 = dest_label_spe_index;
                    while (labels[temp_counter2].find(dest_label) != std::string::npos) {
                        if (edge_num[temp_counter1][temp_counter2] > 0) {
                            std::cout << "\t|" << labels[temp_counter1] << "->" << labels[temp_counter2] << "|: " << edge_num[temp_counter1][temp_counter2] << std::endl;
                        }
                        temp_counter2++;
                    }
                    temp_counter1++;
                }
            }
        }
    }

    for (long unsigned i = 0; i < edge_num.size(); i++) {
        for (long unsigned j = 0; j < edge_num[i].size(); j++) {
            sum_edge +=  edge_num[i][j];
        }
    }

    std::cout << "|E|: " << sum_edge << std::endl;

end = std::chrono::high_resolution_clock::now();
double load_edges_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout << "--------------------------------------------------------------------" << std::endl;
    std::cout << "Storing graph and labels..." << std::endl;

start = std::chrono::high_resolution_clock::now();
    
    std::ofstream graph_descriptor(output_data_graph_file, std::ios::binary);

    graph_descriptor.write((char*)&vertex_num, sizeof(ui));
    graph_descriptor.write((char*)&size_vertex_num_offset, sizeof(ui));
    graph_descriptor.write((char*)vertex_num_offset, sizeof(ui) * size_vertex_num_offset);

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
    
    graph_descriptor.write((char*)in_degree, sizeof(ui) * vertex_num);
    graph_descriptor.write((char*)out_degree, sizeof(ui) * vertex_num);
    for (ui i = 0; i < vertex_num; i++) {
        graph_descriptor.write((char*)in_neighbors_array[i], sizeof(VertexID) * in_degree[i]);
    }
    for (ui i = 0; i < vertex_num; i++) {
        graph_descriptor.write((char*)out_neighbors_array[i], sizeof(VertexID) * out_degree[i]);
    }

    graph_descriptor.close();

    VertexID* label_offset = new VertexID[size_vertex_num_offset];
    label_offset[0] = 0;
    for (VertexID i = 0; i < label_num; i++) {
        label_offset[i + 1] = label_offset[i] + labels[i].length();
    }
    char* labels_array = new char[label_offset[label_num]];
    for (VertexID i = 0; i < label_num; i++) {
        std::copy(labels[i].begin(), labels[i].end(), labels_array + label_offset[i]);
    }

    std::ofstream label_descriptor(output_label_file, std::ios::binary);

    label_descriptor.write((char*)&label_num, sizeof(VertexID));
    label_descriptor.write((char*)label_offset, sizeof(VertexID) * size_vertex_num_offset);
    label_descriptor.write((char*)labels_array, sizeof(char) * label_offset[label_num]);

    label_descriptor.close();

end = std::chrono::high_resolution_clock::now();
double store_graph_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

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
    printf("Load vertices time (seconds): %.4lf\n", NANOSECTOSEC(load_vertices_time_in_ns));
    printf("Load edges time (seconds): %.4lf\n", NANOSECTOSEC(load_edges_time_in_ns));
    printf("Store graph and label file time (seconds): %.4lf\n", NANOSECTOSEC(store_graph_time_in_ns));
    std::cout << "End." << std::endl;

    return 0;
}