#include <stdio.h>


long get_current_core_frequency() {
    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
    if (cpuinfo == NULL) {
        perror("Failed to open /proc/cpuinfo");
        return -1;
    }

    long frequency = -1;
    char line[256];
    while (fgets(line, sizeof(line), cpuinfo)) {
        if (sscanf(line, "cpu MHz : %ld", &frequency) == 1) {
            break;
        }
    }

    fclose(cpuinfo);

    return frequency;
}

int main(){
    printf("%ld", get_current_core_frequency());

}