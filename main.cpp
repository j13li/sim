#include "main.h"
#include "functions.h"
using namespace std;

/* 
Usage: sim <options>
    Options:
        -l      lowest request rate
        -h      highest request rate
        -t      request rate step
        -r      number of requests to make per run, default TOTAL_REQUESTS
        -s      scheduling policy, valid options are 
            "FCFS"  first come first serve
            "TH"    threshold method
            "S1"    only use server 1
            "S2"    only use server 2
            "QL"	assign jobs depending on each server's queue length
			"RR"	round robin
        -p      parameter for scheduling policies
            QL		queue length difference between server 1 and 2
            TH      queue length must be longer than param to start serving jobs to server 2
        -o      timeout value, number of seconds a job will wait in the queue
                before being discarded with a response time of 0
        -f      output file
*/
int main(int argc, char *argv[]) {
    // Initialization
    init();

    // Parse options
    extern char *optarg;
    extern int optind, optopt;
    int option_char;

    // Invokes member function `int operator ()(void);'
    while ((option_char = getopt (argc, argv, "l:h:t:r:s:p:o:f:x:y:")) != -1) {
        switch (option_char) {
            case 'l': low_rate = atof(optarg); break;
            case 'h': high_rate = atof(optarg); break;
            case 't': rate_step = atof (optarg); break;            
            case 'r': total_requests = atoi (optarg); break;
            case 's': scheduler = optarg; break;
            case 'p': param = atof(optarg); break;
            case 'o': timeout = atof(optarg); break;
            case 'f': outfile_path = optarg; break;
            case 'x': thresh_low = atof (optarg); break;
            case 'y': thresh_high = atof (optarg); break;
            case '?': fprintf (stderr, "usage: %s <options>\n", argv[0]);
        }
    }
 
    // Print header
    ostringstream os;
    os << "rate\tavg_resp1\tavg_resp2\tavg_resp\tutil_s1\tutil_s2\tavg_power_s1\tavg_power_s2\tavg_power\tthresh" << endl;
    cout << os.str();
    if(outfile_path != "") {
        outfile.open (outfile_path, ios::trunc);
        if(outfile.is_open()) { 
            outfile << os.str();
            outfile.close();
        }
    }

    for(curr_rate = low_rate; curr_rate <= (high_rate + 0.05); curr_rate += rate_step) {
        double i;
        for(i = thresh_low; i <= thresh_high + 0.05; i++) {
            if (thresh_low != thresh_high) { param = i; }
            initsim();
            // Schedule first arrival
            event_set.push(sim_event(ARRIVAL, sim_clock + calc_exp(curr_rate)));
            // Main loop
            while(total_complete < total_requests) {
                // Remove the next scheduled event from the event set
                sim_event e = event_set.top();
                event_set.pop();
                sim_clock = e.event_time;

                // Execute the event routine depending on type
                switch(e.event_type) {
                case ARRIVAL:
                    arrival_routine();
                    break;
                case START_SERVICE:
                    start_service_routine(e.s);
                    break;
                case DEPARTURE:
                    departure_routine(&e);
                    break;
                default:
                    break;
                }
            }
            // Calculate stats
            double s1_avg_resp = calc_mean(s1->resp_times);
            double s2_avg_resp = calc_mean(s2->resp_times);
            double avg_resp = calc_mean(all_resp);

            double s1_util = s1->serv_t / (sim_clock - warmup_time);
            double s2_util = s2->serv_t / (sim_clock - warmup_time);
            double s1_idle = (sim_clock - warmup_time) - s1->serv_t;
            double s2_idle = (sim_clock - warmup_time) - s2->serv_t;
 
            if(scheduler == "S1") { s2_idle = 0; }
            if(scheduler == "S2") { s1_idle = 0; }

            double s1_idle_power = s1_idle * S1_IDLE_POWER / (sim_clock - warmup_time);
            double s1_serv_power = s1_util * S1_MAX_POWER;
            double s2_idle_power = s2_idle * S2_IDLE_POWER / (sim_clock - warmup_time);
            double s2_serv_power = s2_util * S2_MAX_POWER;
            double s1_power = s1_idle_power + s1_serv_power;
            double s2_power = s2_idle_power + s2_serv_power;

            // Print data
            ostringstream os;
            os << curr_rate << "\t" << s1_avg_resp << "\t" << s2_avg_resp << "\t" << avg_resp << "\t" << s1_util << "\t" << s2_util << "\t" << s1_power << "\t" << s2_power << "\t" << s1_power + s2_power << "\t" << (int)param << endl;
            cout << os.str();
            if(outfile_path != "") {
                outfile.open (outfile_path, ios::app);
                if(outfile.is_open()) { 
                    outfile << os.str();
                    outfile.close();
                }
            }
        }
    }
    return 0;
}

void init() {
    // Seed random number generator
    srand(time(NULL));
    // Set parameters
    total_requests = TOTAL_REQUESTS;

    // Default options
    low_rate = high_rate = rate_step = 1;
    thresh_low = thresh_high = 0;
    scheduler = "FCFS";
    param = -1;
    timeout = 0;
    outfile_path = "";
}

