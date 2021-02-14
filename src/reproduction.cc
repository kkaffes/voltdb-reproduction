#include <getopt.h>
#include <atomic>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>

#include "BenchmarkUtil.h"
#include "voltdb-client-cpp/include/Client.h"
#include "voltdb-client-cpp/include/ClientConfig.h"
#include "voltdb-client-cpp/include/Parameter.hpp"
#include "voltdb-client-cpp/include/ParameterSet.hpp"
#include "voltdb-client-cpp/include/ProcedureCallback.hpp"
#include "voltdb-client-cpp/include/Row.hpp"
#include "voltdb-client-cpp/include/Table.h"
#include "voltdb-client-cpp/include/TableIterator.h"
#include "voltdb-client-cpp/include/WireType.h"

// Message size.
static int msg_size = 2048;

// Power multiplier for the latency array.
// We can record at most 2^26 = 67108864 latencies
static const int kArrayExp = 26;
static const size_t kMaxEntries = (1L << kArrayExp);

// Report latency/throughput stats every interval.
static int measureIntervalMsec = 2000;  // 2sec in ms.

// Total benchmarking time.
static int totalExecTimeMsec = 10000;  // 10sec in ms.

static bool mainFinished = false;  // Control whether to stop the experiment.

// Record latencies in a single big array.
static double* msgLatencies;
// The current index of the msgLatencies array.
static std::atomic<uint32_t> msgLatsArrayIndex;
// The indices for each interval boundary (add a new one every interval).
static std::vector<uint32_t> msgIndices;
// Timestamp of each interval starts, in userc.
static std::vector<uint64_t> timeStampsUsec;

// Synthetic username, passwd.
static const std::string kTestUser = "testuser";
static const std::string kTestPwd = "testpassword";

/*
 * Client thread.
 *
 */
static void ClientThread(const std::string& serverAddr) {
   // Create a local VoltDB client.
  voltdb::ClientConfig config(kTestUser, kTestPwd, voltdb::HASH_SHA1);
  voltdb::Client client = voltdb::Client::create(config);
  srand(time(NULL));
  client.createConnection(serverAddr);

  // Create the data for the record.
  std::string data(msg_size, '0');

  int record_id = 0;

  do {
    // Measure the time it takes to insert a record.
    uint64_t startTime = BenchmarkUtil::getCurrTimeUsec();
    std::vector<voltdb::Parameter> parameterTypes(2);
    parameterTypes[0] = voltdb::Parameter(voltdb::WIRE_TYPE_INTEGER);
    parameterTypes[1] = voltdb::Parameter(voltdb::WIRE_TYPE_STRING);
    voltdb::Procedure procedure("InsertRecord", parameterTypes);

    voltdb::ParameterSet* params = procedure.params();
    params->addInt32(record_id).addString(data);
    voltdb::InvocationResponse r = client.invoke(procedure);
    if (r.failure()) {
      std::cerr << "SendMessage procedure failed. " << r.toString();
      exit(-1);
    }
    uint64_t endTime = BenchmarkUtil::getCurrTimeUsec();

    // Record the latency.
    auto aryIndex = msgLatsArrayIndex.fetch_add(1);
    if (aryIndex >= kMaxEntries) {
      std::cerr << "Array msgLatencies out of bounds: " << aryIndex
                << std::endl;
      exit(1);
    }
    msgLatencies[aryIndex] = double(endTime - startTime);

    // Advance the record counter.
    record_id++;
  } while(!mainFinished);

  return;
}

/*
 * Actually run the benchmark.
 */
static bool runBenchmark(const std::string& serverAddr,
                         const std::string& outputFile) {
  mainFinished = false;

  std::thread* clientThread;

  // Initialize measurement arrays.
  uint64_t currTime = BenchmarkUtil::getCurrTimeUsec();
  timeStampsUsec.push_back(currTime);
  msgLatencies = new double[kMaxEntries];
  memset(msgLatencies, 0, kMaxEntries * sizeof(double));
  msgLatsArrayIndex.store(0);
  msgIndices.push_back(0);

  clientThread = new std::thread(&ClientThread, serverAddr);

  currTime = BenchmarkUtil::getCurrTimeUsec();
  uint64_t endTime = currTime + (totalExecTimeMsec * 1000);
  do {
    std::this_thread::sleep_for(std::chrono::milliseconds(measureIntervalMsec));
    std::cerr << "runBenchmark recording performance...\n";
    currTime = BenchmarkUtil::getCurrTimeUsec();
    msgIndices.push_back(msgLatsArrayIndex.load());
    timeStampsUsec.push_back(currTime);
  } while (currTime < endTime);

  mainFinished = true;

  clientThread->join();
  delete clientThread;

  // Processing the results.
  std::cerr << "Post processing results...\n";
  bool res = BenchmarkUtil::processResults(
      msgLatencies, msgIndices, timeStampsUsec, outputFile, "Reproduction Benchmark");
  if (!res) {
    std::cerr << "[Warning]: failed to write results to " << outputFile << "\n";
  }

  // Clean up.
  delete[] msgLatencies;
  msgLatencies = nullptr;
  timeStampsUsec.clear();
  return true;
}

static void Usage(char** argv, const std::string& msg = "") {
  if (!msg.empty()) { std::cerr << "ERROR: " << msg << std::endl; }
  std::cerr << "Usage: " << argv[0] << "[options]\n";
  std::cerr << "\t-h: show this message\n";
  std::cerr << "\t-o <output log file path>: default "
            << "reproduction_results.csv\n";
  //TODO: support list of servers.
  std::cerr << "\t-s <DB server>: default 'localhost'\n";
  std::cerr << "\t-i <measurement interval>: default " << measureIntervalMsec
            << " msec\n";
  std::cerr << "\t-t <total execution time>: default " << totalExecTimeMsec
            << " msec\n";
  std::cerr << "\t-m <message size>: default " << msg_size << std::endl;
  std::cerr << std::endl;
  exit(1);
}

int main(int argc, char** argv) {
  std::string serverAddr("localhost");
  std::string outputFile("reproduction_results.csv");

  // Parse input arguments and prepare for the experiment.
  int opt;
  while ((opt = getopt(argc, argv, "h:s:i:t:m:")) != -1) {
    switch (opt) {
      case 's':
        serverAddr = optarg;
        break;
      case 'i':
        measureIntervalMsec = atoi(optarg);
        break;
      case 't':
        totalExecTimeMsec = atoi(optarg);
        break;
      case 'm':
        msg_size = atoi(optarg);
        break;
      case 'h':
      default:
        Usage(argv);
        break;
    }
  }

  std::cerr << "Message size: " << msg_size << " bytes" << std::endl;
  std::cerr << "VoltDB server address: " << serverAddr << std::endl;
  std::cerr << "Measurement interval: " << measureIntervalMsec << " msec\n";
  std::cerr << "Total execution time: " << totalExecTimeMsec << " msec\n";

  bool res;
  res = runBenchmark(serverAddr, outputFile);
  if (!res) {
    std::cerr << "Failed to run benchmark." << std::endl;
    exit(1);
  }

  return 0;
}


