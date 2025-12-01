/**
 * @file interrupts_101309988_101298662_EP_RR.cpp
 * @author Dorian Bansoodeb 101309988
 * @author Justin Kim 101298662
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include "interrupts_101309988_101298662.hpp"

void EP(std::vector<PCB> &ready_queue) {
    std::sort(
        ready_queue.begin(),
        ready_queue.end(),
        [](const PCB &first, const PCB &second){
            return (first.priority < second.priority); 
        }
    );
}

void record_memory_status(unsigned int time) {
    std::ofstream memlog("memory_status.txt", std::ios::app);
    if (!memlog.is_open()) return;

    memlog << "================ MEMORY STATUS ================\n";
    memlog << "Time: " << time << "\n\n";

    memlog << "Partition | Size | Status\n";
    memlog << "------------------------------------------\n";

    int total_used = 0;
    int total_free = 0;

    for (auto &part : memory_partitions) {
        memlog << part.partition_number << "         | "
               << part.size << "MB | "
               << part.code << "\n";

        if (part.code == "free")
            total_free += part.size;
        else
            total_used += part.size;
    }

    memlog << "\nTotal Used: " << total_used << " MB\n";
    memlog << "Total Free: " << total_free << " MB\n";
    memlog << "==============================================\n\n";
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(
    std::vector<PCB> list_processes) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

    //quantum
    unsigned int time_in_slice = 0;  //how long the running process used the CPU 
    const unsigned int QUANTUM = 100;

    //Initialize an empty running process
    idle_CPU(running);

    std::string execution_status;

    //make the output table (the header row)
    execution_status = print_exec_header();

    //Loop while till there are no ready or waiting processes.
    //This is the main reason I have job_list, you don't have to use it.
    while(!all_process_terminated(job_list) || job_list.empty()) {

        //Inside this loop, there are three things you must do:
        // 1) Populate the ready queue with processes as they arrive
        // 2) Manage the wait queue
        // 3) Schedule processes from the ready queue

        //Population of ready queue is given to you as an example.
        //Go through the list of proceeses
        for(auto &process : list_processes) {
            if(process.arrival_time == current_time && process.state == NEW) {//check if the AT = current time
                //if so, assign memory and put the process into the ready queue
                bool mem_ok = assign_memory(process);

                job_list.push_back(process); //Add it to the list of processes

                if(mem_ok) {
                    process.state = READY;  //Set the process state to READY
                    ready_queue.push_back(process); //Add the process to the ready queue

                    execution_status += print_exec_status(current_time, process.PID, NEW, READY);
                    record_memory_status(current_time);//BONUS
                }
            }
        }

        ///////////////////////MANAGE WAIT QUEUE/////////////////////////
        //This mainly involves keeping track of how long a process must remain in the ready queue
        for(size_t i = 0; i < wait_queue.size(); ) {
            PCB p = wait_queue[i];

            if(p.io_complete_time <= (int)current_time) {
                states old_state = p.state;
                p.state = READY;

                ready_queue.push_back(p);
                execution_status += print_exec_status(current_time, p.PID, old_state, READY);
                sync_queue(job_list, p);

                wait_queue.erase(wait_queue.begin() + i);
            } else {
                i++;
            }
        }

        /////////////////////////////////////////////////////////////////

        //////////////////////////SCHEDULER//////////////////////////////
        EP(ready_queue);
        /////////////////////////////////////////////////////////////////
        
        //Schedule a process if the CPU is idle
        if(running.PID == -1 && !ready_queue.empty()) {

            running = ready_queue.back();   //highest priority after EP()
            ready_queue.pop_back();

            states old_state = running.state;
            running.state = RUNNING;

            if(running.start_time < 0) {
                running.start_time = current_time;
            }

            execution_status += print_exec_status(current_time, running.PID, old_state, RUNNING);
            sync_queue(job_list, running);

            time_in_slice = 0; //reset time in slice for RR
        }

        if(running.PID != -1) {

            if(running.remaining_time > 0) {
                running.remaining_time--;
            }

            if(running.io_freq > 0) {
                running.time_to_next_io--;
            }

            time_in_slice++;
            sync_queue(job_list, running);

            // termination has highest priority
            // termination has highest priority
            if (running.remaining_time == 0) {
                unsigned int transition = current_time + 1;
                states old_state = running.state;

                running.state = TERMINATED;
                running.finish_time = transition;

                free_memory(running);
                record_memory_status(transition); // BONUS

                execution_status += print_exec_status(
                    transition, 
                    running.PID, 
                    old_state, 
                    TERMINATED
                );

                sync_queue(job_list, running);
                idle_CPU(running);

                // attempts to load processs that couldnt be loaded before
                for (auto &p : list_processes) {
                    if (p.state == NEW && p.arrival_time <= transition) {
                        bool mem_ok = assign_memory(p);
                        if (mem_ok) {
                            p.state = READY;
                            ready_queue.push_back(p);
                            sync_queue(job_list, p);
                            execution_status += print_exec_status(
                                transition, 
                                p.PID, 
                                NEW, 
                                READY
                            );
                        }
                    }
                }
            }
            // only check I/O if it doesn't terminate
            else if(running.io_freq > 0 && running.time_to_next_io == 0) {

                unsigned int transition = current_time + 1;
                states old_state = running.state;
                running.state = WAITING;

                running.io_complete_time = transition + running.io_duration;
                running.time_to_next_io = running.io_freq;

                execution_status += print_exec_status(transition, running.PID, old_state, WAITING);

                wait_queue.push_back(running);
                sync_queue(job_list, running);
                idle_CPU(running);

            }
            // only check quantum if it doesn't terminate or go to I/O
            else if(time_in_slice == QUANTUM) {

                states old_state = running.state;
                running.state = READY;

                execution_status += print_exec_status(current_time, running.PID, old_state, READY);
                record_memory_status(current_time);//BONUS

                ready_queue.push_back(running);
                sync_queue(job_list, running);
                idle_CPU(running);

                time_in_slice = 0;
            }
        }   

        current_time++;
    }
    
    //Close the output table
    execution_status += print_exec_footer();

    return std::make_tuple(execution_status);
}


int main(int argc, char** argv) {

    //Get the input file from the user
    if(argc != 2) {
        std::cout << "ERROR!\nExpected 1 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_input_file.txt>" << std::endl;
        return -1;
    }

    //Open the input file
    auto file_name = argv[1];
    std::ifstream input_file;
    input_file.open(file_name);

    //Ensure that the file actually opens
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << file_name << std::endl;
        return -1;
    }

    //Parse the entire input file and populate a vector of PCBs.
    //To do so, the add_process() helper function is used (see include file).
    std::string line;
    std::vector<PCB> list_process;
    while(std::getline(input_file, line)) {
        if(line.empty()) continue;
        auto input_tokens = split_delim(line, ", ");
        auto new_process = add_process(input_tokens);
        list_process.push_back(new_process);
    }
    input_file.close();

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process);

    write_output(exec, "execution.txt");

    return 0;
}