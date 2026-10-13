#include "lab2.h"
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <cstdlib>

using namespace std;

int main(int argc, char *argv[])
{
#ifdef _WIN32
    system("chcp 65001");
#endif

    if (argc > 1 && string(argv[1]) == "gen")
    {
        generate_files();
        return 0;
    }

#ifdef NDEBUG
    cout << "Build: RELEASE" << endl;
#else
    cout << "Build: DEBUG (results are not representative!)" << endl;
#endif
    cout << "Hardware concurrency: " << thread::hardware_concurrency() << endl;

    vector<Data> datas;
    for (size_t n : SIZES)
        datas.push_back(read_file(file_name(n)));

    for (const auto &data : datas)
        library_experiments(data);

    for (const auto &data : datas)
        own_experiments(data);

    return 0;
}
