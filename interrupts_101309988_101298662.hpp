/**
 * @file interrupts.hpp
 * @author Sasisekhar Govind
 * @brief template main.cpp file for Assignment 3 Part 1 of SYSC4001
 * 
 */

#ifndef INTERRUPTS_101309988_101298662_HPP_
#define INTERRUPTS_101309988_101298662_HPP_

#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<tuple>
#include<random>
#include<utility>
#include<sstream>
#include<iomanip>
#include<algorithm>

#define ADDR_BASE   0
#define VECTOR_SIZE 2

//An enumeration of states to make assignment easier
enum states {
    NEW,
    READY,
    RUNNING,
    WAITING,
    TERMINATED,
    NOT_ASSIGNED
};
std::ostream& operator<<(std::ostream& os, const enum states& s) { //Overloading the << operator to make printing of the enum easier

	std::string state_names[] = {
                                "NEW",
                                "READY",
                                "RUNNING",
                                "WAITING",
                                "TERMINATED",
                                "NOT_ASSIGNED"
    };
    return (os << state_names[s]);
}

struct memory_partition{
    unsigned int partition_number;
    unsigned int size;
    std::string code; // "free", "init", or program name
} memory_partitions[] = {
    {1, 40, "free"},
    {2, 25, "free"},
    {3, 15, "free"},
    {4, 10, "free"},
    {5, 8,  "free"},
    {6, 2,  "init"}
};

struct PCB{
    int             PID;
    std::string     program_name;   //name
    unsigned int    size;           // MB
    unsigned int    arrival_time;

    int             start_time;
    int             finish_time;    

    unsigned int    processing_time;
    unsigned int    remaining_time;

    unsigned int    io_freq;
    unsigned int    io_duration;
    int             time_to_next_io;  //countdown while running
    int             io_complete_time; //used while waiting

    int             priority;       //for External Priority

    int             partition_number;
    enum states     state;
};

struct external_file{
    std::string     program_name;
    unsigned int    size;
};

//------------------------------------HELPER FUNCTIONS FOR THE SIMULATOR------------------------------
// Following function was taken from stackoverflow; helper function for splitting strings
std::vector<std::string> split_delim(std::string input, std::string delim) {
    std::vector<std::string> tokens;
    std::size_t pos = 0;
    std::string token;
    while ((pos = input.find(delim)) != std::string::npos) {
        token = input.substr(0, pos);
        tokens.push_back(token);
        input.erase(0, pos + delim.length());
    }
    tokens.push_back(input);

    return tokens;
}

//Function that takes a queue as an input and outputs a string table of PCBs
std::string print_PCB(std::vector<PCB> _PCB) {
    const int tableWidth = 83;

    std::stringstream buffer;
    
    // Print top border
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;
    
    // Print headers
    buffer << "|"
              << std::setfill(' ') << std::setw(4) << "PID"
              << std::setw(2) << "|"
              << std::setfill(' ') << std::setw(11) << "Partition"
              << std::setw(2) << "|"
              << std::setfill(' ') << std::setw(5) << "Size"
              << std::setw(2) << "|"
              << std::setfill(' ') << std::setw(13) << "Arrival Time"
              << std::setw(2) << "|"
              << std::setfill(' ') << std::setw(11) << "Start Time"
              << std::setw(2) << "|"
              << std::setfill(' ') << std::setw(14) << "Remaining Time"
              << std::setw(2) << "|"
              << std::setfill(' ') << std::setw(11) << "State"
              << std::setw(2) << "|" << std::endl;
    
    // Print separator
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;
    
    // Print each PCB entry
    for (const auto& program : _PCB) {
        buffer << "|"
                  << std::setfill(' ') << std::setw(4) << program.PID
                  << std::setw(2) << "|"
                  << std::setw(11) << program.partition_number
                  << std::setw(2) << "|"
                  << std::setw(5) << program.size
                  << std::setw(2) << "|"
                  << std::setw(13) << program.arrival_time
                  << std::setw(2) << "|"
                  << std::setw(11) << program.start_time
                  << std::setw(2) << "|"
                  << std::setw(14) << program.remaining_time
                  << std::setw(2) << "|"
                  << std::setw(11) << program.state
                  << std::setw(2) << "|" << std::endl;
    }
    
    // Print bottom border
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;

    return buffer.str();
}

