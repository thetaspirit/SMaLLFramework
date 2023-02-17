// A set of functions to enable testing
// We have timing, initialization, allocation, logging and equivalence checking

#include <stdint.h>
#include <sys/time.h>
#include <string>
#include <vector>
#include <iterator>
#include <fstream>
#include <algorithm> // std::min_element
#include <iterator>
#include <array>
#include <iostream>
// #include<functional>
#include <numeric>

//****************************************************************************
//Timing
//****************************************************************************

// Change this to log min of all runs etc
#define TIME_ZERO 0

//TODO: Change rdtsc(void) when moving to different architectures
#if uarch == ZEN2
static __inline__ unsigned long long rdtsc(void)
{
    unsigned hi, lo;
    __asm__ __volatile__("rdtsc"
                         : "=a"(lo), "=d"(hi));
    return ((unsigned long long)lo) | (((unsigned long long)hi) << 32);
}
#elif uarch==ARM || uarch==REF
#include <time.h>
static __inline__ unsigned long long rdtsc(void)
{
    struct timespec gettime_now;
    clock_gettime(CLOCK_REALTIME, &gettime_now);
    return  gettime_now.tv_nsec ;
}
#endif

/// @todo unnecessary?
#define ACCUM_time(a, b) \
    {                    \
        a += b;          \
    }

/// @todo unnecessary? Fix duplication with ACCUM_time
#define REDUCE(a, b) \
  {                  \
    a += b;          \
  }

/// @todo change to use std::min<T> directly
#define MIN(a, b)            \
    {                        \
        a = (b < a) ? b : a; \
    }

#define AVG(accum, trials, avg)       \
    {                                 \
        avg = (1.0 * accum) / trials; \
    }

//****************************************************************************
//logging
//****************************************************************************

void print_build_info_check()
{
    //    string archs[] = {"reference", "zen2", "arm"};
    printf("W_ob =  %d \n C_ob = %d \n SIMD = %d \n", W_ob, C_ob, SIMD);
}

void print_stats(std::vector<unsigned long long> v, const char *benchmark)
{
    if (v.size() != 0)
    {
        unsigned long long sum = std::accumulate(v.begin(), v.end(), 0.0);
        unsigned long long mean = sum / v.size();
        unsigned long long min_elem = *min_element(v.begin(), v.end());
        unsigned long long max_elem = *max_element(v.begin(), v.end());
        printf("Average for %s: %llu, \t ", benchmark, mean);
        printf("Min for %s    : %llu, \t", benchmark, min_elem);
        printf("Max for %s    : %llu \n", benchmark, max_elem);
    }
}

#define print_flops(ops, time)                      \
    {                                               \
        printf("%.4lf\t", (ops) / (1.0 * time));    \
    }

#define print_cycles(time)                  \
    {                                       \
        printf("%.2lf\t", 1.0 * (time));    \
    }

//****************************************************************************
//Allocation
//****************************************************************************

dtype * alloc (uint32_t numel)
{
    dtype *ptr_dc;

    int ret = posix_memalign((void **)&ptr_dc, 64, numel * sizeof(dtype));

    if (ret)
    {
        return NULL;
    }
    return ptr_dc;
}


//****************************************************************************
// Initialization Options
//****************************************************************************

void init(float * ptr, uint32_t numel)
{
  float * cur_ptr = ptr;
  for(uint32_t i = 0 ; i < numel ; i++)
  {
    *(cur_ptr++) = 2.0*((dtype) rand()/ RAND_MAX) - 1;
  }
}

// template <typename>
void init(dtype *ptr, uint32_t numel)
{
  dtype *cur_ptr = ptr;
  for (uint32_t i = 0; i < numel; i++)
  {
        *(cur_ptr++) = rand()  % 100;
  }
}

void init_ones(dtype *ptr, uint32_t numel)
{
  dtype *cur_ptr = ptr;
  for (uint32_t i = 0; i < numel; i++)
  {
    *(cur_ptr++) = 1.0;
    // printf("%.2f \n", *(cur_ptr - 1));
  }
}

