#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
///////////////////////struct goes here//////////
struct cacheLine {
  int valid;
  unsigned int tag;
  int dirty;
  unsigned int dirty_address;
};
class cache;
typedef struct cacheLine cl;

struct cache_info {
  string name;
  string cacheStructure;
  int ways;
  int capacity;
  int blocksize;
  char write_allocate;
  string replacement;
  int next;
};
typedef struct cache_info cache_inf;
//////////////////////cache class go here:
class cache
{
 protected:
  vector<vector<cl> > sets;

 public:
  string name;
  int capacity;  //L1D L1I ......
  int blocksize;
  int numSet;
  int ways;
  char write_allocate;
  string replacement;
  unsigned int tag_size;
  unsigned int index_size;
  unsigned int offset_size;
  string type;
  cache * next;

  int inst_count;
  int i_miss;
  int read_count;
  int r_miss;
  int write_count;
  int w_miss;
  int total;
  int compulsories[3];
  int conflicts[3];
  int capacities[3];
  char * blocks;
  vector<unsigned int> history;

  vector<vector<int> > ageBits;          //each cache line has one ageBit
  vector<int> fBit;                      //each set has one fBit
  unsigned int get_way(unsigned index);  //use replacement policyt to get a way

  cache() {}
  explicit cache(cache_inf & cf);
  bool hit(unsigned int address);
  unsigned int get_a_way(unsigned int index);
  virtual void read_hit(unsigned int address, int inst);
  virtual void read_miss(unsigned int address, int wm, int inst);
  virtual void write_hit(unsigned int address);
  virtual void write_miss(unsigned int address, int inst);
  void classify_miss(unsigned int address, int inst);
  void print_result();
  virtual ~cache() {}
};

cache::cache(cache_inf & cf) :
    name(cf.name),
    capacity(cf.capacity),
    blocksize(cf.blocksize),
    numSet(cf.capacity / (cf.blocksize * cf.ways)),
    ways(cf.ways),
    write_allocate(cf.write_allocate),
    replacement(cf.replacement),
    tag_size(24 - log2(blocksize) - log2(numSet)),
    index_size(log2(numSet)),
    offset_size(log2(blocksize)),
    type(cf.cacheStructure),
    inst_count(0),
    i_miss(0),
    read_count(0),
    r_miss(0),
    write_count(0),
    w_miss(0),
    total(0)

{
  sets.resize(numSet);
  for (int i = 0; i < numSet; i++) {
    sets[i].resize(ways);
    for (int j = 0; j < ways; j++) {
      sets[i][j].valid = 0;  //intialize cache line
      sets[i][j].tag = 0;
      sets[i][j].dirty = 0;
    }
  }
  //miss helper
  for (int i = 0; i < 3; i++) {
    compulsories[i] = 0;
    conflicts[i] = 0;
    capacities[i] = 0;
  }
  int n = 16777216 / blocksize;  //24 bit /  blocksize =  # of bloks
  blocks = new char[n];
  for (int j = 0; j < n; j++) {
    blocks[j] = 'n';
  }

  //for replacement policies
  ageBits.resize(numSet);
  fBit.resize(numSet);
  for (int i = 0; i < numSet; i++) {
    ageBits[i].resize(ways);
    for (int j = 0; j < ways; j++) {
      ageBits[i][j] = 0;
    }
    fBit[i] = 0;
  }
}

bool cache::hit(unsigned int address) {
  unsigned int add_tag = address >> (24 - tag_size);
  unsigned int index = (address << (8 + tag_size)) >> (8 + tag_size + offset_size);
  for (int i = 0; i < ways; i++) {
    if ((sets[index][i].tag == add_tag) && (sets[index][i].valid == 1)) {
      return true;
    }
  }
  return false;
}

void cache::classify_miss(unsigned int address, int inst) {
  unsigned int block_address = address / blocksize;
  if (blocks[block_address] == 'n') {  //block visited before? if no
    compulsories[inst]++;
    blocks[block_address] = 'y';
  }
  else {
    std::set<unsigned int> st;  // contains unique elements
    for (unsigned i = history.size() - 1; i >= 0; i--) {
      if (history[i] != block_address) {  //if not found
        st.insert(history[i]);
      }
      else {  //if found
        if (st.size() < (unsigned)(numSet * ways)) {
          conflicts[inst]++;
        }
        else {
          capacities[inst]++;
        }
        break;
      }
    }
  }
  history.push_back(block_address);
}

