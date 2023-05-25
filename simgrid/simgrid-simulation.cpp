
#include "simgrid/s4u.hpp"
#include <vector>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");


/* Static scheduler to distribute 100 equal tasks in a round-robin fashion. */
static void scheduler_static(simgrid::s4u::Mailbox** mailbox_list, int num_workers) {
  int num_tasks = 100;
  int wid = 0;
  simgrid::s4u::Mailbox* worker_mailbox = nullptr;

  for(int i=0; i < num_tasks; i++) {
    wid = i % num_workers;
    worker_mailbox = mailbox_list[wid];

    XBT_INFO("Sending task to mailbox %s", worker_mailbox->get_name().c_str());

    /* -- Send a 1-Byte payload (latency bound) ... */
    auto* payload = new double(sg4::Engine::get_clock());
    worker_mailbox->put(payload, 1);
  }

  /*- Send finish signal to all workers */
  for(int i=0; i < num_tasks; i++) {
    wid = i;
    worker_mailbox = mailbox_list[wid];

    XBT_INFO("Sending finish signal to mailbox %s", worker_mailbox->get_name().c_str());
    
    auto* payload = new double(-1.0);
    worker_mailbox->put(payload, 1);
  }

}

/* Basic worker that receives . */
static void worker_basic(simgrid::s4u::Mailbox* mailbox) {
  do {

    XBT_INFO("Receiving from mailbox %s", mailbox->get_name().c_str());
    /* - Receive the task from scheduler ....*/
    auto sender_time          = mailbox_in->get_unique<double>();

    if (sender_time > 0) { 
      double communication_time = sg4::Engine::get_clock() - *sender_time;
      XBT_INFO("Communication time (latency bound) %f", communication_time);

      /* - Compute task */
      auto* computation_amount = new double(sg4::this_actor::get_host()->get_speed());
      sg4::ExecPtr exec        = sg4::this_actor::exec_init(2 * (*computation_amount));
      exec->wait(); 
    }

  } while (sender_time > 0); /* Finish signal is negative */
}


int main(int argc, char* argv[])
{
  simgrid::s4u::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  int num_workers = 4;
  simgrid::s4u::Mailbox* mailboxes[num_workers];

  for (int i=1; i <= num_workers; i++) {
    // Create Mailbox for worker i
    mailboxes[i] = e.mailbox_by_name_or_create("MAILBOX_W%d", i);

    // Create Actor for worker i
    simgrid::s4u::Actor::create("worker", e.host_by_name("HOST_%d", i), worker_basic, mailboxes[i]);
  }

  // Create Actor for scheduler
  simgrid::s4u::Actor::create("scheduler", e.host_by_name("HOST_%d", num_workers+1), scheduler_static, mailboxes, num_workers);

  e.run();

  XBT_INFO("Simulation time %g", simgrid::s4u::Engine::get_clock());

  return 0;
}
