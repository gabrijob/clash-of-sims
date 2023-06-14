
#include <simgrid/s4u.hpp>
#include "simgrid/s4u/VirtualMachine.hpp"
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_ntasks, "Messages specific for this s4u N-tasks experiment.");

/* Static scheduler to distribute num_tasks equal tasks in a circular fashion. */
static void scheduler_circular(std::vector<sg4::Mailbox*> mailbox_list, int num_tasks, double task_size) {

  int wid = 0;
  sg4::Mailbox* worker_mailbox = nullptr;
  int num_workers = mailbox_list.size();
  XBT_INFO("Number of available workers %d", num_workers);

  for(int i=0; i < num_tasks; i++) {
    wid = i % num_workers;
    worker_mailbox = mailbox_list[wid];

    XBT_INFO("Sending task to mailbox %s", worker_mailbox->get_name().c_str());

    /* -- Send a 1-Byte payload (latency bound) ... */
    //auto* payload = new double(sg4::Engine::get_clock());
    auto* payload = new double(task_size);
    worker_mailbox->put(payload, 1);
  }

  /*- Send finish signal to all workers */
  for(int wid=0; wid < num_workers; wid++) {
    worker_mailbox = mailbox_list[wid];

    XBT_INFO("Sending finish signal to mailbox %s", worker_mailbox->get_name().c_str());
    
    auto* payload = new double(-1.0);
    worker_mailbox->put(payload, 1);
  }

}

/* Basic worker that receives tasks. */
static void worker_basic(sg4::Mailbox* mailbox) {
  std::unique_ptr<double, std::default_delete<double> > task; // Deve ter um jeito melhor de declarar essa linha
  do {
    XBT_INFO("Receiving from mailbox %s", mailbox->get_name().c_str());
    /* - Receive the task from scheduler ....*/
    task = mailbox->get_unique<double>();

    if (*task >= 0) {     
      /* - Compute task */
      auto* computation_amount = new double(*task);
      XBT_INFO("Computation amount %f", *computation_amount);
      //sg4::ExecPtr exec        = sg4::this_actor::exec_init(*computation_amount);
      //exec->wait(); 
      sg4::this_actor::execute(*task);
    }

  } while (*task >= 0); /* Finish signal is negative */
  XBT_INFO("Terminating");
}


void save_results_to_CSV(const std::string& filename, int nb_w, double speed, int nb_tasks, double task_size, float sim_time, float exec_time) {
    // Check if the file exists
    bool fileExists = std::ifstream(filename).good();

    // Open the file in append mode
    std::ofstream file;
    file.open(filename, std::ios::app);

    if (!file.is_open()) {
        // Failed to open the file
        std::cerr << "Error opening the file: " << filename << std::endl;
        return;
    }

    // Create a string stream to build the CSV row
    std::stringstream ss;

    if (!fileExists) {
        // Write the header row if the file is newly created
        ss << "Nb. Workers, VM speed, Nb. tasks, Task size, Sim. time, Exec. time" << std::endl;
    }

    // Append the variables to the string stream with comma separators
    ss << nb_w << "," << speed << "," << nb_tasks << "," << task_size << "," << sim_time << "," << exec_time << std::endl;

    // Write the string stream content to the file
    file << ss.str();
    file.close();
}


int main(int argc, char* argv[])
{
  if (argc < 5) {
    std::cerr << "Usage: ./simgrid-simulation.bin [OPT...] <platform_file> <num_workers> <num_tasks> <task_size> <schedule_policy> \n";
    return 1;
  }

  // Parse command-line arguments
  int num_workers = std::stoi(argv[2]);
  int num_tasks = std::stoi(argv[3]);
  double task_size = std::stod(argv[4]) * 1000000; // in Mflops
  std::string sched_policy = argv[5];

  // Start S4U Engine
  int sg_argc = argc -3;
  sg4::Engine e(&sg_argc, argv);
  e.load_platform(argv[1]);

  // Verify if enough hosts
  if (num_workers + 1 > e.get_host_count()) {
    std::cerr << "Too many workers, maximum is [" << e.get_host_count() << "] for the platform [" << argv[1] << "] \n";
    return 1;
  }

  //sg4::Mailbox* mailboxes[num_workers];
  std::vector<sg4::Mailbox*> mailboxes;
  std::vector<sg4::Mailbox*>::iterator it;
  it = mailboxes.begin();

  for (int i=0; i < num_workers; i++) {
    // Create Mailbox for worker i
    const std::string mailbox_name =  "MAILBOX_W" + std::to_string(i);
    it = mailboxes.insert ( it , sg4::Mailbox::by_name(mailbox_name) );
    advance(it,1);

    // Create worker VM on host
    std::string hostname = "HOST_" + std::to_string(i+1);
    std::string vmname = "VM_" + std::to_string(i+1);
    sg4::VirtualMachine* vm_host = new sg4::VirtualMachine(vmname, e.host_by_name(hostname), 4);
    vm_host->start();

    // Create Actor for worker i
    sg4::Actor::create("worker", vm_host, worker_basic, mailboxes[i]);
    //sg4::Actor::create("worker", e.host_by_name(hostname), worker_basic, mailboxes[i]);
  }

  // Create Actor for scheduler
  std::string hostname = "HOST_" + std::to_string(num_workers+1);
  std::string vmname = "VM_" + std::to_string(num_workers+1);
  sg4::VirtualMachine* vm_host = new sg4::VirtualMachine(vmname, e.host_by_name(hostname), 4);
  vm_host->start();
  /*std::function<void()> *scheduler_func = nullptr;
  switch (sched_policy) {
    case "rr":
      scheduler_func = scheduler_round_robin;
      break;
    default:
      scheduler_func = scheduler_round_robin;
      break;

  }*/
  sg4::Actor::create("scheduler", vm_host, scheduler_circular, mailboxes, num_tasks, task_size);

  // Start the timer
  auto exec_start = std::chrono::steady_clock::now();

  // Run simulation
  e.run();

  // End the timer
  auto exec_end = std::chrono::steady_clock::now();

  // Calculate the duration
  std::chrono::duration<double> exec_duration = exec_end - exec_start;
  auto sim_duration = sg4::Engine::get_clock();
  XBT_INFO("Simulation time %g seconds || Execution time %f seconds", sim_duration, exec_duration.count());
  save_results_to_CSV("results.csv", num_workers, e.host_by_name("HOST_5")->get_speed(), num_tasks, task_size, sim_duration, exec_duration.count());

  return 0;
}