unsigned int cache::get_a_way(unsigned int index) {
  //find LRU and MRU bit first
  int max_age = 0;  //LRU
  unsigned max_way = 0;
  int min_age = 10000;  //MRU
  unsigned min_way = 0;
  for (int i = 0; i < ways; i++) {
    if (ageBits[index][i] > max_age) {
      max_age = ageBits[index][i];
      max_way = i;
    }
    if (ageBits[index][i] < min_age) {
      min_age = ageBits[index][i];
      min_way = i;
    }
  }
  //policies
  if (replacement == "LRU") {
    ageBits[index][max_way] = 0;
    for (unsigned int i = 0; i < (unsigned)ways; i++) {
      if (i != max_way) {
        ageBits[index][i]++;
      }
    }

    return max_way;
  }
  else if (replacement == "RND") {
    unsigned int temp = rand() % ways;
    ageBits[index][temp] = 0;
    for (unsigned int i = 0; i < (unsigned)ways; i++) {
      if (i != temp) {
        ageBits[index][i]++;
      }
    }
    return temp;
  }
  else if (replacement == "NMRU") {
    unsigned int temp;
    while ((temp = rand() % ways) == min_way) {
    }
    for (unsigned int i = 0; i < (unsigned)ways; i++) {
      if (i != temp) {
        ageBits[index][i]++;
      }
    }
    ageBits[index][temp] = 0;
    return temp;
  }
  else {  //FIFO
    unsigned int temp = fBit[index];
    fBit[index] = (fBit[index] + 1) % ways;
    return temp;
  }
}

//inst informs read from instruction fetch(2) or data read(0)
void cache::read_hit(unsigned int address, int inst) {
  //when read hit, we need to update total, count,ageBit

  //update data
  total++;
  if (name == "L1" || name == "L2") {
    if (inst == 0) {
      read_count++;
    }
    else if (inst == 2) {
      inst_count++;
    }
  }
  else if (name == "L1$D" || name == "L2$D") {
    read_count++;
  }
  else if (name == "L1$I" || name == "L2$I") {
    inst_count++;
  }
  //update ageBits
  if (type != "DM") {
    unsigned int add_tag = address >> (24 - tag_size);
    unsigned int index = (address << (8 + tag_size)) >> (8 + tag_size + offset_size);
    int i = 0;
    for (; i < ways; i++) {
      if (sets[index][i].tag == add_tag) {
        ageBits[index][i] = 0;
        i++;
        break;
      }
      else {
        ageBits[index][i]++;
      }
    }
    for (; i < ways; i++) {
      ageBits[index][i]++;
    }
  }
}

void cache::read_miss(unsigned int address, int wm, int inst) {
  //two situatons for read_miss:
  //1.a normal read request
  //2.a write miss happens, it will retrieve block first if write-allocate(update count and w_miss aldready, don't increment count and r_miss)
  unsigned int add_tag = address >> (24 - tag_size);
  unsigned int index = (address << (8 + tag_size)) >> (8 + tag_size + offset_size);
  //replacement policies choose a way first
  unsigned int rway = 0;
  if (type != "DM") {
    rway = get_a_way(index);
    //this function automatically update ageBits and fBit, we don't need update on our own
  }

  if (wm != 1) {  //if not read from write miss

    //update data
    total++;
    if (name == "L1" || name == "L2") {
      if (inst == 0) {
        read_count++;
        r_miss++;
      }
      else {
        inst_count++;
        i_miss++;
      }
    }
    else if (name == "L1$D" || name == "L2$D") {
      read_count++;
      r_miss++;
    }
    else if (name == "L1$I" || name == "L2$I") {
      inst_count++;
      i_miss++;
    }
    //classify miss
    classify_miss(address, inst);
  }

  //write it to next level first if it is dirty
  if (next != NULL) {
    if (sets[index][rway].dirty == 1) {
      if (next->hit(sets[index][rway].dirty_address)) {
        next->write_hit(sets[index][rway].dirty_address);
      }
      else {
        next->write_miss(sets[index][rway].dirty_address, 1);
      }
    }
  }
  // then request data for miss
  if (next != NULL) {
    if (next->hit(address)) {
      next->read_hit(address, inst);
    }
    else {
      next->read_miss(address, 0, inst);
    }
  }
  //update block
  sets[index][rway].tag = add_tag;
  sets[index][rway].valid = 1;  //////////
  if (wm == 1) {
    sets[index][rway].dirty = 1;
    sets[index][rway].dirty_address = address;
  }
  else {
    sets[index][rway].dirty = 0;
  }
}

