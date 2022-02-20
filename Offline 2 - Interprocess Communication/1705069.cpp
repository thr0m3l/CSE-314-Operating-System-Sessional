#include <bits/stdc++.h>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <iostream>
#include <pthread.h>
#include <queue>
#include <random>
#include <semaphore.h>
#include <string>
#include <sys/types.h>
#include <unistd.h>
#include <vector>

#define TIME 100000
#define N_PASSENGER 100
#define AVG_ARRIVAL 200
#define P_VIP 0.5
#define P_LOST 0.8

using namespace std;
using namespace std::chrono;

vector<pair<int, int>> passengers;
vector<bool> is_lost;

high_resolution_clock::time_point t1;

// get time in MS
int get_time() {
  high_resolution_clock::time_point t2 = high_resolution_clock::now();
  duration<double> time_span = duration_cast<duration<double>>(t2 - t1);
  return time_span.count() * 10000 / 1000;
}

void generate_passengers() {
  // uniformly-distributed integer random number generator
  std::random_device rd;
  // mt19937: Pseudo-random number generation
  std::mt19937 rng(rd());

  double averageArrival = 15;
  double lambda = 1 / averageArrival;
  std::exponential_distribution<double> exp(lambda);

  double newArrivalTime;

  vector<int> isVip(N_PASSENGER, 0);

  for (int i = 0; i < 100 * P_VIP; i++) {
    isVip[i] = 1;
  }

  for (int i = 0; i < N_PASSENGER; i++) {
    is_lost.push_back(0);
  }

  for (int i = 0; i < P_LOST * 100; i++) {
    is_lost[i] = 1;
  }

  shuffle(isVip.begin(), isVip.end(), default_random_engine(0));
  shuffle(is_lost.begin(), is_lost.end(), default_random_engine(1));

  for (int i = 0; i < N_PASSENGER; ++i) {
    // generates the next random number in the distribution
    passengers.push_back({exp.operator()(rng), isVip[i]});
  }
}

sem_t nEmptyKiosks;
sem_t nEmptyBelts;

int kiosk_wait, belt_wait, boarding_wait, vip_wait;

queue<int> free_kiosks;
queue<int> free_belts;
queue<int> vip_queue;

bool dir = 0; // 0 -> Left to right, 1 -> Right to left

pthread_mutex_t lock_kiosk_queue;
pthread_mutex_t lock_belt_queue;
pthread_mutex_t lock_vip_queue;
pthread_mutex_t lock_vip_direction;
pthread_mutex_t lock_special_kiosk;
pthread_mutex_t lock_boarding_gate;
pthread_mutex_t lock_dir;
pthread_mutex_t lock_rl;

// synchronized queue pop //
int getAvailableKiosk() {
  pthread_mutex_lock(&lock_kiosk_queue);
  int result = free_kiosks.front();
  free_kiosks.pop();
  pthread_mutex_unlock(&lock_kiosk_queue);
  return result;
}

int getAvailableBelt() {
  pthread_mutex_lock(&lock_belt_queue);
  int result = free_belts.front();
  free_belts.pop();
  pthread_mutex_unlock(&lock_belt_queue);
  return result;
}

int vip_pop() {
  pthread_mutex_lock(&lock_vip_queue);
  vip_queue.pop();
  int sz = vip_queue.size();
  pthread_mutex_unlock(&lock_vip_queue);
  return sz;
}

//

// synchronized queue push
void freeKiosk(int number) {
  pthread_mutex_lock(&lock_kiosk_queue);
  free_kiosks.push(number);
  sem_post(&nEmptyKiosks);
  pthread_mutex_unlock(&lock_kiosk_queue);
}

void freeBelt(int number) {
  pthread_mutex_lock(&lock_belt_queue);
  free_belts.push(number);
  sem_post(&nEmptyBelts);
  pthread_mutex_unlock(&lock_belt_queue);
}

void vip_push(int number) {
  pthread_mutex_lock(&lock_vip_queue);
  vip_queue.push(number);
  pthread_mutex_unlock(&lock_vip_queue);
}

string get_passenger_string(u_int64_t n) {
  string passenger = to_string(n);

  if (passengers[n].second == 1) {
    passenger += "(VIP)";
  }

  return passenger;
}

//

int get_vip_queue_size() {
  int result;
  pthread_mutex_lock(&lock_vip_queue);
  result = vip_queue.size();
  pthread_mutex_unlock(&lock_vip_queue);
  return result;
}

void process_kiosk(void *arg) {
  string passenger = get_passenger_string((u_int64_t)arg);

  string msg = "Passenger " + passenger + " arrived at the airport at time " +
               to_string(get_time()) + "\n";
  cout << msg;
  sem_wait(&nEmptyKiosks);
  int nKiosk = getAvailableKiosk();

  msg = "Passenger " + passenger + " started self check in at kiosk " +
        to_string(nKiosk) + " from time " + to_string(get_time()) + "\n";
  cout << msg;
  usleep(kiosk_wait);
  freeKiosk(nKiosk);

  msg = "Passenger " + passenger + " has finished check in at time " +
        to_string(get_time()) + "\n";
  cout << msg;
}

