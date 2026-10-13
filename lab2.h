#ifndef LAB2_H
#define LAB2_H
#include <vector>
#include <string>

using Data = std::vector<int>;

const size_t SIZES[] = {1000, 10000, 100000, 1000000, 5000000};

std::string file_name(size_t n);
void generate_files();
Data read_file(const std::string &name);

void library_experiments(const Data &data); 
void own_experiments(const Data &data);    

#endif
