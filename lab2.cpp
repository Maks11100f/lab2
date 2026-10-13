#include "lab2.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <functional>

#include <execution>
#include <algorithm>
#include <numeric>

#include <chrono>
#include <format>
#include <cmath>

#include <thread>
#include <random>

using namespace std;

template <class F>
double timeit(F &&f, int repeat = 1)
{
    f();
    auto time1 = chrono::steady_clock::now();
    for (int i = 0; i < repeat; ++i)
        f();
    auto time2 = chrono::steady_clock::now();
    chrono::duration<double, milli> tdiff = time2 - time1;
    return tdiff.count() / repeat;
}

auto light = [](int x)
{ return double(x) * x; };
auto heavy = [](int x)
{ return sqrt(double(x)) * sin(x) + cos(x); };

string file_name(size_t n)
{
    return "data_" + to_string(n) + ".txt";
}

void generate_files()
{
    random_device r;
    seed_seq seeds{r(), r(), r(), r(), r(), r()};
    mt19937 e(seeds);
    uniform_int_distribution<int> d(0, 999);
    for (size_t n : SIZES)
    {
        ofstream f(file_name(n));
        for (size_t i = 0; i < n; ++i)
            f << d(e) << '\n';
        cout << "generated " << file_name(n) << endl;
    }
}

Data read_file(const string &name)
{
    Data data;
    ifstream f(name);
    int x;
    while (f >> x)
        data.push_back(x);
    return data;
}


void report(const string &what, double time, double result)
{
    cout << format("   {:<16}{:>12.4f} ms   result = {:.6e}", what, time, result) << endl;
}

template <class F>
void library_test(const Data &data, F f, const string &f_name)
{
    int repeat = int(max<size_t>(1, 10000000 / data.size()));
    double res = 0;
    cout << format("-- transform: {}, reduce: +  (repeat = {})", f_name, repeat) << endl;

    double t = 0;
    t = timeit([&]()
               { res = transform_reduce(data.begin(), data.end(), 0.0, plus<>(), f); }, repeat);
    report("no policy", t, res);
    t = timeit([&]()
               { res = transform_reduce(execution::seq, data.begin(), data.end(), 0.0, plus<>(), f); }, repeat);
    report("seq", t, res);
    t = timeit([&]()
               { res = transform_reduce(execution::par, data.begin(), data.end(), 0.0, plus<>(), f); }, repeat);
    report("par", t, res);
    t = timeit([&]()
               { res = transform_reduce(execution::unseq, data.begin(), data.end(), 0.0, plus<>(), f); }, repeat);
    report("unseq", t, res);
    t = timeit([&]()
               { res = transform_reduce(execution::par_unseq, data.begin(), data.end(), 0.0, plus<>(), f); }, repeat);
    report("par_unseq", t, res);
}

void library_experiments(const Data &data)
{
    cout << format("\n=== Library transform_reduce, n = {}", data.size()) << endl;
    library_test(data, light, "x*x (light)");
    library_test(data, heavy, "sqrt(x)*sin(x)+cos(x) (heavy)");
}

template <typename Iterator, typename T, typename UnaryOp>
T own_transform_reduce(Iterator first, Iterator last, T init, UnaryOp f, size_t K)
{
    size_t const length = size_t(distance(first, last));
    if (!length)
        return init;
    K = min(K, length);
    size_t const block_size = length / K;
    size_t const rest = length % K;
    vector<T> results(K, T{});
    vector<thread> threads(K);
    Iterator block_start = first;
    for (size_t i = 0; i < K; ++i)
    {
        Iterator block_end = block_start;
        advance(block_end, block_size + (i < rest ? 1 : 0));
        threads[i] = thread([block_start, block_end, &f, &res = results[i]]()
                            { res = transform_reduce(block_start, block_end, T{}, plus<>(), f); });
        block_start = block_end;
    }
    for (auto &entry : threads)
        entry.join();
    return reduce(results.begin(), results.end(), init);
}

void own_experiments(const Data &data)
{
    const int repeat = 5;
    const size_t hc = thread::hardware_concurrency();
    vector<size_t> Ks;
    for (size_t K = 1; K <= 2 * hc; ++K)
        Ks.push_back(K);
    for (size_t K = 4 * hc; K <= 64 * hc; K *= 2)
        Ks.push_back(K);

    cout << format("\n=== Own parallel transform_reduce, n = {}, repeat = {}", data.size(), repeat) << endl;
    cout << format("{:>6} | {:>16} | {:>16}", "K", "x*x, ms", "heavy, ms") << endl;
    cout << string(44, '-') << endl;

    size_t bestK_light = 0, bestK_heavy = 0;
    double best_light = 1e100, best_heavy = 1e100;
    double res1 = 0, res2 = 0;
    for (size_t K : Ks)
    {
        double t1 = timeit([&]()
                           { res1 = own_transform_reduce(data.begin(), data.end(), 0.0, light, K); }, repeat);
        double t2 = timeit([&]()
                           { res2 = own_transform_reduce(data.begin(), data.end(), 0.0, heavy, K); }, repeat);
        cout << format("{:>6} | {:>16.4f} | {:>16.4f}", K, t1, t2) << endl;
        if (t1 < best_light)
            best_light = t1, bestK_light = K;
        if (t2 < best_heavy)
            best_heavy = t2, bestK_heavy = K;
    }
    cout << format("(results for the last K: x*x = {:.6e}, heavy = {:.6e})", res1, res2) << endl;
    cout << format("Best K (x*x):   K = {}, time = {:.4f} ms, K / hardware_concurrency = {} / {} = {:.2f}",
                   bestK_light, best_light, bestK_light, hc, double(bestK_light) / hc)
         << endl;
    cout << format("Best K (heavy): K = {}, time = {:.4f} ms, K / hardware_concurrency = {} / {} = {:.2f}",
                   bestK_heavy, best_heavy, bestK_heavy, hc, double(bestK_heavy) / hc)
         << endl;
}
