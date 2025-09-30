//
// Created by Марк Калюжин on 28.09.2025.
//

#ifndef CS332_PROVIDER_H
#define CS332_PROVIDER_H

#include <ios>
#include <queue>
#include <vector>
#include <chrono>
#include <thread>
#include <iomanip>
#include <utility>
#include <iostream>
#include <unordered_set>
#include "opencv2/opencv.hpp"

#pragma GCC optimize("Ofast,unroll-loops")
#pragma GCC target("avx2,popcnt,lzcnt,abm,bmi,bmi2,fma,tune=native")

using namespace std;

#define endln "\n"
#define HASH_SIZE 100000
#define MICROSECONDS_TO_SECONDS 1000000.0
#define NUM_THREADS 2
#define ll long long
#define ull unsigned long long
#define vll vector<ll>
#define vull vector<ull>

template<typename T>
using vec = vector<T>;

#endif //CS332_PROVIDER_H