void cache::write_hit(unsigned int address) {
  //update data
  total++;
  write_count++;

  unsigned int add_tag = address >> (24 - tag_size);
  unsigned int index = (address << (8 + tag_size)) >> (8 + tag_size + offset_size);

  if (type != "DM") {
    int i = 0;
    for (; i < ways; i++) {
      if (sets[index][i].tag == add_tag) {
        ageBits[index][i] = 0;
        sets[index][i].dirty = 1;
        sets[index][i].dirty_address = address;
        i++;
        break;
      }
      else {
        ageBits[index][i]++;
      }
    }
    for (; i < ways; i++) {
      ageBits[index][i]++;
    }
  }
  else {
    sets[index][0].dirty = 1;
    sets[index][0].dirty_address = address;
  }
}

void cache::write_miss(unsigned int address, int inst) {
  total++;
  write_count++;
  w_miss++;

  classify_miss(address, inst);
  if (write_allocate == 'n') {  //no-write-allocate
    //don't update current block but write to next cache
    if (next != NULL) {
      if (next->hit(address)) {
        next->write_hit(address);
      }
      else {
        next->write_miss(address, inst);
      }
    }
  }
  else {  //write-allocate
    // must first writeback to next level if it is dirty
    read_miss(address, 1, 0);
    //in the read miss it will update tag, valid
    //how to update dirty
  }
}

void cache::print_result() {
  cout << setw(20) << "cache " << name << endl;
  cout << setw(20) << "Metrics" << setw(10) << "Total" << setw(10) << "Instrn" << setw(10) << "Data"
       << setw(10) << "Read" << setw(10) << "Write" << endl;

  cout << setfill('-') << setw(82) << " " << setfill(' ') << endl;

  cout << setw(20) << "Demand Fetches" << setw(10) << total << setw(10) << inst_count << setw(10)
       << read_count + write_count << setw(10) << read_count << setw(10) << write_count << endl;

  cout << setw(20) << "Demand Misses" << setw(10) << i_miss + w_miss + r_miss << setw(10) << i_miss
       << setw(10) << r_miss + w_miss << setw(10) << r_miss << setw(10) << w_miss << endl;

  cout << setw(20) << "Compulsory misses" << setw(10)
       << (compulsories[0] + compulsories[1] + compulsories[2]) << setw(10) << compulsories[2]
       << setw(10) << (compulsories[0] + compulsories[1]) << setw(10) << compulsories[0] << setw(10)
       << compulsories[1] << endl;

  cout << setw(20) << "Capacity misses" << setw(10) << capacities[0] + capacities[1] + capacities[2]
       << setw(10) << capacities[2] << setw(10) << capacities[0] + capacities[1] << setw(10)
       << capacities[0] << setw(10) << capacities[1] << endl;

  cout << setw(20) << "Conflict misses" << setw(10) << conflicts[0] + conflicts[1] + conflicts[2]
       << setw(10) << conflicts[2] << setw(10) << conflicts[0] + conflicts[1] << setw(10)
       << conflicts[0] << setw(10) << conflicts[1] << endl;
  cout << setfill('-') << setw(82) << " " << setfill(' ') << endl;
}
////////////////////simulator class goes here//////////////////
class simulator
{
 private:
  //fields
  int num1;
  int num2;
  vector<cache *> caches;
  ifstream infile;

 public:
  simulator() {}
  explicit simulator(cache_inf cf[], int num1, int num2, string & filename);
  void run_simulate();
  void print_result();
  ~simulator();
};

