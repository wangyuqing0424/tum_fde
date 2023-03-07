using namespace std;
#include "JoinQuery.hpp"
#include <assert.h>
#include <fstream>
#include <iostream>
#include <thread>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>
#include <charconv>


//https://hyper-db.de/tpch.png

//---------------------------------------------------------------------------
JoinQuery::JoinQuery(std::string lineitem, std::string order,
                     std::string customer)
{  getCustomerMap(&(customer[0]), this->customer_map);
   MemoryMappedOrder(&(order[0]), this->orders_map);
   MemoryMappedLine(&(lineitem[0]), this->lineitem_map);
   MemoryMappedCustomer(&(customer[0]), this->customer_vec);
   //
}
//---------------------------------------------------------------------------
//most part of this code taking from the lecture codes
constexpr uint64_t broadcast(uint8_t c) 
{
   return (static_cast<uint64_t>(c) << 0) | (static_cast<uint64_t>(c) << 8) | (static_cast<uint64_t>(c) << 16) | (static_cast<uint64_t>(c) << 24) | (static_cast<uint64_t>(c) << 32) | (static_cast<uint64_t>(c) << 40) | (static_cast<uint64_t>(c) << 48) | (static_cast<uint64_t>(c) << 56);
}

template <char separator>
static const char *findSeparaton(const char *iter, const char *end)
{   
   auto end8 = end - 8;
   constexpr uint64_t pattern = broadcast(separator);
   constexpr uint64_t high = broadcast(0x80);
   constexpr uint64_t low = ~high;
   
   for (; iter < end8; iter += 8) {
      uint64_t block = *reinterpret_cast<const uint64_t *>(iter);
      uint64_t lowChars = (~block) & high;
      uint64_t foundPattern = ~((((block & low) ^ pattern) + low) & high);
      uint64_t matches = foundPattern & lowChars;     
      if (matches) return iter + (__builtin_ctzll(matches) >> 3) + 1;
   }

   // Check the last few characters explicitly
   while ((iter < end) && ((*iter) != separator)) ++iter;
   if (iter < end) ++iter;
   return iter;
}


template <char separator>
static const char *findNthSeparaton(const char *iter, const char *end, unsigned n)
{
   auto end8 = end - 8;
   constexpr uint64_t pattern = broadcast(separator);
   constexpr uint64_t high = broadcast(0x80);
   constexpr uint64_t low = ~high;

   for (; iter < end8; iter += 8) {
      uint64_t block = *reinterpret_cast<const uint64_t *>(iter);
      uint64_t lowChars = (~block) & high;
      uint64_t foundPattern = ~((((block & low) ^ pattern) + low) & high);
      uint64_t matches = foundPattern & lowChars;
      if (matches) {
         unsigned hits = __builtin_popcountll(matches);  
         if (hits >= n) {
            for (; n > 1; n--)
               matches &= matches - 1; 
            return iter + (__builtin_ctzll(matches) >> 3) + 1;
         }
         n -= hits;
      }
   }
   for (; iter < end; ++iter)
      if ((*iter) == separator)
         if ((--n) == 0) return iter + 1;

   return end;
}
//---------------------------------------------------------------------------
void JoinQuery::getCustomerMap(const char *file,
                              unordered_multimap<string, unsigned> &map)
{
    int handle = open(file, O_RDONLY);
    lseek(handle, 0, SEEK_END);
    auto length = lseek(handle, 0, SEEK_CUR);
    void *data =
            mmap(nullptr, length, PROT_READ, MAP_SHARED | MAP_POPULATE, handle, 0);
    madvise(data, length, MADV_SEQUENTIAL);
    madvise(data, length, MADV_WILLNEED);
    auto begin = static_cast<const char *>(data), end = begin + length;

    for (auto iter = begin; iter < end;) {
        auto last = findSeparaton<'|'>(iter, end);
        int cust_id = 0;
        from_chars(iter, last, cust_id);

        last = findNthSeparaton<'|'>(iter, end, 6);
        iter = last;
        last = findSeparaton<'|'>(iter, end);
        int size = last - iter - 1;
        string mkt(iter, size);
        map.insert({mkt, cust_id});
        iter = findSeparaton<'\n'>(iter, end);
    }

    munmap(data, length);
    close(handle);
}
void JoinQuery::MemoryMappedOrder(const char *fileName, unordered_multimap<unsigned, unsigned> &map)
{
   int handle = open(fileName, O_RDONLY);
   lseek(handle, 0, SEEK_END);
   auto length = lseek(handle, 0, SEEK_CUR);
   void *data =
       mmap(nullptr, length, PROT_READ, MAP_SHARED | MAP_POPULATE, handle, 0);
   auto begin = static_cast<const char *>(data), end = begin + length;
   for (auto iter = begin; iter < end;) {
      auto last = findSeparaton<'|'>(iter, end);
      int orderkey = 0;
      from_chars(iter, last, orderkey);
      iter = last;
      last = findSeparaton<'|'>(iter, end);
      int custkey = 0;
      from_chars(iter, last , custkey);
map.insert({ custkey, orderkey});
      iter = findSeparaton<'\n'>(iter, end);
   }
   munmap(data, length);
   close(handle);
}