template<uint32_t _C_ob>
void init_arange(dtype *ptr, uint32_t H, uint32_t W, uint32_t C)
{
  dtype *cur_ptr = ptr;
  for (uint32_t i = 0; i < C; i+=_C_ob)
  {
    for(uint32_t j = 0 ; j < H; j++)
    {
      for(uint32_t k = 0; k < W; k++)
      {
        for(uint32_t ii = 0; ii < _C_ob; ii++)
        {
           *(cur_ptr++) =  ii + i + k*(C) + j*(W*C);
          //  printf("%.2f \n", *(cur_ptr - 1));
        }
      }
    }
  }
}
void init_norm(dtype *ptr, uint32_t numel, uint32_t C_o)
{
  dtype *cur_ptr = ptr;
  dtype norm = (1.0*C_o)/(1.0*numel);
  for (uint32_t i = 0; i < numel; i++)
  {
    *(cur_ptr++) = norm;
  }
}
bool equals(uint32_t numel, float *unfused, float *fused, float tolerance = 1e-8)
{
  bool check = 1;
  float *unfused_ptr = unfused;
  float *fused_ptr = fused;
  printf("begin correctness check\n");

  for (uint32_t i = 0; i < numel; i++)
  {
    float diff = *(fused_ptr) - *(unfused_ptr);
    printf("%d %.4f %.4f %.4f\n", i, *(fused_ptr), *(unfused_ptr), diff);

    if(fabs(diff) > tolerance)
    {
      printf("%d %.4f %.4f %.4f\n", i, *(fused_ptr), *(unfused_ptr), diff);
      check = 0;
    }
    unfused_ptr++;
    fused_ptr++;
  }
  printf("end of correctness check\n");
  return check;
}

bool equals(uint32_t numel, dtype *unfused, dtype *fused, dtype tolerance = 1e-8)
{
  bool check = 1;
  dtype *unfused_ptr = unfused;
  dtype *fused_ptr = fused;
  printf("begin correctness check\n");

  for (uint32_t i = 0; i < numel; i++)
  {
    dtype diff = *(fused_ptr) - *(unfused_ptr);
    printf("%d %d %d %d\n", i, *(fused_ptr), *(unfused_ptr), diff);

    if (fabs(diff) > tolerance)
    {
      printf("%d %d %d %d\n", i, *(fused_ptr), *(unfused_ptr), diff);
      check = 0;
    }
    unfused_ptr++;
    fused_ptr++;
  }
  printf("end of correctness check\n");
  return check;
}

template<uint32_t num_ops, uint32_t trials>
void write_results(uint64_t * fused_timing)
{
  // std::string path = "Results/logfile";
  // std::string path_to_log_file = path + file;
  // FILE *outfile = fopen(path.c_str(), "w");

  fprintf(stderr, "Unfused ");
  for (uint32_t j = 0; j < num_ops; j++)
  {
    fprintf(stderr, "Fused %d\t", j);
  }
  fprintf(stderr, "\n");
  for (uint32_t i = 0; i < trials; i++)
  {
    // fprintf(stderr, "%lu\t", timing[i]);
    for (uint32_t j = 0; j < num_ops + 1 ; j++)
    {
      fprintf(stderr, "%lu\t", fused_timing[j*trials + i]);
    }
    fprintf(stderr, "\n");
  }
}

timespec time1, time2;
long diff = 0;

long time_difference(timespec start, timespec end)
{
  timespec temp;
  if (end.tv_nsec < start.tv_nsec)
  {
    temp.tv_sec = end.tv_sec - start.tv_sec - 1;
    temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
  }
  else
  {
    temp.tv_sec = end.tv_sec - start.tv_sec;
    temp.tv_nsec = end.tv_nsec - start.tv_nsec;
  }
  return (temp.tv_sec * 1000000000 + temp.tv_nsec);
}
