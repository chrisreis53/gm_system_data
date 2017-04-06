#include <cstdlib>
#include <iostream>
#include <fstream>
#include <string>

#include "../example.h"

using namespace gmsec::api;
using namespace gmsec::api::util;

struct proc_data {
  double uptime;
  double idle;
  double proc_percent;
  long memfree;
  long memtotal;
  long memavailable;
  long memactive;
  long meminactive;
  long memused;
  long memswap;
  long memswapfree;
  long rxbytes;
  long rxpackets;
  long txbytes;
  long txpackets;
};

bool network_init(true);
static unsigned long long lastTotalUser, lastTotalUserNice, lastTotalSys, lastTotalIdle;
static unsigned long lastRXBytes, lastRXPackets, lastTXBytes, lastTXPackets;

int wait_millis(int ms){

  TimeUtil::millisleep(ms);

  return 0;
}

/***
For setting up initial variables used in calculations
***/
void init(){
    FILE* file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &lastTotalUser, &lastTotalUserNice,
        &lastTotalSys, &lastTotalIdle);
    fclose(file);
    wait_millis(100);
}

void getSystemData(proc_data *pd){
    FILE* file;

    //Uptime and Idle
    file = fopen("/proc/uptime", "r");
    fscanf(file, "%lf %lf", &pd->uptime, &pd->idle);
    fclose(file);

    //Percent Utilization
    unsigned long long user, nice, sys, idle, total;
    double percent;
    file = fopen("/proc/stat", "r");
    fscanf(file, "cpu %llu %llu %llu %llu", &user, &nice,&sys, &idle);
    fclose(file);

    //Calculate Percent Utilization
    total = (user - lastTotalUser)+(nice - lastTotalUserNice)+(sys - lastTotalSys)+(idle - lastTotalIdle);
    if (user < lastTotalUser || nice < lastTotalUserNice || sys < lastTotalSys || idle < lastTotalIdle){
      percent = -1.0;
    } else {
      percent = ((user - lastTotalUser)+(nice - lastTotalUserNice)+(sys - lastTotalSys));
      percent /= total;
      percent *= 100;
    }

    pd->proc_percent = percent;
    lastTotalIdle = idle;
    lastTotalSys = sys;
    lastTotalUser = user;
    lastTotalUserNice = nice;

    //memory
    char buff[100];
    unsigned long ul_MemTotal, ul_MemFree, ul_MemAvailable, ul_MemActive, ul_MemInactive, ul_MemSwap, ul_MemSwapFree;
    file = fopen("/proc/meminfo", "r");
    while (fgets(buff,75,file) != NULL) {
      if (strstr(buff,"MemTotal:")) {
        sscanf(buff, "MemTotal: %lu kB", &ul_MemTotal);
      } else if (strstr(buff,"MemFree:")) {
        sscanf(buff, "MemFree: %lu kB", &ul_MemFree);
      } else if (strstr(buff,"MemAvailable:")) {
        sscanf(buff, "MemAvailable: %lu kB", &ul_MemAvailable);
      } else if (strstr(buff,"Active:")) {
        sscanf(buff, "Active: %lu kB", &ul_MemActive);
      } else if (strstr(buff,"SwapTotal:")) {
        sscanf(buff, "SwapTotal: %lu kB", &ul_MemSwap);
      } else if (strstr(buff,"SwapFree:")) {
        sscanf(buff, "SwapFree: %lu kB", &ul_MemSwapFree);
      } else if (strstr(buff,"MemInactive:")) {
        sscanf(buff, "MemInactive: %lu kB", &ul_MemInactive);
      }
    }
    fclose(file);

    pd->memfree = ul_MemFree;
    pd->memtotal = ul_MemTotal;
    pd->memused = ul_MemTotal - ul_MemFree;
    pd->memavailable = ul_MemAvailable;
    pd->memactive = ul_MemActive;
    pd->meminactive = ul_MemInactive;
    pd->memswap = ul_MemSwap;
    pd->memswapfree = ul_MemSwapFree;

    //Network Data
    unsigned long ul_TXBytes, ul_TXPackets,ul_RXBytes, ul_RXPackets;
    file = fopen("/proc/net/dev", "r");
    while (fgets(buff,100,file) != NULL) {
      if (strstr(buff,"enp0s3:")) {
        //std::cout << buff << '\n';
        sscanf(buff, "enp0s3: %lu %lu %*u %*u %*u %*u %*u %*u %lu %lu", &ul_RXBytes, &ul_RXPackets, &ul_TXBytes, &ul_TXPackets);
      }
    }
    fclose(file);
    //std::cout << ul_RXBytes << " " << ul_RXPackets << " " << ul_TXBytes << " " << ul_TXPackets << '\n';
    if(network_init){
      lastRXBytes = ul_RXBytes;
      lastRXPackets = ul_RXPackets;
      lastTXBytes = ul_TXBytes;
      lastTXPackets = ul_TXPackets;
      network_init = false;
    }
    //return network data
    pd->rxbytes=ul_RXBytes - lastRXBytes;
    pd->rxpackets=ul_RXPackets - lastRXPackets;
    pd->txbytes=ul_TXBytes - lastTXBytes;
    pd->txpackets=ul_TXPackets - lastTXPackets;

    //update variables
    lastRXBytes = ul_RXBytes;
    lastRXPackets = ul_RXPackets;
    lastTXBytes = ul_TXBytes;
    lastTXPackets = ul_TXPackets;

}

// int main(int argc, char const *argv[]) {
//   proc_data sysinfo;
//   init();
//   for (size_t i = 0; i < 50; i++) {
//     getSystemData(&sysinfo);
//     std::cout << "Uptime " << sysinfo.uptime << " " << "Utilization " << sysinfo.proc_percent << '\n';
//     wait_millis(2000);
//   }
//
//   return 0;
// }
