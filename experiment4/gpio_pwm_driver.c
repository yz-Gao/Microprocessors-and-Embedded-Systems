// gpio_pwm_driver.c
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/kthread.h>

#define DRIVER_NAME "gpio_pwm"

// GPIO寄存器基地址
#define GPIO_BASE 0x1fe00500

// GPIO控制寄存器
#define GPIO_OEN   (GPIO_BASE + 0x00)  // 输出使能寄存器
#define GPIO_O     (GPIO_BASE + 0x10)  // 输出寄存器  
#define GPIO_I     (GPIO_BASE + 0x20)  // 输入寄存器

// 通用配置寄存器0 - 用于功能复用控制
#define GENERAL_CFG0 0x1fe00420

// 使用GPIO60
#define GPIO_PIN   60

// nand_sel位在通用配置寄存器0的第9位
#define NAND_SEL_BIT 9

static volatile void *gpio_oen;
static volatile void *gpio_out;
static volatile void *gpio_in;
static volatile void *general_cfg0;

static struct task_struct *pwm_thread;
static int pwm_running = 0;

// 确保GPIO60配置为GPIO功能而不是NAND功能
static void configure_gpio60_function(void)
{
    unsigned long *cfg_reg = (unsigned long *)general_cfg0;
    unsigned long current_val;
    
    if (!general_cfg0) {
        printk(KERN_ERR "Failed to map general config register\n");
        return;
    }
    
    // 读取当前配置
    current_val = *cfg_reg;
    printk(KERN_INFO "GENERAL_CFG0 before: 0x%016lx\n", current_val);
    
    // 确保nand_sel位为0（GPIO功能）
    // 手册说明：0=GPIO, 1=NAND，默认是0
    *cfg_reg = current_val & ~(1UL << NAND_SEL_BIT);
    
    // 再次读取确认配置
    current_val = *cfg_reg;
    printk(KERN_INFO "GENERAL_CFG0 after: 0x%016lx\n", current_val);
    
    // 检查nand_sel位是否确实为0
    if (current_val & (1UL << NAND_SEL_BIT)) {
        printk(KERN_WARNING "GPIO60 may still be in NAND mode (nand_sel=1)\n");
    } else {
        printk(KERN_INFO "GPIO60 configured as GPIO function (nand_sel=0)\n");
    }
}

// GPIO写函数
static void gpio_write(int pin, int value)
{
    unsigned long *reg = (unsigned long *)gpio_out;
    unsigned long current_val = *reg;
    unsigned long bit_mask = 1UL << pin;
    
    if (value)
        *reg = current_val | bit_mask;
    else
        *reg = current_val & ~bit_mask;
    
    printk(KERN_DEBUG "GPIO%d set to %s\n", pin, value ? "HIGH" : "LOW");
}

// GPIO方向设置
static void gpio_set_direction(int pin, int output)
{
    unsigned long *reg = (unsigned long *)gpio_oen;
    unsigned long current_val = *reg;
    unsigned long bit_mask = 1UL << pin;
    
    if (output) {
        *reg = current_val & ~bit_mask;  // 0表示输出
        printk(KERN_DEBUG "GPIO%d set as OUTPUT\n", pin);
    } else {
        *reg = current_val | bit_mask;   // 1表示输入
        printk(KERN_DEBUG "GPIO%d set as INPUT\n", pin);
    }
}

// 读取GPIO当前状态
static int gpio_read(int pin)
{
    unsigned long *reg = (unsigned long *)gpio_in;
    unsigned long current_val = *reg;
    unsigned long bit_mask = 1UL << pin;
    
    return (current_val & bit_mask) ? 1 : 0;
}

// 读取所有相关寄存器状态（用于调试）
static void dump_registers(void)
{
    if (gpio_oen && gpio_out && gpio_in && general_cfg0) {
        printk(KERN_INFO "=== Register Dump ===\n");
        printk(KERN_INFO "GPIO_OEN: 0x%016llx\n", *(unsigned long long *)gpio_oen);
        printk(KERN_INFO "GPIO_OUT: 0x%016llx\n", *(unsigned long long *)gpio_out);
        printk(KERN_INFO "GPIO_IN:  0x%016llx\n", *(unsigned long long *)gpio_in);
        printk(KERN_INFO "GEN_CFG0: 0x%016llx\n", *(unsigned long long *)general_cfg0);
        printk(KERN_INFO "=====================\n");
    }
}

// PWM线程函数
static int pwm_thread_func(void *data)
{
    int cycle_count = 0;
    
    while (!kthread_should_stop()) {
        if (pwm_running) {
            // 2Hz方波：高电平250ms，低电平250ms
            gpio_write(GPIO_PIN, 1);
            msleep(250);
            gpio_write(GPIO_PIN, 0);
            msleep(250);
            
            cycle_count++;
            if (cycle_count % 10 == 0) {
                printk(KERN_DEBUG "GPIO60 PWM running, cycles: %d\n", cycle_count);
            }
        } else {
            msleep(100);
        }
    }
    return 0;
}

// 设备文件操作函数
static int dev_open(struct inode *inode, struct file *file)
{
    printk(KERN_DEBUG "GPIO PWM device opened\n");
    return 0;
}

static int dev_release(struct inode *inode, struct file *file)
{
    printk(KERN_DEBUG "GPIO PWM device closed\n");
    return 0;
}