//Overloaded function that takes a single PCB as input
std::string print_PCB(PCB _PCB) {
    std::vector<PCB> temp;
    temp.push_back(_PCB);
    return print_PCB(temp);
}

std::string print_exec_header() {

    const int tableWidth = 49;

    std::stringstream buffer;
    
    // Print top border
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;
    
    // Print headers
    buffer  << "|"
            << std::setfill(' ') << std::setw(18) << "Time of Transition"
            << std::setw(2) << "|"
            << std::setfill(' ') << std::setw(3) << "PID"
            << std::setw(2) << "|"
            << std::setfill(' ') << std::setw(10) << "Old State"
            << std::setw(2) << "|"
            << std::setfill(' ') << std::setw(10) << "New State"
            << std::setw(2) << "|" << std::endl;
    
    // Print separator
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;

    return buffer.str();

}

std::string print_exec_status(unsigned int current_time, int PID, states old_state, states new_state) {

    const int tableWidth = 49;

    std::stringstream buffer;

    buffer  << "|"
            << std::setfill(' ') << std::setw(18) << current_time
            << std::setw(2) << "|"
            << std::setw(3) << PID
            << std::setw(2) << "|"
            << std::setw(10) << old_state
            << std::setw(2) << "|"
            << std::setw(10) << new_state
            << std::setw(2) << "|" << std::endl;

    return buffer.str();
}

std::string print_exec_footer() {
    const int tableWidth = 49;
    std::stringstream buffer;

    // Print bottom border
    buffer << "+" << std::setfill('-') << std::setw(tableWidth) << "+" << std::endl;

    return buffer.str();
}

//Synchronize the process in the process queue
void sync_queue(std::vector<PCB> &process_queue, PCB _process) {
    for(auto &process : process_queue) {
        if(process.PID == _process.PID) {
            process = _process;
        }
    }
}

//Writes a string to a file
void write_output(std::string execution, const char* filename) {
    std::ofstream output_file(filename);

    if (output_file.is_open()) {
        output_file << execution;
        output_file.close();  // Close the file when done
        std::cout << "File content overwritten successfully." << std::endl;
    } else {
        std::cerr << "Error opening file!" << std::endl;
    }

    std::cout << "Output generated in " << filename << ".txt" << std::endl;
}

//--------------------------------------------FUNCTIONS FOR THE "OS"-------------------------------------

//Assign memory partition to program
bool assign_memory(PCB &program) {
    for(int i = 5; i >= 0; i--) {
        if(program.size <= memory_partitions[i].size
           && memory_partitions[i].code == "free") {

            memory_partitions[i].code = program.program_name; // or PID string if no name
            program.partition_number = memory_partitions[i].partition_number;
            return true;
        }
    }
    return false;
}

//Free a memory partition
bool free_memory(PCB &program){
    for(int i = 5; i >= 0; i--) {
        if(program.partition_number == (int)memory_partitions[i].partition_number) {
            memory_partitions[i].code = "free";
            program.partition_number = -1;
            return true;
        }
    }
    return false;
}

//Convert a list of strings into a PCB
PCB add_process(std::vector<std::string> tokens) {
    PCB process;
    process.PID = std::stoi(tokens[0]);
    process.size = std::stoi(tokens[1]);
    process.arrival_time = std::stoi(tokens[2]);
    process.processing_time = std::stoi(tokens[3]);
    process.remaining_time = process.processing_time;

    process.io_freq = std::stoi(tokens[4]);
    process.io_duration = std::stoi(tokens[5]);

    // if priority column exists:
    process.priority = (tokens.size() > 6) ? std::stoi(tokens[6]) : 0;

    process.time_to_next_io = process.io_freq;
    process.io_complete_time = -1;

    process.start_time = -1;
    process.finish_time = -1;
    process.partition_number = -1;
    process.program_name = "P" + std::to_string(process.PID); // or from file if you add a name column
    process.state = NEW;
    return process;
}