void process_belt(void *arg) {
  string passenger = get_passenger_string((u_int64_t)arg);

  string msg = "Passenger " + passenger +
               " has started waiting for security check at time " +
               to_string(get_time()) + "\n";
  cout << msg;

  sem_wait(&nEmptyBelts);
  int nBelt = getAvailableBelt();
  msg = "Passenger " + passenger + " started the security check in belt  " +
        to_string(nBelt) + " from time " + to_string(get_time()) + "\n";
  cout << msg;
  usleep(belt_wait);
  freeBelt(nBelt);
  msg = "Passenger " + passenger + " has crossed the security check at time " +
        to_string(get_time()) + "\n";
  cout << msg;
}

void process_vip_lr(int passenger) {
  // left to right

  pthread_mutex_lock(&lock_vip_queue);
  vip_queue.push(passenger);
  int size = vip_queue.size();
  string msg = "Passenger " + to_string(passenger) +
               "(VIP) waiting at the VIP channel (left to right) at time " +
               to_string(get_time()) + "\n";

  cout << msg;
  if (size == 1) {
    pthread_mutex_lock(&lock_vip_direction);

    pthread_mutex_lock(&lock_dir);
    dir = 0;
    pthread_mutex_unlock(&lock_dir);
  }

  pthread_mutex_unlock(&lock_vip_queue);

  usleep(vip_wait);

  msg = "Passenger " + to_string(passenger) +
        "(VIP) reached boarding gate at time " + to_string(get_time()) + "\n";
  cout << msg;

  pthread_mutex_lock(&lock_vip_queue);
  vip_queue.pop();

  if (vip_queue.size() == 0) {
    pthread_mutex_unlock(&lock_vip_direction);

    pthread_mutex_lock(&lock_dir);
    dir = 1;
    pthread_mutex_unlock(&lock_dir);
  }

  pthread_mutex_unlock(&lock_vip_queue);
}

void process_vip_rl(int passenger) {
  string pstring = get_passenger_string(passenger);
  string msg = "Passenger " + pstring +
               " started waiting at the VIP channel (right to left) at time " +
               to_string(get_time()) + "\n";
  cout << msg;

  bool curr_dir;

  pthread_mutex_lock(&lock_rl);

  pthread_mutex_lock(&lock_dir);
  curr_dir = dir;
  pthread_mutex_unlock(&lock_dir);

  if (dir) {
    pthread_mutex_unlock(&lock_rl);
    usleep(vip_wait);

  } else {
    pthread_mutex_lock(&lock_vip_direction);
    pthread_mutex_unlock(&lock_rl);

    usleep(vip_wait);

    pthread_mutex_unlock(&lock_vip_direction);
  }

  msg = "Passenger " + pstring + " reached the special kiosk at time " +
        to_string(get_time()) + "\n";

  cout << msg;
}

void process_special_kiosk(int passenger) {
  string pstring = get_passenger_string(passenger);
  string msg = "Passenger " + pstring +
               " started waiting at the special kiosk at time " +
               to_string(get_time()) + "\n";
  cout << msg;

  pthread_mutex_lock(&lock_special_kiosk);

  usleep(kiosk_wait);

  pthread_mutex_unlock(&lock_special_kiosk);

  msg = "Passenger " + pstring +
        " received another boarding-pass at special kiosk at time " +
        to_string(get_time()) + "\n";
  cout << msg;
}

void process_boarding_gate(int passenger) {
  string pstring = get_passenger_string(passenger);
  string msg = "Passenger " + pstring + " started boarding the plane at time " +
               to_string(get_time()) + "\n";
  cout << msg;

  pthread_mutex_lock(&lock_boarding_gate);

  usleep(boarding_wait);

  pthread_mutex_unlock(&lock_boarding_gate);

  msg = "Passenger " + pstring + " boarded the plane at time " +
        to_string(get_time()) + "\n";
  cout << msg;
}

void *processAll(void *arg) {
  u_int64_t p = (u_int64_t)arg;
  process_kiosk(arg);
  if (!passengers[p].second)
    process_belt(arg);
  else {
    process_vip_lr(p);
  }

  if (is_lost[p]) {
    process_vip_rl(p);
    process_special_kiosk(p);
    process_vip_lr(p);
  }

  process_boarding_gate(p);

  pthread_exit(NULL);
}

void init() {
  int M, N, P, w, x, y, z;
  cin >> M >> N >> P >> w >> x >> y >> z;

  sem_init(&nEmptyKiosks, 0, M);
  sem_init(&nEmptyBelts, 0, N * P);

  kiosk_wait = w * TIME;
  belt_wait = x * TIME;
  boarding_wait = y * TIME;
  vip_wait = z * TIME;

  pthread_mutex_init(&lock_kiosk_queue, NULL);
  pthread_mutex_init(&lock_belt_queue, NULL);
  pthread_mutex_init(&lock_special_kiosk, NULL);
  pthread_mutex_init(&lock_vip_queue, NULL);
  pthread_mutex_init(&lock_boarding_gate, NULL);
  pthread_mutex_init(&lock_dir, NULL);
  pthread_mutex_init(&lock_rl, NULL);
  pthread_mutex_init(&lock_vip_direction, NULL);

  for (int i = 1; i <= M; i++) {
    free_kiosks.push(i);
  }

  for (int i = 1; i <= P; i++) {
    for (int j = 1; j <= N; j++) {
      free_belts.push(j);
    }
  }

  generate_passengers();

  t1 = high_resolution_clock::now();
}

int main(void) {

  init();

  pthread_t threads[1000];

  for (uint64_t i = 0; i < N_PASSENGER; i++) {

    pthread_create(&threads[i], NULL, processAll, (void *)i);
    usleep(passengers[i].first * TIME / 10);
  }

  pthread_exit(NULL);
}