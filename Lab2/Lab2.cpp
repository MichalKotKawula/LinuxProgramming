//Michal Kot-Kawula     mkot-kawula@myseneca.ca     #128945193

#include "pidutil.h"

int main() {
    vector<int> pidList;
    ErrStatus stat = GetAllPids(pidList);

//Call GetAllPids() and GetNameByPid() to print out all pids and their names.    
    for (int pid : pidList) {
        string Pidname;
        stat = GetNameByPid(pid, Pidname);
        if (stat != Err_OK) {
            cout << "Error getting name for pid " << pid << ": " << GetErrorMsg(stat) << endl;
            continue;
        }
        cout << "Pid: " << pid << ", Name: " << Pidname << endl;
    }

//Set pid to 1. 
//Call GetNameByPid() and print out the name of pid 1.
    int pid = 1;
    string Pidname;
    stat = GetNameByPid(pid, Pidname);
    if (stat != Err_OK) {
        cout << "Error: " << GetErrorMsg(stat) << endl;
    } else {
        cout << "Name of pid 1: " << Pidname << endl;
    }

//Set name to "Lab2". Call GetPidByName() to get the pid of Lab2. 
//Print "Lab2" and the pid of Lab2.
    Pidname = "Lab2";
    stat = GetPidByName(Pidname, pid);
    if (stat != Err_OK) {
        cout << "Error: " << GetErrorMsg(stat) << endl;
    } else {
        cout << "Pid of " << Pidname << ": " << pid << endl;
    }


//Set name to "Lab22". Call GetPidByName() to get the pid of Lab22. 
//There should not be a process called Lab22, therefore this should test your error message generation system.
    Pidname = "Lab22";
    stat = GetPidByName(Pidname, pid);
    if (stat != Err_OK) {
        cout << "Error: " << GetErrorMsg(stat) << endl;
    } else {
        cout << "Pid of " << Pidname << ": " << pid << endl;
    }


//If any errors are generated in the calls to these functions, 
//the error must be printed out by a call to the function GetErrorMsg() with the error number as an argument.
    if (stat != Err_OK) {
        cout << "Error: " << GetErrorMsg(stat) << endl;
        return -1;
    }


    return 0;
}
