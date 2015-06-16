#pragma once

// Default number of requests to make per run
#define TOTAL_REQUESTS 10000000
#define TOTAL_RUN_TIME 100000
#define WARMUP_FACTOR 0.3

// Slow Server
#define S1_SERVICE_RATE 10
// Fast Server
#define S2_SERVICE_RATE 100

// Power parameters
#define S1_IDLE_POWER 0
#define S1_MAX_POWER 5
#define S2_IDLE_POWER 0
#define S2_MAX_POWER 100

// Timeout parameters
#define S1_TIMEOUT 0
#define S2_TIMEOUT 0