#include <iostream>
#include <fstream>
#include <string>
#include <dirent.h>
#include <algorithm>

#define PROC_DIR "/proc/"
#define STATUS_FILE "/status"

// Check if number is positive
bool IsNumber(const std::string& s) {
    return !s.empty() && std::all_of(s.begin(), s.end(), ::isdigit);
}

// Get the VM RSS value from file
int getVM(const std::string& statFilePath) {
    std::ifstream file(statFilePath);
    if (!file.is_open()) {
        std::cout << "Failed to open status file: " << statFilePath << std::endl;
        return -1;
    }

//Store each line from file
    std::string line;
    int vm_rss = 1;  

//check for vmRSS and extract
    while (getline(file, line)) {
        if (line.find("VmRSS:") == 0) {
            std::sscanf(line.c_str() + 6, "%d", &vm_rss);
            break;
        }
    }

    file.close();
    return vm_rss;
}

//Main function
int main() {

//open process directory
    DIR *dir = opendir(PROC_DIR);
    if (!dir) {
        perror("Couldn't open Directory");
        return 1;
    }

    dirent *entry;
    
    //loop through each element in directory
    while ((entry = readdir(dir)) != NULL) {
    
     //check if directory name is a positive numerical value
        if (IsNumber(entry->d_name)) {
            std::string statFilePath = PROC_DIR + std::string(entry->d_name) + STATUS_FILE;
            int vm_rss = getVM(statFilePath);
            
            //check if file size more than 10K KB
            if (vm_rss > 10000) {  
                std::cout << "Process ID: " << entry->d_name << ", Memory Used: " << vm_rss << " kB\n";
            }
        }
    }

    closedir(dir);
    return 0;
}
