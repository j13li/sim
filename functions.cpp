#include "functions.h"
using namespace std;

// Calculate exponentially distributed next event time based on rate
double calc_exp(double rate) {
    // Generate a random double between 0 and 1, not including 0
    double u;
    do { u = (double) rand() / RAND_MAX; }
    while (u == 0);
    return -log(u) / rate;
}

// Calculate mean for array
double calc_mean(vector<double> v) {
    double sum = 0;
    for (int i = 0; i < v.size(); i++) {
        sum = sum + v[i];
    }
    return sum / (double) v.size();
}
