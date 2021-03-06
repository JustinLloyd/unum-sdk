// (c) 2019-2020 minim.co
// unum helper platform specific utils code

#include "unum.h"

// Factory reset, should never return
int util_platform_factory_reset(void)
{
    log("%s: performing factory reset...\n", __func__);
    notify_rc("resetdefault");
    sleep(60);
    return -1;
}

// Enumerite the list of the IP interfaces for the platform.
// For each interface a caller's callback is invoked until all the interfaces
// are enumerated.
// flags - flags indicating which interfaces to enumerate
//        UTIL_IF_ENUM_RTR_LAN - include all IP routing LAN interfaces
//        UTIL_IF_ENUM_RTR_WAN - include IP routing WAN interface
//        ... - see util_net.h
// f - callback function to invoke per interface
// data - data to pass to the callback function
// Returns: 0 - success, number of times the callback has failed
int util_platform_enum_ifs(int flags, UTIL_IF_ENUM_CB_t f, void *data)
{
    int ret = 0;
    int failed = 0;

    // Asus 1300 has just a primary Interface unlike 1750
    if(0 != (UTIL_IF_ENUM_RTR_LAN & flags))
    {
        int ii;
        char *ifs[] = { PLATFORM_GET_MAIN_LAN_NET_DEV() };

        for(ii = 0; ii < UTIL_ARRAY_SIZE(ifs) && ifs[ii] != NULL; ++ii) {
            ret = f(ifs[ii], data);
            failed += (ret == 0 ? 0 : 1);
        }
    }

    // We only support 1 WAN interface and no AP operation mode.
    // Note: This might need to be updated to deal with PPPoE properly
    if(0 != (UTIL_IF_ENUM_RTR_WAN & flags)) {
        ret = f(PLATFORM_GET_MAIN_WAN_NET_DEV(), data);
        failed += (ret == 0 ? 0 : 1);
    }

    return failed;
}

// Platform function for renewing DHCP lease on WAN interface
int platform_release_renew(void)
{
    FILE *pid_file;
    int pid, result;
    char cmd_buf[64];

    char pid_file_name[] = "/var/run/udhcpc0.pid";

    // Open PID File from Well Known Location
    pid_file = fopen(pid_file_name,"r");
    if(pid_file == NULL) {
        log("%s: Could not open PID file at %s\n", __func__, pid_file_name);
        return 1;
    }

    // Read Process ID
    fscanf(pid_file, "%d", &pid);

    // Close File
    fclose(pid_file);

    // Build Command String & NULL Terminate
    snprintf(cmd_buf, 64, "kill -SIGUSR2 %d", pid);
    cmd_buf[63] = '\0';

    // Execute Command & Print Result
    log("%s: Performing release/renew using cmd <%s>\n", __func__, cmd_buf);
    result = system(cmd_buf);
    log("%s: Result = %d\n", __func__, result);
    if(result) {
        log("%s: error performing SIGUSR2\n");
        return result;
    }

    // Build Command String & NULL Terminate
    snprintf(cmd_buf, 64, "kill -SIGUSR1 %d", pid);
    cmd_buf[63] = '\0';

    // Execute Command & Print Result
    log("%s: Performing release/renew using cmd <%s>\n", __func__, cmd_buf);
    result = system(cmd_buf);
    log("%s: Result = %d\n", __func__, result);

    // Wait for Network to Come Up
    sleep(5);

    return result;

}