//Returns true if all processes in the queue have terminated
bool all_process_terminated(std::vector<PCB> processes) {

    for(auto process : processes) {
        if(process.state != TERMINATED) {
            return false;
        }
    }

    return true;
}

//Terminates a given process
void terminate_process(PCB &running, std::vector<PCB> &job_queue) {
    running.remaining_time = 0;
    running.state = TERMINATED;
    free_memory(running);
    sync_queue(job_queue, running);
}

//set the process in the ready queue to runnning
void run_process(PCB &running, std::vector<PCB> &job_queue, std::vector<PCB> &ready_queue, unsigned int current_time) {
    running = ready_queue.back();
    ready_queue.pop_back();
    running.start_time = current_time;
    running.state = RUNNING;
    sync_queue(job_queue, running);
}

void idle_CPU(PCB &running) {
    running.start_time = 0;
    running.processing_time = 0;
    running.remaining_time = 0;
    running.arrival_time = 0;
    running.io_duration = 0;
    running.io_freq = 0;
    running.partition_number = 0;
    running.size = 0;
    running.state = NOT_ASSIGNED;
    running.PID = -1;
}

std::pair<std::string, int> intr_boilerplate(int current_time, int intr_num, int context_save_time, std::vector<std::string> vectors) {

    std::string execution = "";

    execution += std::to_string(current_time) + ", " + std::to_string(1) + ", switch to kernel mode\n";
    current_time++;

    execution += std::to_string(current_time) + ", " + std::to_string(context_save_time) + ", context saved\n";
    current_time += context_save_time;
    
    char vector_address_c[10];
    sprintf(vector_address_c, "0x%04X", (ADDR_BASE + (intr_num * VECTOR_SIZE)));
    std::string vector_address(vector_address_c);

    execution += std::to_string(current_time) + ", " + std::to_string(1) + ", find vector " + std::to_string(intr_num) 
                    + " in memory position " + vector_address + "\n";
    current_time++;

    execution += std::to_string(current_time) + ", " + std::to_string(1) + ", load address " + vectors.at(intr_num) + " into the PC\n";
    current_time++;

    return std::make_pair(execution, current_time);
}

std::tuple<std::vector<std::string>, std::vector<int>, std::vector<external_file>>parse_args(int argc, char** argv) {
    if(argc != 5) {
        std::cout << "ERROR!\nExpected 4 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./interrutps <your_trace_file.txt> <your_vector_table.txt> <your_device_table.txt> <your_external_files.txt>" << std::endl;
        exit(1);
    }

    std::ifstream input_file;
    input_file.open(argv[1]);
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << argv[1] << std::endl;
        exit(1);
    }
    input_file.close();

    input_file.open(argv[2]);
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << argv[2] << std::endl;
        exit(1);
    }

    std::string vector;
    std::vector<std::string> vectors;
    while(std::getline(input_file, vector)) {
        vectors.push_back(vector);
    }
    input_file.close();

    std::string duration;
    std::vector<int> delays;
    input_file.open(argv[3]);

    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << argv[3] << std::endl;
        exit(1);
    }

    while(std::getline(input_file, duration)) {
        delays.push_back(std::stoi(duration));
    }
    input_file.close();

    std::vector<external_file> external_files;
    input_file.open(argv[4]);
    if (!input_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << argv[4] << std::endl;
        exit(1);
    }

    std::string file_content;
    while(std::getline(input_file, file_content)) {
        external_file entry;
        auto file_info      = split_delim(file_content, ",");

        entry.program_name  = file_info[0];
        entry.size          = std::stoi(file_info[1]);
        external_files.push_back(entry);
    }

    input_file.close();


    return {vectors, delays, external_files};
}

void record_memory_status(unsigned int time);

#endif