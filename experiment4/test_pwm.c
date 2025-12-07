// test_pwm.c
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>

void print_usage(const char *program_name)
{
    printf("GPIO60 PWM Controller\n");
    printf("Usage: %s <command>\n", program_name);
    printf("Commands:\n");
    printf("  start    - Start 2Hz PWM on GPIO60\n");
    printf("  stop     - Stop PWM output\n");
    printf("  status   - Check current status\n");
    printf("  toggle   - Toggle PWM state\n");
    printf("  debug    - Dump register states (kernel log)\n");
    printf("  state    - Read current GPIO state\n");
}

int main(int argc, char *argv[])
{
    int fd;
    char buffer[100];
    
    if (argc != 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    // 打开设备文件
    fd = open("/dev/gpio_pwm", O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        printf("Make sure the driver is loaded and device file exists:\n");
        printf("  # insmod gpio_pwm_driver.ko\n");
        printf("  # mknod /dev/gpio_pwm c 240 0\n");
        return 1;
    }
    
    if (strcmp(argv[1], "start") == 0) {
        if (write(fd, "1", 1) != 1) {
            perror("Failed to start PWM");
            close(fd);
            return 1;
        }
        printf("2Hz PWM started on GPIO60\n");
        
    } else if (strcmp(argv[1], "stop") == 0) {
        if (write(fd, "0", 1) != 1) {
            perror("Failed to stop PWM");
            close(fd);
            return 1;
        }
        printf("PWM stopped on GPIO60\n");
        
    } else if (strcmp(argv[1], "status") == 0) {
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer)-1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            printf("%s", buffer);
        } else {
            perror("Failed to read status");
        }
        
    } else if (strcmp(argv[1], "toggle") == 0) {
        // 先读取当前状态
        ssize_t bytes_read = read(fd, buffer, sizeof(buffer)-1);
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            // 根据状态切换
            if (strstr(buffer, "Running")) {
                write(fd, "0", 1);
                printf("PWM stopped\n");
            } else {
                write(fd, "1", 1);
                printf("2Hz PWM started\n");
            }
        }
        
    } else if (strcmp(argv[1], "debug") == 0) {
        if (write(fd, "d", 1) != 1) {
            perror("Failed to send debug command");
            close(fd);
            return 1;
        }
        printf("Debug command sent. Check kernel logs with 'dmesg | tail -20'\n");
        
    } else if (strcmp(argv[1], "state") == 0) {
        int state = ioctl(fd, 2);
        if (state >= 0) {
            printf("GPIO60 current state: %s\n", state ? "HIGH" : "LOW");
        } else {
            perror("Failed to read GPIO state");
        }
        
    } else {
        print_usage(argv[0]);
        close(fd);
        return 1;
    }
    
    close(fd);
    return 0;
}