static ssize_t dev_write(struct file *file, const char *buffer,
                        size_t length, loff_t *offset)
{
    char cmd;
    
    if (copy_from_user(&cmd, buffer, 1))
        return -EFAULT;
    
    if (cmd == '1') {
        pwm_running = 1;
        printk(KERN_INFO "GPIO%d PWM started (2Hz square wave)\n", GPIO_PIN);
    } else if (cmd == '0') {
        pwm_running = 0;
        gpio_write(GPIO_PIN, 0);  // 停止时确保输出低电平
        printk(KERN_INFO "GPIO%d PWM stopped\n", GPIO_PIN);
    } else if (cmd == 'd') {
        // 调试命令：dump寄存器状态
        dump_registers();
    }
    
    return length;
}

static ssize_t dev_read(struct file *file, char *buffer,
                       size_t length, loff_t *offset)
{
    char status[100];
    int current_state = gpio_read(GPIO_PIN);
    int len;
    
    len = snprintf(status, sizeof(status), 
                  "GPIO%d: PWM %s, Current state: %s\n", 
                  GPIO_PIN, 
                  pwm_running ? "Running (2Hz)" : "Stopped",
                  current_state ? "HIGH" : "LOW");
    
    if (copy_to_user(buffer, status, len))
        return -EFAULT;
    
    return len;
}

static long dev_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
        case 0: // 停止
            pwm_running = 0;
            gpio_write(GPIO_PIN, 0);
            break;
        case 1: // 开始
            pwm_running = 1;
            break;
        case 2: // 状态
            return gpio_read(GPIO_PIN);
        default:
            return -EINVAL;
    }
    return 0;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = dev_open,
    .release = dev_release,
    .read = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
};

static int __init gpio_pwm_init(void)
{
    int ret;
    
    printk(KERN_INFO "=== Initializing GPIO%d PWM driver ===\n", GPIO_PIN);
    
    // 映射通用配置寄存器0
    general_cfg0 = ioremap(GENERAL_CFG0, 8);
    
    // 映射GPIO控制寄存器
    gpio_oen = ioremap(GPIO_OEN, 8);
    gpio_out = ioremap(GPIO_O, 8);
    gpio_in = ioremap(GPIO_I, 8);
    
    if (!general_cfg0 || !gpio_oen || !gpio_out || !gpio_in) {
        printk(KERN_ERR "Failed to map one or more registers\n");
        ret = -ENOMEM;
        goto error_ioremap;
    }
    
    printk(KERN_INFO "All registers mapped successfully\n");
    
    // 配置GPIO60为GPIO功能（而不是NAND功能）
    configure_gpio60_function();
    
    // 注册字符设备
    ret = register_chrdev(240, DRIVER_NAME, &fops);
    if (ret < 0) {
        printk(KERN_ERR "Failed to register character device, error %d\n", ret);
        goto error_register;
    }
    
    // 配置GPIO为输出模式
    gpio_set_direction(GPIO_PIN, 1);
    gpio_write(GPIO_PIN, 0);  // 初始化为低电平
    
    printk(KERN_INFO "GPIO%d configured as output, initial state: LOW\n", GPIO_PIN);
    
    // 创建PWM线程
    pwm_thread = kthread_run(pwm_thread_func, NULL, "gpio_pwm_thread");
    if (IS_ERR(pwm_thread)) {
        ret = PTR_ERR(pwm_thread);
        printk(KERN_ERR "Failed to create PWM thread, error %d\n", ret);
        goto error_thread;
    }
    
    // 打印初始寄存器状态
    dump_registers();
    
    printk(KERN_INFO "=== GPIO%d PWM driver loaded successfully ===\n", GPIO_PIN);
    return 0;

error_thread:
    unregister_chrdev(240, DRIVER_NAME);
error_register:
    if (general_cfg0) iounmap(general_cfg0);
    if (gpio_oen) iounmap(gpio_oen);
    if (gpio_out) iounmap(gpio_out);
    if (gpio_in) iounmap(gpio_in);
error_ioremap:
    return ret;
}

static void __exit gpio_pwm_exit(void)
{
    printk(KERN_INFO "=== Unloading GPIO%d PWM driver ===\n", GPIO_PIN);
    
    if (pwm_thread) {
        kthread_stop(pwm_thread);
        printk(KERN_INFO "PWM thread stopped\n");
    }
    
    // 关闭GPIO输出
    gpio_write(GPIO_PIN, 0);
    printk(KERN_INFO "GPIO%d set to LOW\n", GPIO_PIN);
    
    unregister_chrdev(240, DRIVER_NAME);
    printk(KERN_INFO "Character device unregistered\n");
    
    // 取消所有映射
    if (general_cfg0) iounmap(general_cfg0);
    if (gpio_oen) iounmap(gpio_oen);
    if (gpio_out) iounmap(gpio_out);
    if (gpio_in) iounmap(gpio_in);
    printk(KERN_INFO "All registers unmapped\n");
    
    printk(KERN_INFO "=== GPIO%d PWM driver unloaded successfully ===\n", GPIO_PIN);
}

module_init(gpio_pwm_init);
module_exit(gpio_pwm_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Student");
MODULE_DESCRIPTION("GPIO PWM Driver for Loongson 2K1000 using GPIO60");
MODULE_VERSION("3.0");