simulator::simulator(cache_inf cf[], int _num1, int _num2, string & filename) :
    num1(_num1),
    num2(_num2),
    infile(filename.c_str()) {
  if (!infile.is_open()) {
    cout << "can't open input infile" << endl;
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < num1 + num2; i++) {
    cache * temp = new cache(cf[i]);
    caches.push_back(temp);
  }
  //connect them
  if (num2 == 0) {
    caches[0]->next = NULL;
    if (num1 == 2) {
      caches[1]->next = NULL;
    }
  }
  else {
    caches[0]->next = caches[2];
    if (num1 == 2) {
      caches[1]->next = (num2 == 2) ? caches[3] : caches[2];
    }
  }
}
simulator::~simulator() {
  //delete caches
  for (int i = 0; i < num1 + num2; i++) {
    delete caches[i];
  }
  infile.close();
}

void simulator::run_simulate() {
  //problem I and D
  int D = 0;  //data cache index
  if (num1 == 2) {
    D = 1;
  }

  string curr;
  while (getline(infile, curr)) {
    stringstream ss(curr);
    int instruction;
    unsigned int address;

    ss >> hex >> instruction >> address;

    if (instruction == 0) {  //data read

      if (caches[D]->hit(address)) {
        caches[D]->read_hit(address, 0);
      }
      else {
        caches[D]->read_miss(address, 0, 0);
      }
    }

    else if (instruction == 1) {  //data write

      if (caches[D]->hit(address)) {
        caches[D]->write_hit(address);
      }
      else {
        caches[D]->write_miss(address, 1);
      }
    }
    else {  //instruction fetch

      if (caches[0]->hit(address)) {
        caches[0]->read_hit(address, 2);
      }
      else {
        caches[0]->read_miss(address, 0, 2);
      }
    }
  }
}

void simulator::print_result() {
  for (int i = 0; i < num1 + num2; i++) {
    caches[i]->print_result();
  }
}

///////////////////////functions go here//////////////////////
cache_inf input_info() {
  cache_inf cf;
  cout << "The cache name" << endl;
  cin >> cf.name;
  cout << "cache structure:" << endl;
  cin >> cf.cacheStructure;
  if (cf.cacheStructure != "DM") {
    cout << "replacement policy:" << endl;
    cin >> cf.replacement;
    if (cf.cacheStructure != "FA") {
      cout << "ways:" << endl;
      cin >> cf.ways;
    }
    else {
      cf.ways = cf.capacity / cf.blocksize;
    }
  }
  else {
    cf.ways = 1;
  }
  cout << "total size:" << endl;
  cin >> cf.capacity;
  cout << "block size:" << endl;
  cin >> cf.blocksize;
  cout << "write-allocate?(y/n)" << endl;
  cin >> cf.write_allocate;
  return cf;
}
void make_simulator(int numCaches1, int numCaches2) {
  int temp = numCaches1 + numCaches2;
  cache_inf * cf = new cache_inf[temp];

  for (int i = 0; i < numCaches1 + numCaches2; i++) {
    cf[i] = input_info();
  }

  if (numCaches2 == 0) {
    for (int i = 0; i < numCaches1; i++) {
      cf[i].next = 0;
    }
  }
  else {
    for (int i = numCaches1; i < numCaches1 + numCaches2; i++) {
      cf[i].next = 0;
    }
    cf[0].next = 2;
    if (numCaches1 == 2) {
      cf[1].next = (numCaches2 == 2) ? 3 : 2;
    }
  }

  string filename;
  cout << "load file:" << endl;
  cin >> filename;
  simulator sim(cf, numCaches1, numCaches2, filename);
  sim.run_simulate();
  delete[] cf;
  sim.print_result();
}

int main() {
  string s_or_d;  //single or double
  string u_or_s;  //unified or split

  cout << "single/double layer?" << endl;
  cin >> s_or_d;
  if (s_or_d == "single") {
    cout << "unified or split?" << endl;
    cin >> u_or_s;
    int i;
    i = (u_or_s == "unified") ? 1 : 2;
    make_simulator(i, 0);
  }
  else {
    int i;
    cout << "L1 is unified or split?" << endl;
    cin >> u_or_s;
    i = (u_or_s == "unified") ? 1 : 2;
    int j;
    cout << "L2 is unified or split?" << endl;
    cin >> u_or_s;
    j = (u_or_s == "unified") ? 1 : 2;
    make_simulator(i, j);
  }

  return EXIT_SUCCESS;
}
