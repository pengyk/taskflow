// This example demonstrates how to assign priorities to tasks.
// Task priorities determine the relative order in which ready tasks
// are executed by worker threads.

#include <taskflow/taskflow.hpp>
#include <atomic>

int main() {

  tf::Executor executor(1);  // use 1 worker to clearly show priority ordering
  tf::Taskflow taskflow("priority-demo");

  std::vector<std::string> execution_order;
  std::mutex mutex;

  // Create tasks with different priorities
  // All tasks are independent (no dependencies), so execution order
  // depends solely on their priorities
  auto A = taskflow.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("A (HIGH)");
  });

  auto B = taskflow.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("B (NORMAL)");
  });

  auto C = taskflow.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("C (LOW)");
  });

  auto D = taskflow.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("D (HIGH)");
  });

  auto E = taskflow.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("E (LOW)");
  });

  auto F = taskflow.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("F (NORMAL)");
  });

  // Assign names and priorities
  A.name("A").priority(tf::TaskPriority::HIGH);
  B.name("B").priority(tf::TaskPriority::NORMAL);
  C.name("C").priority(tf::TaskPriority::LOW);
  D.name("D").priority(tf::TaskPriority::HIGH);
  E.name("E").priority(tf::TaskPriority::LOW);
  F.name("F").priority(tf::TaskPriority::NORMAL);

  std::cout << "Task priorities:\n";
  std::cout << "  A: HIGH   (value=" << static_cast<unsigned>(A.priority()) << ")\n";
  std::cout << "  B: NORMAL (value=" << static_cast<unsigned>(B.priority()) << ")\n";
  std::cout << "  C: LOW    (value=" << static_cast<unsigned>(C.priority()) << ")\n";
  std::cout << "  D: HIGH   (value=" << static_cast<unsigned>(D.priority()) << ")\n";
  std::cout << "  E: LOW    (value=" << static_cast<unsigned>(E.priority()) << ")\n";
  std::cout << "  F: NORMAL (value=" << static_cast<unsigned>(F.priority()) << ")\n";

  std::cout << "\nExecuting tasks (with 1 worker thread)...\n";
  std::cout << "Expected: HIGH tasks first, then NORMAL, then LOW\n\n";

  executor.run(taskflow).wait();

  std::cout << "Actual execution order:\n";
  for(size_t i = 0; i < execution_order.size(); ++i) {
    std::cout << "  " << (i+1) << ". " << execution_order[i] << "\n";
  }

  // Demonstrate priority with dependent tasks
  std::cout << "\n--- Priority with Dependencies ---\n";

  tf::Taskflow tf2;
  execution_order.clear();

  // init -> [high_task, normal_task, low_task] -> final
  auto init = tf2.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("init");
  }).name("init");

  auto high_task = tf2.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("high_task (HIGH)");
  }).name("high_task").priority(tf::TaskPriority::HIGH);

  auto normal_task = tf2.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("normal_task (NORMAL)");
  }).name("normal_task").priority(tf::TaskPriority::NORMAL);

  auto low_task = tf2.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("low_task (LOW)");
  }).name("low_task").priority(tf::TaskPriority::LOW);

  auto final_task = tf2.emplace([&]() {
    std::lock_guard<std::mutex> lock(mutex);
    execution_order.push_back("final");
  }).name("final");

  // Set up dependencies: init -> {high, normal, low} -> final
  init.precede(high_task, normal_task, low_task);
  final_task.succeed(high_task, normal_task, low_task);

  std::cout << "Graph: init -> [high_task, normal_task, low_task] -> final\n";
  std::cout << "Expected: init, then high/normal/low by priority, then final\n\n";

  executor.run(tf2).wait();

  std::cout << "Actual execution order:\n";
  for(size_t i = 0; i < execution_order.size(); ++i) {
    std::cout << "  " << (i+1) << ". " << execution_order[i] << "\n";
  }

  return 0;
}
