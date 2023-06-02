
#include <simgrid/s4u.hpp>
#include <vector>
#include <string>
namespace sg4 = simgrid::s4u;

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_test, "Messages specific for this s4u example");

/* Static scheduler to distribute 100 equal tasks in a round-robin fashion. */
static void scheduler_static(std::vector<sg4::Mailbox*> mailbox_list) {
  int num_tasks = 100;
  int wid = 0;
  sg4::Mailbox* worker_mailbox = nullptr;
  int num_workers = mailbox_list.size();
  XBT_INFO("Number of available workers %d", num_workers);

  for(int i=0; i < num_tasks; i++) {
    wid = i % num_workers;
    XBT_INFO("For WID %d", wid);
    worker_mailbox = mailbox_list[wid];

    XBT_INFO("Sending task to mailbox %s", worker_mailbox->get_name().c_str());

    /* -- Send a 1-Byte payload (latency bound) ... */
    auto* payload = new double(sg4::Engine::get_clock());
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

/* Basic worker that receives . */
static void worker_basic(sg4::Mailbox* mailbox) {
  std::unique_ptr<double, std::default_delete<double> > sender_time; // Deve ter um jeito melhor de declarar essa linha
  do {
    XBT_INFO("Receiving from mailbox %s", mailbox->get_name().c_str());
    /* - Receive the task from scheduler ....*/
    sender_time = mailbox->get_unique<double>();
    XBT_INFO("Sender time %f", *sender_time);

    if (*sender_time >= 0) { 
      double communication_time = sg4::Engine::get_clock() - *sender_time;
      XBT_INFO("Communication time (latency bound) %f", communication_time);

      /* - Compute task */
      auto* computation_amount = new double(sg4::this_actor::get_host()->get_speed());
      sg4::ExecPtr exec        = sg4::this_actor::exec_init(2 * (*computation_amount));
      exec->wait(); 
    }

  } while (*sender_time >= 0); /* Finish signal is negative */
}


int main(int argc, char* argv[])
{
  sg4::Engine e(&argc, argv);
  e.load_platform(argv[1]);

  int num_workers = 4;
  //sg4::Mailbox* mailboxes[num_workers];
  std::vector<sg4::Mailbox*> mailboxes;
  std::vector<sg4::Mailbox*>::iterator it;
  it = mailboxes.begin();

  for (int i=0; i < num_workers; i++) {
    // Create Mailbox for worker i
    const std::string mailbox_name =  "MAILBOX_W" + std::to_string(i);
    it = mailboxes.insert ( it , sg4::Mailbox::by_name(mailbox_name) );
    advance(it,1);

    // Create Actor for worker i
    std::string hostname = "HOST_" + std::to_string(i+1);
    sg4::Actor::create("worker", e.host_by_name(hostname), worker_basic, mailboxes[i]);
  }

  // Create Actor for scheduler
  std::string hostname = "HOST_" + std::to_string(num_workers+1);
  sg4::Actor::create("scheduler", e.host_by_name(hostname), scheduler_static, mailboxes);

  e.run();

  XBT_INFO("Simulation time %g", sg4::Engine::get_clock());

  return 0;
}
