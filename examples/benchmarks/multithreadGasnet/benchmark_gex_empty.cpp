#include "bcl/containers/experimental/arh/arh.hpp"
#include "bcl/containers/experimental/arh/arh_tools.hpp"
#include <sched.h>
#include <queue>
#include <mutex>
#include "include/cxxopts.hpp"

std::atomic<size_t> issued = 0;
std::atomic<size_t> received = 0;
size_t hidx_empty_req;
size_t hidx_reply;
bool full_mode = false;

void empty_req_handler(gex_Token_t token) {
  {
    // do nothing
  }
  gex_AM_ReplyShort(token, hidx_reply, 0);
}

void reply_handler(gex_Token_t token) {
  received++;
}

void worker() {
  size_t num_ops = 10000;

  srand48(ARH::my_worker());
  ARH::barrier();
  ARH::tick_t start = ARH::ticks_now();

  if (full_mode || ARH::my_worker() == 0) {

    for (size_t i = 0; i < num_ops; i++) {
      size_t remote_proc = lrand48() % ARH::nprocs();
      while (remote_proc == ARH::my_proc()) {
        remote_proc = lrand48() % ARH::nprocs();
      }

      issued++;
      gex_AM_RequestShort(BCL::tm, remote_proc, hidx_empty_req, 0);

      while (received < issued) {
        gasnet_AMPoll();
      }
    }

  }

  ARH::barrier();
  ARH::tick_t end = ARH::ticks_now();

  double duration = ARH::ticks_to_ns(end - start) / 1e3;
  double latency = duration / num_ops;
  ARH::print("Setting: full_mode = %d; duration = %.2lf s; num_ops = %lu\n", full_mode, duration / 1e6, num_ops);
  ARH::print("latency: %.2lf us\n", latency);

}

int main(int argc, char** argv) {
  cxxopts::Options options("ARH Benchmark", "Benchmark of ARH system");
  options.add_options()
      ("full", "Enable full mode")
      ;
  auto result = options.parse(argc, argv);
  try {
    full_mode = result.count("full");
  } catch (...) {
    full_mode = false;
  }

  ARH::init(15, 16);

  gex_AM_Entry_t entry[2];
  hidx_empty_req = ARH::handler_num++;
  hidx_reply = ARH::handler_num++;

  entry[0].gex_index = hidx_empty_req;
  entry[0].gex_fnptr = (gex_AM_Fn_t) empty_req_handler;
  entry[0].gex_flags = GEX_FLAG_AM_SHORT | GEX_FLAG_AM_REQUEST;
  entry[0].gex_nargs = 0;

  entry[1].gex_index = hidx_reply;
  entry[1].gex_fnptr = (gex_AM_Fn_t) reply_handler;
  entry[1].gex_flags = GEX_FLAG_AM_SHORT | GEX_FLAG_AM_REPLY;
  entry[1].gex_nargs = 0;

  gex_EP_RegisterHandlers(BCL::ep, entry, 2);

  ARH::run(worker);
  ARH::finalize();

  return 0;
}