// Initialize a simulation run
void initsim() {
    sim_clock = 0;
    warmup = false;
    
    // Recreate servers
    delete s1;
    delete s2;
    s1 = new server(1, S1_SERVICE_RATE);
    s2 = new server(2, S2_SERVICE_RATE);

    // Empty the event set
    while(!event_set.empty()) {
        event_set.pop();
    }
    all_resp.clear();
    total_complete = 0;
	// Set the last job to server 2 by default
	last_server = s2;

    while(!common_job_queue.empty()) {
        common_job_queue.pop();
    }
}

// Routine for processing arrival events
void arrival_routine() {
#ifdef DEBUG
    printf("Arrival event\tTime %f\n", sim_clock);
#endif
    // Schedule the next arrival event
    event_set.push(sim_event(ARRIVAL, sim_clock + calc_exp(curr_rate)));
    // Add new job to queue
    server * srv;
    if (scheduler == "FCFS" || scheduler == "TH") { srv = NULL; }	
    else if(scheduler == "QL") {
		// Queue job for server 2 if server 1's queue length is longer by p
		if(s1->job_queue.size() > s2->job_queue.size() + (int)param ) {	srv = s2; }
		else { srv = s1; }
	}
	// Round robin
	else if (scheduler == "RR") {
		if(last_server == s2) { last_server = s1; }
        else { last_server = s2; }
		srv = last_server;
	}
    else if (scheduler == "S1") { srv = s1; }
    else if (scheduler == "S2") { srv = s2; }
    // Scheduler not defined??
    else { exit(2); }
    if(srv == NULL) {
        common_job_queue.push(sim_job(sim_clock));
        start_service();
    }
    else {
        srv->job_queue.push(sim_job(sim_clock));
        start_service_routine(srv);
    }
}

// Check for queued jobs and start them
void start_service() {
    // If threshold is negative, check s2 first
    if(( scheduler == "TH" || scheduler == "QL") && param < 0 ) {
        // If system is idle, start service
        if(s2->status == IDLE) {
            start_service_routine(s2);
        }
        if(s1->status == IDLE) {
            start_service_routine(s1);
        }
    }
    else {
        // If system is idle, start service
        if(s1->status == IDLE) {
            start_service_routine(s1);
        }
        if(s2->status == IDLE) {
            start_service_routine(s2);
        }
    }
}

// Routine for starting service on a job
void start_service_routine(server * s) {
#ifdef DEBUG
    printf("Service event\tServer %d\tTime %f\n", s->n, sim_clock);
#endif
    queue<sim_job> * q;
    if(scheduler == "FCFS" || scheduler == "TH") {
        q = &common_job_queue;
    }
    else {
        q = &s->job_queue;
    }
    if(q->empty() || s->status == BUSY) { return; }
    server * s_to_check;
    if(scheduler == "TH" && param < 0) {
        s_to_check = s1;
    } else {
        s_to_check = s2;
    }
    // For threshold method, process the job at the nonpreferred server only if queue length exceeds m
    if(s == s_to_check && scheduler == "TH" && (int)q->size() <= abs((int)param)) {
         // Check again in less-prefered server's service time
        sim_event dummyEvent = sim_event(START_SERVICE, sim_clock + calc_exp(s->serv_rate));
        dummyEvent.s = s;
        event_set.push(dummyEvent);
        return;
    }
    
    // Remove the job from the front of the queue
    sim_job j = q->front();
    q->pop();

    double wait_time = sim_clock - j.arrival_t;
#ifdef DEBUG
    printf("Wait Time %f\n", wait_time);
#endif
    // If a timeout is specified, drop the job once the connection times out
    if(timeout > 0 && wait_time > timeout) {
        total_complete++;
        start_service();
        return;
    }

    // Set the server to busy and calculate the service time
    s->status = BUSY;
    s->last_service_start = sim_clock;
    s->idle_t += sim_clock - s->last_service_stop;
    double serv_t = calc_exp(s->serv_rate);
#ifdef DEBUG
    printf("Service Time %f\n", serv_t);
#endif
    // Schedule departure event and store the wait and service time for this job
    sim_event d = sim_event(DEPARTURE, sim_clock + serv_t);
    d.s = s;
    d.wait_t = wait_time;
    d.serv_t = serv_t;
    event_set.push(d);
}

// Routine for processing departure events
void departure_routine(sim_event * e) {
#ifdef DEBUG
    printf("Departure event\tServer %d\tTime %f\n", e->s, sim_clock);
#endif
    if ( !warmup && total_complete > total_requests * WARMUP_FACTOR ) {
        warmup = true;
        warmup_time = sim_clock;
    }
    // Record stats if system is stable
    if (warmup) {
        // Increment the completed count for the job class
        // Add the wait time and service time to the totals for the job class
        double resp_time = e->wait_t + e->serv_t;
        e->s->completed++;
        e->s->resp_times.push_back(resp_time);
        e->s->serv_t += e->serv_t;
        all_resp.push_back(resp_time);
    }
    e->s->status = IDLE;
    e->s->last_service_stop = sim_clock;
    total_complete++;
    start_service();
}
