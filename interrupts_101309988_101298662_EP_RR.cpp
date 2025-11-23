/**
 * @file interrupts.cpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#include<interrupts_101309988_101298662.hpp>

void FCFS(std::vector<PCB> &ready_queue) {
    std::sort( 
                ready_queue.begin(),
                ready_queue.end(),
                []( const PCB &first, const PCB &second ){
                    return (first.arrival_time > second.arrival_time); 
                } 
            );
}

std::tuple<std::string /* add std::string for bonus mark */ > run_simulation(
    std::vector<PCB> list_processes,
               std::vector<std::string> vectors,
               std::vector<int> delays,
               std::vector<external_file> external_files) {

    std::vector<PCB> ready_queue;   //The ready queue of processes
    std::vector<PCB> wait_queue;    //The wait queue of processes
    std::vector<PCB> job_list;      //A list to keep track of all the processes. This is similar
                                    //to the "Process, Arrival time, Burst time" table that you
                                    //see in questions. You don't need to use it, I put it here
                                    //to make the code easier :).

    unsigned int current_time = 0;
    PCB running;

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
        FCFS(ready_queue); //example of FCFS is shown here
        /////////////////////////////////////////////////////////////////

        if(running.PID == -1 && !ready_queue.empty()) {

            running = ready_queue.back();
            ready_queue.pop_back();

            states old_state = running.state;
            running.state = RUNNING;

            if(running.start_time < 0) {
                running.start_time = current_time;
            }

            execution_status += print_exec_status(current_time, running.PID, old_state, RUNNING);
            sync_queue(job_list, running);
        }

        if(running.PID != -1) {

            if(running.remaining_time > 0) {
                running.remaining_time--;
            }

            if(running.io_freq > 0) {
                running.time_to_next_io--;
            }

            sync_queue(job_list, running);

            if(running.io_freq > 0 && running.time_to_next_io == 0 && running.remaining_time > 0) {
                states old_state = running.state;
                running.state = WAITING;

                running.io_complete_time = current_time + running.io_duration;
                running.time_to_next_io = running.io_freq;

                execution_status += print_exec_status(current_time, running.PID, old_state, WAITING);

                wait_queue.push_back(running);
                sync_queue(job_list, running);
                idle_CPU(running);
            }

            if(running.remaining_time == 0) {
                states old_state = running.state;
                running.state = TERMINATED;
                running.finish_time = current_time;

                free_memory(running);
                execution_status += print_exec_status(current_time, running.PID, old_state, TERMINATED);

                sync_queue(job_list, running);
                idle_CPU(running);
                for(auto &p : list_processes) {
                    if(p.state == NEW && p.arrival_time <= current_time) {
                        bool mem_ok = assign_memory(p);
                        if(mem_ok) {
                            p.state = READY;
                            ready_queue.push_back(p);
                            sync_queue(job_list, p);
                            execution_status += print_exec_status(current_time, p.PID, NEW, READY);
                        }
                    }
                }
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
    if(argc != 5) {
        std::cout << "ERROR!\nExpected 4 arguments, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrupts <your_input_file.txt> <your_vector_table.txt> <your_device_table.txt> <your_external_files.txt>" << std::endl;
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

    //Load vectors/delays/external_files using your A2-style parser
    auto [vectors, delays, external_files] = parse_args(argc, argv);

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

    //Sort by arrival time (needed for correct simulation order)
    std::sort(list_process.begin(), list_process.end(),
              [](PCB& a, PCB& b){ return a.arrival_time < b.arrival_time; });

    //With the list of processes, run the simulation
    auto [exec] = run_simulation(list_process, vectors, delays, external_files);

    write_output(exec, "execution.txt");

    return 0;
}