void JoinQuery::MemoryMappedLine(const char* fileName,unordered_multimap<unsigned, unsigned> &map)
{  
   int handle = open(fileName, O_RDONLY);
   lseek(handle, 0, SEEK_END);
   auto length = lseek(handle, 0, SEEK_CUR);
   void *data = mmap(nullptr, length, PROT_READ, MAP_SHARED | MAP_POPULATE, handle, 0);
   const char* begin= static_cast<char*>(data);
   const char* end = static_cast<char*>(data) + length;
   for (auto iter = begin; iter < end;) {
      auto last = findSeparaton<'|'>(iter, end);
      int orderkey = 0;
      from_chars(iter, last, orderkey);
      iter = findNthSeparaton<'|'>(iter, end, 4);
      last = findSeparaton<'|'>(iter, end);
      int quantity = 0;
      from_chars(iter, last , quantity);
map.insert({orderkey, quantity});
      iter = findSeparaton<'\n'>(iter, end);
   }
   munmap(data, length);
   close(handle);
}

void JoinQuery::MemoryMappedCustomer(const char *fileName, vector<string> &ids)
{
   int handle = open(fileName, O_RDONLY);
   handle = ::open(fileName, O_RDONLY);
   auto length = lseek(handle, 0, SEEK_END);
   auto size = lseek(handle, 0, SEEK_CUR);
   void *data = mmap(nullptr, size, PROT_READ, MAP_SHARED | MAP_POPULATE, handle, 0);
   const char* begin= static_cast<char*>(data);
   const char* end = static_cast<char*>(data) + length; 
   for(auto iter = begin; iter < end;){
      auto last = findNthSeparaton <'|'>(iter, end,6);
      iter = last;
      last = findSeparaton<'|'>(iter, end);
      int size = last - iter - 1;
      string mkt(iter, size);
  ids.push_back(mkt);
      iter = findSeparaton <'\n'>(iter, end);
   }
   munmap(data, length);
   close(handle);
}

size_t JoinQuery::avg(std::string segmentParam)
{
    unsigned long long int sum = 0;
    unsigned long long int count = 0;

    auto iterators = customer_map.equal_range(segmentParam);
    for (auto iterator = iterators.first; iterator != iterators.second;
         ++iterator) {
        auto iters = orders_map.equal_range(iterator->second);
        for (auto iter = iters.first; iter != iters.second; ++iter) {
            auto its = lineitem_map.equal_range(iter->second);
            for (auto it = its.first; it != its.second; ++it) {
                sum += it->second;
                count += 1;
            }
        }
    }

    size_t avg = sum * 100 / count;
    return avg;
}
// change to fit our function
//---------------------------------------------------------------------------
size_t JoinQuery::lineCount(std::string rel)
{
   std::ifstream relation(rel);
   assert(relation);  // make sure the provided string references a file
   size_t n = 0;
   for (std::string line; std::getline(relation, line);) n++;
   return n;
}
// this part there is no need for us to change
//---------------------------------------------------------------------------
