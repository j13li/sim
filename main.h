#pragma once
#include "options.h"
#ifndef __GNUC__
#include "getopt.h"
#else
#include <getopt.h>
#endif
#include <algorithm>
#include <queue>
#include <vector>
#include <stdio.h>
#include <cstdlib>
#include <time.h>
#include <string>
#include <list>
#include <iostream>
#include <fstream>
#include <map>
#include <cmath>
#include <sstream>  //for std::istringstream
#include <iterator> //for std::istream_iterator
#include <vector>   //for std::vector

using namespace std;

enum EVENT_TYPE {
    ARRIVAL,
    START_SERVICE,
    DEPARTURE
};

enum STATUS { 
    IDLE = 0,
    BUSY = 1
};

class sim_job {
public:
    double arrival_t;
    sim_job(double t) : arrival_t(t) {}

};

class server {
public:
    int n;
    STATUS status;
    queue<sim_job> job_queue;
    int completed;
    double serv_rate;
    vector<double> resp_times;
    double idle_t;
    double serv_t;
    double last_service_start;
    double last_service_stop;

    server(int n, double s) : n(n), serv_rate(s) { 
        status = IDLE;
        completed = 0;
        idle_t = 0;
        serv_t = 0;
        last_service_start = 0;
        last_service_stop = 0;
    }
    
    ~server() {
        resp_times.clear();
        while(!job_queue.empty()) {
            job_queue.pop();
        }
    }
};

class sim_event {
public:
    double event_time;
    double wait_t;
    double serv_t;
    EVENT_TYPE event_type;
    server * s;

    bool operator < (sim_event other) const {
        return event_time < other.event_time;
    }

    // Constructors
    sim_event() {}
    sim_event(EVENT_TYPE p, double t) : event_type(p), event_time(t) {}
};

class Compare {
public:
    bool operator() (const sim_event lhs, const sim_event rhs) {
        return lhs.event_time > rhs.event_time;
    }
};

void init();
void initsim();
void arrival_routine();
void start_service();
void start_service_routine(server * s);
void departure_routine(sim_event * e);

/* System parameters */
// Total requests to make per run
int total_requests;
// Request rate range
double low_rate, high_rate, rate_step;
// Scheduling policy
string scheduler;
// Parameter for some policies
double param;
// Threshold
double thresh_low, thresh_high;
double timeout;
// Output file
char * outfile_path;
ofstream outfile;
string param_fname;
/* ---------------------*/

/* Statistical counters */
int total_complete;
vector<double> all_resp;
/* ---------------------*/

// Servers
server * s1;
server * s2;
// Global queues
priority_queue<sim_event, vector<sim_event>, Compare> event_set;
queue<sim_job> common_job_queue;
// Main sim clock
double sim_clock;
// Current status
// Keep track of the server the previous job was scheduled for
server * last_server;
double curr_rate;
bool warmup;
double warmup_time;