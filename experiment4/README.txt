使用说明

本驱动程序旨在控制龙芯教育派 GPIO60 引脚输出频率约为 2Hz 的方波。

一、编译
$ make ARCH=loongarch CROSS_COMPILE=loongarch64-linux-gnu-

二、部署
1.加载驱动模块
# insmod gpio_pwm_driver.ko
2.创建设备文件
# mknod /dev/gpio_pwm c 240 0
3.编译测试程序
$ loongarch64-linux-gnu-gcc test_pwm.c -o test_pwm -static

三、驱动程序测试
1.加载驱动并查看寄存器初始状态
# insmod gpio_pwm_driver.ko
# dmesg | tail -20
2.检查GPIO60是否真的可以控制
# echo '1' > /dev/gpio_pwm
# cat /dev/gpio_pwm  # 应该显示Running
执行以上命令，可以正常输出方波，证明内核文件无误。

四、使用
1.方波输出
# ./test_pwm start    // 应该显示"2Hz PWM started on GPIO60"，并且用示波器/万用表能观察到方波
2.状态检测
# ./test_pwm status   // 应该显示"GPIO60: PWM Running, Current state: HIGH/LOW"
3.查看寄存器状态
# ./test_pwm debug    // 会在内核日志中输出寄存器状态（用dmesg | tail -10查看）
4.停止输出
# ./test_pwm stop     // 应该显示"PWM stopped on GPIO60"，方波停止
