#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <cstring>
#include <cerrno>

using namespace std;

// FIXED SCREEN INFORMATION
void showFixedScreenInfo(int framebufferFD) {
    struct fb_fix_screeninfo fxInfo;
    
    //if error getting ioctal
    if (ioctl(framebufferFD, FBIOGET_FSCREENINFO, &fxInfo) < 0) {
        cerr << "Error getting FIXED screen info: " << strerror(errno) << endl;
        return;
    }

    cout << "Fixed Screen Information:" << endl;
    cout << "Screen Visual (FB_VISUAL): " << fxInfo.visual << endl;
    cout << "Screen Accelerator (FB_ACCEL): " << fxInfo.accel << endl;
    cout << "Screen Capabilities (FB_CAP): " << fxInfo.capabilities << endl;

    // Output Values
    //Screen Visual (FB_VISUAL): 2
    //Screen Accelerator (FB_ACCEL): 0
    //Screen Capabilities (FB_CAP): 0
}

//VARIABLE SCREEN INFORMATION
void showVarScreenInfo(int framebufferFD) {
    struct fb_var_screeninfo varInfo;

    //if error getting ioctal
    if (ioctl(framebufferFD, FBIOGET_VSCREENINFO, &varInfo) < 0) {
        cerr << "Error getting VARIABLE screen info: " << strerror(errno) << endl;
        return;
    }

    cout << "Variable Screen Information:" << endl;
    cout << "Screen X-Resolution: " << varInfo.xres << endl;
    cout << "Screen Y-Resolution: " << varInfo.yres << endl;
    cout << "Screen Bits Per Pixel: " << varInfo.bits_per_pixel << endl;

    // Output Values
    // Screen X-Resolution: 1920
    // Screen Y-Resolution: 969
    // Screen Bits Per Pixel: 32
}



//MAIN
int main() {
    

    //Opening framebuffer device
    int framebufferFD = open("/dev/fb0", O_RDONLY | O_NONBLOCK);
    if (framebufferFD == -1) {
        cerr << "Error opening Framebuffer File Descriptor: " << strerror(errno) << endl;
        return 1;
    }
    
    // Handling standard error
    int logFileFD = open("Screen.log", O_WRONLY | O_CREAT | O_TRUNC);
    if (logFileFD == -1) {
        cerr << "Error opening log file: " << strerror(errno) << endl;
        return 1;
    }
    
    // Duplicate standard error to log file
    dup2(logFileFD, STDERR_FILENO);
    close(logFileFD);

    while (true) {
        cout << "Select an option:" << endl;
        cout << "1. Fixed Screen Info" << endl;
        cout << "2. Variable Screen Info" << endl;
        cout << "0. Exit" << endl;
        
        //get user choice and switch
        int choice;
        cin >> choice;

        switch (choice) {
            case 1:
                showFixedScreenInfo(framebufferFD);
                break;
            case 2:
                showVarScreenInfo(framebufferFD);
                break;
            case 0:
                close(framebufferFD);
                return 0;
            default:
                cerr << "Invalid choice" << endl;
        }
    }

    close(framebufferFD);
    return 0;
}

